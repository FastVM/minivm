
#include "../opt.h"
#include "../build.h"

bool vm_ir_opt_arg_ok(uint8_t op)
{
    return op == VM_IR_IOP_ADD || op == VM_IR_IOP_SUB
        || op == VM_IR_IOP_MUL || op == VM_IR_IOP_DIV || op == VM_IR_IOP_MOD
        || op == VM_IR_IOP_GET || op == VM_IR_IOP_TYPE
        || op == VM_IR_IOP_MOVE;
}

void vm_ir_opt_arg(size_t *ptr_nops, vm_ir_block_t **ptr_blocks)
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
        vm_ir_arg_t **regs = vm_alloc0(sizeof(vm_ir_arg_t *) * block->nregs);
        for (size_t j = 0; j < block->nregs; j++)
        {
            regs[j] = vm_ir_arg_reg(j);
        }
        for (ptrdiff_t j = 0; j < block->len; j++)
        {
            vm_ir_instr_t *instr = block->instrs[j];
            for (size_t k = 0; instr->args[k] != NULL; k++)
            {
                if (instr->args[k]->type == VM_IR_ARG_REG)
                {
                    if (instr->args[k]->type == VM_IR_ARG_REG)
                    {
                        instr->args[k] = regs[instr->args[k]->reg];
                    }
                }
            }
            if (instr->out != NULL && instr->out->type == VM_IR_ARG_REG)
            {
                if (vm_ir_opt_arg_ok(instr->op))
                {
                    // block->instrs[j] = vm_ir_new(vm_ir_instr_t, .op = VM_IR_IOP_NOP);
                    regs[instr->out->reg] = vm_ir_arg_instr(instr);
                }
                else
                {
                    regs[instr->out->reg] = vm_ir_arg_reg(instr->out->reg);
                }
            }
        }
        for (size_t j = 0; j < 2; j++)
        {
            if (block->branch->args[j] != NULL && block->branch->args[j]->type == VM_IR_ARG_REG)
            {
                block->branch->args[j] = regs[block->branch->args[j]->reg];
            }
        }
        free(regs);
    }
}
