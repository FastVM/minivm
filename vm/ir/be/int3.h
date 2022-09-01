#if !defined(VM_HEADER_IR_BE_INT3)
#define VM_HEADER_IR_BE_INT3

#include "../ir.h"
#include "../../opcode.h"

void vm_ir_be_int3(size_t nblocks, vm_ir_block_t *blocks);
void vm_run_arch_int(size_t nops, vm_opcode_t *opcodes);

#endif
