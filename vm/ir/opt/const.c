
#include "../opt.h"
#include "../build.h"

enum
{
    VM_IR_OPT_CONST_REG_NOT_SET,
    VM_IR_OPT_CONST_REG_NOT_NEEDED,
    VM_IR_OPT_CONST_REG_NEEDED,
    VM_IR_OPT_CONST_REG_HAS_VALUE,
};

void vm_ir_opt_const(size_t *ptr_nops, vm_ir_block_t **ptr_blocks)
{
    size_t nops = *ptr_nops;
    vm_ir_block_t *blocks = *ptr_blocks;
    for (size_t i = 0; i < nops; i++)
    {
        vm_ir_block_t *block = &blocks[i];
        if (block->id == i)
        {
            uint8_t *named = vm_alloc0(sizeof(uint8_t) * block->nregs);
            ptrdiff_t *regs = vm_alloc0(sizeof(ptrdiff_t) * block->nregs);
            for (size_t j = 0; j < block->len; j++)
            {
                vm_ir_instr_t *instr = block->instrs[j];
                for (size_t k = 0; instr->args[k] != NULL; k++)
                {
                    vm_ir_arg_t *arg = instr->args[k];
                    if (arg->type == VM_IR_ARG_REG)
                    {
                        if (named[arg->reg] == VM_IR_OPT_CONST_REG_HAS_VALUE)
                        {
                            arg->type = VM_IR_ARG_NUM;
                            arg->num = regs[arg->reg];
                        }
                        else if (named[arg->reg] == VM_IR_OPT_CONST_REG_NOT_NEEDED)
                        {
                            named[arg->reg] = VM_IR_OPT_CONST_REG_NEEDED;
                        }
                    }
                }
                if (instr->out != NULL)
                {
                    vm_ir_arg_t *out = instr->out;
                    named[out->reg] = VM_IR_OPT_CONST_REG_NOT_NEEDED;
                    if (instr->op == VM_IR_IOP_MOVE)
                    {
                        vm_ir_arg_t *arg0 = instr->args[0];
                        if (arg0->type == VM_IR_ARG_NUM)
                        {
                            named[out->reg] = VM_IR_OPT_CONST_REG_HAS_VALUE;
                            regs[out->reg] = arg0->num;
                        }
                    }
                }
            }
            vm_free(named);
            vm_free(regs);
        }
    }
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
            for (size_t k = 0; instr->args[k] != NULL; k++)
            {
                if (instr->args[k]->type != VM_IR_ARG_REG)
                {
                    continue;
                }
                ptrs[instr->args[k]->reg] = 1;
            }
            if (instr->out != NULL && instr->out->type == VM_IR_ARG_REG)
            {
                if (ptrs[instr->out->reg] == 0 && instr->op != VM_IR_IOP_CALL)
                {
                    instr->op = VM_IR_IOP_NOP;
                }
            }
        }
        vm_free(ptrs);
    }
}
