#include "x64.h"

const uint8_t vm_x64_instr_argc_table[VM_X64_OPCODE_MAX_OPCODE] = {
    [VM_X64_OPCODE_CMP] = 2,
    [VM_X64_OPCODE_TEST] = 2,
    [VM_X64_OPCODE_JMP] = 1,
    [VM_X64_OPCODE_JE] = 1,
    [VM_X64_OPCODE_JL] = 1,
    [VM_X64_OPCODE_JB] = 1,
    [VM_X64_OPCODE_ADD] = 2,
    [VM_X64_OPCODE_SUB] = 2,
    [VM_X64_OPCODE_MUL] = 1,
    [VM_X64_OPCODE_DIV] = 1,
    [VM_X64_OPCODE_MOV] = 2,
    [VM_X64_OPCODE_PUSH] = 1,
    [VM_X64_OPCODE_POP] = 1,
    [VM_X64_OPCODE_CALL] = 1,
    [VM_X64_OPCODE_XOR] = 2,
};

void vm_x64_print_opcode(FILE *file,vm_x64_opcode_t opcode) {
    switch (opcode) {
    case VM_X64_OPCODE_CMP: {
        fprintf(file, "cmp");
        break;
    }
    case VM_X64_OPCODE_TEST: {
        fprintf(file, "test");
        break;
    }
    case VM_X64_OPCODE_JMP: {
        fprintf(file, "jmp");
        break;
    }
    case VM_X64_OPCODE_JE: {
        fprintf(file, "je");
        break;
    }
    case VM_X64_OPCODE_JL: {
        fprintf(file, "jl");
        break;
    }
    case VM_X64_OPCODE_JB: {
        fprintf(file, "jb");
        break;
    }
    case VM_X64_OPCODE_ADD: {
        fprintf(file, "add");
        break;
    }
    case VM_X64_OPCODE_SUB: {
        fprintf(file, "sub");
        break;
    }
    case VM_X64_OPCODE_MUL: {
        fprintf(file, "mul");
        break;
    }
    case VM_X64_OPCODE_DIV: {
        fprintf(file, "div");
        break;
    }
    case VM_X64_OPCODE_MOV: {
        fprintf(file, "mov");
        break;
    }
    case VM_X64_OPCODE_PUSH: {
        fprintf(file, "push");
        break;
    }
    case VM_X64_OPCODE_POP: {
        fprintf(file, "pop");
        break;
    }
    case VM_X64_OPCODE_CALL: {
        fprintf(file, "call");
        break;
    }
    case VM_X64_OPCODE_XOR: {
        fprintf(file, "xor");
        break;
    }
    }
}

void vm_x64_print_reg(FILE *file, uint8_t num, uint8_t bitsize) {
    static const char *table[8] = {
        "ax",
        "cx",
        "dx",
        "bx",
        "sp",
        "bp",
        "si",
        "di",
    };
    if (num < 8) {
        if (bitsize == 8) {
            fprintf(file, "%cl", *table[num]);
        } else if (bitsize == 16) {
            fprintf(file, "%s", table[num]);
        } else if (bitsize == 32) {
            fprintf(file, "e%s", table[num]);
        } else if (bitsize == 64) {
            fprintf(file, "r%s", table[num]);
        } else {
            __builtin_trap();
        }
    } else {
        if (bitsize == 8) {
            fprintf(file, "r%ib", num);
        } else if (bitsize == 16) {
            fprintf(file, "r%iw", num);
        } else if (bitsize == 32) {
            fprintf(file, "r%id", num);
        } else if (bitsize == 64) {
            fprintf(file, "r%i", num);
        } else {
            __builtin_trap();
        }
    }
}

void vm_x64_print_instr(FILE *file, vm_x64_instr_t instr) {
    vm_x64_print_opcode(file, instr.opcode);
    for (uint8_t i = 0; i < vm_x64_instr_argc_table[instr.opcode]; i++) {
        fprintf(file, " ");
        vm_x64_arg_t arg = instr.args[i];
        switch (arg.type) {
        case VM_X64_ARG_REG: {
            vm_x64_print_reg(file, arg.reg, instr.bitsize);
            break;
        }
        case VM_X64_ARG_LOAD: {
            if (instr.bitsize == 8) {
                fprintf(file, "byte");
            }
            if (instr.bitsize == 16) {
                fprintf(file, "word");
            }
            if (instr.bitsize == 32) {
                fprintf(file, "dword");
            }
            if (instr.bitsize == 64) {
                fprintf(file, "qword");
            }
            fprintf(file, " [");
            vm_x64_print_reg(file, arg.reg, 64);
            fprintf(file, "]");
            break;
        }
        case VM_X64_ARG_LABEL: {
            fprintf(file, "=>%"PRIu32, arg.label);
            break;
        }
        case VM_X64_ARG_IMM32: {
            fprintf(file, "%"PRIu32, arg.imm32);
            break;
        }
        case VM_X64_ARG_IMM64: {
            fprintf(file, "%"PRIu64, arg.imm64);
            break;
        }
        }
    }
    fprintf(file, "\n");
}

