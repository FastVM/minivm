
#include "lua.h"

void vm_ir_be_lua_print(vm_ir_be_lua_state_t *out, vm_ir_arg_t *arg)
{
    switch (arg->type)
    {
    case VM_IR_ARG_FUNC:
    {
        fprintf(out->file, "f%zu", arg->func->id);
        break;
    }
    case VM_IR_ARG_REG:
    {
        fprintf(out->file, "r%zu", arg->reg);
        break;
    }
    case VM_IR_ARG_STR:
    {
        fprintf(out->file, "`%s`", arg->str);
        break;
    }
    case VM_IR_ARG_NUM:
    {
        fprintf(out->file, "%zi", arg->num);
        break;
    }
    }
}

void vm_ir_be_lua_print_target(vm_ir_be_lua_state_t *out, vm_ir_block_t *block, size_t target, size_t indent)
{
    vm_ir_be_lua_print_body(out, block->branch->targets[target], indent);
}

void vm_ir_be_lua_print_body(vm_ir_be_lua_state_t *out, vm_ir_block_t *block, size_t indentz)
{
    int indent = indentz;
    // if (out->back[block->id] || 1)
    // {
    for (size_t i = 0; i < out->nblocks; i++)
    {
        if (out->blocks[i] == block->id)
        {
            fprintf(out->file, "%*cgoto bb%zu\n", (int) indent, ' ', block->id);
            return;
        }
    }
    out->blocks[out->nblocks++] = block->id;
    fprintf(out->file, "%*c::bb%zu::\n", indent, ' ', block->id);
    // }
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
        fprintf(out->file, "%*c", indent, ' ');
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
                fprintf(out->file, "local ");
                regs[instr->out->reg] = 1;
            }
            fprintf(out->file, "r%zu = ", instr->out->reg);
            vm_ir_be_lua_print(out, instr->args[0]);
            fprintf(out->file, "\n");
            break;
        }
        case VM_IR_IOP_ADD:
        {
            if (regs[instr->out->reg] == 0)
            {
                fprintf(out->file, "local ");
                regs[instr->out->reg] = 1;
            }
            fprintf(out->file, "r%zu = ", instr->out->reg);
            vm_ir_be_lua_print(out, instr->args[0]);
            fprintf(out->file, " + ");
            vm_ir_be_lua_print(out, instr->args[1]);
            fprintf(out->file, "\n");
            break;
        }
        case VM_IR_IOP_SUB:
        {
            if (regs[instr->out->reg] == 0)
            {
                fprintf(out->file, "local ");
                regs[instr->out->reg] = 1;
            }
            fprintf(out->file, "r%zu = ", instr->out->reg);
            vm_ir_be_lua_print(out, instr->args[0]);
            fprintf(out->file, " - ");
            vm_ir_be_lua_print(out, instr->args[1]);
            fprintf(out->file, "\n");
            break;
        }
        case VM_IR_IOP_MUL:
        {
            if (regs[instr->out->reg] == 0)
            {
                fprintf(out->file, "local ");
                regs[instr->out->reg] = 1;
            }
            fprintf(out->file, "r%zu = ", instr->out->reg);
            vm_ir_be_lua_print(out, instr->args[0]);
            fprintf(out->file, " * ");
            vm_ir_be_lua_print(out, instr->args[1]);
            fprintf(out->file, "\n");
            break;
        }
        case VM_IR_IOP_DIV:
        {
            if (regs[instr->out->reg] == 0)
            {
                fprintf(out->file, "local ");
                regs[instr->out->reg] = 1;
            }
            fprintf(out->file, "r%zu = math.floor(", instr->out->reg);
            vm_ir_be_lua_print(out, instr->args[0]);
            fprintf(out->file, " / ");
            vm_ir_be_lua_print(out, instr->args[1]);
            fprintf(out->file, ")\n");
            break;
        }
        case VM_IR_IOP_MOD:
        {
            if (regs[instr->out->reg] == 0)
            {
                fprintf(out->file, "local ");
                regs[instr->out->reg] = 1;
            }
            fprintf(out->file, "r%zu = math.floor(", instr->out->reg);
            vm_ir_be_lua_print(out, instr->args[0]);
            fprintf(out->file, " %% ");
            vm_ir_be_lua_print(out, instr->args[1]);
            fprintf(out->file, ")\n");
            break;
        }
        case VM_IR_IOP_ADDR:
        {
            if (regs[instr->out->reg] == 0)
            {
                fprintf(out->file, "local ");
                regs[instr->out->reg] = 1;
            }
            fprintf(out->file, "r%zu = ", instr->out->reg);
            vm_ir_be_lua_print(out, instr->args[0]);
            fprintf(out->file, "\n");
            break;
        }
        case VM_IR_IOP_CALL:
        {
            if (regs[instr->out->reg] == 0)
            {
                fprintf(out->file, "local ");
                regs[instr->out->reg] = 1;
            }
            fprintf(out->file, "r%zu = ", instr->out->reg);
            vm_ir_be_lua_print(out, instr->args[0]);
            fprintf(out->file, "(");
            for (size_t a = 1; instr->args[a] != NULL; a++)
            {
                if (a != 1)
                {
                    fprintf(out->file, ", ");
                }
                vm_ir_be_lua_print(out, instr->args[a]);
            }
            fprintf(out->file, ")");
            fprintf(out->file, "\n");
            break;
        }
        case VM_IR_IOP_ARR:
        {
            if (regs[instr->out->reg] == 0)
            {
                fprintf(out->file, "local ");
                regs[instr->out->reg] = 1;
            }
            fprintf(out->file, "r%zu = {length = ", instr->out->reg);
            vm_ir_be_lua_print(out, instr->args[0]);
            fprintf(out->file, "}");
            fprintf(out->file, "\n");
            break;
        }
        case VM_IR_IOP_GET:
        {
            if (regs[instr->out->reg] == 0)
            {
                fprintf(out->file, "local ");
                regs[instr->out->reg] = 1;
            }
            fprintf(out->file, "r%zu = ", instr->out->reg);
            vm_ir_be_lua_print(out, instr->args[0]);
            fprintf(out->file, "[");
            vm_ir_be_lua_print(out, instr->args[1]);
            fprintf(out->file, "]");
            fprintf(out->file, "\n");
            break;
        }
        case VM_IR_IOP_SET:
        {
            vm_ir_be_lua_print(out, instr->args[0]);
            fprintf(out->file, "[");
            vm_ir_be_lua_print(out, instr->args[1]);
            fprintf(out->file, "]");
            fprintf(out->file, " = ");
            vm_ir_be_lua_print(out, instr->args[2]);
            fprintf(out->file, "\n");
            break;
        }
        case VM_IR_IOP_LEN:
        {
            if (regs[instr->out->reg] == 0)
            {
                fprintf(out->file, "local ");
                regs[instr->out->reg] = 1;
            }
            fprintf(out->file, "r%zu = ", instr->out->reg);
            vm_ir_be_lua_print(out, instr->args[0]);
            fprintf(out->file, ".length\n");
            fprintf(out->file, "\n");
            break;
        }
        case VM_IR_IOP_TYPE:
        {
            if (regs[instr->out->reg] == 0)
            {
                fprintf(out->file, "local ");
                regs[instr->out->reg] = 1;
            }
            fprintf(out->file, "r%zu = typeof ", instr->out->reg);
            vm_ir_be_lua_print(out, instr->args[0]);
            fprintf(out->file, " == 'number' ? 0 : 1\n");
            break;
        }
        case VM_IR_IOP_OUT:
        {
            fprintf(out->file, "io.write(string.char(");
            vm_ir_be_lua_print(out, instr->args[0]);
            fprintf(out->file, "))");
            fprintf(out->file, "\n");
            break;
        }
        }
    }
    if (block->branch->op != VM_IR_BOP_JUMP)
    {
        fprintf(out->file, "%*c", indent, ' ');
    }
    switch (block->branch->op)
    {
    case VM_IR_BOP_JUMP:
    {
        vm_ir_be_lua_print_target(out, block, 0, indent);
        break;
    }
    case VM_IR_BOP_EQUAL:
    {
        fprintf(out->file, "if ");
        vm_ir_be_lua_print(out, block->branch->args[0]);
        fprintf(out->file, " == ");
        vm_ir_be_lua_print(out, block->branch->args[1]);
        fprintf(out->file, " then\n");
        vm_ir_be_lua_print_target(out, block, 1, indent + 4);
        fprintf(out->file, "%*celse\n", indent, ' ');
        vm_ir_be_lua_print_target(out, block, 0, indent + 4);
        fprintf(out->file, "%*cend\n", indent, ' ');
        break;
    }
    case VM_IR_BOP_LESS:
    {
        fprintf(out->file, "if ");
        vm_ir_be_lua_print(out, block->branch->args[0]);
        fprintf(out->file, " < ");
        vm_ir_be_lua_print(out, block->branch->args[1]);
        fprintf(out->file, " then\n");
        vm_ir_be_lua_print_target(out, block, 1, indent + 4);
        fprintf(out->file, "%*celse\n", indent, ' ');
        vm_ir_be_lua_print_target(out, block, 0, indent + 4);
        fprintf(out->file, "%*cend\n", indent, ' ');
        break;
    }
    case VM_IR_BOP_EXIT:
    {
        fprintf(out->file, "return nil\n");
        break;
    }
    case VM_IR_BOP_RET:
    {
        fprintf(out->file, "return ");
        vm_ir_be_lua_print(out, block->branch->args[0]);
        fprintf(out->file, "\n");
        break;
    }
    case VM_IR_BOP_BOOL:
    {
        fprintf(out->file, "if ");
        vm_ir_be_lua_print(out, block->branch->args[0]);
        fprintf(out->file, " ~= 0 then\n");
        vm_ir_be_lua_print_target(out, block, 1, indent + 4);
        fprintf(out->file, "%*celse\n", indent, ' ');
        vm_ir_be_lua_print_target(out, block, 0, indent + 4);
        fprintf(out->file, "%*cend\n", indent, ' ');
        break;
    }
    }
    out->nblocks--;
    free(regs);
}

