#pragma once

struct vm_state_t;
typedef struct vm_state_t vm_state_t;

#include "vm.h"
#include "gc.h"
#include "thread.h"

struct vm_state_t
{
    vm_gc_t gc;
    vm_opcode_t *xops;
    void (*putchar)(vm_state_t *state, char chr);

    size_t index;
    size_t nops;
    const vm_opcode_t *ops; 
    
    vm_stack_frame_t *frames;
    vm_stack_frame_t *frame;
    
    vm_obj_t *globals;
    vm_obj_t *locals;

    ptrdiff_t gas;

    vm_pool_t *pool;
};

vm_state_t *vm_state_new(size_t n, const char *args[n]);
void vm_state_del(vm_state_t *state);
void vm_state_set_ops(vm_state_t *state, size_t n, const vm_opcode_t *ops);

