
#include "../opt.h"
#include "../build.h"

enum
{
    VM_IR_OPT_DEAD_REG_DEF,
    VM_IR_OPT_DEAD_REG_ARG,
    VM_IR_OPT_DEAD_REG_UNK,
};

void vm_ir_opt_dead(size_t *ptr_nops, vm_ir_block_t **ptr_blocks)
{
    vm_ir_block_t *blocks = *ptr_blocks;
    size_t nblocks = *ptr_nops;
    size_t regalloc = 0;
    for (size_t i = 0; i < nblocks; i++)
    {
        vm_ir_block_t *block = &blocks[i];
        if (block->id != i)
        {
            continue;
        }
        size_t nregs = 1;
        for (size_t j = 0; j < block->len; j++)
        {
            vm_ir_instr_t *instr = block->instrs[j];
            if (instr->out != NULL && instr->out->type == VM_IR_ARG_REG && instr->out->reg >= nregs)
            {
                nregs = instr->out->reg + 1;
            }
            for (size_t k = 0; instr->args[k] != NULL; k++)
            {
                vm_ir_arg_t *arg = instr->args[k];
                if (arg->type == VM_IR_ARG_REG && arg->reg >= nregs)
                {
                    nregs = arg->reg + 1;
                }
            }
        }
        for (size_t j = 0; j < 2; j++)
        {
            if (block->branch->args[j] != NULL && block->branch->args[j]->type == VM_IR_ARG_REG && block->branch->args[j]->reg >= nregs)
            {
                nregs = block->branch->args[j]->reg + 1;
            }
        }
        block->nregs = nregs;
        if (nregs >= regalloc)
        {
            regalloc = nregs;
        }
    }
    uint8_t *regs = vm_malloc(sizeof(uint8_t) * regalloc);
    for (size_t i = 0; i < nblocks; i++)
    {
        vm_ir_block_t *block = &blocks[i];
        if (block->id != i)
        {
            continue;
        }
        for (size_t j = 0; j < block->nregs; j++)
        {
            regs[j] = VM_IR_OPT_DEAD_REG_UNK;
        }
        size_t nargs = 0;
        for (size_t j = 0; j < block->len; j++)
        {
            vm_ir_instr_t *instr = block->instrs[j];
            for (size_t k = 0; instr->args[k] != NULL; k++)
            {
                vm_ir_arg_t *arg = instr->args[k];
                if (arg->type == VM_IR_ARG_REG && regs[arg->reg] == VM_IR_OPT_DEAD_REG_UNK)
                {
                    regs[arg->reg] = VM_IR_OPT_DEAD_REG_ARG;
                    nargs += 1;
                }
            }
            if (instr->out && instr->out->type == VM_IR_ARG_REG && regs[instr->out->reg] == VM_IR_OPT_DEAD_REG_UNK)
            {
                regs[instr->out->reg] = VM_IR_OPT_DEAD_REG_DEF;
            }
        }
        for (size_t j = 0; j < 2; j++)
        {
            if (block->branch->args[j] != NULL && block->branch->args[j]->type == VM_IR_ARG_REG && regs[block->branch->args[j]->reg] == VM_IR_OPT_DEAD_REG_UNK)
            {
                regs[block->branch->args[j]->reg] = VM_IR_OPT_DEAD_REG_ARG;
                nargs += 1;   
            }
        }
        block->nargs = 0;
        block->args = vm_malloc(sizeof(size_t) * nargs);
        for (size_t reg = 0; reg < block->nregs; reg++)
        {
            if (regs[reg] == VM_IR_OPT_DEAD_REG_ARG)
            {
                block->args[block->nargs++] = reg;
            }
        }
    }
    vm_free(regs);
}
