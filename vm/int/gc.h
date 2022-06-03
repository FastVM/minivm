#pragma once

#include "../lib.h"
#include <gmp.h>

struct vm_gc_t;
typedef struct vm_gc_t vm_gc_t;

struct vm_gc_t {
    MP_INT *nums;
    uint32_t head;
    uint32_t alloc;
    uint8_t *marks;
    uint32_t *moves;
    uint32_t max;
};


void vm_gc_init(vm_gc_t *restrict out);
uint32_t vm_gc_num(vm_gc_t *restrict gc);
void vm_gc_stop(vm_gc_t gc);
void vm_gc_run(vm_gc_t *restrict gc, size_t nregs, void *regs);