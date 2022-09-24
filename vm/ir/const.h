
#if !defined(VM_HEADER_IR_CONST)
#define VM_HEADER_IR_CONST

#include "ir.h"

void vm_ir_opt_const(size_t nops, vm_ir_block_t *blocks);
void vm_ir_opt_dead(size_t nops, vm_ir_block_t *blocks);

#endif
