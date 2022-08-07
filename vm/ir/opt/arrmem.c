#include "../build.h"

static inline void vm_ir_opt_block(vm_ir_block_t *block)
{
    for (size_t i = 0; i < block->len; i++)
    {
        vm_ir_instr_t *instr = block->instrs[i];
    }
}

void vm_ir_opt_arrmem(size_t nblocks, vm_ir_block_t *blocks)
{
    for (size_t i = 0; i < nblocks; i++)
    {
        vm_ir_block_t *block = &blocks[i];
        if (block->id != i)
        {
            continue;
        }
        vm_ir_opt_block(block);
    }
}
