#pragma once

struct vm_state_t;
typedef struct vm_state_t vm_state_t;

#include "gc.h"
#include "vm.h"

struct vm_state_t {
  vm_gc_t gc;

  size_t index;
  size_t nops;
  const vm_opcode_t *ops;

  vm_stack_frame_t *frames;
  size_t framenum;

  vm_obj_t *globals;
  size_t nlocals;
};

vm_state_t *vm_state_new(size_t n, const char *args[n]);
void vm_state_del(vm_state_t *state);
void vm_state_set_ops(vm_state_t *state, size_t n, const vm_opcode_t *ops);
