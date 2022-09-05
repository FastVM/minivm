
#if !defined(VM_HEADER_IR_OPT)
#define VM_HEADER_IR_OPT

#include "ir.h"

void vm_ir_opt_const(size_t *ptr_nops, vm_ir_block_t **ptr_blocks);
void vm_ir_opt_dead(size_t *ptr_nops, vm_ir_block_t **ptr_blocks);
void vm_ir_opt_reg(size_t nblocks, vm_ir_block_t *blocks);
void vm_ir_opt_denop(size_t nblocks, vm_ir_block_t *blocks);

static void vm_ir_opt_all(size_t *nops, vm_ir_block_t **blocks) {
  vm_ir_opt_const(nops, blocks);
  // vm_ir_opt_reg(*nops, *blocks);
  vm_ir_opt_dead(nops, blocks);
  vm_ir_opt_denop(*nops, *blocks);
}

#endif
