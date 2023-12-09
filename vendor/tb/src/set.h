#pragma once
#include "common.h"
#include "builtins.h"

#define SET_INITIAL_CAP 32

typedef struct Set {
    size_t capacity;
    uint64_t* data;
} Set;

static Set set_create_in_arena(TB_Arena* arena, size_t cap) {
    void* ptr = tb_arena_alloc(arena, ((cap + 63) / 64) * sizeof(uint64_t));
    memset(ptr, 0, ((cap + 63) / 64) * sizeof(uint64_t));

    return (Set){
        .capacity = cap,
        .data = ptr,
    };
}

static Set set_create(size_t cap) {
    void* ptr = tb_platform_heap_alloc(((cap + 63) / 64) * sizeof(uint64_t));
    memset(ptr, 0, ((cap + 63) / 64) * sizeof(uint64_t));

    return (Set){
        .capacity = cap,
        .data = ptr,
    };
}

static void set_free(Set* s) {
    tb_platform_heap_free(s->data);
}

static void set_clear(Set* s) {
    memset(s->data, 0, ((s->capacity + 63) / 64) * sizeof(uint64_t));
}

// return true if changes
static bool set_union(Set* dst, Set* src) {
    assert(dst->capacity >= src->capacity);
    size_t n = (src->capacity + 63) / 64;

    uint64_t changes = 0;
    FOREACH_N(i, 0, n) {
        uint64_t old = dst->data[i];
        uint64_t new = old | src->data[i];

        dst->data[i] = new;
        changes |= (old ^ new);
    }
    return changes;
}

static bool set_equals(Set* dst, Set* src) {
    assert(dst->capacity == src->capacity);
    size_t n = (dst->capacity + 63) / 64;

    FOREACH_N(i, 0, n) {
        if (dst->data[i] != src->data[i]) return false;
    }

    return true;
}

static void set_copy(Set* dst, Set* src) {
    assert(dst->capacity >= src->capacity);
    memcpy(dst->data, src->data, ((src->capacity + 63) / 64) * sizeof(uint64_t));
}

static size_t set_popcount(Set* s) {
    size_t n = (s->capacity + 63) / 64;

    size_t sum = 0;
    FOREACH_N(i, 0, n) {
        sum += tb_popcount64(s->data[i]);
    }

    return sum;
}

// Grabs a free slot
static ptrdiff_t set_pop_any(Set* s) {
    size_t slots = (s->capacity + 63) / 64;

    for (size_t i = 0; i < slots; i++) {
        if (s->data[i] == UINT64_MAX) continue;

        int index = s->data[i] != 0 ? tb_ffs64(~s->data[i]) - 1 : 0;
        if (i*64 + index >= s->capacity) continue;

        s->data[i] |= (1ull << index);
        return i*64 + index;
    }

    return -1;
}

static bool set_first_time(Set* s, size_t index) {
    size_t slots = (s->capacity + 63) / 64;
    lldiv_t d = lldiv(index, 64);

    if (d.quot >= slots) {
        s->capacity = index * 2;
        size_t new_slots = (s->capacity + 63) / 64;

        s->data = realloc(s->data, new_slots * sizeof(uint64_t));
        if (s->data == NULL) {
            fprintf(stderr, "TB error: Set out of memory!");
            abort();
        }

        memset(s->data + slots, 0, (new_slots - slots) * sizeof(uint64_t));
    }

    if ((s->data[d.quot] & (1ull << d.rem)) == 0) {
        s->data[d.quot] |= (1ull << d.rem);
        return true;
    }

    return false;
}

static void set_put(Set* s, size_t index) {
    size_t quot = index / 64;
    size_t rem  = index % 64;

    if (quot >= s->capacity) {
        size_t old = s->capacity;

        s->capacity = quot * 2;
        s->data = realloc(s->data, s->capacity * sizeof(uint64_t));
        if (s->data == NULL) {
            fprintf(stderr, "TB error: Set out of memory!");
            abort();
        }

        memset(s->data + old, 0, (s->capacity - old) * sizeof(uint64_t));
    }

    s->data[quot] |= (1ull << rem);
}

static void set_remove(Set* s, size_t index) {
    size_t quot = index / 64;
    size_t rem  = index % 64;
    if (quot < s->capacity) {
        s->data[quot] &= ~(1ull << rem);
    }
}

static bool set_get(Set* s, size_t index) {
    size_t quot = index / 64;
    size_t rem  = index % 64;
    if (quot >= s->capacity) {
        return false;
    }

    return s->data[quot] & (1ull << rem);
}
