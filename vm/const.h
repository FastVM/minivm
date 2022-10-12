
#if !defined(VM_HEADER_IR_CONST)
#define VM_HEADER_IR_CONST

#include "ir.h"

void vm_opt_const(size_t nops, vm_block_t *blocks);
void vm_opt_dead(size_t nops, vm_block_t *blocks);

#endif
