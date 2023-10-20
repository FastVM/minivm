// This is built specifically for pointer hash sets
#ifndef NL_HASH_SET_H
#define NL_HASH_SET_H

#define NL_HASHSET_TOMB ((void*) UINTPTR_MAX)

#define NL_HASHSET_HIGH_BIT (~(SIZE_MAX >> ((size_t) 1)))
#define NL_HASHSET_INDEX_BITS (SIZE_MAX >> ((size_t) 1))

typedef struct NL_HashSet {
    TB_Arena* allocator;

    size_t exp, count;
    void** data;
} NL_HashSet;

typedef uint32_t (*NL_HashFunc)(void* a);
typedef bool (*NL_CompareFunc)(void* a, void* b);

NL_HashSet nl_hashset_alloc(size_t cap);
NL_HashSet nl_hashset_arena_alloc(TB_Arena* arena, size_t cap);
void nl_hashset_free(NL_HashSet hs);

void nl_hashset_clear(NL_HashSet* restrict hs);
bool nl_hashset_remove(NL_HashSet* restrict hs, void* ptr);
bool nl_hashset_put(NL_HashSet* restrict hs, void* ptr);
size_t nl_hashset_lookup(NL_HashSet* restrict hs, void* ptr);

void* nl_hashset_get2(NL_HashSet* restrict hs, void* ptr, NL_HashFunc hash, NL_CompareFunc cmp);

// this one takes a custom hash function
void* nl_hashset_put2(NL_HashSet* restrict hs, void* ptr, NL_HashFunc hash, NL_CompareFunc cmp);
void nl_hashset_remove2(NL_HashSet* restrict hs, void* ptr, NL_HashFunc hash, NL_CompareFunc cmp);

#define nl_hashset_capacity(hs) (1ull << (hs)->exp)
#define nl_hashset_for(it, hs)  for (void **it = (hs)->data, **_end_ = &it[nl_hashset_capacity(hs)]; it != _end_; it++) if (*it != NULL && *it != NL_HASHSET_TOMB)

#endif /* NL_HASH_SET_H */

#ifdef NL_HASH_SET_IMPL
#include <common.h>

#define NL_HASHSET_HASH(ptr) ((((uintptr_t) ptr) * 11400714819323198485ull) >> 32ull)

NL_HashSet nl_hashset_alloc(size_t cap) {
    cap = (cap * 4) / 3;
    if (cap < 4) cap = 4;

    // next power of two
    #if defined(_MSC_VER) && !defined(__clang__)
    size_t exp = 64 - _lzcnt_u64(cap - 1);
    #else
    size_t exp = 64 - __builtin_clzll(cap - 1);
    #endif

    cap = (cap == 1 ? 1 : 1 << exp);

    return (NL_HashSet){ .exp = exp, .data = cuik_calloc(cap, sizeof(void*)) };
}

NL_HashSet nl_hashset_arena_alloc(TB_Arena* arena, size_t cap) {
    cap = (cap * 4) / 3;
    if (cap < 4) cap = 4;

    // next power of two
    #if defined(_MSC_VER) && !defined(__clang__)
    size_t exp = 64 - _lzcnt_u64(cap - 1);
    #else
    size_t exp = 64 - __builtin_clzll(cap - 1);
    #endif

    cap = (cap == 1 ? 1 : 1 << exp);

    void* data = tb_arena_alloc(arena, cap * sizeof(void*));
    memset(data, 0, cap * sizeof(void*));
    return (NL_HashSet){ .allocator = arena, .exp = exp, .data = data };
}

void nl_hashset_free(NL_HashSet hs) {
    if (hs.allocator == NULL) {
        cuik_free(hs.data);
    } else {
        tb_arena_pop(hs.allocator, hs.data, (1ull << hs.exp) * sizeof(void*));
    }
}

