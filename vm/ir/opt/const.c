
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
    bool redo = true;
    while (redo)
    {
        redo = false;
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
                        if (instr->op == VM_IR_IOP_ADD)
                        {
                            vm_ir_arg_t *arg0 = instr->args[0];
                            vm_ir_arg_t *arg1 = instr->args[1];
                            if (arg0->type == VM_IR_ARG_NUM && arg1->type == VM_IR_ARG_NUM)
                            {
                                if (arg0->num == 0)
                                {
                                    instr->op = VM_IR_IOP_MOVE;
                                    instr->args[0] = arg1;
                                }
                                else if (arg1->num == 0)
                                {
                                    instr->op = VM_IR_IOP_MOVE;
                                    instr->args[0] = arg0;
                                }
                                else
                                {
                                    named[out->reg] = VM_IR_OPT_CONST_REG_HAS_VALUE;
                                    regs[out->reg] = arg0->num + arg1->num;
                                    instr->op = VM_IR_IOP_MOVE;
                                    instr->args[0] = vm_ir_arg_num(arg0->num + arg1->num);
                                }
                                redo = true;
                            }
                        }
                        if (instr->op == VM_IR_IOP_SUB)
                        {
                            vm_ir_arg_t *arg0 = instr->args[0];
                            vm_ir_arg_t *arg1 = instr->args[1];
                            if (arg0->type == VM_IR_ARG_NUM && arg1->type == VM_IR_ARG_NUM)
                            {
                                if (arg1->num == 0)
                                {
                                    instr->op = VM_IR_IOP_MOVE;
                                    instr->args[0] = arg0;
                                }
                                else
                                {
                                    named[out->reg] = VM_IR_OPT_CONST_REG_HAS_VALUE;
                                    regs[out->reg] = arg0->num - arg1->num;
                                    instr->op = VM_IR_IOP_MOVE;
                                    instr->args[0] = vm_ir_arg_num(arg0->num - arg1->num);
                                }
                                redo = true;
                            }
                        }
                        if (instr->op == VM_IR_IOP_MUL)
                        {
                            vm_ir_arg_t *arg0 = instr->args[0];
                            vm_ir_arg_t *arg1 = instr->args[1];
                            if (arg0->type == VM_IR_ARG_NUM && arg1->type == VM_IR_ARG_NUM)
                            {
                                if (arg0->num == 1)
                                {
                                    instr->op = VM_IR_IOP_MOVE;
                                    instr->args[0] = arg1;
                                }
                                else if (arg1->num == 1)
                                {
                                    instr->op = VM_IR_IOP_MOVE;
                                    instr->args[0] = arg0;
                                }
                                else
                                {
                                    named[out->reg] = VM_IR_OPT_CONST_REG_HAS_VALUE;
                                    regs[out->reg] = arg0->num * arg1->num;
                                    instr->op = VM_IR_IOP_MOVE;
                                    instr->args[0] = vm_ir_arg_num(arg0->num * arg1->num);
                                }
                                redo = true;
                            }
                        }
                    }
                }
                for (size_t i = 0; i < 2; i++)
                {
                    vm_ir_arg_t *arg = block->branch->args[i];
                    if (arg != NULL && arg->type == VM_IR_ARG_REG && named[arg->reg] == VM_IR_OPT_CONST_REG_HAS_VALUE)
                    {
                        arg->type = VM_IR_ARG_NUM;
                        arg->num = regs[arg->reg];
                    }
                }
                vm_free(named);
                vm_free(regs);
            }
        }
    }
}
