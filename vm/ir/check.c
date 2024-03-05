
#include "check.h"

#include "../std/io.h"
#include "rblock.h"

static vm_type_t vm_check_get_tag(vm_arg_t arg) {
    if (arg.type == VM_ARG_LIT) {
        return arg.lit.tag;
    }
    if (arg.type == VM_ARG_REG) {
        return arg.reg_tag;
    }
    return VM_TYPE_UNK;
}

bool vm_check_is_math(vm_type_t type) {
    return vm_type_eq(type, VM_TYPE_I8) || vm_type_eq(type, VM_TYPE_I16) || vm_type_eq(type, VM_TYPE_I32) || vm_type_eq(type, VM_TYPE_I64) || vm_type_eq(type, VM_TYPE_F32) || vm_type_eq(type, VM_TYPE_F64);
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
            vm_type_t a0 = vm_check_get_tag(instr.args[0]);
            vm_type_t a1 = vm_check_get_tag(instr.args[1]);
            if (vm_check_is_math(a0) && vm_check_is_math(a1) && vm_type_eq(a0, a1)) {
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
        case VM_BOP_BLE:
        case VM_BOP_BLT: {
            vm_type_t a0 = vm_check_get_tag(branch.args[0]);
            vm_type_t a1 = vm_check_get_tag(branch.args[1]);
            if (vm_check_is_math(a0) && vm_check_is_math(a1) && vm_type_eq(a0, a1)) {
                return NULL;
            }
            if (vm_type_eq(a0, VM_TYPE_BOOL) && vm_type_eq(a1, VM_TYPE_BOOL)) {
                return NULL;
            }
            vm_io_buffer_t buf = {0};
            vm_io_buffer_format(&buf, "bad less than: ");
            vm_io_format_branch(&buf, branch);
            return buf.buf;
        }
        case VM_BOP_CALL: {
            if (branch.args[0].type == VM_ARG_RFUNC) {
                if (branch.args[0].rfunc != NULL) {
                    return NULL;
                }
            }
            if (vm_type_eq(vm_check_get_tag(branch.args[0]), VM_TYPE_FFI)) {
                return NULL;
            }
            if (vm_type_eq(vm_check_get_tag(branch.args[0]), VM_TYPE_FUN)) {
                return NULL;
            }
            if (vm_type_eq(vm_check_get_tag(branch.args[0]), VM_TYPE_CLOSURE)) {
                return NULL;
            }
            vm_io_buffer_t buf = {0};
            vm_io_buffer_format(&buf, "can't call type ");
            vm_io_format_type(&buf, vm_arg_to_tag(branch.args[0]));
            return buf.buf;
        }
        case VM_BOP_GET: {
            if (vm_type_eq(vm_check_get_tag(branch.args[0]), VM_TYPE_TAB)) {
                return NULL;
            }
            if (vm_type_eq(vm_check_get_tag(branch.args[0]), VM_TYPE_CLOSURE)) {
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
