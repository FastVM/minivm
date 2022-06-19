
#include "build.h"

enum
{
    VM_IR_INFO_REG_DEF,
    VM_IR_INFO_REG_ARG,
    VM_IR_INFO_REG_UNK,
};

void vm_ir_info(size_t *ptr_nops, vm_ir_block_t **ptr_blocks)
{
    vm_ir_block_t *blocks = *ptr_blocks;
    size_t nblocks = *ptr_nops;
    size_t regalloc = 0;
    uint8_t **all_regs = vm_malloc(sizeof(size_t *) * nblocks);
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
    for (size_t i = 0; i < nblocks; i++)
    {
        vm_ir_block_t *block = &blocks[i];
        if (block->id != i)
        {
            continue;
        }
        uint8_t *regs = vm_malloc(sizeof(uint8_t) * regalloc);
        all_regs[i] = regs;
        for (size_t j = 0; j < block->nregs; j++)
        {
            regs[j] = VM_IR_INFO_REG_UNK;
        }
        size_t nargs = 0;
        for (size_t j = 0; j < block->len; j++)
        {
            vm_ir_instr_t *instr = block->instrs[j];
            for (size_t k = 0; instr->args[k] != NULL; k++)
            {
                vm_ir_arg_t *arg = instr->args[k];
                if (arg->type == VM_IR_ARG_REG && regs[arg->reg] == VM_IR_INFO_REG_UNK)
                {
                    regs[arg->reg] = VM_IR_INFO_REG_ARG;
                    nargs += 1;
                }
            }
            if (instr->out && instr->out->type == VM_IR_ARG_REG && regs[instr->out->reg] == VM_IR_INFO_REG_UNK)
            {
                regs[instr->out->reg] = VM_IR_INFO_REG_DEF;
            }
        }
        for (size_t j = 0; j < 2; j++)
        {
            if (block->branch->args[j] != NULL && block->branch->args[j]->type == VM_IR_ARG_REG && regs[block->branch->args[j]->reg] == VM_IR_INFO_REG_UNK)
            {
                regs[block->branch->args[j]->reg] = VM_IR_INFO_REG_ARG;
                nargs += 1;   
            }
        }
        block->nargs = 0;
        block->args = vm_malloc(sizeof(size_t) * nargs);
        for (size_t reg = 0; reg < block->nregs; reg++)
        {
            if (regs[reg] == VM_IR_INFO_REG_ARG)
            {
                block->args[block->nargs++] = reg;
            }
        }
    }
    redo:
    for (size_t i = 0; i < nblocks; i++)
    {
        vm_ir_block_t *block = &blocks[i];
        if (block->id != i)
        {
            continue;
        }
        for (size_t t = 0; t < 2; t++)
        {
            vm_ir_block_t *target = block->branch->targets[t];
            if (target == NULL)
            {
                continue;
            }
            size_t total = block->nargs + target->nargs;
            size_t *next = vm_malloc(sizeof(size_t) * total);
            size_t nargs = 0;
            size_t bi = 0;
            size_t ti = 0;
            for (;;)
            {
                if (bi == block->nargs)
                {
                    while (ti < target->nargs)
                    {
                        size_t newreg = target->args[ti++];
                        if (all_regs[i][newreg] != VM_IR_INFO_REG_DEF)
                        {
                            next[nargs++] = newreg;
                        }
                    }
                    break;    
                }
                else if (ti == target->nargs)
                {
                    while (bi < block->nargs)
                    {
                        next[nargs++] = block->args[bi++];
                    }
                    break;    
                }
                else if (block->args[bi] == target->args[ti])
                {
                    next[nargs++] = block->args[bi++];
                    ti += 1;
                }
                else if (block->args[bi] > target->args[ti])
                {
                    size_t newreg = target->args[ti++];
                    if (all_regs[i][newreg] != VM_IR_INFO_REG_DEF)
                    {
                        next[nargs++] = newreg;
                    }
                }
                else if (block->args[bi] < target->args[ti])
                {
                    next[nargs++] = block->args[bi++];
                }
            }
            if (nargs != block->nargs)
            {
                vm_free(block->args);
                block->args = next;
                block->nargs = nargs;
                goto redo;
            }
            else
            {
                vm_free(next);
            }
        }
    }
    for (size_t i = 0; i < nblocks; i++)
    {
        vm_ir_block_t *block = &blocks[i];
        if (block->id != i)
        {
            continue;
        }
        vm_free(all_regs[i]);
    }
    vm_free(all_regs);
}
