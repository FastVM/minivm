
#include "check.h"

#include "rblock.h"

static vm_tag_t vm_check_get_tag(vm_arg_t arg) {
    if (arg.type == VM_ARG_NUM) {
        return arg.num.tag;
    }
    if (arg.type == VM_ARG_REG) {
        return arg.reg_tag;
    }
    return VM_TAG_UNK;
}

static bool vm_check_is_math(vm_tag_t arg) {
    return arg == VM_TAG_I8 || arg == VM_TAG_I16 || arg == VM_TAG_I32 || arg == VM_TAG_I64 || arg == VM_TAG_F32 || arg == VM_TAG_F64;
}

bool vm_check_instr(vm_instr_t instr) {
    switch (instr.op) {
        case VM_IOP_NOP: {
            return true;
        }
        case VM_IOP_MOVE: {
            return true;
        }
        case VM_IOP_ADD:
        case VM_IOP_SUB:
        case VM_IOP_MUL:
        case VM_IOP_DIV:
        case VM_IOP_MOD: {
            vm_tag_t a0 = vm_check_get_tag(instr.args[0]);
            vm_tag_t a1 = vm_check_get_tag(instr.args[1]);
            return vm_check_is_math(a0) && vm_check_is_math(a1) && a0 == a1;
        }
        default: {
            return true;
        }
    }
}

bool vm_check_branch(vm_branch_t branch) {
    switch (branch.op) {
        case VM_BOP_BEQ:
        case VM_BOP_BLT: {
            vm_tag_t a0 = vm_check_get_tag(branch.args[0]);
            vm_tag_t a1 = vm_check_get_tag(branch.args[1]);
            return vm_check_is_math(a0) && vm_check_is_math(a1) && a0 == a1;
        }
        case VM_BOP_CALL: {
            if (branch.args[0].type == VM_ARG_RFUNC) {
                if (branch.args[0].rfunc != NULL) {
                    for (size_t i = 1; i < VM_TAG_MAX; i++) {
                        vm_block_t *next = vm_rblock_version(branch.rtargets[i]);
                        if (vm_check_block(next)) {
                            return true;
                        }
                    }
                    return false;
                }
            }
            if (branch.args[0].type == VM_ARG_REG) {
                if (branch.args[0].reg_tag == VM_TAG_FFI) {
                    return true;
                }
            }
            return false;
        }
        case VM_BOP_GET: {
            if (branch.args[0].type == VM_ARG_REG) {
                if (branch.args[0].reg_tag == VM_TAG_TAB) {
                    for (size_t i = 1; i < VM_TAG_MAX; i++) {
                        vm_block_t *next = vm_rblock_version(branch.rtargets[i]);
                        if (vm_check_block(next)) {
                            return true;
                        }
                    }
                }
            }
            return false;
        }
        default: {
            return true;
        }
    }
}

bool vm_check_block(vm_block_t *block) {
    if (block == NULL) {
        return false;
    }
    if (block->checked) {
        return block->check;
    }
    bool ret;
    for (size_t i = 0; i < block->len; i++) {
        if (!vm_check_instr(block->instrs[i])) {
            ret = false;
            goto ret;
        }
    }
    if (!vm_check_branch(block->branch)) {
        ret = false;
        goto ret;
    }
    ret = true;
    goto ret;
ret:;
    block->checked = true;
    block->check = ret;
    return ret;
}
