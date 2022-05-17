#pragma once

#include "opcode.h"

union vm_value_t;
typedef union vm_value_t vm_value_t;
union vm_value_t {
  uint64_t u;
  int64_t i;
  double f;
  vm_value_t *p;
};

int vm_run_arch_int(size_t nops, const vm_opcode_t *ops);
