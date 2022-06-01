#pragma once

#include "lib.h"
#include "opcode.h"

int vm_reg_is_used(size_t nops, const vm_opcode_t *ops, uint8_t *jumps, size_t index, size_t reg, size_t nbuf, size_t* buf, size_t head);