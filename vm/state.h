#pragma once

struct vm_state_t;
typedef struct vm_state_t vm_state_t;

#include "vm.h"
#include "gc.h"

struct vm_state_t
{
    vm_gc_t *gc;
    vm_gc_entry_t *global;
    void (*putchar)(vm_state_t *state, char chr);
};

void vm_run(vm_state_t *state, size_t len, const vm_opcode_t *mem);
vm_state_t *vm_state_new(void);
void vm_state_del(vm_state_t *);

