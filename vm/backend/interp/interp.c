
#include "interp.h"
#include "../../obj.h"

#if 0
#define VM_OPCODE_DEBUG(s) printf("%s\n", #s);
#else
#define VM_OPCODE_DEBUG(s) 
#endif

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
    VM_OP_MOVE_I = VM_MAX_IOP,
    VM_OP_MOVE_R,

    VM_OP_ADD_RI,
    VM_OP_ADD_IR,
    VM_OP_ADD_RR,

    VM_OP_SUB_RI,
    VM_OP_SUB_IR,
    VM_OP_SUB_RR,

    VM_OP_MUL_RI,
    VM_OP_MUL_IR,
    VM_OP_MUL_RR,

    VM_OP_DIV_RI,
    VM_OP_DIV_IR,
    VM_OP_DIV_RR,

    VM_OP_IDIV_RI,
    VM_OP_IDIV_IR,
    VM_OP_IDIV_RR,

    VM_OP_MOD_RI,
    VM_OP_MOD_IR,
    VM_OP_MOD_RR,

    VM_OP_JUMP,

    VM_OP_BB_R,

    VM_OP_BLT_RI,
    VM_OP_BLT_IR,
    VM_OP_BLT_RR,

    VM_OP_BLE_RI,
    VM_OP_BLE_IR,
    VM_OP_BLE_RR,

    VM_OP_BEQ_RI,
    VM_OP_BEQ_IR,
    VM_OP_BEQ_RR,
    
    VM_OP_RET_I,
    VM_OP_RET_R,

    VM_OP_LOAD,
    VM_OP_GET,
    VM_OP_CALL,
    
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
        }
        default: {
            __builtin_trap();
        }
    }
}

void vm_interp_out_arg(vm_std_value_t *regs, vm_arg_t key, vm_std_value_t value) {
    regs[key.reg] = value;
}

vm_std_value_t vm_interp_add(vm_std_value_t v1, vm_std_value_t v2) {
    vm_std_value_t out;
    #define OP(x, y) ((x)+(y))
    #include "binop.inc"
    return out;
}

vm_std_value_t vm_interp_sub(vm_std_value_t v1, vm_std_value_t v2) {
    vm_std_value_t out;
    #define OP(x, y) ((x)-(y))
    #include "binop.inc"
    return out;
}

vm_std_value_t vm_interp_mul(vm_std_value_t v1, vm_std_value_t v2) {
    vm_std_value_t out;
    #define OP(x, y) ((x)/(y))
    #include "binop.inc"
    return out;
}

vm_std_value_t vm_interp_div(vm_std_value_t v1, vm_std_value_t v2) {
    vm_std_value_t out;
    #define OP(x, y) ((x)/(y))
    #include "binop.inc"
    return out;
}

vm_std_value_t vm_interp_idiv(vm_std_value_t v1, vm_std_value_t v2) {
    vm_std_value_t out;
    #define OP(x, y) ((x)/(y))
    #define OP_F(x, y) floor((x)/(y))
    #include "binop.inc"
    return out;
}

vm_std_value_t vm_interp_mod(vm_std_value_t v1, vm_std_value_t v2) {
    vm_std_value_t out;
    #define OP(x, y) ((x)%(y))
    #define OP_F(x, y) fmod((x),(y))
    #include "binop.inc"
    return out;
}

