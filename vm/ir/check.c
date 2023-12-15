
#include "check.h"

#include "rblock.h"

static vm_tag_t vm_check_get_tag(vm_arg_t arg) {
    if (arg.type == VM_ARG_LIT) {
        return arg.lit.tag;
    }
    if (arg.type == VM_ARG_REG) {
        return arg.reg_tag;
    }
    return VM_TAG_UNK;
}

bool vm_check_is_math(vm_tag_t arg) {
    return arg == VM_TAG_I8 || arg == VM_TAG_I16 || arg == VM_TAG_I32 || arg == VM_TAG_I64 || arg == VM_TAG_F32 || arg == VM_TAG_F64;
}

const char *vm_check_instr(vm_instr_t instr) {
    switch (instr.op) {
        case VM_IOP_NOP: {
            return NULL;
        }
        case VM_IOP_MOVE: {
            return NULL;
        }
        case VM_IOP_ADD:
        case VM_IOP_SUB:
        case VM_IOP_MUL:
        case VM_IOP_DIV:
        case VM_IOP_MOD: {
            vm_tag_t a0 = vm_check_get_tag(instr.args[0]);
            vm_tag_t a1 = vm_check_get_tag(instr.args[1]);
            if (vm_check_is_math(a0) && vm_check_is_math(a1) && a0 == a1) {
                return NULL;
            }
            return "bad math";
        }
        default: {
            return NULL;
        }
    }
}

const char *vm_check_branch(vm_branch_t branch) {
    switch (branch.op) {
        case VM_BOP_BEQ: {
            return NULL;
        }
        case VM_BOP_BLT: {
            vm_tag_t a0 = vm_check_get_tag(branch.args[0]);
            vm_tag_t a1 = vm_check_get_tag(branch.args[1]);
            if (vm_check_is_math(a0) && vm_check_is_math(a1) && a0 == a1) {
                return NULL;
            }
            if (a0 == VM_TAG_BOOL && a1 == VM_TAG_BOOL) {
                return NULL;
            }
            return "bad less than";
        }
        case VM_BOP_CALL: {
            if (branch.args[0].type == VM_ARG_RFUNC) {
                if (branch.args[0].rfunc != NULL) {
                    return NULL;
                }
            }
            if (vm_check_get_tag(branch.args[0]) == VM_TAG_FFI) {
                return NULL;
            }
            if (vm_check_get_tag(branch.args[0]) == VM_TAG_FUN) {
                return NULL;
            }
            if (vm_check_get_tag(branch.args[0]) == VM_TAG_CLOSURE) {
                return NULL;
            }
            // vm_io_format_branch(stdout, branch);
            // fprintf(stdout, "\n");
            return "can't call non-func";
        }
        case VM_BOP_GET: {
            if (vm_check_get_tag(branch.args[0]) == VM_TAG_TAB) {
                return NULL;
            }
            if (vm_check_get_tag(branch.args[0]) == VM_TAG_CLOSURE) {
                return NULL;
            }
            return "can't index value: not a table";
        }
        default: {
            return NULL;
        }
    }
}

const char *vm_check_block(vm_block_t *block) {
    if (block == NULL) {
        return false;
    }
    if (block->checked) {
        return block->check;
    }
    const char *ret;
    for (size_t i = 0; i < block->len; i++) {
        const char *err = vm_check_instr(block->instrs[i]);
        if (err != NULL) {
            ret = err;
            goto ret;
        }
    }
    const char *err = vm_check_branch(block->branch);
    if (err != NULL) {
        ret = err;
        goto ret;
    }
    ret = NULL;
    goto ret;
ret:;
    block->checked = true;
    block->check = ret;
    return ret;
}
