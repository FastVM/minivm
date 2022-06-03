#include "gc.h"
#include "../value.h"

void vm_gc_init(vm_gc_t *restrict out) {
    out->alloc = 0;
    out->nums = NULL;
    out->moves = NULL;
    out->marks = NULL;
    out->head = 0;
    out->max = VM_GC_MIN;
}

uint32_t vm_gc_num(vm_gc_t *restrict gc) {
    uint32_t next_head = gc->head + 1;
    if (gc->alloc <= next_head) {
        uint32_t next_alloc = next_head * 2;
        gc->nums = vm_realloc(gc->nums, sizeof(MP_INT) * next_alloc);
        gc->moves = vm_realloc(gc->moves, sizeof(uint32_t) * next_alloc);
        gc->marks = vm_realloc(gc->marks, sizeof(uint8_t) * next_alloc);
        for (uint32_t i = gc->alloc; i < next_alloc; i++) {
            mpz_init(&gc->nums[i]);
            gc->marks[i] = 0;
        }
        gc->alloc = next_alloc;
    }
    return gc->head++;
}

void vm_gc_run(vm_gc_t *restrict gc, size_t nregs, void *vregs) {
    vm_value_t *regs = vregs;
    for (uint32_t i = 0; i < nregs; i++) {
        vm_value_t reg = regs[i];
        if (VM_VALUE_IS_BIGINT(reg))  {
            uint32_t nth = VM_VALUE_GET_INT64(reg);
            if (nth < gc->alloc) {
                if (gc->marks[nth] == 0) {
                    gc->marks[nth] = 1;
                }
            }
        }
    }
    uint32_t used = 0;
    for (uint32_t i = 0; i < gc->alloc; i++) {
        if (gc->marks[i] != 0) {
            gc->marks[i] = 0;
            if (i != used) {
                MP_INT tmp = gc->nums[used];
                gc->nums[used] = gc->nums[i];
                gc->nums[i] = tmp;
            }
            gc->moves[i] = used;
            used += 1;
        }
    }
    for (uint32_t i = 0; i < nregs; i++) {
        vm_value_t reg = regs[i];
        if (VM_VALUE_IS_BIGINT(reg)) {
            uint32_t nth = VM_VALUE_GET_INT64(reg);
            if (nth < gc->alloc) {
                regs[i] = VM_VALUE_SET_BIGINT(gc->moves[nth]);
            }
        }
    }
    gc->head = used;
    gc->max = gc->alloc - (gc->alloc >> 3);
    if (gc->max < VM_GC_MIN) {
        gc->max = VM_GC_MIN;
    }
}

void vm_gc_stop(vm_gc_t gc) {
    for (size_t i = 0; i < gc.alloc; i++) {
        mpz_clear(&gc.nums[i]);
    }
    vm_free(gc.marks);
    vm_free(gc.moves);
    vm_free(gc.nums);
}