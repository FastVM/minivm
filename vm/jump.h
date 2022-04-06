#pragma once

#include "opcode.h"

enum vm_jump_scanned {
  VM_JUMP_IN = 1,
  VM_JUMP_OUT = 2, 
  VM_JUMP_INIT = 4,
  VM_JUMP_REACH = 8,
};

void vm_jump_reachable_from(size_t index, size_t nops, const vm_opcode_t *ops, uint8_t *jumps);
void vm_jump_reachable(size_t nops, const vm_opcode_t *ops, uint8_t *jumps);

uint8_t *vm_jump_base(size_t nops, const vm_opcode_t *ops);
uint8_t *vm_jump_all(size_t nops, const vm_opcode_t *ops);
