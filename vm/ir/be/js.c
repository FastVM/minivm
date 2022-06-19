
#include "js.h"

void vm_ir_be_js_print(FILE *out, vm_ir_arg_t *arg)
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

void vm_ir_be_js_print_target(FILE *out, vm_ir_block_t *block, size_t target)
{
    if (block->id < block->branch->targets[target]->id)
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
    else
    {
        fprintf(out, "bb%zu.bind(null", block->branch->targets[target]->id);
        for (size_t na = 0; na < block->branch->targets[target]->nargs; na++)
        {
            fprintf(out, ", r%zu", block->branch->targets[target]->args[na]);
        }
        fprintf(out, ")");
    }
}

void vm_ir_be_js(size_t nops, vm_ir_block_t *blocks)
{
    FILE *out = fopen("out.js", "w");
    for (size_t i = 0; i < nops; i++)
    {
        vm_ir_block_t *block = &blocks[i];
        if (block->id != i)
        {
            continue;
        }
        fprintf(out, "const bb%zu = (", block->id);
        for (size_t a = 0; a < block->nargs; a++)
        {
            if (a != 0)
            {
                fprintf(out, ", ");
            }
            fprintf(out, "r%zu", block->args[a]);
        }
        fprintf(out, ") => {\n");
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
                fprintf(out, "var r%zu = ", instr->out->reg);
                vm_ir_be_js_print(out, instr->args[0]);
                fprintf(out, ";\n");
                break;
            }
            case VM_IR_IOP_ADD:
            {
                fprintf(out, "var r%zu = ", instr->out->reg);
                vm_ir_be_js_print(out, instr->args[0]);
                fprintf(out, " + ");
                vm_ir_be_js_print(out, instr->args[1]);
                fprintf(out, "|0;\n");
                break;
            }
            case VM_IR_IOP_SUB:
            {
                fprintf(out, "var r%zu = ", instr->out->reg);
                vm_ir_be_js_print(out, instr->args[0]);
                fprintf(out, " - ");
                vm_ir_be_js_print(out, instr->args[1]);
                fprintf(out, "|0;\n");
                break;
            }
            case VM_IR_IOP_MUL:
            {
                fprintf(out, "var r%zu = ", instr->out->reg);
                vm_ir_be_js_print(out, instr->args[0]);
                fprintf(out, " * ");
                vm_ir_be_js_print(out, instr->args[1]);
                fprintf(out, "|0;\n");
                break;
            }
            case VM_IR_IOP_DIV:
            {
                fprintf(out, "var r%zu = ", instr->out->reg);
                vm_ir_be_js_print(out, instr->args[0]);
                fprintf(out, " / ");
                vm_ir_be_js_print(out, instr->args[1]);
                fprintf(out, "|0;\n");
                break;
            }
            case VM_IR_IOP_MOD:
            {
                fprintf(out, "var r%zu = ", instr->out->reg);
                vm_ir_be_js_print(out, instr->args[0]);
                fprintf(out, " %% ");
                vm_ir_be_js_print(out, instr->args[1]);
                fprintf(out, "|0;\n");
                break;
            }
            case VM_IR_IOP_ADDR:
            {
                fprintf(out, "var r%zu = ", instr->out->reg);
                vm_ir_be_js_print(out, instr->args[0]);
                fprintf(out, ";\n");
                break;
            }
            case VM_IR_IOP_CALL:
            {
                fprintf(out, "var r%zu = call(", instr->out->reg);
                vm_ir_be_js_print(out, instr->args[0]);
                fprintf(out, "(");
                for (size_t a = 1; instr->args[a] != NULL; a++)
                {
                    if (a != 1)
                    {
                        fprintf(out, ", ");
                    }
                    vm_ir_be_js_print(out, instr->args[a]);
                }
                fprintf(out, ")");
                fprintf(out, ")");
                fprintf(out, ";\n");
                break;
            }
            case VM_IR_IOP_ARR:
            {
                fprintf(out, "var r%zu = new Array(", instr->out->reg);
                vm_ir_be_js_print(out, instr->args[0]);
                fprintf(out, ")");
                fprintf(out, ";\n");
                break;
            }
            case VM_IR_IOP_GET:
            {
                fprintf(out, "var r%zu = ", instr->out->reg);
                vm_ir_be_js_print(out, instr->args[0]);
                fprintf(out, "[");
                vm_ir_be_js_print(out, instr->args[1]);
                fprintf(out, "]");
                fprintf(out, ";\n");
                break;
            }
            case VM_IR_IOP_SET:
            {
                vm_ir_be_js_print(out, instr->args[0]);
                fprintf(out, "[");
                vm_ir_be_js_print(out, instr->args[1]);
                fprintf(out, "]");
                fprintf(out, " = ");
                vm_ir_be_js_print(out, instr->args[2]);
                fprintf(out, ";\n");
                break;
            }
            case VM_IR_IOP_LEN:
            {
                fprintf(out, "var r%zu = ", instr->out->reg);
                vm_ir_be_js_print(out, instr->args[0]);
                fprintf(out, ".length;\n");
                fprintf(out, ";\n");
                break;
            }
            case VM_IR_IOP_TYPE:
            {
                fprintf(out, "var r%zu = typeof ", instr->out->reg);
                vm_ir_be_js_print(out, instr->args[0]);
                fprintf(out, " == 'number' ? 0 : 1;\n");
                break;
            }
            case VM_IR_IOP_OUT:
            {
                fprintf(out, "out(");
                vm_ir_be_js_print(out, instr->args[0]);
                fprintf(out, ")");
                fprintf(out, ";\n");
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
            vm_ir_be_js_print_target(out, block, 0);
            fprintf(out, ";\n");
            break;
        }
        case VM_IR_BOP_EQUAL:
        {
            fprintf(out, "return ");
            vm_ir_be_js_print(out, block->branch->args[0]);
            fprintf(out, " === ");
            vm_ir_be_js_print(out, block->branch->args[1]);
            fprintf(out, " ? ");
            vm_ir_be_js_print_target(out, block, 1);
            fprintf(out, " : ");
            vm_ir_be_js_print_target(out, block, 0);
            fprintf(out, ";\n");
            break;
        }
        case VM_IR_BOP_LESS:
        {
            fprintf(out, "return ");
            vm_ir_be_js_print(out, block->branch->args[0]);
            fprintf(out, " < ");
            vm_ir_be_js_print(out, block->branch->args[1]);
            fprintf(out, " ? ");
            vm_ir_be_js_print_target(out, block, 1);
            fprintf(out, " : ");
            vm_ir_be_js_print_target(out, block, 0);
            fprintf(out, ";\n");
            break;
        }
        case VM_IR_BOP_EXIT:
        {
            fprintf(out, "return null;\n");
            break;
        }
        case VM_IR_BOP_RET:
        {
            fprintf(out, "return ");
            vm_ir_be_js_print(out, block->branch->args[0]);
            fprintf(out, ";\n");
            break;
        }
        case VM_IR_BOP_BOOL:
        {
            fprintf(out, "return ");
            vm_ir_be_js_print(out, block->branch->args[0]);
            fprintf(out, " ? ");
            vm_ir_be_js_print_target(out, block, 1);
            fprintf(out, " : ");
            vm_ir_be_js_print_target(out, block, 0);
            fprintf(out, ";\n");
            break;
        }
        }
        fprintf(out, "};\n");
    }
    fprintf(out, "const out = (chr) => { process.stdout.write(String.fromCharCode(chr)); }\n");
    fprintf(out, "const call = (func) => { while (func instanceof Function) { func = func() } return func; }\n");
    fprintf(out, "call(bb0);\n");
}
