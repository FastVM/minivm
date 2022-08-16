
#include "int.h"
#include "../build.h"

union vm_value_t;
typedef union vm_value_t vm_value_t;

union vm_value_t {
    ptrdiff_t num;
    vm_value_t *array;
    vm_ir_block_t *block;
}; 

vm_value_t vm_ir_be_int_block(vm_ir_block_t *block, vm_value_t *locals) {
    while (true) {
        for (size_t arg = 0; arg < block->len; arg++) {
            vm_ir_instr_t *instr = block->instrs[arg];
            switch (instr->op) {
            case VM_IR_IOP_NOP: {
                break;
            }
            case VM_IR_IOP_MOVE: {
                if (instr->out && instr->out->type == VM_IR_ARG_REG)
                {
                    switch (instr->args[0]->type) 
                    {
                    case VM_IR_ARG_REG:
                    {
                        locals[instr->out->reg] = locals[instr->args[0]->reg];
                        break;
                    }
                    case VM_IR_ARG_NUM:
                    {
                        locals[instr->out->reg].num = instr->args[0]->num;
                        break;
                    }
                    case VM_IR_ARG_STR:
                    {
                        fprintf(stderr, "NO STRINGS YET\n");
                        __builtin_trap();
                    }
                    case VM_IR_ARG_FUNC:
                    {
                        locals[instr->out->reg].block = instr->args[0]->func;
                        break;
                    }
                    }
                }
                break;
            }
            case VM_IR_IOP_ADD: {
                if (instr->out && instr->out->type == VM_IR_ARG_REG)
                {
                    if (instr->args[0]->type == VM_IR_ARG_REG)
                    {
                        if (instr->args[1]->type == VM_IR_ARG_REG)
                        {
                            locals[instr->out->reg].num = locals[instr->args[0]->reg].num + locals[instr->args[1]->reg].num;
                        }
                        else
                        {
                            locals[instr->out->reg].num = locals[instr->args[0]->reg].num + instr->args[1]->num;
                        }
                    }
                    else
                    {
                        if (instr->args[1]->type == VM_IR_ARG_REG)
                        {
                            locals[instr->out->reg].num = instr->args[0]->num + locals[instr->args[1]->reg].num;
                        }
                        else
                        {
                            locals[instr->out->reg].num = instr->args[0]->num +  instr->args[1]->num;
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_SUB: {
                if (instr->out && instr->out->type == VM_IR_ARG_REG)
                {
                    if (instr->args[0]->type == VM_IR_ARG_REG)
                    {
                        if (instr->args[1]->type == VM_IR_ARG_REG)
                        {
                            locals[instr->out->reg].num = locals[instr->args[0]->reg].num - locals[instr->args[1]->reg].num;
                        }
                        else
                        {
                            locals[instr->out->reg].num = locals[instr->args[0]->reg].num - instr->args[1]->num;
                        }
                    }
                    else
                    {
                        if (instr->args[1]->type == VM_IR_ARG_REG)
                        {
                            locals[instr->out->reg].num = instr->args[0]->num - locals[instr->args[1]->reg].num;
                        }
                        else
                        {
                            locals[instr->out->reg].num = instr->args[0]->num -  instr->args[1]->num;
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_MUL: {
                if (instr->out && instr->out->type == VM_IR_ARG_REG)
                {
                    if (instr->args[0]->type == VM_IR_ARG_REG)
                    {
                        if (instr->args[1]->type == VM_IR_ARG_REG)
                        {
                            locals[instr->out->reg].num = locals[instr->args[0]->reg].num * locals[instr->args[1]->reg].num;
                        }
                        else
                        {
                            locals[instr->out->reg].num = locals[instr->args[0]->reg].num * instr->args[1]->num;
                        }
                    }
                    else
                    {
                        if (instr->args[1]->type == VM_IR_ARG_REG)
                        {
                            locals[instr->out->reg].num = instr->args[0]->num * locals[instr->args[1]->reg].num;
                        }
                        else
                        {
                            locals[instr->out->reg].num = instr->args[0]->num *  instr->args[1]->num;
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_DIV: {
                if (instr->out && instr->out->type == VM_IR_ARG_REG)
                {
                    if (instr->args[0]->type == VM_IR_ARG_REG)
                    {
                        if (instr->args[1]->type == VM_IR_ARG_REG)
                        {
                            locals[instr->out->reg].num = locals[instr->args[0]->reg].num / locals[instr->args[1]->reg].num;
                        }
                        else
                        {
                            locals[instr->out->reg].num = locals[instr->args[0]->reg].num / instr->args[1]->num;
                        }
                    }
                    else
                    {
                        if (instr->args[1]->type == VM_IR_ARG_REG)
                        {
                            locals[instr->out->reg].num = instr->args[0]->num / locals[instr->args[1]->reg].num;
                        }
                        else
                        {
                            locals[instr->out->reg].num = instr->args[0]->num /  instr->args[1]->num;
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_MOD: {
                if (instr->out && instr->out->type == VM_IR_ARG_REG)
                {
                    if (instr->args[0]->type == VM_IR_ARG_REG)
                    {
                        if (instr->args[1]->type == VM_IR_ARG_REG)
                        {
                            locals[instr->out->reg].num = locals[instr->args[0]->reg].num % locals[instr->args[1]->reg].num;
                        }
                        else
                        {
                            locals[instr->out->reg].num = locals[instr->args[0]->reg].num % instr->args[1]->num;
                        }
                    }
                    else
                    {
                        if (instr->args[1]->type == VM_IR_ARG_REG)
                        {
                            locals[instr->out->reg].num = instr->args[0]->num % locals[instr->args[1]->reg].num;
                        }
                        else
                        {
                            locals[instr->out->reg].num = instr->args[0]->num %  instr->args[1]->num;
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_CALL: {
                vm_ir_block_t *next;
                if (instr->args[0]->type == VM_IR_ARG_FUNC)
                {
                    next = instr->args[0]->func;
                }
                else
                {
                    next = locals[instr->args[0]->reg].block;
                }
                vm_value_t *regs = vm_alloc0(sizeof(vm_value_t) * next->nregs);
                for (size_t i = 1; instr->args[i] != NULL && i-1 < next->nargs; i++)
                {
                    if (instr->args[i]->type == VM_IR_ARG_REG)
                    {
                        regs[next->args[i-1]] = locals[instr->args[i]->reg];
                    }
                    else
                    {
                        regs[next->args[i-1]].num = instr->args[i]->num;
                    }
                }
                vm_value_t res = vm_ir_be_int_block(next, regs);
                vm_free(regs);
                if (instr->out && instr->out->type == VM_IR_ARG_REG)
                {
                    locals[instr->out->reg] = res;
                }
                break;
            }
            case VM_IR_IOP_ARR: {
                __builtin_trap();
                break;
            }
            case VM_IR_IOP_GET: {
                __builtin_trap();
                break;
            }
            case VM_IR_IOP_SET: {
                __builtin_trap();
                break;
            }
            case VM_IR_IOP_LEN: {
                __builtin_trap();
                break;
            }
            case VM_IR_IOP_TYPE: {
                __builtin_trap();
                break;
            }
            case VM_IR_IOP_OUT: {
                if (instr->args[0]->type == VM_IR_ARG_NUM)
                {
                    putchar((int) instr->args[0]->num);
                }
                else
                {
                    putchar((int) locals[instr->args[0]->reg].num);
                }
                break;
            }
            }   
        }
        uint8_t next;
        switch(block->branch->op)
        {
        case VM_IR_BOP_JUMP:
        {
            next = 0;
            break;
        }
        case VM_IR_BOP_BOOL:
        {
            if (block->branch->args[0]->type == VM_IR_ARG_NUM)
            {
                next = block->branch->args[0]->num != 0 ? 1 : 0;
            }
            else
            {
                next = locals[block->branch->args[0]->reg].num != 0 ? 1 : 0;
            }
            break;
        }
        case VM_IR_BOP_LESS:
        {
            if (block->branch->args[0]->type == VM_IR_ARG_NUM)
            {
                if (block->branch->args[1]->type == VM_IR_ARG_NUM)
                {
                    next = block->branch->args[0]->num < block->branch->args[1]->num ? 1 : 0;
                }
                else
                {
                    next = block->branch->args[0]->num < locals[block->branch->args[1]->reg].num ? 1 : 0;
                }
            }
            else
            {
                if (block->branch->args[1]->type == VM_IR_ARG_NUM)
                {
                    next = locals[block->branch->args[0]->reg].num < block->branch->args[1]->num ? 1 : 0;
                }
                else
                {
                    next = locals[block->branch->args[0]->reg].num <locals[block->branch->args[1]->reg].num ? 1 : 0;
                }
            }
            break;
        }
        case VM_IR_BOP_EQUAL:
        {
            if (block->branch->args[0]->type == VM_IR_ARG_NUM)
            {
                if (block->branch->args[1]->type == VM_IR_ARG_NUM)
                {
                    next = block->branch->args[0]->num == block->branch->args[1]->num ? 1 : 0;
                }
                else
                {
                    next = block->branch->args[0]->num == locals[block->branch->args[1]->reg].num ? 1 : 0;
                }
            }
            else
            {
                if (block->branch->args[1]->type == VM_IR_ARG_NUM)
                {
                    next = locals[block->branch->args[0]->reg].num == block->branch->args[1]->num ? 1 : 0;
                }
                else
                {
                    next = locals[block->branch->args[0]->reg].num == locals[block->branch->args[1]->reg].num ? 1 : 0;
                }
            }
            break;
        }
        case VM_IR_BOP_RET:
        {
            vm_value_t val;
            if (block->branch->args[0]->type == VM_IR_ARG_NUM)
            {
                val.num = block->branch->args[0]->num;
            }
            else
            {
                val = locals[block->branch->args[0]->reg];
            }
            return val;
        }
        case VM_IR_BOP_EXIT:
        {
            exit(0);
        }
        }
        vm_value_t args[8];
        for (size_t i = 0; i < block->branch->targets[next]->nargs; i++)
        {
            vm_ir_arg_t *arg = block->branch->pass[next][i];
            if (arg->type == VM_IR_ARG_REG)
            {
                args[i] = locals[arg->reg];
            }
            else
            {
                args[i].num = arg->num;
            }
        }
        for (size_t i = 0; i < block->branch->targets[next]->nargs; i++)
        {
            locals[block->branch->targets[next]->args[i]] = args[i];
        }
        block = block->branch->targets[next];
        continue;
    }
}

void vm_ir_be_int(size_t nblocks, vm_ir_block_t *blocks)
{
    vm_ir_block_t *cur = &blocks[0];
    vm_value_t values[16];
    vm_ir_be_int_block(cur, values);
}
