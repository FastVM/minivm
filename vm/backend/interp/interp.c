
#include "interp.h"
#include "../../obj.h"

struct vm_interp_t;
typedef struct vm_interp_t vm_interp_t;

struct vm_interp_t {
    vm_config_t *config;
    vm_blocks_t *blocks;
    vm_table_t *std;
    vm_std_value_t *regs;
    vm_std_closure_t closure;
};

enum {
    VM_IOP_MOVE_I = VM_MAX_IOP,
    VM_IOP_MOVE_R,

    VM_IOP_ADD_RI,
    VM_IOP_ADD_IR,
    VM_IOP_ADD_RR,

    VM_IOP_SUB_RI,
    VM_IOP_SUB_IR,
    VM_IOP_SUB_RR,

    VM_IOP_MUL_RI,
    VM_IOP_MUL_IR,
    VM_IOP_MUL_RR,

    VM_IOP_DIV_RI,
    VM_IOP_DIV_IR,
    VM_IOP_DIV_RR,

    VM_IOP_IDIV_RI,
    VM_IOP_IDIV_IR,
    VM_IOP_IDIV_RR,

    VM_IOP_MOD_RI,
    VM_IOP_MOD_IR,
    VM_IOP_MOD_RR,

    VM_MAX_INT_IOP,
};

bool vm_interp_value_lt(vm_std_value_t lhs, vm_std_value_t rhs) {
    switch (vm_type_tag(lhs.tag)) {
        case VM_TAG_I8: {
            switch (vm_type_tag(rhs.tag)) {
                case VM_TAG_I8: {
                    return lhs.value.i8 < rhs.value.i8;
                }
                case VM_TAG_I16: {
                    return lhs.value.i8 < rhs.value.i16;
                }
                case VM_TAG_I32: {
                    return lhs.value.i8 < rhs.value.i32;
                }
                case VM_TAG_I64: {
                    return lhs.value.i8 < rhs.value.i64;
                }
                case VM_TAG_F32: {
                    return lhs.value.i8 < rhs.value.f32;
                }
                case VM_TAG_F64: {
                    return lhs.value.i8 < rhs.value.f64;
                }
                default: {
                    return false;
                }
            }
        }
        case VM_TAG_I16: {
            switch (vm_type_tag(rhs.tag)) {
                case VM_TAG_I8: {
                    return lhs.value.i16 < rhs.value.i8;
                }
                case VM_TAG_I16: {
                    return lhs.value.i16 < rhs.value.i16;
                }
                case VM_TAG_I32: {
                    return lhs.value.i16 < rhs.value.i32;
                }
                case VM_TAG_I64: {
                    return lhs.value.i16 < rhs.value.i64;
                }
                case VM_TAG_F32: {
                    return lhs.value.i16 < rhs.value.f32;
                }
                case VM_TAG_F64: {
                    return lhs.value.i16 < rhs.value.f64;
                }
                default: {
                    return false;
                }
            }
        }
        case VM_TAG_I32: {
            switch (vm_type_tag(rhs.tag)) {
                case VM_TAG_I8: {
                    return lhs.value.i32 < rhs.value.i8;
                }
                case VM_TAG_I16: {
                    return lhs.value.i32 < rhs.value.i16;
                }
                case VM_TAG_I32: {
                    return lhs.value.i32 < rhs.value.i32;
                }
                case VM_TAG_I64: {
                    return lhs.value.i32 < rhs.value.i64;
                }
                case VM_TAG_F32: {
                    return lhs.value.i32 < rhs.value.f32;
                }
                case VM_TAG_F64: {
                    return lhs.value.i32 < rhs.value.f64;
                }
                default: {
                    return false;
                }
            }
        }
        case VM_TAG_I64: {
            switch (vm_type_tag(rhs.tag)) {
                case VM_TAG_I8: {
                    return lhs.value.i64 < rhs.value.i8;
                }
                case VM_TAG_I16: {
                    return lhs.value.i64 < rhs.value.i16;
                }
                case VM_TAG_I32: {
                    return lhs.value.i64 < rhs.value.i32;
                }
                case VM_TAG_I64: {
                    return lhs.value.i64 < rhs.value.i64;
                }
                case VM_TAG_F32: {
                    return lhs.value.i64 < rhs.value.f32;
                }
                case VM_TAG_F64: {
                    return lhs.value.i64 < rhs.value.f64;
                }
                default: {
                    return false;
                }
            }
        }
        case VM_TAG_F32: {
            switch (vm_type_tag(rhs.tag)) {
                case VM_TAG_I8: {
                    return lhs.value.f32 < rhs.value.i8;
                }
                case VM_TAG_I16: {
                    return lhs.value.f32 < rhs.value.i16;
                }
                case VM_TAG_I32: {
                    return lhs.value.f32 < rhs.value.i32;
                }
                case VM_TAG_I64: {
                    return lhs.value.f32 < rhs.value.i64;
                }
                case VM_TAG_F32: {
                    return lhs.value.f32 < rhs.value.f32;
                }
                case VM_TAG_F64: {
                    return lhs.value.f32 < rhs.value.f64;
                }
                default: {
                    return false;
                }
            }
        }
        case VM_TAG_F64: {
            switch (vm_type_tag(rhs.tag)) {
                case VM_TAG_I8: {
                    return lhs.value.f64 < rhs.value.i8;
                }
                case VM_TAG_I16: {
                    return lhs.value.f64 < rhs.value.i16;
                }
                case VM_TAG_I32: {
                    return lhs.value.f64 < rhs.value.i32;
                }
                case VM_TAG_I64: {
                    return lhs.value.f64 < rhs.value.i64;
                }
                case VM_TAG_F32: {
                    return lhs.value.f64 < rhs.value.f32;
                }
                case VM_TAG_F64: {
                    return lhs.value.f64 < rhs.value.f64;
                }
                default: {
                    return false;
                }
            }
        }
        default: {
            return false;
        }
    }
}