void vm_ir_be_lua_print_block(vm_ir_be_lua_state_t *out, vm_ir_block_t *block)
{
    fprintf(out->file, "f%zu = function(", block->id);
    for (size_t a = 0; a < block->nargs; a++)
    {
        if (a != 0)
        {
            fprintf(out->file, ", ");
        }
        fprintf(out->file, "r%zu", block->args[a]);
    }
    fprintf(out->file, ")\n");
    vm_ir_be_lua_print_body(out, block, 4);
    fprintf(out->file, "end\n");
}

void vm_ir_be_lua_main(vm_ir_be_lua_state_t *out, size_t nops, vm_ir_block_t *blocks)
{
    fprintf(out->file, "local ");
    for (size_t i = 0; i < nops; i++)
    {
        if (blocks[i].id != i || (!blocks[i].isfunc && i != 0))
        {
            continue;
        }
        if (i != 0)
        {
            fprintf(out->file, ", ");
        }
        fprintf(out->file, "f%zu", i);
    }
    fprintf(out->file, "\n");
    for (size_t i = 0; i < nops; i++)
    {
        vm_ir_block_t *block = &blocks[i];
        if (block->id != i || (!block->isfunc && i != 0))
        {
            continue;
        }
        vm_ir_be_lua_print_block(out, block);
    }
    fprintf(out->file, "f0()\n");
}

void vm_ir_be_lua(FILE *out, size_t nops, vm_ir_block_t *blocks)
{
    vm_ir_be_lua_state_t state;
    state.file = out;
    state.back = vm_alloc0(sizeof(uint8_t) * nops);
    state.nblocks = 0;
    state.blocks = vm_malloc(sizeof(size_t) * 256);
    for (size_t i = 0; i < nops; i++)
    {
        vm_ir_block_t *block = &blocks[i];
        if (block->id != i || (!block->isfunc && i != 0))
        {
            continue;
        }
        for (size_t j = 0; j < 2; j++)
        {
            vm_ir_block_t *target = block->branch->targets[j];
            if (target == NULL)
            {
                break;
            }
            if (block->id < target->id)
            {
                state.back[target->id] = 1;
            }
        }
    }
    vm_ir_be_lua_main(&state, nops, blocks);
    vm_free(state.back);
    vm_free(state.blocks);
}
