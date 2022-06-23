
#include "lua.h"

void vm_ir_be_lua_print(FILE *out, vm_ir_arg_t *arg)
{
    switch (arg->type)
    {
    case VM_IR_ARG_FUNC:
    {
        fprintf(out, "f%zu", arg->func->id);
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

void vm_ir_be_lua_print_target(FILE *out, vm_ir_block_t *block, size_t target, size_t indent)
{
    vm_ir_block_t *next = block->branch->targets[target];
    if (next->id > block->id)
    {
        vm_ir_be_lua_print_body(out, next, indent);
    }
    else
    {
        // fprintf(out, "%*creturn bb%zu(", (int) indent, ' ', next->id);
        // for (size_t na = 0; na < next->nargs; na++)
        // {
        //     if (na != 0)
        //     {
        //         fprintf(out, ", ");
        //     }
        //     fprintf(out, "r%zu", next->args[na]);
        // }
        // fprintf(out, ")\n");
        fprintf(out, "%*cgoto bb%zu\n", (int) indent, ' ', next->id);
    }
}

void vm_ir_be_lua_print_body(FILE *out, vm_ir_block_t *block, size_t indentz)
{
    int indent = indentz;
    fprintf(out, "%*c::bb%zu::\n", indent, ' ', block->id);
    uint8_t *regs = vm_alloc0(sizeof(uint8_t) * block->nregs);
    for (size_t a = 0; a < block->nargs; a++)
    {
        regs[block->args[a]] = 1;
    }
    for (size_t j = 0; j < block->len; j++)
    {
        vm_ir_instr_t *instr = block->instrs[j];
        if (instr->op == VM_IR_IOP_NOP)
        {
            continue;
        }
        fprintf(out, "%*c", indent, ' ');
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
    fprintf(out, "%*c", indent, ' ');
    switch (block->branch->op)
    {
    case VM_IR_BOP_JUMP:
    {
        fprintf(out, "do\n");
        vm_ir_be_lua_print_target(out, block, 0, indent + 4);
        fprintf(out, "%*cend\n", indent, ' ');
        break;
    }
    case VM_IR_BOP_EQUAL:
    {
        fprintf(out, "if ");
        vm_ir_be_lua_print(out, block->branch->args[0]);
        fprintf(out, " == ");
        vm_ir_be_lua_print(out, block->branch->args[1]);
        fprintf(out, " then\n");
        vm_ir_be_lua_print_target(out, block, 1, indent + 4);
        fprintf(out, "%*celse\n", indent, ' ');
        vm_ir_be_lua_print_target(out, block, 0, indent + 4);
        fprintf(out, "%*cend\n", indent, ' ');
        break;
    }
    case VM_IR_BOP_LESS:
    {
        fprintf(out, "if ");
        vm_ir_be_lua_print(out, block->branch->args[0]);
        fprintf(out, " < ");
        vm_ir_be_lua_print(out, block->branch->args[1]);
        fprintf(out, " then\n");
        vm_ir_be_lua_print_target(out, block, 1, indent + 4);
        fprintf(out, "%*celse\n", indent, ' ');
        vm_ir_be_lua_print_target(out, block, 0, indent + 4);
        fprintf(out, "%*cend\n", indent, ' ');
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
        fprintf(out, " ~= 0 then\n");
        vm_ir_be_lua_print_target(out, block, 1, indent + 4);
        fprintf(out, "%*celse\n", indent, ' ');
        vm_ir_be_lua_print_target(out, block, 0, indent + 4);
        fprintf(out, "%*cend\n", indent, ' ');
        break;
    }
    }
    free(regs);
}

void vm_ir_be_lua_print_block(FILE *out, vm_ir_block_t *block)
{
    fprintf(out, "f%zu = function(", block->id);
    for (size_t a = 0; a < block->nargs; a++)
    {
        if (a != 0)
        {
            fprintf(out, ", ");
        }
        fprintf(out, "r%zu", block->args[a]);
    }
    fprintf(out, ")\n");
    vm_ir_be_lua_print_body(out, block, 4);
    fprintf(out, "end\n");
}

void vm_ir_be_lua(FILE *out, size_t nops, vm_ir_block_t *blocks)
{
    fprintf(out, "local ");
    for (size_t i = 0; i < nops; i++)
    {
        if (blocks[i].id != i || (!blocks[i].isfunc && i != 0))
        {
            continue;
        }
        if (i != 0)
        {
            fprintf(out, ", ");
        }
        fprintf(out, "f%zu", i);
    }
    fprintf(out, "\n");
    for (size_t i = 0; i < nops; i++)
    {
        vm_ir_block_t *block = &blocks[i];
        if (block->id != i || (!block->isfunc && i != 0))
        {
            continue;
        }
        vm_ir_be_lua_print_block(out, block);
    }
    fprintf(out, "f0()\n");
}
