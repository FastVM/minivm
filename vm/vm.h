#pragma once

#include "opcode.h"

union vm_value_t;
typedef union vm_value_t vm_value_t;
union vm_value_t {
  uint64_t u;
  double f;
  vm_value_t *p;
};

typedef vm_value_t (*vm_func_t)(void *gc, vm_value_t* args);

int vm_run_arch_int(size_t nops, const vm_opcode_t *ops, const vm_func_t *funcs);
