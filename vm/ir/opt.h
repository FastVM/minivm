
#if !defined(VM_HEADER_IR_OPT)
#define VM_HEADER_IR_OPT

#include "ir.h"

void vm_ir_opt_const(size_t *nops, vm_ir_block_t **block);
void vm_ir_opt_dead(size_t *nops, vm_ir_block_t **block);

static void vm_ir_opt_all(size_t *nops, vm_ir_block_t **blocks)
{
    vm_ir_opt_const(nops, blocks);
    vm_ir_opt_dead(nops, blocks);
}

#endif