bool vm_interp_value_le(vm_std_value_t lhs, vm_std_value_t rhs) {
    switch (vm_type_tag(lhs.tag)) {
        case VM_TAG_I8: {
            switch (vm_type_tag(rhs.tag)) {
                case VM_TAG_I8: {
                    return lhs.value.i8 <= rhs.value.i8;
                }
                case VM_TAG_I16: {
                    return lhs.value.i8 <= rhs.value.i16;
                }
                case VM_TAG_I32: {
                    return lhs.value.i8 <= rhs.value.i32;
                }
                case VM_TAG_I64: {
                    return lhs.value.i8 <= rhs.value.i64;
                }
                case VM_TAG_F32: {
                    return lhs.value.i8 <= rhs.value.f32;
                }
                case VM_TAG_F64: {
                    return lhs.value.i8 <= rhs.value.f64;
                }
                default: {
                    return false;
                }
            }
        }
        case VM_TAG_I16: {
            switch (vm_type_tag(rhs.tag)) {
                case VM_TAG_I8: {
                    return lhs.value.i16 <= rhs.value.i8;
                }
                case VM_TAG_I16: {
                    return lhs.value.i16 <= rhs.value.i16;
                }
                case VM_TAG_I32: {
                    return lhs.value.i16 <= rhs.value.i32;
                }
                case VM_TAG_I64: {
                    return lhs.value.i16 <= rhs.value.i64;
                }
                case VM_TAG_F32: {
                    return lhs.value.i16 <= rhs.value.f32;
                }
                case VM_TAG_F64: {
                    return lhs.value.i16 <= rhs.value.f64;
                }
                default: {
                    return false;
                }
            }
        }
        case VM_TAG_I32: {
            switch (vm_type_tag(rhs.tag)) {
                case VM_TAG_I8: {
                    return lhs.value.i32 <= rhs.value.i8;
                }
                case VM_TAG_I16: {
                    return lhs.value.i32 <= rhs.value.i16;
                }
                case VM_TAG_I32: {
                    return lhs.value.i32 <= rhs.value.i32;
                }
                case VM_TAG_I64: {
                    return lhs.value.i32 <= rhs.value.i64;
                }
                case VM_TAG_F32: {
                    return lhs.value.i32 <= rhs.value.f32;
                }
                case VM_TAG_F64: {
                    return lhs.value.i32 <= rhs.value.f64;
                }
                default: {
                    return false;
                }
            }
        }
        case VM_TAG_I64: {
            switch (vm_type_tag(rhs.tag)) {
                case VM_TAG_I8: {
                    return lhs.value.i64 <= rhs.value.i8;
                }
                case VM_TAG_I16: {
                    return lhs.value.i64 <= rhs.value.i16;
                }
                case VM_TAG_I32: {
                    return lhs.value.i64 <= rhs.value.i32;
                }
                case VM_TAG_I64: {
                    return lhs.value.i64 <= rhs.value.i64;
                }
                case VM_TAG_F32: {
                    return lhs.value.i64 <= rhs.value.f32;
                }
                case VM_TAG_F64: {
                    return lhs.value.i64 <= rhs.value.f64;
                }
                default: {
                    return false;
                }
            }
        }
        case VM_TAG_F32: {
            switch (vm_type_tag(rhs.tag)) {
                case VM_TAG_I8: {
                    return lhs.value.f32 <= rhs.value.i8;
                }
                case VM_TAG_I16: {
                    return lhs.value.f32 <= rhs.value.i16;
                }
                case VM_TAG_I32: {
                    return lhs.value.f32 <= rhs.value.i32;
                }
                case VM_TAG_I64: {
                    return lhs.value.f32 <= rhs.value.i64;
                }
                case VM_TAG_F32: {
                    return lhs.value.f32 <= rhs.value.f32;
                }
                case VM_TAG_F64: {
                    return lhs.value.f32 <= rhs.value.f64;
                }
                default: {
                    return false;
                }
            }
        }
        case VM_TAG_F64: {
            switch (vm_type_tag(rhs.tag)) {
                case VM_TAG_I8: {
                    return lhs.value.f64 <= rhs.value.i8;
                }
                case VM_TAG_I16: {
                    return lhs.value.f64 <= rhs.value.i16;
                }
                case VM_TAG_I32: {
                    return lhs.value.f64 <= rhs.value.i32;
                }
                case VM_TAG_I64: {
                    return lhs.value.f64 <= rhs.value.i64;
                }
                case VM_TAG_F32: {
                    return lhs.value.f64 <= rhs.value.f32;
                }
                case VM_TAG_F64: {
                    return lhs.value.f64 <= rhs.value.f64;
                }
                default: {
                    return false;
                }
            }
        }
        default: {
            return false;
        }
    }
}

