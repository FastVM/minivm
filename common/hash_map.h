#ifndef NL_HASH_MAP_H
#define NL_HASH_MAP_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>

#include "../bdwgc/private/gc/gc.h"

#define cuik_malloc(size)        GC_malloc(size)
#define cuik_calloc(count, size) GC_malloc((count) * (size))
#define cuik_free(ptr)           GC_free(ptr)
#define cuik_realloc(ptr, size)  GC_realloc(ptr, size)
#define cuik_strdup(x)           GC_strdup(x)

#define NL_Map(K, V) struct { K k; V v; }*
#define NL_Strmap(T) struct { NL_Slice k; T v; }*

typedef struct {
    size_t length;
    const uint8_t* data;
} NL_Slice;

/////////////////////////////////////////////////
// public macros
/////////////////////////////////////////////////
#define nl_map_is_strmap(map) _Generic((map)->k, NL_Slice: true, default: false)

#define nl_map_create(map, initial_cap) ((map) = ((void*) nl_map__alloc(initial_cap, sizeof(*map))->kv_table))

#define nl_map_put(map, key, value)                                                  \
do {                                                                                 \
    NL_MapInsert ins__ = (nl_map_is_strmap(map) ? nl_map__inserts : nl_map__insert)((map), sizeof(*(map)), sizeof(key), &(key)); \
    (map) = ins__.new_map;                                                           \
    (map)[ins__.index].v = (value);                                                  \
} while (0)

#define nl_map_puti(map, key, out_index)                                             \
do {                                                                                 \
    NL_MapInsert ins__ = (nl_map_is_strmap(map) ? nl_map__inserts : nl_map__insert)((map), sizeof(*(map)), sizeof(key), &(key)); \
    (map) = ins__.new_map;                                                           \
    (out_index) = ins__.index;                                                       \
} while (0)

#define nl_map_put_cstr(map, key, value)                                             \
do {                                                                                 \
    NL_Slice key_ = { strlen(key), (const uint8_t*) (key) };                         \
    NL_MapInsert ins__ = (nl_map_is_strmap(map) ? nl_map__inserts : nl_map__insert)((map), sizeof(*(map)), sizeof(key_), &key_); \
    (map) = ins__.new_map;                                                           \
    (map)[ins__.index].v = (value);                                                  \
} while (0)

#define nl_map_puti_cstr(map, key, out_index)                                        \
do {                                                                                 \
    NL_Slice key_ = { strlen(key), (const uint8_t*) (key) };                         \
    NL_MapInsert ins__ = (nl_map_is_strmap(map) ? nl_map__inserts : nl_map__insert)((map), sizeof(*(map)), sizeof(key_), &key_); \
    (map) = ins__.new_map;                                                           \
    (out_index) = ins__.index;                                                       \
} while (0)

#define nl_map_remove(map, key) ((map) != NULL ? nl_map__remove(map, sizeof(*map), sizeof(key), &(key)) : -1)

#define nl_map_get(map, key) ((map) != NULL ? (nl_map_is_strmap(map) ? nl_map__gets : nl_map__get)(((NL_MapHeader*)(map)) - 1, sizeof(*map), sizeof(key), &(key)) : -1)
#define nl_map_get_checked(map, key) ((map)[nl_map__check(nl_map_get(map, key))].v)
#define nl_map_get_cstr(map, key) ((map) != NULL ? nl_map__gets(((NL_MapHeader*)(map)) - 1, sizeof(*map), sizeof(NL_Slice), &(NL_Slice){ strlen(key), (const uint8_t*) (key) }) : -1)

#define nl_map_for_str(it, map) \
for (size_t it = 0; it < nl_map_get_capacity(map); it++) if ((map)[it].k.length != 0)

#define nl_map_for(it, map) \
for (size_t it = 0; it < nl_map_get_capacity(map); it++) if ((map)[it].k != 0 && (map)[it].k != (void*) (uintptr_t) -1)

#define nl_map_free(map) \
do {                                              \
    if ((map) != NULL) {                          \
        nl_map__free(((NL_MapHeader*)(map)) - 1); \
        (map) = NULL;                             \
    }                                             \
} while (0)

/////////////////////////////////////////////////
// internals
/////////////////////////////////////////////////
#define nl_map__get_header(map) (((NL_MapHeader*)(map)) - 1)
#define nl_map_get_capacity(map) ((map) ? 1ull << nl_map__get_header(map)->exp : 0)

typedef struct {
    void* new_map;
    size_t index;
} NL_MapInsert;

// behind the array the user manages there's some
// information about the rest of the string map
typedef struct {
    size_t count;
    size_t exp;
    char kv_table[];
} NL_MapHeader;

