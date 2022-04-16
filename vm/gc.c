#include "gc.h"

void vm_gc_init(vm_gc_t *restrict gc) {
    size_t alloc = 1 << 24;
    gc->used = 0;
    gc->free = NULL;
    gc->low = vm_alloc0(sizeof(vm_pair_t) * alloc);
    gc->high = &gc->low[alloc - 1];
    gc->marks = vm_alloc0(sizeof(uint8_t) * alloc);
    gc->count = 0;
    gc->maxcount = 1 << 12;
}

void vm_gc_deinit(vm_gc_t *restrict gc) {
    vm_free(gc->low);
    vm_free(gc->marks);
}

void *vm_gc_alloc(vm_gc_t *restrict gc) {
    if (gc->free != NULL) {
        vm_pair_t *ret = gc->free;
        gc->free = (vm_pair_t *) ret->second;
        gc->marks[ret - gc->low] = 1;
        return ret;
    }
    size_t nth = gc->used++;
    gc->marks[nth] = 1;
    return &gc->low[nth];
}

void vm_gc_dealloc(vm_gc_t *restrict gc, vm_pair_t *pair) {
    gc->marks[pair - gc->low] = 0;
    pair->second = (size_t) gc->free;
    gc->free = pair;
}

void vm_gc_mark(vm_gc_t *restrict gc, size_t val) {
    vm_pair_t *pval = (vm_pair_t *) val;
    if (gc->low <= pval && pval <= gc->high) {
        size_t nth = pval - gc->low;
        if (gc->marks[nth] == 1) {
            gc->marks[nth] = 2;
            vm_gc_mark(gc, pval->first);
            vm_gc_mark(gc, pval->second);
        }
    }
}

void vm_gc_collect(vm_gc_t *restrict gc, size_t nstack, size_t *stack) {
    gc->count++;
    if (gc->count >= gc->maxcount) {
        size_t count = 0;
        for (size_t i = 0; i < nstack; i++) {
            vm_gc_mark(gc, stack[i]);
        }
        for (size_t i = 0; i < gc->used; i++) {
            size_t mark = gc->marks[i];
            if (mark == 1) {
                vm_gc_dealloc(gc, &gc->low[i]);
            } else if (mark == 2) {
                gc->marks[i] = 1;
                count += 1;
            }
        }
        gc->count = 0;
        gc->maxcount = count;
    } 
}