vm_std_value_t vm_interp_arg(vm_std_value_t *regs, vm_arg_t arg) {
    switch (arg.type) {
        case VM_ARG_REG: {
            return regs[arg.reg];
        }
        case VM_ARG_LIT: {
            return arg.lit;
        }
        case VM_ARG_FUN: {
            return (vm_std_value_t) {
                .tag = VM_TAG_FUN,
                .value.i32 = (int32_t) arg.func->id,
            };
        }
        case VM_ARG_RFUNC: {
            __builtin_trap();
            break;
        }
    }
}

void vm_interp_out_arg(vm_std_value_t *regs, vm_arg_t key, vm_std_value_t value) {
    regs[key.reg] = value;
}

vm_std_value_t vm_interp_block(vm_interp_t *interp, vm_std_value_t *regs, vm_block_t *block) {
    vm_std_value_t *next_regs = &regs[block->nregs];
new_block:;
    size_t ninstr = block->len;
    vm_instr_t *instrs = block->instrs;
    vm_branch_t *branch = &block->branch;
    for (size_t i = 0; i < ninstr; i++) {
        vm_instr_t instr;
        goto no_redo_instr;
    redo_instr:;
        instrs[i] = instr;
        goto after_read_instr;
    no_redo_instr:;
        instr = instrs[i];
    after_read_instr:;
        switch (instr.op) {
            case VM_IOP_NOP: {
                break;
            }
            case VM_IOP_MOVE: {
                if (instr.args[0].type == VM_ARG_LIT) {
                    instr.op = VM_IOP_MOVE_I;
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_REG) {
                    instr.op = VM_IOP_MOVE_R;
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_FUN) {
                    instr.op = VM_IOP_MOVE_I;
                    instr.args[0] = (vm_arg_t) {
                        .type = VM_ARG_LIT,
                        .lit = (vm_std_value_t) {
                            .tag = VM_TAG_FUN,
                            .value.i32 = (int32_t) instr.args[0].func->id,
                        },
                    };
                    goto redo_instr;
                } else {
                    __builtin_trap();
                    break;
                }
            }
            case VM_IOP_ADD: {
                if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_LIT) {
                    vm_std_value_t out;
                    vm_std_value_t v1 = instr.args[0].lit;
                    vm_std_value_t v2 = instr.args[1].lit;
                    #define OP(x, y) ((x)+(y))
                    #include "binop.inc"
                    vm_arg_t *args = vm_malloc(sizeof(vm_arg_t));
                    args[0] = (vm_arg_t) {
                        .type = VM_ARG_LIT,
                        .lit = out,
                    };
                    instr = (vm_instr_t) {
                        .op = VM_IOP_MOVE_I,
                        .out = instr.out,
                        .args = args,
                    };
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                    instr.op = VM_IOP_ADD_IR;
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                    instr.op = VM_IOP_ADD_RI;
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    instr.op = VM_IOP_ADD_RR;
                    goto redo_instr;
                } else {
                    __builtin_trap();
                    break;
                }
            }
            case VM_IOP_SUB: {
                if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_LIT) {
                    vm_std_value_t out;
                    vm_std_value_t v1 = instr.args[0].lit;
                    vm_std_value_t v2 = instr.args[1].lit;
                    #define OP(x, y) ((x)-(y))
                    #include "binop.inc"
                    vm_arg_t *args = vm_malloc(sizeof(vm_arg_t));
                    args[0] = (vm_arg_t) {
                        .type = VM_ARG_LIT,
                        .lit = out,
                    };
                    instr = (vm_instr_t) {
                        .op = VM_IOP_MOVE_I,
                        .out = instr.out,
                        .args = args,
                    };
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                    instr.op = VM_IOP_SUB_IR;
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                    instr.op = VM_IOP_SUB_RI;
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    instr.op = VM_IOP_SUB_RR;
                    goto redo_instr;
                } else {
                    __builtin_trap();
                    break;
                }
            }
            case VM_IOP_MUL: {
                if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_LIT) {
                    vm_std_value_t out;
                    vm_std_value_t v1 = instr.args[0].lit;
                    vm_std_value_t v2 = instr.args[1].lit;
                    #define OP(x, y) ((x)*(y))
                    #include "binop.inc"
                    vm_arg_t *args = vm_malloc(sizeof(vm_arg_t));
                    args[0] = (vm_arg_t) {
                        .type = VM_ARG_LIT,
                        .lit = out,
                    };
                    instr = (vm_instr_t) {
                        .op = VM_IOP_MOVE_I,
                        .out = instr.out,
                        .args = args,
                    };
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                    instr.op = VM_IOP_MUL_IR;
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                    instr.op = VM_IOP_MUL_RI;
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    instr.op = VM_IOP_MUL_RR;
                    goto redo_instr;
                } else {
                    __builtin_trap();
                    break;
                }
            }
            case VM_IOP_DIV: {
                if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_LIT) {
                    vm_std_value_t out;
                    vm_std_value_t v1 = instr.args[0].lit;
                    vm_std_value_t v2 = instr.args[1].lit;
                    #define OP(x, y) ((x)/(y))
                    #include "binop.inc"
                    vm_arg_t *args = vm_malloc(sizeof(vm_arg_t));
                    args[0] = (vm_arg_t) {
                        .type = VM_ARG_LIT,
                        .lit = out,
                    };
                    instr = (vm_instr_t) {
                        .op = VM_IOP_MOVE_I,
                        .out = instr.out,
                        .args = args,
                    };
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                    instr.op = VM_IOP_DIV_IR;
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                    instr.op = VM_IOP_DIV_RI;
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    instr.op = VM_IOP_DIV_RR;
                    goto redo_instr;
                } else {
                    __builtin_trap();
                    break;
                }
            }
            case VM_IOP_IDIV: {
                if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_LIT) {
                    vm_std_value_t out;
                    vm_std_value_t v1 = instr.args[0].lit;
                    vm_std_value_t v2 = instr.args[1].lit;
                    #define OP(x, y) ((x)/(y))
                    #define OP_F(x, y) floor((x)/(y))
                    #include "binop.inc"
                    vm_arg_t *args = vm_malloc(sizeof(vm_arg_t));
                    args[0] = (vm_arg_t) {
                        .type = VM_ARG_LIT,
                        .lit = out,
                    };
                    instr = (vm_instr_t) {
                        .op = VM_IOP_MOVE_I,
                        .out = instr.out,
                        .args = args,
                    };
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                    instr.op = VM_IOP_IDIV_IR;
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                    instr.op = VM_IOP_IDIV_RI;
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    instr.op = VM_IOP_IDIV_RR;
                    goto redo_instr;
                } else {
                    __builtin_trap();
                    break;
                }
            }
            case VM_IOP_MOD: {
                if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_LIT) {
                    vm_std_value_t out;
                    vm_std_value_t v1 = instr.args[0].lit;
                    vm_std_value_t v2 = instr.args[1].lit;
                    #define OP(x, y) ((x)%(y))
                    #define OP_F(x, y) fmod((x),(y))
                    #include "binop.inc"
                    vm_arg_t *args = vm_malloc(sizeof(vm_arg_t));
                    args[0] = (vm_arg_t) {
                        .type = VM_ARG_LIT,
                        .lit = out,
                    };
                    instr = (vm_instr_t) {
                        .op = VM_IOP_MOVE_I,
                        .out = instr.out,
                        .args = args,
                    };
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                    instr.op = VM_IOP_MOD_IR;
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                    instr.op = VM_IOP_MOD_RI;
                    goto redo_instr;
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    instr.op = VM_IOP_MOD_RR;
                    goto redo_instr;
                } else {
                    __builtin_trap();
                    break;
                }
            }
            case VM_IOP_TABLE_SET: {
                vm_std_value_t v1 = vm_interp_arg(regs, instr.args[0]);
                vm_std_value_t v2 = vm_interp_arg(regs, instr.args[1]);
                vm_std_value_t v3 = vm_interp_arg(regs, instr.args[2]);
                vm_table_set(v1.value.table, v2.value, v3.value, v2.tag, v3.tag);
                break;
            }
            case VM_IOP_TABLE_NEW: {
                vm_interp_out_arg(regs, instr.out, (vm_std_value_t) {
                    .tag = VM_TAG_TAB,
                    .value.table = vm_table_new(),
                });
                break;
            }
            case VM_IOP_TABLE_LEN: {
                vm_std_value_t v1 = vm_interp_arg(regs, instr.args[0]);
                vm_interp_out_arg(regs, instr.out, VM_STD_VALUE_NUMBER(interp->config, v1.value.table->len));
                break;
            }
            case VM_IOP_STD: {
                vm_interp_out_arg(regs, instr.out, (vm_std_value_t) {
                    .tag = VM_TAG_TAB,
                    .value.table = interp->std,
                });
                break;
            }
            case VM_IOP_MOVE_I: {
                vm_interp_out_arg(regs, instr.out, instr.args[0].lit);
                break;
            }
            case VM_IOP_MOVE_R: {
                vm_interp_out_arg(regs, instr.out, regs[instr.args[0].reg]);
                break;
            }
            case VM_IOP_ADD_RI: {
                vm_std_value_t out;
                vm_std_value_t v1 = regs[instr.args[0].reg];
                vm_std_value_t v2 = instr.args[1].lit;
                #define OP(x, y) ((x)+(y))
                #include "binop.inc"
                vm_interp_out_arg(regs, instr.out, out);
                break;
            }
            case VM_IOP_ADD_IR: {
                vm_std_value_t out;
                vm_std_value_t v1 = instr.args[0].lit;
                vm_std_value_t v2 = regs[instr.args[1].reg];
                #define OP(x, y) ((x)+(y))
                #include "binop.inc"
                vm_interp_out_arg(regs, instr.out, out);
                break;
            }
            case VM_IOP_ADD_RR: {
                vm_std_value_t out;
                vm_std_value_t v1 = regs[instr.args[0].reg];
                vm_std_value_t v2 = regs[instr.args[1].reg];
                #define OP(x, y) ((x)+(y))
                #include "binop.inc"
                vm_interp_out_arg(regs, instr.out, out);
                break;
            }
            case VM_IOP_SUB_RI: {
                vm_std_value_t out;
                vm_std_value_t v1 = regs[instr.args[0].reg];
                vm_std_value_t v2 = instr.args[1].lit;
                #define OP(x, y) ((x)-(y))
                #include "binop.inc"
                vm_interp_out_arg(regs, instr.out, out);
                break;
            }
            case VM_IOP_SUB_IR: {
                vm_std_value_t out;
                vm_std_value_t v1 = instr.args[0].lit;
                vm_std_value_t v2 = regs[instr.args[1].reg];
                #define OP(x, y) ((x)-(y))
                #include "binop.inc"
                vm_interp_out_arg(regs, instr.out, out);
                break;
            }
            case VM_IOP_SUB_RR: {
                vm_std_value_t out;
                vm_std_value_t v1 = regs[instr.args[0].reg];
                vm_std_value_t v2 = regs[instr.args[1].reg];
                #define OP(x, y) ((x)-(y))
                #include "binop.inc"
                vm_interp_out_arg(regs, instr.out, out);
                break;
            }
            case VM_IOP_MUL_RI: {
                vm_std_value_t out;
                vm_std_value_t v1 = regs[instr.args[0].reg];
                vm_std_value_t v2 = instr.args[1].lit;
                #define OP(x, y) ((x)*(y))
                #include "binop.inc"
                vm_interp_out_arg(regs, instr.out, out);
                break;
            }
            case VM_IOP_MUL_IR: {
                vm_std_value_t out;
                vm_std_value_t v1 = instr.args[0].lit;
                vm_std_value_t v2 = regs[instr.args[1].reg];
                #define OP(x, y) ((x)*(y))
                #include "binop.inc"
                vm_interp_out_arg(regs, instr.out, out);
                break;
            }
            case VM_IOP_MUL_RR: {
                vm_std_value_t out;
                vm_std_value_t v1 = regs[instr.args[0].reg];
                vm_std_value_t v2 = regs[instr.args[1].reg];
                #define OP(x, y) ((x)*(y))
                #include "binop.inc"
                vm_interp_out_arg(regs, instr.out, out);
                break;
            }
            case VM_IOP_DIV_RI: {
                vm_std_value_t out;
                vm_std_value_t v1 = regs[instr.args[0].reg];
                vm_std_value_t v2 = instr.args[1].lit;
                #define OP(x, y) ((x)/(y))
                #include "binop.inc"
                vm_interp_out_arg(regs, instr.out, out);
                break;
            }
            case VM_IOP_DIV_IR: {
                vm_std_value_t out;
                vm_std_value_t v1 = instr.args[0].lit;
                vm_std_value_t v2 = regs[instr.args[1].reg];
                #define OP(x, y) ((x)/(y))
                #include "binop.inc"
                vm_interp_out_arg(regs, instr.out, out);
                break;
            }
            case VM_IOP_DIV_RR: {
                vm_std_value_t out;
                vm_std_value_t v1 = regs[instr.args[0].reg];
                vm_std_value_t v2 = regs[instr.args[1].reg];
                #define OP(x, y) ((x)/(y))
                #include "binop.inc"
                vm_interp_out_arg(regs, instr.out, out);
                break;
            }
            case VM_IOP_IDIV_RI: {
                vm_std_value_t out;
                vm_std_value_t v1 = regs[instr.args[0].reg];
                vm_std_value_t v2 = instr.args[1].lit;
                #define OP(x, y) ((x)/(y))
                #define OP_F(x, y) floor((x)/(y))
                #include "binop.inc"
                vm_interp_out_arg(regs, instr.out, out);
                break;
            }
            case VM_IOP_IDIV_IR: {
                vm_std_value_t out;
                vm_std_value_t v1 = instr.args[0].lit;
                vm_std_value_t v2 = regs[instr.args[1].reg];
                #define OP(x, y) ((x)/(y))
                #define OP_F(x, y) floor((x)/(y))
                #include "binop.inc"
                vm_interp_out_arg(regs, instr.out, out);
                break;
            }
            case VM_IOP_IDIV_RR: {
                vm_std_value_t out;
                vm_std_value_t v1 = regs[instr.args[0].reg];
                vm_std_value_t v2 = regs[instr.args[1].reg];
                #define OP(x, y) ((x)/(y))
                #define OP_F(x, y) floor((x)/(y))
                #include "binop.inc"
                vm_interp_out_arg(regs, instr.out, out);
                break;
            }
            case VM_IOP_MOD_RI: {
                vm_std_value_t out;
                vm_std_value_t v1 = regs[instr.args[0].reg];
                vm_std_value_t v2 = instr.args[1].lit;
                #define OP(x, y) ((x)%(y))
                #define OP_F(x, y) fmod((x),(y))
                #include "binop.inc"
                vm_interp_out_arg(regs, instr.out, out);
                break;
            }
            case VM_IOP_MOD_IR: {
                vm_std_value_t out;
                vm_std_value_t v1 = instr.args[0].lit;
                vm_std_value_t v2 = regs[instr.args[1].reg];
                #define OP(x, y) ((x)%(y))
                #define OP_F(x, y) fmod((x),(y))
                #include "binop.inc"
                vm_interp_out_arg(regs, instr.out, out);
                break;
            }
            case VM_IOP_MOD_RR: {
                vm_std_value_t out;
                vm_std_value_t v1 = regs[instr.args[0].reg];
                vm_std_value_t v2 = regs[instr.args[1].reg];
                #define OP(x, y) ((x)%(y))
                #define OP_F(x, y) fmod((x),(y))
                #include "binop.inc"
                vm_interp_out_arg(regs, instr.out, out);
                break;
            }
        }
    }
    switch (branch->op) {
        default: {
            __builtin_trap();
        }
        case VM_BOP_JUMP: {
            block = branch->targets[0];
            goto new_block;
        }
        case VM_BOP_BB: {
            vm_std_value_t v1 = vm_interp_arg(regs, branch->args[0]);
            if (v1.tag == VM_TAG_NIL || (v1.tag == VM_TAG_BOOL && !v1.value.b)) {
                block = branch->targets[1];
                goto new_block;
            } else {
                block = branch->targets[0];
                goto new_block;
            }
        }
        case VM_BOP_BLT: {
            vm_std_value_t v1 = vm_interp_arg(regs, branch->args[0]);
            vm_std_value_t v2 = vm_interp_arg(regs, branch->args[1]);
            if (vm_interp_value_lt(v1, v2)) {
                block = branch->targets[0];
                goto new_block;
            } else {
                block = branch->targets[1];
                goto new_block;
            }
        }
        case VM_BOP_BLE: {
            vm_std_value_t v1 = vm_interp_arg(regs, branch->args[0]);
            vm_std_value_t v2 = vm_interp_arg(regs, branch->args[1]);
            if (vm_interp_value_le(v1, v2)) {
                block = branch->targets[0];
                goto new_block;
            } else {
                block = branch->targets[1];
                goto new_block;
            }
        }
        case VM_BOP_BEQ: {
            vm_std_value_t v1 = vm_interp_arg(regs, branch->args[0]);
            vm_std_value_t v2 = vm_interp_arg(regs, branch->args[1]);
            if (vm_value_eq(v1, v2)) {
                block = branch->targets[0];
                goto new_block;
            } else {
                block = branch->targets[1];
                goto new_block;
            }
        }
        case VM_BOP_RET: {
            return vm_interp_arg(regs, branch->args[0]);
        }
        case VM_BOP_LOAD: {
            vm_std_value_t v1 = vm_interp_arg(regs, branch->args[0]);
            vm_std_value_t v2 = vm_interp_arg(regs, branch->args[0]);
            switch (v2.tag) {
                case VM_TAG_I8: {
                    vm_interp_out_arg(regs, branch->out, v1.value.closure[v2.value.i8]);
                    break;
                }
                case VM_TAG_I16: {
                    vm_interp_out_arg(regs, branch->out, v1.value.closure[v2.value.i16]);
                    break;
                }
                case VM_TAG_I32: {
                    vm_interp_out_arg(regs, branch->out, v1.value.closure[v2.value.i32]);
                    break;
                }
                case VM_TAG_I64: {
                    vm_interp_out_arg(regs, branch->out, v1.value.closure[v2.value.i16]);
                    break;
                }
                case VM_TAG_F32: {
                    vm_interp_out_arg(regs, branch->out, v1.value.closure[(int64_t) v2.value.f32]);
                    break;
                }
                case VM_TAG_F64: {
                    vm_interp_out_arg(regs, branch->out, v1.value.closure[(int64_t) v2.value.f64]);
                    break;
                }
            }
            block = branch->targets[0];
            goto new_block;
        }
        case VM_BOP_GET: {
            vm_std_value_t v1 = vm_interp_arg(regs, branch->args[0]);
            vm_std_value_t v2 = vm_interp_arg(regs, branch->args[1]);
            vm_pair_t pair;
            pair.key_tag = v2.tag;
            pair.key_val = v2.value;
            vm_table_get_pair(v1.value.table, &pair);
            vm_std_value_t out = (vm_std_value_t) {
                .tag = pair.val_tag,
                .value = pair.val_val,
            };
            vm_interp_out_arg(regs, branch->out, out);
            block = branch->targets[0];
            goto new_block;
        }
        case VM_BOP_CALL: {
            vm_std_value_t v1 = vm_interp_arg(regs, branch->args[0]);
            switch (v1.tag) {
                case VM_TAG_CLOSURE: {
                    next_regs[0] = v1;
                    size_t i = 1;
                    while (branch->args[i].type != VM_TAG_UNK) {
                        next_regs[i] = vm_interp_arg(regs, branch->args[i]);
                        i += 1;
                    }
                    next_regs[i].tag = VM_TAG_UNK;
                    // vm_std_value_t got = vm_interp_block(interp, next_regs, interp->blocks->blocks[v1.value.closure[0].value.i32]);
                    vm_std_value_t got = vm_interp_block(interp, next_regs, interp->blocks->blocks[v1.value.closure[0].value.i32]);
                    vm_interp_out_arg(regs, branch->out, got);
                    block = branch->targets[0];
                    goto new_block;
                }
                case VM_TAG_FFI: {
                    size_t i = 0;
                    while (branch->args[i + 1].type != VM_TAG_UNK) {
                        next_regs[i] = vm_interp_arg(regs, branch->args[i + 1]);
                        i += 1;
                    }
                    next_regs[i].tag = VM_TAG_UNK;
                    v1.value.ffi(&interp->closure, next_regs);
                    vm_interp_out_arg(regs, branch->out, next_regs[0]);
                    block = branch->targets[0];
                    goto new_block;
                }
                default: {
                    return (vm_std_value_t) {
                        .tag = VM_TAG_ERROR,
                        .value.str = "can only call functions",
                    };
                }
            }
        }
    }
}

vm_std_value_t vm_interp_run(vm_config_t *config, vm_block_t *entry, vm_blocks_t *blocks, vm_table_t *std) {
    vm_interp_t interp = (vm_interp_t) {
        .config = config,
        .blocks = blocks,
        .std = std,
        .closure.config = config,
        .closure.blocks = blocks,
    };

    vm_std_value_t *regs = vm_malloc(sizeof(vm_std_value_t) * 65536);
    
    vm_std_value_t ret = vm_interp_block(&interp, regs, entry);

    vm_free(regs);

    return ret;
}
