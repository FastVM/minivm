
#include "ir.h"

#include "../std/io.h"

void vm_block_realloc(vm_block_t *block, vm_instr_t instr) {
    if (block->len + 4 >= block->alloc) {
        block->alloc = (block->len + 4) * 4;
        block->instrs =
            vm_realloc(block->instrs, sizeof(vm_instr_t) * block->alloc);
    }
    block->instrs[block->len++] = instr;
}

void vm_io_format_arg(vm_io_buffer_t *out, vm_arg_t val) {
    switch (val.type) {
        case VM_ARG_LIT: {
            vm_io_print_lit(out, val.lit);
            break;
        }
        case VM_ARG_REG: {
            if (vm_type_eq(val.reg_tag, VM_TYPE_UNK)) {
                vm_io_buffer_format(out, "%%%zu", (size_t)val.reg);
            } else {
                vm_io_buffer_format(out, "%%%zu:", (size_t)val.reg);
                vm_io_format_type(out, val.reg_tag);
            }
            break;
        }
        case VM_ARG_FUN: {
            if (val.func == NULL) {
                vm_io_buffer_format(out, "<null.fun>");
            } else {
                vm_io_buffer_format(out, ".%zi", val.func->id);
            }
            break;
        }
        case VM_ARG_RFUNC: {
            vm_io_buffer_format(out, "<rfunc.%zi>", val.rfunc->block->id);
            break;
        };
    }
}

void vm_io_format_type(vm_io_buffer_t *out, vm_type_t type) {
    switch (vm_type_tag(type)) {
        case VM_TAG_UNK: {
            vm_io_buffer_format(out, "unk");
            break;
        }
        case VM_TAG_NIL: {
            vm_io_buffer_format(out, "nil");
            break;
        }
        case VM_TAG_BOOL: {
            vm_io_buffer_format(out, "bool");
            break;
        }
        case VM_TAG_I8: {
            vm_io_buffer_format(out, "i8");
            break;
        }
        case VM_TAG_I16: {
            vm_io_buffer_format(out, "i16");
            break;
        }
        case VM_TAG_I32: {
            vm_io_buffer_format(out, "i32");
            break;
        }
        case VM_TAG_I64: {
            vm_io_buffer_format(out, "i64");
            break;
        }
        case VM_TAG_F32: {
            vm_io_buffer_format(out, "f32");
            break;
        }
        case VM_TAG_F64: {
            vm_io_buffer_format(out, "f64");
            break;
        }
        case VM_TAG_STR: {
            vm_io_buffer_format(out, "str");
            break;
        }
        case VM_TAG_CLOSURE: {
            vm_io_buffer_format(out, "closure");
            break;
        }
        case VM_TAG_FUN: {
            vm_io_buffer_format(out, "rawfunc");
            break;
        }
        case VM_TAG_TAB: {
            vm_io_buffer_format(out, "table");
            break;
        }
        case VM_TAG_FFI: {
            vm_io_buffer_format(out, "ffi");
            break;
        }
        case VM_TAG_ERROR: {
            vm_io_buffer_format(out, "error");
            break;
        }
        default: {
            vm_io_buffer_format(out, "<tag: invalid>");
        }
    }
}

void vm_io_format_branch(vm_io_buffer_t *out, vm_branch_t val) {
    if (val.out.type != VM_ARG_NONE) {
        vm_io_format_arg(out, val.out);
        vm_io_buffer_format(out, " <- ");
    }
    switch (val.op) {
        case VM_BOP_JUMP: {
            vm_io_buffer_format(out, "jump");
            break;
        }
        case VM_BOP_BB: {
            vm_io_buffer_format(out, "bb");
            break;
        }
        case VM_BOP_BTYPE: {
            vm_io_buffer_format(out, "btype");
            break;
        }
        case VM_BOP_BLT: {
            vm_io_buffer_format(out, "blt");
            break;
        }
        case VM_BOP_BLE: {
            vm_io_buffer_format(out, "ble");
            break;
        }
        case VM_BOP_BEQ: {
            vm_io_buffer_format(out, "beq");
            break;
        }
        case VM_BOP_RET: {
            vm_io_buffer_format(out, "ret");
            break;
        }
        case VM_BOP_GET: {
            vm_io_buffer_format(out, "get");
            break;
        }
        case VM_BOP_LOAD: {
            vm_io_buffer_format(out, "load");
            break;
        }
        case VM_BOP_CALL: {
            vm_io_buffer_format(out, "call");
            break;
        }
    }
    if (!vm_type_eq(val.tag, VM_TYPE_UNK)) {
        vm_io_buffer_format(out, ".");
        vm_io_format_type(out, val.tag);
    }
    if (val.op == VM_BOP_CALL) {
        vm_io_buffer_format(out, " ");
        vm_io_format_arg(out, val.args[0]);
        vm_io_buffer_format(out, "(");
        for (size_t i = 1; val.args[i].type != VM_ARG_NONE; i++) {
            if (i != 1) {
                vm_io_buffer_format(out, ", ");
            }
            vm_io_format_arg(out, val.args[i]);
        }
        vm_io_buffer_format(out, ")");
    } else {
        for (size_t i = 0; val.args[i].type != VM_ARG_NONE; i++) {
            vm_io_buffer_format(out, " ");
            vm_io_format_arg(out, val.args[i]);
        }
    }
    if (val.op == VM_BOP_LOAD || val.op == VM_BOP_GET || val.op == VM_BOP_CALL) {
        vm_io_buffer_format(out, " then .%zi", (size_t)val.targets[0]->id);
        vm_io_buffer_format(out, "(");
        for (size_t i = 0; i < val.targets[0]->nargs; i++) {
            if (i != 0) {
                vm_io_buffer_format(out, ", ");
            }
            vm_io_format_arg(out, val.targets[0]->args[i]);
        }
        vm_io_buffer_format(out, ")");
    } else {
        if (val.targets[0]) {
            vm_io_buffer_format(out, " .%zi", (size_t)val.targets[0]->id);
            vm_io_buffer_format(out, "(");
            for (size_t i = 0; i < val.targets[0]->nargs; i++) {
                if (i != 0) {
                    vm_io_buffer_format(out, ", ");
                }
                vm_io_format_arg(out, val.targets[0]->args[i]);
            }
            vm_io_buffer_format(out, ")");
        }
        if (val.targets[1]) {
            vm_io_buffer_format(out, " .%zi", (size_t)val.targets[1]->id);
            vm_io_buffer_format(out, "(");
            for (size_t i = 0; i < val.targets[1]->nargs; i++) {
                if (i != 0) {
                    vm_io_buffer_format(out, ", ");
                }
                vm_io_format_arg(out, val.targets[1]->args[i]);
            }
            vm_io_buffer_format(out, ")");
        }
    }
}

