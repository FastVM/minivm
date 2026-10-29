
#include "obj.h"
#include "string.h"
#include "../vm/gc.h"

mx_obj_closure_t *mx_obj_closure_new(mx_t *vm, mx_ir_block_t *block, uint32_t len) {
    mx_obj_closure_t *closure = mx_gc_alloc(vm, sizeof(mx_obj_closure_t) + sizeof(mx_obj_upval_t *) * len);
    closure->head.kind = MX_KIND_CLOSURE;
    closure->block = block;
    closure->len = len;
    for (uint32_t i = 0; i < len; i++) {
        closure->upvals[i] = NULL;
    }
    return closure;
}

mx_obj_upval_t *mx_obj_upval_new_closed(mx_t *vm, mx_obj_t value) {
    mx_obj_upval_t *uv = mx_gc_alloc(vm, sizeof(mx_obj_upval_t));
    uv->head.kind = vm->kinds.upval;
    uv->at = 0;
    uv->thread = NULL;
    uv->next = NULL;
    uv->kind_at = 0;
    uv->value = value;
    return uv;
}

mx_obj_t mx_obj_new_string(mx_t *vm, const char *str) {
    return mx_obj_of_string(mx_str_from_cstr_gc(vm, str));
}

mx_obj_t mx_obj_intern(mx_t *vm, const char *str) {
    return mx_obj_intern_strn(vm, str, (uint32_t)strlen(str));
}

mx_str_t *mx_intern_tomb(void) {
    return (mx_str_t *)1;
}

void mx_intern_grow(mx_t *vm) {
    uint32_t ncap = MX_INTERN_MIN_SLOTS;
    if (vm->interns.cap >= MX_INTERN_MIN_SLOTS) {
        ncap = vm->interns.cap * 2;
    }
    mx_str_t **nmap = mx_mem_calloc(sizeof(mx_str_t *) * ncap);
    for (uint32_t i = 0; i < vm->interns.cap; i++) {
        mx_str_t *b = vm->interns.slots[i];
        if (b != NULL && b != mx_intern_tomb()) {
            uint32_t j = b->hash & (ncap - 1);
            while (nmap[j] != NULL) {
                j = (j + 1) & (ncap - 1);
            }
            nmap[j] = b;
        }
    }
    mx_mem_free(vm->interns.slots);
    vm->interns.slots = nmap;
    vm->interns.cap = ncap;
    vm->interns.tombs = 0;
}

mx_obj_t mx_intern_insert(mx_t *vm, const char *bytes, uint32_t len) {
    if (vm->interns.cap == 0) {
        mx_intern_grow(vm);
    }
    uint32_t hash = mx_str_hashn(bytes, len);
    uint32_t mask = vm->interns.cap - 1;
    uint32_t i = hash & mask;
    uint32_t tomb = 0;
    bool has_tomb = false;
    while (vm->interns.slots[i] != NULL) {
        mx_str_t *b = vm->interns.slots[i];
        if (b == mx_intern_tomb()) {
            if (!has_tomb) {
                tomb = i;
                has_tomb = true;
            }
        } else if (b->hash == hash && b->len == len && memcmp(b->buf, bytes, len) == 0) {
            return mx_obj_of_string(b);
        }
        i = (i + 1) & mask;
    }
    if ((vm->interns.used + vm->interns.tombs + 1) * MX_INTERN_LOAD_DEN
        > vm->interns.cap * MX_INTERN_LOAD_NUM) {
        mx_intern_grow(vm);
        mask = vm->interns.cap - 1;
        i = hash & mask;
        while (vm->interns.slots[i] != NULL) {
            i = (i + 1) & mask;
        }
        has_tomb = false;
    }
    mx_str_t *buf = mx_str_new_gc_sized(vm, len);
    mx_str_append(buf, bytes, len);
    buf->hash = hash;
    buf->is_interned = true;
    if (vm->interns.young.len == vm->interns.young.cap) {
        vm->interns.young.cap = (vm->interns.young.len + 1) * 2;
        vm->interns.young.ptr = mx_mem_realloc(vm->interns.young.ptr, sizeof(mx_str_t *) * vm->interns.young.cap);
    }
    vm->interns.young.ptr[vm->interns.young.len] = buf;
    vm->interns.young.len += 1;
    if (has_tomb) {
        vm->interns.slots[tomb] = buf;
        vm->interns.tombs -= 1;
        vm->interns.used += 1;
    } else {
        vm->interns.slots[i] = buf;
        vm->interns.used += 1;
    }
    return mx_obj_of_string(buf);
}

mx_obj_t mx_obj_intern_strn(mx_t *vm, const char *bytes, uint32_t len) {
    return mx_intern_insert(vm, bytes, len);
}

void mx_intern_remove(mx_t *vm, mx_str_t *buf) {
    uint32_t mask = vm->interns.cap - 1;
    uint32_t i = buf->hash & mask;
    for (uint32_t j = 0; j < vm->interns.young.len; j++) {
        if (vm->interns.young.ptr[j] == buf) {
            vm->interns.young.ptr[j] = vm->interns.young.ptr[vm->interns.young.len - 1];
            vm->interns.young.len -= 1;
            break;
        }
    }
    bool removed = false;
    while (vm->interns.slots[i] != NULL && !removed) {
        if (vm->interns.slots[i] == buf) {
            vm->interns.slots[i] = mx_intern_tomb();
            vm->interns.used -= 1;
            vm->interns.tombs += 1;
            removed = true;
        } else {
            i = (i + 1) & mask;
        }
    }
}
