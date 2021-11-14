#pragma once

struct vm_state_t;
typedef struct vm_state_t vm_state_t;

#include "vm.h"
#include "gc.h"

struct vm_state_t
{
    vm_gc_t *gc;
    vm_obj_t global;
    void (*putchar)(vm_state_t *state, char chr);
};

void vm_run(vm_state_t *state, const vm_opcode_t *mem);
vm_state_t *vm_state_new(size_t n, const char *args[n]);
void vm_state_del(vm_state_t *);