void vm_io_format_instr(vm_io_buffer_t *out, vm_instr_t val) {
    if (val.op == VM_IOP_NOP) {
        vm_io_buffer_format(out, "nop");
        return;
    }
    if (val.out.type != VM_ARG_NONE) {
        vm_io_format_arg(out, val.out);
        vm_io_buffer_format(out, " <- ");
    }
    switch (val.op) {
        case VM_IOP_MOVE: {
            vm_io_buffer_format(out, "move");
            break;
        }
        case VM_IOP_ADD: {
            vm_io_buffer_format(out, "add");
            break;
        }
        case VM_IOP_SUB: {
            vm_io_buffer_format(out, "sub");
            break;
        }
        case VM_IOP_MUL: {
            vm_io_buffer_format(out, "mul");
            break;
        }
        case VM_IOP_DIV: {
            vm_io_buffer_format(out, "div");
            break;
        }
        case VM_IOP_IDIV: {
            vm_io_buffer_format(out, "idiv");
            break;
        }
        case VM_IOP_MOD: {
            vm_io_buffer_format(out, "mod");
            break;
        }
        case VM_IOP_TABLE_SET: {
            vm_io_buffer_format(out, "set");
            break;
        }
        case VM_IOP_TABLE_NEW: {
            vm_io_buffer_format(out, "new");
            break;
        }
        case VM_IOP_STD: {
            vm_io_buffer_format(out, "std");
            break;
        }
        case VM_IOP_TABLE_LEN: {
            vm_io_buffer_format(out, "len");
            break;
        }
        default: {
            vm_io_buffer_format(out, "<instr: %zu>", (size_t)val.op);
            break;
        }
    }
    if (!vm_type_eq(val.tag, VM_TYPE_UNK)) {
        vm_io_buffer_format(out, ".");
        vm_io_format_type(out, val.tag);
    }
    for (size_t i = 0; val.args[i].type != VM_ARG_NONE; i++) {
        vm_io_buffer_format(out, " ");
        vm_io_format_arg(out, val.args[i]);
    }
}

void vm_io_format_block(vm_io_buffer_t *out, vm_block_t *val) {
    if (val == NULL) {
        printf("<block: null>\n");
    }
    vm_io_buffer_format(out, ".%zi(", val->id);
    for (size_t i = 0; i < val->nargs; i++) {
        if (i != 0) {
            vm_io_buffer_format(out, ", ");
        }
        vm_io_format_arg(out, val->args[i]);
    }
    vm_io_buffer_format(out, "):\n");
    for (size_t i = 0; i < val->len; i++) {
        if (val->instrs[i].op == VM_IOP_NOP) {
            continue;
        }
        vm_io_buffer_format(out, "    ");
        vm_io_format_instr(out, val->instrs[i]);
        vm_io_buffer_format(out, "\n");
    }
    if (val->branch.op != VM_BOP_FALL) {
        vm_io_buffer_format(out, "    ");
        vm_io_format_branch(out, val->branch);
        vm_io_buffer_format(out, "\n");
    } else {
        vm_io_buffer_format(out, "    <fall>\n");
    }
}

void vm_io_format_blocks(vm_io_buffer_t *out, vm_blocks_t *blocks) {
    for (size_t i = 0; i < blocks->len; i++) {
        vm_block_t *block = blocks->blocks[i];
        if (block->id < 0) {
            continue;
        }
        vm_io_format_block(out, block);
    }
}

