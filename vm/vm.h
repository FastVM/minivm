#pragma once

#include "opcode.h"

typedef uint64_t vm_loc_t;
typedef uint64_t vm_reg_t;
typedef uint64_t vm_uint_t;
typedef double vm_float_t;

union vm_value_t;
typedef union vm_value_t vm_value_t;
union vm_value_t {
  vm_uint_t u;
  vm_float_t f;
  vm_value_t *p;
};

typedef vm_value_t (*vm_func_t)(void *gc, vm_value_t* args);

int vm_run_arch_int(size_t nops, const vm_opcode_t *ops, const vm_func_t *funcs);
