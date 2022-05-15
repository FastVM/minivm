#pragma once

#include "opcode.h"

int vm_run_arch_int(size_t nops, const vm_opcode_t *ops);
