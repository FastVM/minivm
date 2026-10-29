
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
    mx_scratch_t *c = vm->scratch;
    c->arena = arena;
    c->intern.cap = 1024;
    c->intern.used = 0;
    c->intern.slots = mx_arena_alloc(arena, sizeof(const char *) * c->intern.cap);
    memset(c->intern.slots, 0, sizeof(const char *) * c->intern.cap);
    c->intern.hashes = mx_arena_alloc(arena, sizeof(uint32_t) * c->intern.cap);
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
        size_t ocap = c->intern.cap;
        const char **optr = c->intern.slots;
        uint32_t *ohash = c->intern.hashes;
        c->intern.cap = ocap * 2;
        c->intern.slots = mx_arena_alloc(c->arena, sizeof(const char *) * c->intern.cap);
        memset(c->intern.slots, 0, sizeof(const char *) * c->intern.cap);
        c->intern.hashes = mx_arena_alloc(c->arena, sizeof(uint32_t) * c->intern.cap);
        size_t nmask = c->intern.cap - 1;
        for (size_t k = 0; k < ocap; k++) {
            if (optr[k] != NULL) {
                size_t at = ohash[k] & nmask;
                while (c->intern.slots[at] != NULL) {
                    at = (at + 1) & nmask;
                }
                c->intern.slots[at] = optr[k];
                c->intern.hashes[at] = ohash[k];
            }
        }
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
