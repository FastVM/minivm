

#include "../build.h"
#include "../opt.h"

void vm_ir_opt_denop(size_t nops, vm_ir_block_t *blocks) {
  for (size_t i = 0; i < nops; i++) {
    vm_ir_block_t *block = &blocks[i];
    if (block->id != i) {
      continue;
    }
    size_t head = 0;
    for (size_t j = 0; j < block->len; j++) {
      if (block->instrs[j]->op != VM_IR_IOP_NOP) {
        block->instrs[head++] = block->instrs[j];
      }
    }
    block->len = head;
  }
}