inline static NL_Slice nl_slice__cstr(const char* key) {
    return (NL_Slice){strlen(key), (const uint8_t*)key};
}

NL_MapHeader* nl_map__alloc(size_t cap, size_t entry_size);
NL_MapInsert nl_map__insert(void* map, size_t entry_size, size_t key_size, const void* key);
NL_MapInsert nl_map__inserts(void* map, size_t entry_size, size_t key_size, const void* key);
ptrdiff_t nl_map__get(NL_MapHeader* restrict table, size_t entry_size, size_t key_size, const void* key);
ptrdiff_t nl_map__gets(NL_MapHeader* restrict table, size_t entry_size, size_t key_size, const void* key);
void nl_map__free(NL_MapHeader* restrict table);
void nl_map__remove(void* map, size_t entry_size, size_t key_size, const void* key);

inline static ptrdiff_t nl_map__check(ptrdiff_t x) {
    assert(x >= 0 && "map entry not found!");
    return x;
}

#endif /* NL_HASH_MAP_H */

#ifdef NL_MAP_IMPL

// FNV1A
inline static uint32_t nl_map__raw_hash(size_t len, const void *key) {
    const uint8_t* data = key;
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++) {
        h = (data[i] ^ h) * 0x01000193;
    }

    return h;
}

void nl_map__free(NL_MapHeader* restrict table) {
    cuik_free(table);
}

NL_MapHeader* nl_map__alloc(size_t cap, size_t entry_size) {
    cap = (cap * 4) / 3;
    if (cap < 4) cap = 4;

    // next power of two
    #if defined(_MSC_VER) && !defined(__clang__)
    size_t exp = 64 - _lzcnt_u64(cap - 1);
    #else
    size_t exp = 64 - __builtin_clzll(cap - 1);
    #endif

    cap = (cap == 1 ? 1 : 1 << exp);

    NL_MapHeader* table = cuik_calloc(1, sizeof(NL_MapHeader) + (cap * entry_size));
    table->exp = exp;
    table->count = 0;
    return table;
}

static bool nl_map__is_zero(const char* ptr, size_t size) {
    // we're almost exclusively using this code for pointer keys
    if (size == sizeof(void*)) {
        return *((uintptr_t*) ptr) == 0;
    } else {
        for (size_t i = 0; i < size; i++) {
            if (ptr[i] != 0) return false;
        }

        return true;
    }
}

static bool nl_map__is_one(const char* ptr, size_t size) {
    // we're almost exclusively using this code for pointer keys
    if (size == sizeof(void*)) {
        return *((uintptr_t*) ptr) == UINTPTR_MAX;
    } else {
        for (size_t i = 0; i < size; i++) {
            if (ptr[i] != (char)0xFF) return false;
        }

        return true;
    }
}

NL_MapHeader* nl_map__rehash(NL_MapHeader* table, size_t entry_size, size_t key_size, bool is_strmap) {
    size_t count = 1u << table->exp;

    // Allocate bigger hashmap
    NL_MapHeader* new_table = nl_map__alloc(count * 2, entry_size);
    if (is_strmap) {
        for (size_t i = 0; i < count; i++) {
            NL_Slice* slot_entry = (NL_Slice*) &table->kv_table[i * entry_size];
            if (slot_entry->length > 0) {
                NL_MapInsert ins = nl_map__inserts(new_table->kv_table, entry_size, key_size, slot_entry);
                memcpy(&new_table->kv_table[ins.index*entry_size + key_size], &slot_entry[1], entry_size - key_size);
            }
        }
    } else {
        for (size_t i = 0; i < count; i++) {
            const char* slot_entry = (const char*) &table->kv_table[i * entry_size];
            if (!nl_map__is_zero(slot_entry, key_size) && !nl_map__is_one(slot_entry, key_size)) {
                NL_MapInsert ins = nl_map__insert(new_table->kv_table, entry_size, key_size, slot_entry);
                memcpy(&new_table->kv_table[ins.index*entry_size + key_size], &slot_entry[key_size], entry_size - key_size);
            }
        }
    }

    nl_map__free(table);
    return new_table;
}

