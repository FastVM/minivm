
#include "scratch.h"

void mx_scratch_init(mx_t *vm) {
    vm->scratch = mx_mem_calloc(sizeof(mx_scratch_t));
}

void mx_scratch_free(mx_t *vm) {
    mx_mem_free(vm->scratch);
    vm->scratch = NULL;
}

uint32_t mx_scratch_hash(const char *str, size_t len) {
    uint32_t h = MX_VM_FNV_OFFSET;
    for (size_t i = 0; i < len; i++) {
        h = (h ^ (uint8_t)str[i]) * MX_VM_FNV_PRIME;
    }
    return h;
}

void mx_scratch_push(mx_t *vm, mx_arena_t *arena) {
    const char **slots = mx_arena_alloc(arena, sizeof(const char *) * 1024);
    memset(slots, 0, sizeof(const char *) * 1024);
    vm->scratch->arena = arena;
    vm->scratch->intern = (mx_scratch_intern_t) {
        .slots = slots,
        .hashes = mx_arena_alloc(arena, sizeof(uint32_t) * 1024),
        .used = 0,
        .cap = 1024,
    };
}

void mx_scratch_pop(mx_t *vm) {
    mx_arena_free(&vm->scratch->temp);
    *vm->scratch = (mx_scratch_t) {0};
}

const char *mx_scratch_intern_range(mx_t *vm, const char *str, size_t len) {
    mx_scratch_t *c = vm->scratch;
    uint32_t hash = mx_scratch_hash(str, len);
    size_t mask = c->intern.cap - 1;
    size_t i = hash & mask;
    while (c->intern.slots[i] != NULL) {
        const char *have = c->intern.slots[i];
        if (c->intern.hashes[i] == hash && strncmp(have, str, len) == 0 && have[len] == '\0') {
            return have;
        }
        i = (i + 1) & mask;
    }
    char *copy = mx_arena_alloc(c->arena, len + 1);
    memcpy(copy, str, len);
    copy[len] = '\0';
    c->intern.slots[i] = copy;
    c->intern.hashes[i] = hash;
    c->intern.used += 1;
    if (c->intern.used * 4 > c->intern.cap * 3) {
        size_t cap = c->intern.cap * 2;
        const char **slots = mx_arena_alloc(c->arena, sizeof(const char *) * cap);
        memset(slots, 0, sizeof(const char *) * cap);
        uint32_t *hashes = mx_arena_alloc(c->arena, sizeof(uint32_t) * cap);
        for (size_t k = 0; k < c->intern.cap; k++) {
            if (c->intern.slots[k] != NULL) {
                size_t at = c->intern.hashes[k] & (cap - 1);
                while (slots[at] != NULL) {
                    at = (at + 1) & (cap - 1);
                }
                slots[at] = c->intern.slots[k];
                hashes[at] = c->intern.hashes[k];
            }
        }
        c->intern = (mx_scratch_intern_t) {
            .slots = slots,
            .hashes = hashes,
            .used = c->intern.used,
            .cap = cap,
        };
    }
    return copy;
}

const char *mx_scratch_intern(mx_t *vm, const char *str) {
    return mx_scratch_intern_range(vm, str, strlen(str));
}

void *mx_scratch_alloc(mx_t *vm, size_t size) {
    return mx_arena_alloc(&vm->scratch->temp, size);
}

mx_arena_mark_t mx_scratch_mark(mx_t *vm) {
    return mx_arena_mark(&vm->scratch->temp);
}

void mx_scratch_release(mx_t *vm, mx_arena_mark_t mark) {
    mx_arena_release(&vm->scratch->temp, mark);
}
