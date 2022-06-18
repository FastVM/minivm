
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
    size_t regalloc = 16;
    uint8_t *named = vm_malloc(sizeof(uint8_t) * regalloc);
    size_t *where = vm_malloc(sizeof(size_t) * nops);
    ptrdiff_t *regs = vm_malloc(sizeof(ptrdiff_t) * regalloc);
    for (size_t i = 0; i < nops; i++)
    {
        vm_ir_block_t *block = &blocks[i];
        if (block->id == i)
        {
            for (size_t j = 0; j < regalloc; j++)
            {
                named[j] = VM_IR_OPT_CONST_REG_NOT_SET;
            }
            size_t reguse = 0;
            for (size_t j = 0; j < block->len; j++)
            {
                vm_ir_instr_t *instr = block->instrs[j];
                for (size_t k = 0; instr->args[k] != NULL; k++)
                {
                    vm_ir_arg_t *arg = instr->args[k];
                    if (arg->type == VM_IR_ARG_REG)
                    {
                        if (arg->reg > reguse)
                        {
                            reguse = arg->reg;
                            if (reguse > regalloc)
                            {
                                regalloc = reguse * 2;
                                named = vm_realloc(named, sizeof(uint8_t) * regalloc);
                                where = vm_realloc(where, sizeof(size_t) * regalloc);
                                regs = vm_realloc(regs, sizeof(ptrdiff_t) * regalloc);
                            }
                        }
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
                    where[out->reg] = j;
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
        }
    }
    vm_free(where);
    vm_free(named);
    vm_free(regs);
}
