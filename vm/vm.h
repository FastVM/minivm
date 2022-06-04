#pragma once

#include "opcode.h"

typedef uint64_t vm_loc_t;
typedef uint64_t vm_reg_t;

#include "gc.h"

int vm_run_arch_int(size_t nops, const vm_opcode_t *ops);
