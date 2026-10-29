
#pragma once

#if !defined(MX_HEADER_IR_SCRATCH)
#define MX_HEADER_IR_SCRATCH

#include "../vm/vm.h"

typedef struct mx_scratch_intern_t mx_scratch_intern_t;

struct mx_scratch_intern_t {
    const char **slots;
    uint32_t *hashes;
    size_t used;
    size_t cap;
};

struct mx_scratch_t {
    mx_arena_t *arena;
    mx_arena_t temp;
    mx_scratch_intern_t intern;
};

void mx_scratch_init(mx_t *vm);
void mx_scratch_free(mx_t *vm);
uint32_t mx_scratch_hash(const char *str, size_t len);
void mx_scratch_push(mx_t *vm, mx_arena_t *arena);
void mx_scratch_pop(mx_t *vm);
void *mx_scratch_alloc(mx_t *vm, size_t size);
mx_arena_mark_t mx_scratch_mark(mx_t *vm);
void mx_scratch_release(mx_t *vm, mx_arena_mark_t mark);
const char *mx_scratch_intern_range(mx_t *vm, const char *str, size_t len);
const char *mx_scratch_intern(mx_t *vm, const char *str);

#endif
