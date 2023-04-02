
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
            fprintf(out, ".%zi", val.func->id);
            break;
        }
        case VM_ARG_X64: {
            fprintf(out, "Rv(%zu)", (size_t)val.x64);
            break;
        }
    }
}
void vm_print_tag(FILE *out, vm_tag_t tag) {
    switch (tag) {
        case VM_TAG_NIL: {
            fprintf(out, "nil");
            break;
        }
        case VM_TAG_BOOL: {
            fprintf(out, "bool");
            break;
        }
        case VM_TAG_I64: {
            fprintf(out, "i64");
            break;
        }
        case VM_TAG_F64: {
            fprintf(out, "f64");
            break;
        }
        case VM_TAG_STR: {
            fprintf(out, "str");
            break;
        }
        case VM_TAG_FUNC: {
            fprintf(out, "func");
            break;
        }
        case VM_TAG_TABLE: {
            fprintf(out, "table");
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
        case VM_BOP_BTYPE: {
            fprintf(out, "btype");
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
    if (val.tag != VM_TAG_UNK) {
        fprintf(out, ".");
        vm_print_tag(out, val.tag);
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
        fprintf(out, " .%zi", (size_t)val.targets[0]->id);
        fprintf(out, "(");
        for (size_t i = 0; i < val.targets[0]->nargs; i++) {
            if (i != 0) {
                fprintf(out, ", ");
            }
            vm_print_arg(out, val.targets[0]->args[i]);
        }
        fprintf(out, ")");
    }
    if (val.targets[1]) {
        fprintf(out, " .%zi", (size_t)val.targets[1]->id);
        fprintf(out, "(");
        for (size_t i = 0; i < val.targets[1]->nargs; i++) {
            if (i != 0) {
                fprintf(out, ", ");
            }
            vm_print_arg(out, val.targets[1]->args[i]);
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
        case VM_IOP_CAST: {
            fprintf(out, "cast");
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
        case VM_IOP_PRINT: {
            fprintf(out, "print");
            break;
        }
    }
    if (val.tag != VM_TAG_UNK) {
        fprintf(out, ".");
        vm_print_tag(out, val.tag);
    }
    for (size_t i = 0; val.args[i].type != VM_ARG_NONE; i++) {
        fprintf(out, " ");
        vm_print_arg(out, val.args[i]);
    }
}
void vm_print_block(FILE *out, vm_block_t *val) {
    fprintf(out, ".%zi(", val->id);
    for (size_t i = 0; i < val->nargs; i++) {
        if (i != 0) {
            fprintf(out, ", ");
        }
        vm_print_arg(out, val->args[i]);
    }
    fprintf(out, "):\n");
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
        vm_print_block(out, block);
    }
}

vm_tag_t vm_instr_get_arg_type(vm_instr_t instr, size_t argno) {
    return instr.args[argno].type;
}
uint64_t vm_instr_get_arg_num(vm_instr_t instr, size_t argno) {
    return instr.args[argno].num;
}
const char *vm_instr_get_arg_str(vm_instr_t instr, size_t argno) {
    return instr.args[argno].str;
}
vm_block_t *vm_instr_get_arg_func(vm_instr_t instr, size_t argno) {
    return instr.args[argno].func;
}
size_t vm_instr_get_arg_reg(vm_instr_t instr, size_t argno) {
    return instr.args[argno].reg;
}

enum {
    VM_INFO_REG_UNK,
    VM_INFO_REG_DEF,
    VM_INFO_REG_ARG,
};

void vm_block_info(size_t nblocks, vm_block_t **blocks) {
    uint8_t **all_regs = vm_malloc(sizeof(uint8_t *) * nblocks);
    for (size_t i = 0; i < nblocks; i++) {
        vm_block_t *block = blocks[i];
        if (block->id < 0) {
            continue;
        }
        size_t nregs = 1;
        for (size_t j = 0; j < block->len; j++) {
            vm_instr_t *instr = &block->instrs[j];
            if (instr->out.type == VM_ARG_REG && instr->out.reg >= nregs) {
                nregs = instr->out.reg + 1;
            }
            for (size_t k = 0; instr->args[k].type != VM_ARG_NONE; k++) {
                vm_arg_t arg = instr->args[k];
                if (arg.type == VM_ARG_REG && arg.reg >= nregs) {
                    nregs = arg.reg + 1;
                }
            }
        }
        for (size_t j = 0; j < 2; j++) {
            if (block->branch.args[j].type != VM_ARG_NONE && block->branch.args[j].type == VM_ARG_REG &&
                block->branch.args[j].reg >= nregs) {
                nregs = block->branch.args[j].reg + 1;
            }
        }
        if (nregs > block->nregs) {
            block->nregs = nregs;
        }
    }
    for (size_t i = 0; i < nblocks; i++) {
        vm_block_t *block = blocks[i];
        if (block->id < 0) {
            continue;
        }
        uint8_t *regs = vm_malloc(sizeof(uint8_t) * (block->nregs + 1));
        all_regs[i] = regs;
        for (size_t j = 0; j < block->nregs; j++) {
            regs[j] = VM_INFO_REG_UNK;
        }
        size_t nargs = 0;
        for (size_t j = 0; j < block->len; j++) {
            vm_instr_t *instr = &block->instrs[j];
            for (size_t k = 0; instr->args[k].type != VM_ARG_NONE; k++) {
                vm_arg_t arg = instr->args[k];
                if (arg.type == VM_ARG_REG && regs[arg.reg] == VM_INFO_REG_UNK) {
                    regs[arg.reg] = VM_INFO_REG_ARG;
                    nargs += 1;
                }
            }
            if (instr->out.type == VM_ARG_REG && regs[instr->out.reg] == VM_INFO_REG_UNK) {
                regs[instr->out.reg] = VM_INFO_REG_DEF;
            }
        }
        for (size_t j = 0; j < 2; j++) {
            if (block->branch.args[j].type != VM_ARG_NONE && block->branch.args[j].type == VM_ARG_REG &&
                regs[block->branch.args[j].reg] == VM_INFO_REG_UNK) {
                regs[block->branch.args[j].reg] = VM_INFO_REG_ARG;
                nargs += 1;
            }
        }
        block->nargs = 0;
        block->args = vm_malloc(sizeof(vm_arg_t) * nargs);
        for (size_t reg = 0; reg < block->nregs; reg++) {
            if (regs[reg] == VM_INFO_REG_ARG) {
                block->args[block->nargs++] = (vm_arg_t){
                    .type = VM_ARG_REG,
                    .reg = reg,
                };
                if (reg >= block->nregs) {
                    block->nregs = reg + 1;
                }
            }
        }
    }
    bool redo = true;
    size_t alloc = 16;
    vm_arg_t *next = vm_malloc(sizeof(vm_arg_t) * alloc);
    while (redo) {
        redo = false;
        for (ptrdiff_t i = nblocks - 1; i >= 0; i--) {
            vm_block_t *block = blocks[i];
            if (block->id < 0) {
                continue;
            }
            for (size_t t = 0; t < 2; t++) {
                vm_block_t *target = block->branch.targets[t];
                if (target == NULL) {
                    break;
                }
                size_t total = block->nargs + target->nargs;
                if (total >= alloc) {
                    alloc = total * 2;
                    next = vm_realloc(next, sizeof(vm_arg_t) * alloc);
                }
                size_t nargs = 0;
                size_t bi = 0;
                size_t ti = 0;
                while (true) {
                    if (bi >= block->nargs) {
                        while (ti < target->nargs) {
                            vm_arg_t newreg = target->args[ti++];
                            if (newreg.reg >= blocks[i]->nregs) {
                                all_regs[i] = vm_realloc(all_regs[i], sizeof(uint8_t) * (newreg.reg + 1));
                                for (size_t c = blocks[i]->nregs; c < newreg.reg + 1; c++) {
                                    all_regs[i][c] = VM_INFO_REG_UNK;
                                }
                                blocks[i]->nregs = newreg.reg + 1;
                            }
                            if (all_regs[i][newreg.reg] != VM_INFO_REG_DEF) {
                                next[nargs++] = newreg;
                            }
                        }
                        break;
                    } else if (ti >= target->nargs) {
                        while (bi < block->nargs) {
                            next[nargs++] = block->args[bi++];
                        }
                        break;
                    } else if (block->args[bi].reg == target->args[ti].reg) {
                        next[nargs++] = block->args[bi++];
                        ti += 1;
                    } else if (block->args[bi].reg > target->args[ti].reg) {
                        vm_arg_t newreg = target->args[ti++];
                        if (all_regs[i][newreg.reg] != VM_INFO_REG_DEF) {
                            next[nargs++] = newreg;
                        }
                    } else if (block->args[bi].reg < target->args[ti].reg) {
                        next[nargs++] = block->args[bi++];
                    }
                }
                if (nargs != block->nargs) {
                    vm_free(block->args);
                    block->args = next;
                    block->nargs = nargs;
                    redo = true;
                    next = vm_malloc(sizeof(vm_arg_t) * alloc);
                }
            }
        }
    }
    vm_free(next);
    for (size_t i = 0; i < nblocks; i++) {
        vm_block_t *block = blocks[i];
        if (block->id < 0) {
            continue;
        }
        vm_free(all_regs[i]);
    }
    vm_free(all_regs);
    vm_block_t *func = blocks[0];
    for (size_t i = 0; i < nblocks; i++) {
        vm_block_t *block = blocks[i];
        if (block->id < 0) {
            continue;
        }
        if (block->isfunc) {
            func = block;
        }
        if (block->nregs > func->nregs) {
            func->nregs = block->nregs;
        }
    }
}
