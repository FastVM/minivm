
#include "../build.h"
#include "../ir.h"

void vm_ir_opt_dead(size_t *ptr_nops, vm_ir_block_t **ptr_blocks)
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
        uint8_t *ptrs = vm_alloc0(sizeof(uint8_t) * block->nregs);
        for (size_t t = 0; t < 2; t++)
        {
            if (block->branch->targets[t] == NULL)
            {
                break;
            }
            for (size_t j = 0; j < block->branch->targets[t]->nargs; j++)
            {
                ptrs[block->branch->targets[t]->args[j]] = 1;
            }
        }
        for (size_t r = 0; r < 2; r++)
        {
            if (block->branch->args[r] != NULL && block->branch->args[r]->type == VM_IR_ARG_REG)
            {
                ptrs[block->branch->args[r]->reg] = 1;
            }
        }
        for (ptrdiff_t j = block->len - 1; j >= 0; j--)
        {
            vm_ir_instr_t *instr = block->instrs[j];
            uint8_t outp = 1;
            if (instr->out != NULL && instr->out->type == VM_IR_ARG_REG)
            {
                outp = ptrs[instr->out->reg];
                if (outp == 0 && instr->op != VM_IR_IOP_CALL)
                {
                    block->instrs[j] = vm_ir_new(vm_ir_instr_t, .op = VM_IR_IOP_NOP);
                }
                ptrs[instr->out->reg] = 0;
            }
            if (instr->op == VM_IR_IOP_NOP)
            {
                continue;
            }
            for (size_t k = 0; instr->args[k] != NULL; k++)
            {
                if (instr->args[k]->type != VM_IR_ARG_REG)
                {
                    continue;
                }
                ptrs[instr->args[k]->reg] = 1;
            }
        }
        vm_free(ptrs);
    }
}