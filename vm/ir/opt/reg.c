
#include "../opt.h"
#include "../build.h"

void vm_ir_opt_reg(size_t *ptr_nops, vm_ir_block_t **ptr_blocks)
{
    size_t nops = *ptr_nops;
    vm_ir_block_t *blocks = *ptr_blocks;
    for (size_t i = 0; i < nops; i++)
    {
        vm_ir_block_t *block = &blocks[i];
        if (block->id != i)
        {
            continue;
        }
        vm_ir_print_block(stderr, block);
    }
}