// lookup into hash map for ptr, it's also used to put things in
size_t nl_hashset_lookup(NL_HashSet* restrict hs, void* ptr) {
    assert(ptr);
    uint32_t h = NL_HASHSET_HASH(ptr);

    size_t mask = (1 << hs->exp) - 1;
    size_t first = h & mask, i = first;

    do {
        if (hs->data[i] == NULL) {
            return i;
        } else if (hs->data[i] == ptr) {
            return ~(SIZE_MAX >> ((size_t) 1)) | i; // highest bit set
        }

        i = (i + 1) & mask;
    } while (i != first);

    return SIZE_MAX;
}

bool nl_hashset_put(NL_HashSet* restrict hs, void* ptr) {
    size_t index = nl_hashset_lookup(hs, ptr);

    // rehash (ideally only once? statistically not impossible for two?)
    while (index == SIZE_MAX) {
        assert(hs->allocator == NULL && "arena hashsets can't be resized!");
        NL_HashSet new_hs = nl_hashset_alloc(nl_hashset_capacity(hs));
        nl_hashset_for(p, hs) {
            nl_hashset_put(&new_hs, *p);
        }
        nl_hashset_free(*hs);
        *hs = new_hs;

        // "tail" calls amirite
        index = nl_hashset_lookup(hs, ptr);
    }

    if (index & NL_HASHSET_HIGH_BIT) {
        // slot is already filled
        return false;
    } else {
        hs->count++;
        hs->data[index] = ptr;
        return true;
    }
}

bool nl_hashset_remove(NL_HashSet* restrict hs, void* ptr) {
    size_t index = nl_hashset_lookup(hs, ptr);
    if (index == SIZE_MAX || (index & NL_HASHSET_HIGH_BIT) == 0) {
        return false;
    }

    hs->count--;
    hs->data[index] = NL_HASHSET_TOMB;
    return true;
}

void nl_hashset_remove2(NL_HashSet* restrict hs, void* ptr, NL_HashFunc hash, NL_CompareFunc cmp) {
    uint32_t h = hash(ptr);

    size_t mask = (1 << hs->exp) - 1;
    size_t first = h & mask, i = first;

    do {
        if (hs->data[i] == NULL) {
            break;
        } else if (hs->data[i] == ptr) {
            hs->data[i] = NL_HASHSET_TOMB;
            break;
        }

        i = (i + 1) & mask;
    } while (i != first);
}

void* nl_hashset_get2(NL_HashSet* restrict hs, void* ptr, NL_HashFunc hash, NL_CompareFunc cmp) {
    uint32_t h = hash(ptr);

    size_t mask = (1 << hs->exp) - 1;
    size_t first = h & mask, i = first;

    do {
        if (hs->data[i] == NULL) {
            return NULL;
        } else if (hs->data[i] == NL_HASHSET_TOMB) {
            // go past it
        } else if (hs->data[i] == ptr || cmp(hs->data[i], ptr)) {
            return hs->data[i];
        }

        i = (i + 1) & mask;
    } while (i != first);

    return NULL;
}

// returns old value
void* nl_hashset_put2(NL_HashSet* restrict hs, void* ptr, NL_HashFunc hash, NL_CompareFunc cmp) {
    uint32_t h = hash(ptr);

    size_t mask = (1 << hs->exp) - 1;
    size_t first = h & mask, i = first;

    do {
        if (hs->data[i] == NULL) {
            hs->count++;
            hs->data[i] = ptr;
            return NULL;
        } else if (hs->data[i] == NL_HASHSET_TOMB) {
            // go past it
        } else if (hs->data[i] == ptr || cmp(hs->data[i], ptr)) {
            return hs->data[i];
        }

        i = (i + 1) & mask;
    } while (i != first);

    NL_HashSet new_hs = nl_hashset_alloc(nl_hashset_capacity(hs));
    nl_hashset_for(p, hs) {
        nl_hashset_put2(&new_hs, *p, hash, cmp);
    }
    nl_hashset_free(*hs);
    *hs = new_hs;

    // "tail" calls amirite
    return nl_hashset_put2(hs, ptr, hash, cmp);
}

void nl_hashset_clear(NL_HashSet* restrict hs) {
    memset(hs->data, 0, nl_hashset_capacity(hs) * sizeof(void*));
    hs->count = 0;
}

#endif /* NL_HASH_SET_IMPL */
