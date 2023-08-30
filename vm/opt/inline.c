
#include "../obj.h"
#include "./opt.h"

typedef struct {
    vm_value_t value;
    vm_tag_t tag;
} vm_opt_inline_obj_t;

static vm_arg_t vm_opt_arg_of_value(vm_opt_inline_obj_t arg) {
    switch (arg.tag) {
        case VM_TAG_NIL: {
            return (vm_arg_t){
                .type = VM_ARG_NIL,
            };
        }
        case VM_TAG_BOOL: {
            return (vm_arg_t){
                .logic = arg.value.b,
                .type = VM_ARG_BOOL,
            };
        }
        case VM_TAG_I64: {
            return (vm_arg_t){
                .num = arg.value.i64,
                .type = VM_ARG_NUM,
            };
        }
        case VM_TAG_F64: {
            return (vm_arg_t){
                .num = arg.value.f64,
                .type = VM_ARG_NUM,
            };
        }
        case VM_TAG_STR: {
            return (vm_arg_t){
                .str = arg.value.str,
                .type = VM_ARG_STR,
            };
        }
        case VM_TAG_FFI: {
            return (vm_arg_t){
                .ffi = arg.value.all,
                .type = VM_ARG_FFI,
            };
        }
        case VM_TAG_FUN: {
            return (vm_arg_t){
                .func = arg.value.all,
                .type = VM_ARG_FUNC,
            };
        }
        default: {
            __builtin_trap();
        }
    }
}

static vm_opt_inline_obj_t vm_opt_value_of_arg(vm_opt_inline_obj_t *regs, vm_arg_t arg) {
    switch (arg.type) {
        case VM_ARG_NIL: {
            return (vm_opt_inline_obj_t){
                .tag = VM_TAG_NIL,
            };
        }
        case VM_ARG_BOOL: {
            return (vm_opt_inline_obj_t){
                .value.b = arg.logic,
                .tag = VM_TAG_BOOL,
            };
        }
        case VM_ARG_REG: {
            return regs[arg.reg];
        }
        case VM_ARG_NUM: {
            return (vm_opt_inline_obj_t){
                .value.f64 = arg.num,
                .tag = VM_TAG_F64,
            };
        }
        case VM_ARG_STR: {
            return (vm_opt_inline_obj_t){
                .value.str = arg.str,
                .tag = VM_TAG_STR,
            };
        }
        case VM_ARG_FFI: {
            return (vm_opt_inline_obj_t){
                .value.all = arg.ffi,
                .tag = VM_TAG_FFI,
            };
        }
        case VM_ARG_FUNC: {
            return (vm_opt_inline_obj_t){
                .value.all = arg.func,
                .tag = VM_TAG_FUN,
            };
        }
        default: {
            __builtin_trap();
        }
    }
}