NL_MapInsert nl_map__insert(void* map, size_t entry_size, size_t key_size, const void* key) {
    NL_MapHeader* table;
    if (map == NULL) {
        table = nl_map__alloc(1024, entry_size);
        map = table->kv_table;
    } else {
        table = ((NL_MapHeader*)map) - 1;
    }

    uint32_t cap = 1ull << table->exp;
    if (table->count >= (cap * 3) / 4) {
        // past 75% load... resize
        table = nl_map__rehash(table, entry_size, key_size, false);
        map = table->kv_table;
    }

    uint32_t exp = table->exp;
    uint32_t mask = (1 << table->exp) - 1;
    uint32_t hash = nl_map__raw_hash(key_size, key);

    for (size_t i = hash;;) {
        // hash table lookup
        uint32_t step = (hash >> (32 - exp)) | 1;
        i = (i + step) & mask;

        void* slot_entry = &table->kv_table[i * entry_size];
        if (nl_map__is_zero(slot_entry, key_size) || nl_map__is_one(slot_entry, key_size)) {
            table->count++;
            memcpy(slot_entry, key, key_size);
            return (NL_MapInsert){ map, i };
        } else if (memcmp(slot_entry, key, key_size) == 0) {
            return (NL_MapInsert){ map, i };
        }
    }
}

void nl_map__remove(void* map, size_t entry_size, size_t key_size, const void* key) {
    if (map == NULL) {
        return;
    }

    NL_MapHeader* table = ((NL_MapHeader*)map) - 1;

    uint32_t exp = table->exp;
    uint32_t mask = (1 << table->exp) - 1;
    uint32_t hash = nl_map__raw_hash(key_size, key);

    for (size_t i = hash;;) {
        // hash table lookup
        uint32_t step = (hash >> (32 - exp)) | 1;
        i = (i + step) & mask;

        void* slot_entry = &table->kv_table[i * entry_size];
        if (nl_map__is_zero(slot_entry, key_size)) {
            break;
        } else if (memcmp(slot_entry, key, key_size) == 0) {
            table->count--;
            memset(slot_entry, 0xFF, key_size); // mark key as TOMBSTONE
            break;
        }
    }
}

NL_MapInsert nl_map__inserts(void* map, size_t entry_size, size_t key_size, const void* key) {
    NL_MapHeader* table;
    if (map == NULL) {
        table = nl_map__alloc(256, entry_size);
        map = table->kv_table;
    } else {
        table = ((NL_MapHeader*)map) - 1;
    }

    uint32_t cap = 1ull << table->exp;
    if (table->count >= (cap * 3) / 4) {
        // past 75% load... resize
        table = nl_map__rehash(table, entry_size, key_size, true);
        map = table->kv_table;
    }

    const NL_Slice* key_entry = key;

    uint32_t exp = table->exp;
    uint32_t mask = (1 << table->exp) - 1;
    uint32_t hash = nl_map__raw_hash(key_entry->length, key_entry->data);

    for (size_t i = hash;;) {
        // hash table lookup
        uint32_t step = (hash >> (32 - exp)) | 1;
        i = (i + step) & mask;

        NL_Slice* slot_entry = (NL_Slice*) &table->kv_table[i * entry_size];
        if (slot_entry->length == 0) {
            table->count++;
            memcpy(slot_entry, key, key_size);
            return (NL_MapInsert){ map, i };
        } else if (slot_entry->length == key_entry->length && memcmp(slot_entry->data, key_entry->data, key_entry->length) == 0) {
            return (NL_MapInsert){ map, i };
        }
    }
}

ptrdiff_t nl_map__get(NL_MapHeader* restrict table, size_t entry_size, size_t key_size, const void* key) {
    if (table == NULL) return -1;

    uint32_t exp = table->exp;
    uint32_t mask = (1 << table->exp) - 1;
    uint32_t hash = nl_map__raw_hash(key_size, key);

    for (size_t i = hash;;) {
        // hash table lookup
        uint32_t step = (hash >> (32 - exp)) | 1;
        i = (i + step) & mask;

        void* slot_entry = &table->kv_table[i * entry_size];
        if (nl_map__is_zero(slot_entry, key_size)) {
            return -1;
        } else if (memcmp(slot_entry, key, key_size) == 0) {
            return i;
        }
    }
}

ptrdiff_t nl_map__gets(NL_MapHeader* restrict table, size_t entry_size, size_t key_size, const void* key) {
    if (table == NULL) return -1;

    const NL_Slice* key_entry = key;

    uint32_t exp = table->exp;
    uint32_t mask = (1 << table->exp) - 1;
    uint32_t hash = nl_map__raw_hash(key_entry->length, key_entry->data);

    for (size_t i = hash;;) {
        // hash table lookup
        uint32_t step = (hash >> (32 - exp)) | 1;
        i = (i + step) & mask;

        NL_Slice* slot_entry = (NL_Slice*) &table->kv_table[i * entry_size];
        if (slot_entry->length == 0) {
            return -1;
        } else if (slot_entry->length == key_entry->length && memcmp(slot_entry->data, key_entry->data, key_entry->length) == 0) {
            return i;
        }
    }
}

#endif /* NL_MAP_IMPL */
