#include "builtins.h"

enum {
    MAX_SLOTS = 508
};

typedef struct PoolHeader {
    // population count
    uint64_t used;
    // each bit represents an allocated
    uint64_t allocated[MAX_SLOTS];
    struct PoolHeader* next;

    // in 32bit builds, this would be padded so
    // regardless the header is 4096 bytes
    char data[];
} PoolHeader;

inline static void* pool__alloc_slot(void** ptr, size_t type_size) {
    // find the slot which isn't just filled
    PoolHeader* hdr = *ptr ? ((PoolHeader*) *ptr) - 1 : NULL;
    while (hdr != NULL && hdr->used >= MAX_SLOTS) hdr = hdr->next;

    // if it's null allocate it
    if (hdr == NULL) {
        hdr = tb_platform_valloc(sizeof(PoolHeader) + (MAX_SLOTS * type_size));
        hdr->allocated[0] = 1;
        hdr->used = 1;

        // if it's NULL then it's the first element and thus
        // we wanna actually store it back to the pointer
        if (*ptr == NULL) {
            *ptr = hdr->data;
        }

        return hdr->data;
    }

    for (size_t i = 0; i < MAX_SLOTS; i++) {
        if (hdr->allocated[i] == UINT64_MAX) continue;

        // find some unallocated slot
        int index = hdr->allocated[i] ? tb_ffs64(~hdr->allocated[i]) - 1 : 0;

        hdr->allocated[i] |= (1ull << index);
        hdr->used += 1;

        return &hdr->data[((i * 64) + index) * type_size];
    }

    assert(0 && "TODO: implement the rest of the pool allocator");
    return NULL;
}

inline static void pool__destroy(void** ptr, size_t type_size) {
    if (*ptr == NULL) return;

    PoolHeader* hdr = ((PoolHeader*) *ptr) - 1;
    while (hdr != NULL) {
        PoolHeader* next = hdr->next;
        tb_platform_vfree(hdr, sizeof(PoolHeader) + (MAX_SLOTS * type_size));
        hdr = next;
    }
    *ptr = NULL;
}

inline static size_t pool_popcount(void* ptr) {
    if (ptr == NULL) {
        return 0;
    }

    size_t c = 0;
    for (PoolHeader* hdr = ((PoolHeader*) ptr) - 1; hdr != NULL; hdr = hdr->next) {
        c += tb_popcount64(hdr->used);
    }
    return c;
}

#define Pool(T) T*

#define pool_put(p) \
pool__alloc_slot((void**) &(p), sizeof(*(p)))

#define pool_destroy(p) pool__destroy((void**) &(p), sizeof(*(p)))

#define pool_for(T, it, p) \
for (PoolHeader *p_ = (PoolHeader*)(p), *hdr_ = p_ ? (p_ - 1) : NULL; hdr_; hdr_ = hdr_->next) \
for (size_t a_ = 0; a_ < MAX_SLOTS; a_++) \
if (hdr_->allocated[a_] != 0) \
for (size_t b_ = 0; b_ < 64; b_++) \
if (hdr_->allocated[a_] & (1ull << b_)) \
for (T *it = (T*) &hdr_->data[((a_ * 64) + b_) * sizeof(T)]; it; it = NULL)