vm_std_value_t vm_interp_block(vm_interp_t *interp, vm_std_value_t *regs, vm_block_t *block) {
    vm_std_value_t *next_regs = &regs[block->nregs];
new_block:;
    if (!block->mark) {
        block->mark = true;

        vm_branch_t *branch = &block->branch;

        vm_instr_t instr;

        switch (branch->op) {
            case VM_BOP_JUMP: {
                instr.op = VM_OP_JUMP;
                instr.args = vm_malloc(sizeof(vm_arg_t) * 1);
                instr.args[0].func = branch->targets[0];
                break;
            }
            case VM_BOP_BB: {
                if (branch->args[0].type == VM_ARG_LIT) {
                    vm_std_value_t v1 = branch->args[0].lit;
                    instr.op = VM_OP_JUMP;
                    instr.args = vm_malloc(sizeof(vm_arg_t) * 1);
                    if (v1.tag == VM_TAG_NIL || (v1.tag == VM_TAG_BOOL && !v1.value.b)) {
                        instr.args[0].func = branch->targets[1];
                    } else {
                        instr.args[0].func = branch->targets[0];
                    }
                } else {
                    instr.op = VM_OP_BB_R;
                    instr.args = vm_malloc(sizeof(vm_arg_t) * 3);
                    instr.args[0] = branch->args[0];
                    instr.args[1].func = branch->targets[0];
                    instr.args[2].func = branch->targets[1];
                }
                break;
            }
            case VM_BOP_BLT: {
                if (branch->args[0].type == VM_ARG_LIT && branch->args[1].type == VM_ARG_LIT) {
                    vm_std_value_t v1 = branch->args[0].lit;
                    vm_std_value_t v2 = branch->args[1].lit;
                    instr.op = VM_OP_JUMP;
                    instr.args = vm_malloc(sizeof(vm_arg_t) * 1);
                    if (vm_interp_value_lt(v1, v2)) {
                        instr.args[0].func = branch->targets[1];
                    } else {
                        instr.args[0].func = branch->targets[0];
                    }
                } else {
                    if (branch->args[0].type == VM_ARG_REG && branch->args[1].type == VM_ARG_LIT) {
                        instr.op = VM_OP_BLT_RI;
                    } else if (branch->args[0].type == VM_ARG_LIT && branch->args[1].type == VM_ARG_REG) {
                        instr.op = VM_OP_BLT_IR;
                    } else if (branch->args[0].type == VM_ARG_REG && branch->args[1].type == VM_ARG_REG) {
                        instr.op = VM_OP_BLT_RR;
                    } else {
                        __builtin_trap();
                    }
                    instr.args = vm_malloc(sizeof(vm_arg_t) * 4);
                    instr.args[0] = branch->args[0];
                    instr.args[1] = branch->args[1];
                    instr.args[2].func = branch->targets[0];
                    instr.args[3].func = branch->targets[1];
                }
                break;
            }
            case VM_BOP_BLE: {
                if (branch->args[0].type == VM_ARG_LIT && branch->args[1].type == VM_ARG_LIT) {
                    vm_std_value_t v1 = branch->args[0].lit;
                    vm_std_value_t v2 = branch->args[1].lit;
                    instr.op = VM_OP_JUMP;
                    instr.args = vm_malloc(sizeof(vm_arg_t) * 1);
                    if (vm_interp_value_le(v1, v2)) {
                        instr.args[0].func = branch->targets[1];
                    } else {
                        instr.args[0].func = branch->targets[0];
                    }
                } else {
                    if (branch->args[0].type == VM_ARG_REG && branch->args[1].type == VM_ARG_LIT) {
                        instr.op = VM_OP_BLE_RI;
                    } else if (branch->args[0].type == VM_ARG_LIT && branch->args[1].type == VM_ARG_REG) {
                        instr.op = VM_OP_BLE_IR;
                    } else if (branch->args[0].type == VM_ARG_REG && branch->args[1].type == VM_ARG_REG) {
                        instr.op = VM_OP_BLE_RR;
                    } else {
                        __builtin_trap();
                    }
                    instr.args = vm_malloc(sizeof(vm_arg_t) * 4);
                    instr.args[0] = branch->args[0];
                    instr.args[1] = branch->args[1];
                    instr.args[2].func = branch->targets[0];
                    instr.args[3].func = branch->targets[1];
                }
                break;
            }
            case VM_BOP_BEQ: {
                if (branch->args[0].type == VM_ARG_LIT && branch->args[1].type == VM_ARG_LIT) {
                    vm_std_value_t v1 = branch->args[0].lit;
                    vm_std_value_t v2 = branch->args[1].lit;
                    instr.op = VM_OP_JUMP;
                    instr.args = vm_malloc(sizeof(vm_arg_t) * 1);
                    if (vm_value_eq(v1, v2)) {
                        instr.args[0].func = branch->targets[1];
                    } else {
                        instr.args[0].func = branch->targets[0];
                    }
                } else {
                    if (branch->args[0].type == VM_ARG_REG && branch->args[1].type == VM_ARG_LIT) {
                        instr.op = VM_OP_BEQ_RI;
                    } else if (branch->args[0].type == VM_ARG_LIT && branch->args[1].type == VM_ARG_REG) {
                        instr.op = VM_OP_BEQ_IR;
                    } else if (branch->args[0].type == VM_ARG_REG && branch->args[1].type == VM_ARG_REG) {
                        instr.op = VM_OP_BEQ_RR;
                    } else {
                        __builtin_trap();
                    }
                    instr.args = vm_malloc(sizeof(vm_arg_t) * 4);
                    instr.args[0] = branch->args[0];
                    instr.args[1] = branch->args[1];
                    instr.args[2].func = branch->targets[0];
                    instr.args[3].func = branch->targets[1];
                }
                break;
            }
            case VM_BOP_RET: {
                if (branch->args[0].type == VM_ARG_LIT) {
                    instr.op = VM_OP_RET_I;
                } else if (branch->args[0].type == VM_ARG_REG) {
                    instr.op = VM_OP_RET_R;
                } else {
                    __builtin_trap();
                }
                instr.args = vm_malloc(sizeof(vm_arg_t) * 1);
                instr.args[0] = branch->args[0];
                break;
            }
            case VM_BOP_LOAD: {
                instr.op = VM_OP_LOAD;
                instr.args = branch->args;
                instr.out = branch->out;
                instr.args[0] = branch->args[0];
                instr.args[1] = branch->args[1];
                instr.args[2].func = branch->targets[0];
                break;
            }
            case VM_BOP_GET: {
                instr.op = VM_OP_GET;
                instr.args = vm_malloc(sizeof(vm_arg_t) * 3);
                instr.args[0] = branch->args[0];
                instr.args[1] = branch->args[1];
                instr.args[2].func = branch->targets[0];
                instr.out = branch->out;
                break;
            }
            case VM_BOP_CALL: {
                instr.op = VM_OP_CALL;
                instr.args = branch->args;
                instr.out = branch->out;
                size_t narg = 1;
                while (instr.args[narg].type != VM_ARG_NONE) {
                    narg += 1;
                }
                instr.args[narg].func = branch->targets[0];
                break;
            }
            default: {
                __builtin_trap();
            }
        }

        vm_block_realloc(block, instr);
    }
    vm_instr_t *instrs = block->instrs;
    size_t i = 0;
    vm_instr_t instr;
    goto read_instr;
redo_instr:;
    instrs[i-1] = instr;
    goto after_read_instr;
read_instr:;
    instr = instrs[i++];
after_read_instr:;
    switch (instr.op) {
        case VM_IOP_NOP: VM_OPCODE_DEBUG(VM_IOP_NOP) {
            goto read_instr;
        } 
        case VM_IOP_MOVE: VM_OPCODE_DEBUG(VM_IOP_MOVE) {
            if (instr.args[0].type == VM_ARG_LIT) {
                instr.op = VM_OP_MOVE_I;
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_REG) {
                instr.op = VM_OP_MOVE_R;
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_FUN) {
                instr.op = VM_OP_MOVE_I;
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
                goto read_instr;
            }
        }
        case VM_IOP_ADD: VM_OPCODE_DEBUG(VM_IOP_ADD) {
            if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_LIT) {
                vm_std_value_t v1 = instr.args[0].lit;
                vm_std_value_t v2 = instr.args[1].lit;
                vm_arg_t *args = vm_malloc(sizeof(vm_std_value_t));
                args[0] = (vm_arg_t) {
                    .type = VM_ARG_LIT,
                    .lit = vm_interp_add(v1, v2),
                };
                instr = (vm_instr_t) {
                    .op = VM_OP_MOVE_I,
                    .out = instr.out,
                    .args = args,
                };
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                instr.op = VM_OP_ADD_IR;
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                instr.op = VM_OP_ADD_RI;
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                instr.op = VM_OP_ADD_RR;
                goto redo_instr;
            } else {
                __builtin_trap();
                goto read_instr;
            }
        }
        case VM_IOP_SUB: VM_OPCODE_DEBUG(VM_IOP_SUB) {
            if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_LIT) {
                vm_std_value_t v1 = instr.args[0].lit;
                vm_std_value_t v2 = instr.args[1].lit;
                vm_arg_t *args = vm_malloc(sizeof(vm_std_value_t));
                args[0] = (vm_arg_t) {
                    .type = VM_ARG_LIT,
                    .lit = vm_interp_sub(v1, v2),
                };
                instr = (vm_instr_t) {
                    .op = VM_OP_MOVE_I,
                    .out = instr.out,
                    .args = args,
                };
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                instr.op = VM_OP_SUB_IR;
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                instr.op = VM_OP_SUB_RI;
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                instr.op = VM_OP_SUB_RR;
                goto redo_instr;
            } else {
                __builtin_trap();
                goto read_instr;
            }
        }
        case VM_IOP_MUL: VM_OPCODE_DEBUG(VM_IOP_MUL) {
            if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_LIT) {
                vm_std_value_t v1 = instr.args[0].lit;
                vm_std_value_t v2 = instr.args[1].lit;
                vm_arg_t *args = vm_malloc(sizeof(vm_std_value_t));
                args[0] = (vm_arg_t) {
                    .type = VM_ARG_LIT,
                    .lit = vm_interp_mul(v1, v2),
                };
                instr = (vm_instr_t) {
                    .op = VM_OP_MOVE_I,
                    .out = instr.out,
                    .args = args,
                };
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                instr.op = VM_OP_MUL_IR;
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                instr.op = VM_OP_MUL_RI;
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                instr.op = VM_OP_MUL_RR;
                goto redo_instr;
            } else {
                __builtin_trap();
                goto read_instr;
            }
        }
        case VM_IOP_DIV: VM_OPCODE_DEBUG(VM_IOP_DIV) {
            if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_LIT) {
                vm_std_value_t v1 = instr.args[0].lit;
                vm_std_value_t v2 = instr.args[1].lit;
                vm_arg_t *args = vm_malloc(sizeof(vm_std_value_t));
                args[0] = (vm_arg_t) {
                    .type = VM_ARG_LIT,
                    .lit = vm_interp_div(v1, v2),
                };
                instr = (vm_instr_t) {
                    .op = VM_OP_MOVE_I,
                    .out = instr.out,
                    .args = args,
                };
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                instr.op = VM_OP_DIV_IR;
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                instr.op = VM_OP_DIV_RI;
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                instr.op = VM_OP_DIV_RR;
                goto redo_instr;
            } else {
                __builtin_trap();
                goto read_instr;
            }
        }
        case VM_IOP_IDIV: VM_OPCODE_DEBUG(VM_IOP_IDIV) {
            if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_LIT) {
                vm_std_value_t v1 = instr.args[0].lit;
                vm_std_value_t v2 = instr.args[1].lit;
                vm_arg_t *args = vm_malloc(sizeof(vm_std_value_t));
                args[0] = (vm_arg_t) {
                    .type = VM_ARG_LIT,
                    .lit = vm_interp_idiv(v1, v2),
                };
                instr = (vm_instr_t) {
                    .op = VM_OP_MOVE_I,
                    .out = instr.out,
                    .args = args,
                };
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                instr.op = VM_OP_IDIV_IR;
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                instr.op = VM_OP_IDIV_RI;
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                instr.op = VM_OP_IDIV_RR;
                goto redo_instr;
            } else {
                __builtin_trap();
                goto read_instr;
            }
        }
        case VM_IOP_MOD: VM_OPCODE_DEBUG(VM_IOP_MOD) {
            if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_LIT) {
                vm_std_value_t v1 = instr.args[0].lit;
                vm_std_value_t v2 = instr.args[1].lit;
                vm_arg_t *args = vm_malloc(sizeof(vm_std_value_t));
                args[0] = (vm_arg_t) {
                    .type = VM_ARG_LIT,
                    .lit = vm_interp_mod(v1, v2),
                };
                instr = (vm_instr_t) {
                    .op = VM_OP_MOVE_I,
                    .out = instr.out,
                    .args = args,
                };
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                instr.op = VM_OP_MOD_IR;
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                instr.op = VM_OP_MOD_RI;
                goto redo_instr;
            } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                instr.op = VM_OP_MOD_RR;
                goto redo_instr;
            } else {
                __builtin_trap();
                goto read_instr;
            }
        }
        case VM_IOP_TABLE_SET: VM_OPCODE_DEBUG(VM_IOP_TABLE_SET) {
            vm_std_value_t v1 = vm_interp_arg(regs, instr.args[0]);
            vm_std_value_t v2 = vm_interp_arg(regs, instr.args[1]);
            vm_std_value_t v3 = vm_interp_arg(regs, instr.args[2]);
            vm_table_set(v1.value.table, v2.value, v3.value, v2.tag, v3.tag);
            goto read_instr;
        }
        case VM_IOP_TABLE_NEW: VM_OPCODE_DEBUG(VM_IOP_TABLE_NEW) {
            vm_interp_out_arg(regs, instr.out, (vm_std_value_t) {
                .tag = VM_TAG_TAB,
                .value.table = vm_table_new(),
            });
            goto read_instr;
        }
        case VM_IOP_TABLE_LEN: VM_OPCODE_DEBUG(VM_IOP_TABLE_LEN) {
            vm_std_value_t v1 = vm_interp_arg(regs, instr.args[0]);
            vm_interp_out_arg(regs, instr.out, VM_STD_VALUE_NUMBER(interp->config, v1.value.table->len));
            goto read_instr;
        }
        case VM_IOP_STD: VM_OPCODE_DEBUG(VM_IOP_STD) {
            vm_interp_out_arg(regs, instr.out, (vm_std_value_t) {
                .tag = VM_TAG_TAB,
                .value.table = interp->std,
            });
            goto read_instr;
        }
        case VM_OP_MOVE_I: VM_OPCODE_DEBUG(VM_OP_MOVE_I) {
            vm_interp_out_arg(regs, instr.out, instr.args[0].lit);
            goto read_instr;
        }
        case VM_OP_MOVE_R: VM_OPCODE_DEBUG(VM_OP_MOVE_R) {
            vm_interp_out_arg(regs, instr.out, regs[instr.args[0].reg]);
            goto read_instr;
        }
        case VM_OP_ADD_RI: VM_OPCODE_DEBUG(VM_OP_ADD_RI) {
            vm_interp_out_arg(regs, instr.out, vm_interp_add(regs[instr.args[0].reg], instr.args[1].lit));
            goto read_instr;
        }
        case VM_OP_ADD_IR: VM_OPCODE_DEBUG(VM_OP_ADD_IR) {
            vm_interp_out_arg(regs, instr.out, vm_interp_add(instr.args[0].lit, regs[instr.args[1].reg]));
            goto read_instr;
        }
        case VM_OP_ADD_RR: VM_OPCODE_DEBUG(VM_OP_ADD_RR) {
            vm_interp_out_arg(regs, instr.out, vm_interp_add(regs[instr.args[0].reg], regs[instr.args[1].reg]));
            goto read_instr;
        }
        case VM_OP_SUB_RI: VM_OPCODE_DEBUG(VM_OP_SUB_RI) {
            vm_interp_out_arg(regs, instr.out, vm_interp_sub(regs[instr.args[0].reg], instr.args[1].lit));
            goto read_instr;
        }
        case VM_OP_SUB_IR: VM_OPCODE_DEBUG(VM_OP_SUB_IR) {
            vm_interp_out_arg(regs, instr.out, vm_interp_sub(instr.args[0].lit, regs[instr.args[1].reg]));
            goto read_instr;
        }
        case VM_OP_SUB_RR: VM_OPCODE_DEBUG(VM_OP_SUB_RR) {
            vm_interp_out_arg(regs, instr.out, vm_interp_sub(regs[instr.args[0].reg], regs[instr.args[1].reg]));
            goto read_instr;
        }
        case VM_OP_MUL_RI: VM_OPCODE_DEBUG(VM_OP_MUL_RI) {
            vm_interp_out_arg(regs, instr.out, vm_interp_mul(regs[instr.args[0].reg], instr.args[1].lit));
            goto read_instr;
        }
        case VM_OP_MUL_IR: VM_OPCODE_DEBUG(VM_OP_MUL_IR) {
            vm_interp_out_arg(regs, instr.out, vm_interp_mul(instr.args[0].lit, regs[instr.args[1].reg]));
            goto read_instr;
        }
        case VM_OP_MUL_RR: VM_OPCODE_DEBUG(VM_OP_MUL_RR) {
            vm_interp_out_arg(regs, instr.out, vm_interp_mul(regs[instr.args[0].reg], regs[instr.args[1].reg]));
            goto read_instr;
        }
        case VM_OP_DIV_RI: VM_OPCODE_DEBUG(VM_OP_DIV_RI) {
            vm_interp_out_arg(regs, instr.out, vm_interp_div(regs[instr.args[0].reg], instr.args[1].lit));
            goto read_instr;
        }
        case VM_OP_DIV_IR: VM_OPCODE_DEBUG(VM_OP_DIV_IR) {
            vm_interp_out_arg(regs, instr.out, vm_interp_div(instr.args[0].lit, regs[instr.args[1].reg]));
            goto read_instr;
        }
        case VM_OP_DIV_RR: VM_OPCODE_DEBUG(VM_OP_DIV_RR) {
            vm_interp_out_arg(regs, instr.out, vm_interp_div(regs[instr.args[0].reg], regs[instr.args[1].reg]));
            goto read_instr;
        }
        case VM_OP_IDIV_RI: VM_OPCODE_DEBUG(VM_OP_IDIV_RI) {
            vm_interp_out_arg(regs, instr.out, vm_interp_idiv(regs[instr.args[0].reg], instr.args[1].lit));
            goto read_instr;
        }
        case VM_OP_IDIV_IR: VM_OPCODE_DEBUG(VM_OP_IDIV_IR) {
            vm_interp_out_arg(regs, instr.out, vm_interp_idiv(instr.args[0].lit, regs[instr.args[1].reg]));
            goto read_instr;
        }
        case VM_OP_IDIV_RR: VM_OPCODE_DEBUG(VM_OP_IDIV_RR) {
            vm_interp_out_arg(regs, instr.out, vm_interp_idiv(regs[instr.args[0].reg], regs[instr.args[1].reg]));
            goto read_instr;
        }
        case VM_OP_MOD_RI: VM_OPCODE_DEBUG(VM_OP_MOD_RI) {
            vm_interp_out_arg(regs, instr.out, vm_interp_mod(regs[instr.args[0].reg], instr.args[1].lit));
            goto read_instr;
        }
        case VM_OP_MOD_IR: VM_OPCODE_DEBUG(VM_OP_MOD_IR) {
            vm_interp_out_arg(regs, instr.out, vm_interp_mod(instr.args[0].lit, regs[instr.args[1].reg]));
            goto read_instr;
        }
        case VM_OP_MOD_RR: VM_OPCODE_DEBUG(VM_OP_MOD_RR) {
            vm_interp_out_arg(regs, instr.out, vm_interp_mod(regs[instr.args[0].reg], regs[instr.args[1].reg]));
            goto read_instr;
        }
        case VM_OP_JUMP: VM_OPCODE_DEBUG(VM_OP_JUMP) {
            block = instr.args[0].func;
            goto new_block;
        }
        case VM_OP_BB_R: VM_OPCODE_DEBUG(VM_OP_BB_R) {
            vm_std_value_t v1 = vm_interp_arg(regs, instr.args[0]);
            if (v1.tag == VM_TAG_NIL || (v1.tag == VM_TAG_BOOL && !v1.value.b)) {
                block = instr.args[2].func;
                goto new_block;
            } else {
                block = instr.args[1].func;
                goto new_block;
            }
        }
        case VM_OP_BLT_RI: VM_OPCODE_DEBUG(VM_OP_BLT_RI) {
            vm_std_value_t v1 = regs[instr.args[0].reg];
            vm_std_value_t v2 = instr.args[1].lit;
            if (vm_interp_value_lt(v1, v2)) {
                block = instr.args[2].func;
                goto new_block;
            } else {
                block = instr.args[3].func;
                goto new_block;
            }
        }
        case VM_OP_BLT_IR: VM_OPCODE_DEBUG(VM_OP_BLT_IR) {
            vm_std_value_t v1 = instr.args[0].lit;
            vm_std_value_t v2 = regs[instr.args[1].reg];
            if (vm_interp_value_lt(v1, v2)) {
                block = instr.args[2].func;
                goto new_block;
            } else {
                block = instr.args[3].func;
                goto new_block;
            }
        }
        case VM_OP_BLT_RR: VM_OPCODE_DEBUG(VM_OP_BLT_RR) {
            vm_std_value_t v1 = regs[instr.args[0].reg];
            vm_std_value_t v2 = regs[instr.args[1].reg];
            if (vm_interp_value_lt(v1, v2)) {
                block = instr.args[2].func;
                goto new_block;
            } else {
                block = instr.args[3].func;
                goto new_block;
            }
        }
        case VM_OP_BLE_RI: VM_OPCODE_DEBUG(VM_OP_BLE_RI) {
            vm_std_value_t v1 = regs[instr.args[0].reg];
            vm_std_value_t v2 = instr.args[1].lit;
            if (vm_interp_value_le(v1, v2)) {
                block = instr.args[2].func;
                goto new_block;
            } else {
                block = instr.args[3].func;
                goto new_block;
            }
        }
        case VM_OP_BLE_IR: VM_OPCODE_DEBUG(VM_OP_BLE_IR) {
            vm_std_value_t v1 = instr.args[0].lit;
            vm_std_value_t v2 = regs[instr.args[1].reg];
            if (vm_interp_value_le(v1, v2)) {
                block = instr.args[2].func;
                goto new_block;
            } else {
                block = instr.args[3].func;
                goto new_block;
            }
        }
        case VM_OP_BLE_RR: VM_OPCODE_DEBUG(VM_OP_BLE_RR) {
            vm_std_value_t v1 = regs[instr.args[0].reg];
            vm_std_value_t v2 = regs[instr.args[1].reg];
            if (vm_interp_value_le(v1, v2)) {
                block = instr.args[2].func;
                goto new_block;
            } else {
                block = instr.args[3].func;
                goto new_block;
            }
        }
        case VM_OP_BEQ_RI: VM_OPCODE_DEBUG(VM_OP_BEQ_RI) {
            vm_std_value_t v1 = regs[instr.args[0].reg];
            vm_std_value_t v2 = instr.args[1].lit;
            if (vm_value_eq(v1, v2)) {
                block = instr.args[2].func;
                goto new_block;
            } else {
                block = instr.args[3].func;
                goto new_block;
            }
        }
        case VM_OP_BEQ_IR: VM_OPCODE_DEBUG(VM_OP_BEQ_IR) {
            vm_std_value_t v1 = instr.args[0].lit;
            vm_std_value_t v2 = regs[instr.args[1].reg];
            if (vm_value_eq(v1, v2)) {
                block = instr.args[2].func;
                goto new_block;
            } else {
                block = instr.args[3].func;
                goto new_block;
            }
        }
        case VM_OP_BEQ_RR: VM_OPCODE_DEBUG(VM_OP_BEQ_RR) {
            vm_std_value_t v1 = regs[instr.args[0].reg];
            vm_std_value_t v2 = regs[instr.args[1].reg];
            if (vm_value_eq(v1, v2)) {
                block = instr.args[2].func;
                goto new_block;
            } else {
                block = instr.args[3].func;
                goto new_block;
            }
        }
        case VM_OP_RET_I: VM_OPCODE_DEBUG(VM_OP_RET_I) {
            return instr.args[0].lit;
        }
        case VM_OP_RET_R: VM_OPCODE_DEBUG(VM_OP_RET_R) {
            return regs[instr.args[0].reg];
        }
        case VM_OP_LOAD: VM_OPCODE_DEBUG(VM_OP_LOAD) {
            vm_std_value_t v1 = vm_interp_arg(regs, instr.args[0]);
            vm_std_value_t v2 = vm_interp_arg(regs, instr.args[1]);
            switch (v2.tag) {
                case VM_TAG_I8: {
                    vm_interp_out_arg(regs, instr.out, v1.value.closure[v2.value.i8]);
                    goto read_instr;
                }
                case VM_TAG_I16: {
                    vm_interp_out_arg(regs, instr.out, v1.value.closure[v2.value.i16]);
                    goto read_instr;
                }
                case VM_TAG_I32: {
                    vm_interp_out_arg(regs, instr.out, v1.value.closure[v2.value.i32]);
                    goto read_instr;
                }
                case VM_TAG_I64: {
                    vm_interp_out_arg(regs, instr.out, v1.value.closure[v2.value.i16]);
                    goto read_instr;
                }
                case VM_TAG_F32: {
                    vm_interp_out_arg(regs, instr.out, v1.value.closure[(int64_t) v2.value.f32]);
                    goto read_instr;
                }
                case VM_TAG_F64: {
                    vm_interp_out_arg(regs, instr.out, v1.value.closure[(int64_t) v2.value.f64]);
                    goto read_instr;
                }
            }
            block = instr.args[2].func;
            goto new_block;
        }
        case VM_OP_GET: VM_OPCODE_DEBUG(VM_OP_GET) {
            vm_std_value_t v1 = vm_interp_arg(regs, instr.args[0]);
            vm_std_value_t v2 = vm_interp_arg(regs, instr.args[1]);
            vm_pair_t pair;
            pair.key_tag = v2.tag;
            pair.key_val = v2.value;
            vm_table_get_pair(v1.value.table, &pair);
            vm_std_value_t out = (vm_std_value_t) {
                .tag = pair.val_tag,
                .value = pair.val_val,
            };
            vm_interp_out_arg(regs, instr.out, out);
            block = instr.args[2].func;
            goto new_block;
        }
        case VM_OP_CALL: VM_OPCODE_DEBUG(VM_OP_CALL) {
            vm_std_value_t v1 = vm_interp_arg(regs, instr.args[0]);
            switch (v1.tag) {
                case VM_TAG_CLOSURE: {
                    next_regs[0] = v1;
                    size_t j = 1;
                    while (instr.args[j].type != VM_ARG_NONE) {
                        next_regs[j] = vm_interp_arg(regs, instr.args[j]);
                        j += 1;
                    }
                    next_regs[j].tag = VM_TAG_UNK;
                    // vm_std_value_t got = vm_interp_block(interp, next_regs, interp->blocks->blocks[v1.value.closure[0].value.i32]);
                    vm_std_value_t got = vm_interp_block(interp, next_regs, interp->blocks->blocks[v1.value.closure[0].value.i32]);
                    vm_interp_out_arg(regs, instr.out, got);
                    block = instr.args[j].func;
                    goto new_block;
                }
                case VM_TAG_FFI: {
                    size_t j = 0;
                    while (instr.args[j + 1].type != VM_ARG_NONE) {
                        next_regs[j] = vm_interp_arg(regs, instr.args[j + 1]);
                        j += 1;
                    }
                    next_regs[j].tag = VM_TAG_UNK;
                    v1.value.ffi(&interp->closure, next_regs);
                    vm_interp_out_arg(regs, instr.out, next_regs[0]);
                    block = instr.args[j + 1].func;
                    goto new_block;
                }
                default: {
                    return (vm_std_value_t) {
                        .tag = VM_TAG_ERROR,
                        .value.str = "can only call functions",
                    };
                }
            }
            __builtin_trap();
        }
        default: VM_OPCODE_DEBUG(default) {
            __builtin_trap();
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