static void vm_opt_inline_from(vm_block_t *block) {
    vm_opt_inline_obj_t regs[256] = {0};
    for (size_t i = 0; i < block->len; i++) {
        vm_instr_t instr = block->instrs[i];
    redo:;
        for (size_t i = 0; i < 6; i++) {
            if (instr.args[i].type == VM_ARG_REG) {
                vm_opt_inline_obj_t obj = regs[instr.args[i].reg];
                if (obj.tag != VM_TAG_UNK) {
                    instr.args[i] = vm_opt_arg_of_value(obj);
                }
            }
        }
        switch (instr.op) {
            case VM_IOP_NOP: {
                break;
            }
            case VM_IOP_MOVE: {
                vm_opt_inline_obj_t lhs = vm_opt_value_of_arg(regs, instr.args[0]);
                regs[instr.out.reg] = lhs;
                if (lhs.tag != VM_TAG_UNK) {
                    instr = (vm_instr_t){
                        .op = VM_IOP_MOVE,
                        .out = instr.out,
                        .args[0] = vm_opt_arg_of_value(lhs),
                    };
                }
                break;
            }
            case VM_IOP_ADD: {
                vm_opt_inline_obj_t lhs = vm_opt_value_of_arg(regs, instr.args[0]);
                vm_opt_inline_obj_t rhs = vm_opt_value_of_arg(regs, instr.args[1]);
                if (lhs.tag == VM_TAG_F64 && rhs.tag == VM_TAG_F64) {
                    instr = (vm_instr_t){
                        .op = VM_IOP_MOVE,
                        .out = instr.out,
                        .args[0] = (vm_arg_t) {
                            .num = lhs.value.f64 + rhs.value.f64,
                            .type = VM_ARG_NUM,
                        },
                    };
                    goto redo;
                } else {
                    regs[instr.out.reg] = (vm_opt_inline_obj_t){
                        .tag = VM_TAG_UNK,
                    };
                }
                break;
            }
            case VM_IOP_SUB: {
                vm_opt_inline_obj_t lhs = vm_opt_value_of_arg(regs, instr.args[0]);
                vm_opt_inline_obj_t rhs = vm_opt_value_of_arg(regs, instr.args[1]);
                if (lhs.tag == VM_TAG_F64 && rhs.tag == VM_TAG_F64) {
                    instr = (vm_instr_t){
                        .op = VM_IOP_MOVE,
                        .out = instr.out,
                        .args[0] = (vm_arg_t) {
                            .num = lhs.value.f64 - rhs.value.f64,
                            .type = VM_ARG_NUM,
                        },
                    };
                    goto redo;
                } else {
                    regs[instr.out.reg] = (vm_opt_inline_obj_t){
                        .tag = VM_TAG_UNK,
                    };
                }
                break;
            }
            case VM_IOP_MUL: {
                vm_opt_inline_obj_t lhs = vm_opt_value_of_arg(regs, instr.args[0]);
                vm_opt_inline_obj_t rhs = vm_opt_value_of_arg(regs, instr.args[1]);
                if (lhs.tag == VM_TAG_F64 && rhs.tag == VM_TAG_F64) {
                    instr = (vm_instr_t){
                        .op = VM_IOP_MOVE,
                        .out = instr.out,
                        .args[0] = (vm_arg_t) {
                            .num = lhs.value.f64 * rhs.value.f64,
                            .type = VM_ARG_NUM,
                        },
                    };
                    goto redo;
                } else {
                    regs[instr.out.reg] = (vm_opt_inline_obj_t){
                        .tag = VM_TAG_UNK,
                    };
                }
                break;
            }
            case VM_IOP_DIV: {
                vm_opt_inline_obj_t lhs = vm_opt_value_of_arg(regs, instr.args[0]);
                vm_opt_inline_obj_t rhs = vm_opt_value_of_arg(regs, instr.args[1]);
                if (lhs.tag == VM_TAG_F64 && rhs.tag == VM_TAG_F64) {
                    instr = (vm_instr_t){
                        .op = VM_IOP_MOVE,
                        .out = instr.out,
                        .args[0] = (vm_arg_t) {
                            .num = lhs.value.f64 / rhs.value.f64,
                            .type = VM_ARG_NUM,
                        },
                    };
                    goto redo;
                } else {
                    regs[instr.out.reg] = (vm_opt_inline_obj_t){
                        .tag = VM_TAG_UNK,
                    };
                }
                break;
            }
            case VM_IOP_MOD: {
                vm_opt_inline_obj_t lhs = vm_opt_value_of_arg(regs, instr.args[0]);
                vm_opt_inline_obj_t rhs = vm_opt_value_of_arg(regs, instr.args[1]);
                if (lhs.tag == VM_TAG_F64 && rhs.tag == VM_TAG_F64) {
                    instr = (vm_instr_t){
                        .op = VM_IOP_MOVE,
                        .out = instr.out,
                        .args[0] = (vm_arg_t) {
                            .num = fmod(lhs.value.f64, rhs.value.f64),
                            .type = VM_ARG_NUM,
                        },
                    };
                    goto redo;
                } else {
                    regs[instr.out.reg] = (vm_opt_inline_obj_t){
                        .tag = VM_TAG_UNK,
                    };
                }
                break;
            }
            default: {
                if (instr.out.type == VM_ARG_REG) {
                    regs[instr.out.reg] = (vm_opt_inline_obj_t){
                        .tag = VM_TAG_UNK,
                    };
                }
                break;
            }
        }
        block->instrs[i] = instr;
    }
    vm_branch_t branch = block->branch;
    for (size_t i = 0; i < 2; i++) {
        if (branch.args[i].type == VM_ARG_REG) {
            vm_opt_inline_obj_t obj = regs[branch.args[i].reg];
            if (obj.tag != VM_TAG_UNK) {
                branch.args[i] = vm_opt_arg_of_value(regs[branch.args[i].reg]);
            }
        }
    }
}

void vm_opt_inline(vm_block_t *block) {
    vm_opt_pass(block, .post = &vm_opt_inline_from, .final = &vm_block_info);
}