enum {
    VM_INFO_REG_UNK,
    VM_INFO_REG_DEF,
    VM_INFO_REG_ARG,
};

void vm_block_info(size_t nblocks, vm_block_t **blocks) {
    for (size_t i = 0; i < nblocks; i++) {
        vm_block_t *block = blocks[i];
        if (block->id < 0) {
            continue;
        }
        size_t nregs = 1;
        for (size_t j = 0; j < block->nargs; j++) {
            vm_arg_t arg = block->args[j];
            if (arg.type == VM_ARG_REG && arg.reg >= nregs) {
                nregs = arg.reg + 1;
            }
        }
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
        if (block->branch.out.type == VM_ARG_REG &&
            block->branch.out.reg >= nregs) {
            nregs = block->branch.out.reg + 1;
        }
        for (size_t j = 0; block->branch.args[j].type != VM_ARG_NONE; j++) {
            if (block->branch.args[j].type == VM_ARG_REG && block->branch.args[j].reg >= nregs) {
                nregs = block->branch.args[j].reg + 1;
            }
        }
        block->nregs = nregs;
    }
    bool redo = true;
    while (redo) {
        redo = false;
        for (size_t i = 0; i < nblocks; i++) {
            vm_block_t *block = blocks[i];
            for (size_t t = 0; t < 2; t++) {
                vm_block_t *target = block->branch.targets[t];
                if (target == NULL) {
                    break;
                }
                if (target->nregs > block->nregs) {
                    block->nregs = target->nregs;
                    redo = true;
                }
            }
        }
    }
    redo = true;
    while (redo) {
        redo = false;
        for (size_t i = 0; i < nblocks; i++) {
            vm_block_t *block = blocks[i];
            if (block->id < 0) {
                continue;
            }
            if (block->isfunc) {
                continue;
            }
            uint8_t *regs = vm_malloc(sizeof(uint8_t) * block->nregs);
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
                if (instr->out.type == VM_ARG_REG &&
                    regs[instr->out.reg] == VM_INFO_REG_UNK) {
                    regs[instr->out.reg] = VM_INFO_REG_DEF;
                }
            }
            for (size_t j = 0; block->branch.args[j].type != VM_ARG_NONE; j++) {
                if (block->branch.args[j].type == VM_ARG_REG &&
                    regs[block->branch.args[j].reg] == VM_INFO_REG_UNK) {
                    regs[block->branch.args[j].reg] = VM_INFO_REG_ARG;
                    nargs += 1;
                }
            }
            if (block->branch.out.type == VM_ARG_REG &&
                regs[block->branch.out.reg] == VM_INFO_REG_UNK) {
                regs[block->branch.out.reg] = VM_INFO_REG_DEF;
            }
            for (size_t t = 0; t < 2; t++) {
                vm_block_t *target = block->branch.targets[t];
                if (target == NULL) {
                    break;
                }
                for (size_t a = 0; a < target->nargs; a++) {
                    if (target->args[a].type == VM_ARG_REG &&
                        regs[target->args[a].reg] == VM_INFO_REG_UNK) {
                        regs[target->args[a].reg] = VM_INFO_REG_ARG;
                        nargs += 1;
                    }
                }
            }
            size_t next_nargs = 0;
            vm_arg_t *next_args = vm_malloc(sizeof(vm_arg_t) * nargs);
            for (size_t reg = 0; reg < block->nregs; reg++) {
                if (regs[reg] == VM_INFO_REG_ARG) {
                    next_args[next_nargs++] = (vm_arg_t){
                        .type = VM_ARG_REG,
                        .reg = (uint32_t)reg,
                    };
                }
            }
            vm_free(regs);
            if (next_nargs == block->nargs && !redo) {
                for (size_t a = 0; a < next_nargs; a++) {
                    if (next_args[a].reg != block->args[a].reg) {
                        redo = true;
                        break;
                    }
                }
            } else {
                redo = true;
            }
            vm_free(block->args);
            block->nargs = next_nargs;
            block->args = next_args;
        }
    }
}

vm_type_t vm_arg_to_tag(vm_arg_t arg) {
    if (arg.type == VM_ARG_REG) {
        return arg.reg_tag;
    } else if (arg.type == VM_ARG_LIT) {
        return arg.lit.tag;
    } else if (arg.type == VM_ARG_FUN) {
        return VM_TYPE_FUN;
    } else {
        return VM_TYPE_UNK;
    }
}

void vm_free_block_sub(vm_block_t *block) {
    for (size_t i = 0; i < block->len; i++) {
        vm_free(block->instrs[i].args);
    }
    vm_free(block->branch.args);
    vm_free(block->instrs);
    vm_free(block->args);
    vm_free(block);
}

void vm_free_block(vm_block_t *block) {
    for (size_t i = 0; i < block->len; i++) {
        vm_free(block->instrs[i].args);
    }
    vm_free(block->instrs);
    vm_free(block->branch.args);
    vm_free(block->args);
    vm_free(block->pass);
    vm_free(block->check);
    vm_free(block->cache.keys);
    vm_free(block->cache.values);
    vm_free(block);
}
