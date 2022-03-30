#pragma once

#include "opcode.h"

int vm_run(size_t nops, const vm_opcode_t *ops);
