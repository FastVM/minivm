
#include "ir.h"

void vm_block_realloc(vm_block_t *block, vm_instr_t instr) {
    if (block->len + 4 >= block->alloc) {
        block->alloc = (block->len + 4) * 4;
        block->instrs = vm_realloc(block->instrs, sizeof(vm_instr_t) * block->alloc);
    }
    block->instrs[block->len++] = instr;
}

void vm_print_arg(FILE *out, vm_arg_t val) {
    switch (val.type) {
        case VM_ARG_NIL: {
            fprintf(out, "nil");
            break;
        }
        case VM_ARG_BOOL: {
            fprintf(out, "%s", val.logic ? "true" : "false");
            break;
        }
        case VM_ARG_NUM: {
            fprintf(out, "%lf", val.num);
            break;
        }
        case VM_ARG_STR: {
            fprintf(out, "\"%s\"", val.str);
            break;
        }
        case VM_ARG_REG: {
            fprintf(out, "r%zu", val.reg);
            break;
        }
        case VM_ARG_FUNC: {
            fprintf(out, ".%zu", val.func->id);
            break;
        }
    }
}
void vm_print_branch(FILE *out, vm_branch_t val) {
    switch (val.op) {
        case VM_BOP_JUMP: {
            fprintf(out, "jump");
            break;
        }
        case VM_BOP_BB: {
            fprintf(out, "bb");
            break;
        }
        case VM_BOP_BLT: {
            fprintf(out, "blt");
            break;
        }
        case VM_BOP_BEQ: {
            fprintf(out, "beq");
            break;
        }
        case VM_BOP_RET: {
            fprintf(out, "ret");
            break;
        }
        case VM_BOP_EXIT: {
            fprintf(out, "exit");
            break;
        }
    }
    if (val.args[0].type != VM_ARG_NONE) {
        fprintf(out, " ");
        vm_print_arg(out, val.args[0]);
    }
    if (val.args[1].type != VM_ARG_NONE) {
        fprintf(out, " ");
        vm_print_arg(out, val.args[1]);
    }
    if (val.targets[0]) {
        fprintf(out, " .%zu", (size_t)val.targets[0]->id);
        fprintf(out, "(");
        for (size_t i = 0; i < val.targets[0]->nargs; i++) {
            if (i != 0) {
                fprintf(out, ", ");
            }
            fprintf(out, "r%zu", val.targets[0]->args[i]);
        }
        fprintf(out, ")");
    }
    if (val.targets[1]) {
        fprintf(out, " .%zu", (size_t)val.targets[1]->id);
        fprintf(out, "(");
        for (size_t i = 0; i < val.targets[1]->nargs; i++) {
            if (i != 0) {
                fprintf(out, ", ");
            }
            fprintf(out, "r%zu", val.targets[1]->args[i]);
        }
        fprintf(out, ")");
    }
}
void vm_print_instr(FILE *out, vm_instr_t val) {
    if (val.op == VM_IOP_NOP) {
        fprintf(out, "nop");
        return;
    }
    if (val.out.type != VM_ARG_NONE) {
        vm_print_arg(out, val.out);
        fprintf(out, " <- ");
    }
    switch (val.op) {
        case VM_IOP_MOVE: {
            fprintf(out, "move");
            break;
        }
        case VM_IOP_ADD: {
            fprintf(out, "add");
            break;
        }
        case VM_IOP_SUB: {
            fprintf(out, "sub");
            break;
        }
        case VM_IOP_MUL: {
            fprintf(out, "mul");
            break;
        }
        case VM_IOP_DIV: {
            fprintf(out, "div");
            break;
        }
        case VM_IOP_MOD: {
            fprintf(out, "mod");
            break;
        }
        case VM_IOP_CALL: {
            fprintf(out, "call");
            break;
        }
        case VM_IOP_OUT: {
            fprintf(out, "out");
            break;
        }
    }
    for (size_t i = 0; val.args[i].type != VM_ARG_NONE; i++) {
        fprintf(out, " ");
        vm_print_arg(out, val.args[i]);
    }
}
void vm_print_block(FILE *out, vm_block_t *val) {
    for (size_t i = 0; i < val->len; i++) {
        if (val->instrs[i].op == VM_IOP_NOP) {
            continue;
        }
        fprintf(out, "    ");
        vm_print_instr(out, val->instrs[i]);
        fprintf(out, "\n");
    }
    if (val->branch.op != VM_BOP_FALL) {
        fprintf(out, "    ");
        vm_print_branch(out, val->branch);
        fprintf(out, "\n");
    } else {
        fprintf(out, "    <fall>\n");
    }
}

void vm_print_blocks(FILE *out, size_t nblocks, vm_block_t *blocks) {
    for (size_t i = 0; i < nblocks; i++) {
        vm_block_t *block = &blocks[i];
        if (block->id < 0) {
            continue;
        }
        if (block->isfunc) {
            fprintf(out, "\nfunc .%zu(", i);
        } else {
            fprintf(out, ".%zu(", i);
        }
        for (size_t j = 0; j < block->nargs; j++) {
            if (j != 0) {
                fprintf(out, ", ");
            }
            fprintf(out, "r%zu", block->args[j]);
        }
        fprintf(out, ")\n");
        vm_print_block(out, block);
    }
}
