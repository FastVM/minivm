
#include "js.h"

void vm_ir_be_lua_print(FILE *out, vm_ir_arg_t *arg)
{
    switch (arg->type)
    {
    case VM_IR_ARG_FUNC:
    {
        fprintf(out, "bb%zu", arg->func->id);
        break;
    }
    case VM_IR_ARG_REG:
    {
        fprintf(out, "r%zu", arg->reg);
        break;
    }
    case VM_IR_ARG_STR:
    {
        fprintf(out, "`%s`", arg->str);
        break;
    }
    case VM_IR_ARG_NUM:
    {
        fprintf(out, "%zi", arg->num);
        break;
    }
    }
}

void vm_ir_be_lua_print_target(FILE *out, vm_ir_block_t *block, size_t target)
{
    fprintf(out, "bb%zu(", block->branch->targets[target]->id);
    for (size_t na = 0; na < block->branch->targets[target]->nargs; na++)
    {
        if (na != 0)
        {
            fprintf(out, ", ");
        }
        fprintf(out, "r%zu", block->branch->targets[target]->args[na]);
    }
    fprintf(out, ")");
}

void vm_ir_be_lua(FILE *out, size_t nops, vm_ir_block_t *blocks)
{
    fprintf(out, "local ");
    for (size_t i = 0; i < nops; i++)
    {
        if (blocks[i].id == i)
        {
            if (i != 0)
            {
                fprintf(out, ", ");
            }
            fprintf(out, "bb%zu", i);
        }
    }
    fprintf(out, "\n");
    for (size_t i = 0; i < nops; i++)
    {
        vm_ir_block_t *block = &blocks[i];
        if (block->id != i)
        {
            continue;
        }
        uint8_t *regs = vm_alloc0(sizeof(uint8_t) * block->nregs);
        fprintf(out, "bb%zu = function(", block->id);
        for (size_t a = 0; a < block->nargs; a++)
        {
            regs[block->args[a]] = 1;
            if (a != 0)
            {
                fprintf(out, ", ");
            }
            fprintf(out, "r%zu", block->args[a]);
        }
        fprintf(out, ")\n");
        for (size_t j = 0; j < block->len; j++)
        {
            vm_ir_instr_t *instr = block->instrs[j];
            if (instr->op == VM_IR_IOP_NOP)
            {
                continue;
            }
            fprintf(out, "    ");
            switch (instr->op)
            {
            case VM_IR_IOP_NOP:
            {
                break;
            }
            case VM_IR_IOP_MOVE:
            {
                if (regs[instr->out->reg] == 0)
                {
                    fprintf(out, "local ");
                    regs[instr->out->reg] = 1;
                }
                fprintf(out, "r%zu = ", instr->out->reg);
                vm_ir_be_lua_print(out, instr->args[0]);
                fprintf(out, "\n");
                break;
            }
            case VM_IR_IOP_ADD:
            {
                if (regs[instr->out->reg] == 0)
                {
                    fprintf(out, "local ");
                    regs[instr->out->reg] = 1;
                }
                fprintf(out, "r%zu = ", instr->out->reg);
                vm_ir_be_lua_print(out, instr->args[0]);
                fprintf(out, " + ");
                vm_ir_be_lua_print(out, instr->args[1]);
                fprintf(out, "\n");
                break;
            }
            case VM_IR_IOP_SUB:
            {
                if (regs[instr->out->reg] == 0)
                {
                    fprintf(out, "local ");
                    regs[instr->out->reg] = 1;
                }
                fprintf(out, "r%zu = ", instr->out->reg);
                vm_ir_be_lua_print(out, instr->args[0]);
                fprintf(out, " - ");
                vm_ir_be_lua_print(out, instr->args[1]);
                fprintf(out, "\n");
                break;
            }
            case VM_IR_IOP_MUL:
            {
                if (regs[instr->out->reg] == 0)
                {
                    fprintf(out, "local ");
                    regs[instr->out->reg] = 1;
                }
                fprintf(out, "r%zu = ", instr->out->reg);
                vm_ir_be_lua_print(out, instr->args[0]);
                fprintf(out, " * ");
                vm_ir_be_lua_print(out, instr->args[1]);
                fprintf(out, "\n");
                break;
            }
            case VM_IR_IOP_DIV:
            {
                if (regs[instr->out->reg] == 0)
                {
                    fprintf(out, "local ");
                    regs[instr->out->reg] = 1;
                }
                fprintf(out, "r%zu = math.floor(", instr->out->reg);
                vm_ir_be_lua_print(out, instr->args[0]);
                fprintf(out, " / ");
                vm_ir_be_lua_print(out, instr->args[1]);
                fprintf(out, ")\n");
                break;
            }
            case VM_IR_IOP_MOD:
            {
                if (regs[instr->out->reg] == 0)
                {
                    fprintf(out, "local ");
                    regs[instr->out->reg] = 1;
                }
                fprintf(out, "r%zu = math.floor(", instr->out->reg);
                vm_ir_be_lua_print(out, instr->args[0]);
                fprintf(out, " %% ");
                vm_ir_be_lua_print(out, instr->args[1]);
                fprintf(out, ")\n");
                break;
            }
            case VM_IR_IOP_ADDR:
            {
                if (regs[instr->out->reg] == 0)
                {
                    fprintf(out, "local ");
                    regs[instr->out->reg] = 1;
                }
                fprintf(out, "r%zu = ", instr->out->reg);
                vm_ir_be_lua_print(out, instr->args[0]);
                fprintf(out, "\n");
                break;
            }
            case VM_IR_IOP_CALL:
            {
                if (regs[instr->out->reg] == 0)
                {
                    fprintf(out, "local ");
                    regs[instr->out->reg] = 1;
                }
                fprintf(out, "r%zu = ", instr->out->reg);
                vm_ir_be_lua_print(out, instr->args[0]);
                fprintf(out, "(");
                for (size_t a = 1; instr->args[a] != NULL; a++)
                {
                    if (a != 1)
                    {
                        fprintf(out, ", ");
                    }
                    vm_ir_be_lua_print(out, instr->args[a]);
                }
                fprintf(out, ")");
                fprintf(out, "\n");
                break;
            }
            case VM_IR_IOP_ARR:
            {
                if (regs[instr->out->reg] == 0)
                {
                    fprintf(out, "local ");
                    regs[instr->out->reg] = 1;
                }
                fprintf(out, "r%zu = {length = ", instr->out->reg);
                vm_ir_be_lua_print(out, instr->args[0]);
                fprintf(out, "}");
                fprintf(out, "\n");
                break;
            }
            case VM_IR_IOP_GET:
            {
                if (regs[instr->out->reg] == 0)
                {
                    fprintf(out, "local ");
                    regs[instr->out->reg] = 1;
                }
                fprintf(out, "r%zu = ", instr->out->reg);
                vm_ir_be_lua_print(out, instr->args[0]);
                fprintf(out, "[");
                vm_ir_be_lua_print(out, instr->args[1]);
                fprintf(out, "]");
                fprintf(out, "\n");
                break;
            }
            case VM_IR_IOP_SET:
            {
                vm_ir_be_lua_print(out, instr->args[0]);
                fprintf(out, "[");
                vm_ir_be_lua_print(out, instr->args[1]);
                fprintf(out, "]");
                fprintf(out, " = ");
                vm_ir_be_lua_print(out, instr->args[2]);
                fprintf(out, "\n");
                break;
            }
            case VM_IR_IOP_LEN:
            {
                if (regs[instr->out->reg] == 0)
                {
                    fprintf(out, "local ");
                    regs[instr->out->reg] = 1;
                }
                fprintf(out, "r%zu = ", instr->out->reg);
                vm_ir_be_lua_print(out, instr->args[0]);
                fprintf(out, ".length\n");
                fprintf(out, "\n");
                break;
            }
            case VM_IR_IOP_TYPE:
            {
                if (regs[instr->out->reg] == 0)
                {
                    fprintf(out, "local ");
                    regs[instr->out->reg] = 1;
                }
                fprintf(out, "r%zu = typeof ", instr->out->reg);
                vm_ir_be_lua_print(out, instr->args[0]);
                fprintf(out, " == 'number' ? 0 : 1\n");
                break;
            }
            case VM_IR_IOP_OUT:
            {
                fprintf(out, "io.write(string.char(");
                vm_ir_be_lua_print(out, instr->args[0]);
                fprintf(out, "))");
                fprintf(out, "\n");
                break;
            }
            }
        }
        fprintf(out, "    ");
        switch (block->branch->op)
        {
        case VM_IR_BOP_JUMP:
        {
            fprintf(out, "return ");
            vm_ir_be_lua_print_target(out, block, 0);
            fprintf(out, "\n");
            break;
        }
        case VM_IR_BOP_EQUAL:
        {
            fprintf(out, "if ");
            vm_ir_be_lua_print(out, block->branch->args[0]);
            fprintf(out, " == ");
            vm_ir_be_lua_print(out, block->branch->args[1]);
            fprintf(out, " then return ");
            vm_ir_be_lua_print_target(out, block, 1);
            fprintf(out, " else return ");
            vm_ir_be_lua_print_target(out, block, 0);
            fprintf(out, " end\n");
            break;
        }
        case VM_IR_BOP_LESS:
        {
            fprintf(out, "if ");
            vm_ir_be_lua_print(out, block->branch->args[0]);
            fprintf(out, " < ");
            vm_ir_be_lua_print(out, block->branch->args[1]);
            fprintf(out, " then return ");
            vm_ir_be_lua_print_target(out, block, 1);
            fprintf(out, " else return ");
            vm_ir_be_lua_print_target(out, block, 0);
            fprintf(out, " end\n");
            break;
        }
        case VM_IR_BOP_EXIT:
        {
            fprintf(out, "return nil\n");
            break;
        }
        case VM_IR_BOP_RET:
        {
            fprintf(out, "return ");
            vm_ir_be_lua_print(out, block->branch->args[0]);
            fprintf(out, "\n");
            break;
        }
        case VM_IR_BOP_BOOL:
        {
            fprintf(out, "if ");
            vm_ir_be_lua_print(out, block->branch->args[0]);
            fprintf(out, " ~= 0 then return ");
            vm_ir_be_lua_print_target(out, block, 1);
            fprintf(out, " else return ");
            vm_ir_be_lua_print_target(out, block, 0);
            fprintf(out, " end\n");
            break;
        }
        }
        fprintf(out, "end\n");
        free(regs);
    }
    fprintf(out, "bb0()\n");
}
