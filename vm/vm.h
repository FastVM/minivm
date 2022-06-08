#pragma once

#include "opcode.h"

typedef int64_t vm_loc_t;
typedef int64_t vm_reg_t;

#include "int/gc.h"

int vm_run_arch_int(size_t nops, const vm_opcode_t *ops);