void vm_x64_instr_insert_after(vm_x64_instr_t *after_me, vm_x64_instr_t *instr) {
    vm_x64_instr_t *first = instr;
    vm_x64_instr_t *last = instr;
    vm_x64_instr_t *after_me_next = after_me->next;
    while (last->next != NULL) {
        last = last->next;
    }
    while (first->last != NULL) {
        last = first->last;
    }
    last->next = after_me_next;
    if (after_me_next != NULL) {
        after_me_next->last = last;
    }
    first->last = after_me;
    if (after_me_next != NULL) {
        after_me->next = first;
    }
}

void vm_x64_instr_insert_before(vm_x64_instr_t *before_me, vm_x64_instr_t *instr) {
    vm_x64_instr_t *first = instr;
    vm_x64_instr_t *last = instr;
    vm_x64_instr_t *before_me_last = before_me->next;
    while (last->next != NULL) {
        last = last->next;
    }
    while (first->last != NULL) {
        last = first->last;
    }
    last->next = before_me;
    before_me->last = last;
    first->last = before_me_last;
    before_me_last->next = first;
}

void vm_x64_instr_insert1_after(vm_x64_instr_t *after_me, vm_x64_instr_t instr) {
    vm_x64_instr_t *pinstr = vm_malloc(sizeof(vm_x64_instr_t));
    *pinstr = instr;
    vm_x64_instr_insert_after(after_me, pinstr);
}

void vm_x64_instr_insert1_before(vm_x64_instr_t *before_me, vm_x64_instr_t instr) {
    vm_x64_instr_t *pinstr = vm_malloc(sizeof(vm_x64_instr_t));
    *pinstr = instr;
    vm_x64_instr_insert_before(before_me, pinstr);
}

vm_x64_cache_t *vm_x64_cache_new(void) {
    vm_x64_cache_t *cache = vm_malloc(sizeof(vm_x64_cache_t));
    *cache = (vm_x64_cache_t) {0};
    return cache;
}

vm_block_t *vm_x64_rblock_version(vm_x64_comp_t *comp, vm_rblock_t *rblock) {
    vm_x64_cache_t *cache = rblock->cache;
    if (cache->block != NULL) {
        return cache->block;
    }
    vm_block_t *ret = vm_malloc(sizeof(vm_block_t));
    cache->block = ret;
    *ret = *rblock->block;
    ret->instrs = vm_malloc(sizeof(vm_instr_t) * rblock->block->len);
    rblock->mark = true;
    for (size_t ninstr = rblock->start; ninstr < rblock->block->len; ninstr++) {
        vm_instr_t instr = vm_rblock_type_specialize_instr(rblock->regs, rblock->block->instrs[ninstr]);
        if (!vm_rblock_type_check_instr(rblock->regs, instr)) __builtin_trap();
        ret->instrs[ninstr] = instr;
        if (instr.out.type == VM_ARG_REG) {
            rblock->regs->tags[instr.out.reg] = instr.tag;
        }
    }
    vm_branch_t branch = vm_rblock_type_specialize_branch(rblock->regs, rblock->block->branch);
    if (!vm_rblock_type_check_branch(rblock->regs, branch)) __builtin_trap();
    switch (branch.op) {
    case VM_BOP_JUMP: {
        ret->branch.targets[0] = vm_x64_rblock_version(comp, vm_rblock_new(branch.targets[0], vm_rblock_regs_dup(rblock->regs, 256)));
        break;
    }
    case VM_BOP_BB: {
        ret->branch.targets[0] = vm_x64_rblock_version(comp, vm_rblock_new(branch.targets[0], vm_rblock_regs_dup(rblock->regs, 256)));
        ret->branch.targets[1] = vm_x64_rblock_version(comp, vm_rblock_new(branch.targets[1], vm_rblock_regs_dup(rblock->regs, 256)));
        break;
    }
    case VM_BOP_BLT: {
        ret->branch.targets[0] = vm_x64_rblock_version(comp, vm_rblock_new(branch.targets[0], vm_rblock_regs_dup(rblock->regs, 256)));
        ret->branch.targets[1] = vm_x64_rblock_version(comp, vm_rblock_new(branch.targets[1], vm_rblock_regs_dup(rblock->regs, 256)));
        break;
    }
    case VM_BOP_BEQ: {
        ret->branch.targets[0] = vm_x64_rblock_version(comp, vm_rblock_new(branch.targets[0], vm_rblock_regs_dup(rblock->regs, 256)));
        ret->branch.targets[1] = vm_x64_rblock_version(comp, vm_rblock_new(branch.targets[1], vm_rblock_regs_dup(rblock->regs, 256)));
        break;
    }
    case VM_BOP_RET: {
        break;
    }
    case VM_BOP_EXIT: {
        break;
    }
    default: {
        __builtin_trap();
    }
    }
    ret->branch = branch;
    return ret;
}

void vm_x64_run(vm_block_t *block) {
    vm_x64_comp_t comp = (vm_x64_comp_t) {
        .ncomps = 0,
    };
    vm_tags_t *regs = vm_rblock_regs_empty(256);
    vm_rblock_t *rblock = vm_rblock_new(block, regs);
    vm_block_t *cur = vm_x64_rblock_version(&comp, rblock);
    vm_print_block(stdout, cur);
}
