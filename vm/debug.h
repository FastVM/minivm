#pragma once

#include <vm/vm.h>

const char *vm_opcode_internal_name(opcode_t op);
const char *vm_opcode_name(opcode_t op);
const char *vm_opcode_format(opcode_t op);
void vm_print_opcode(int index, opcode_t *bytecode);