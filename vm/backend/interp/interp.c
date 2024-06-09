
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

vm_std_value_t vm_interp_arg(vm_interp_t *interp, vm_arg_t arg) {
    switch (arg.type) {
        case VM_ARG_REG: {
            return interp->regs[arg.reg];
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

void vm_interp_out_arg(vm_interp_t *interp, vm_arg_t key, vm_std_value_t value) {
    interp->regs[key.reg] = value;
}

vm_std_value_t vm_interp_block(vm_interp_t *interp, vm_block_t *block) {
    vm_std_value_t *next_regs = &interp->regs[block->nregs];
new_block:;
    for (size_t i = 0; i < block->len; i++) {
        vm_instr_t instr = block->instrs[i];
        switch (instr.op) {
            case VM_IOP_NOP: {
                break;
            }
            case VM_IOP_MOVE: {
                vm_std_value_t v1 = vm_interp_arg(interp, instr.args[0]);
                vm_interp_out_arg(interp, instr.out, v1);
                break;
            }
            case VM_IOP_ADD: {
#define OP(x, y) ((x)+(y))
#include "binop.inc"
                break;
            }
            case VM_IOP_SUB: {
#define OP(x, y) ((x)-(y))
#include "binop.inc"
                break;
            }
            case VM_IOP_MUL: {
#define OP(x, y) ((x)*(y))
#include "binop.inc"
                break;
            }
            case VM_IOP_DIV: {
#define OP(x, y) ((x)/(y))
#include "binop.inc"
                break;
            }
            case VM_IOP_IDIV: {
#define OP(x, y) ((x) / (y))
#define OP_F(x, y) fmod((double) (x), (double) (y))
#include "binop.inc"
                break;
            }
            case VM_IOP_MOD: {
#define OP(x, y) ((x) % (y))
#define OP_F(x, y) fmod((double) (x), (double) (y))
#include "binop.inc"
                break;
            }
            case VM_IOP_TABLE_SET: {
                vm_std_value_t v1 = vm_interp_arg(interp, instr.args[0]);
                vm_std_value_t v2 = vm_interp_arg(interp, instr.args[1]);
                vm_std_value_t v3 = vm_interp_arg(interp, instr.args[2]);
                vm_table_set(v1.value.table, v2.value, v3.value, v2.tag, v3.tag);
                break;
            }
            case VM_IOP_TABLE_NEW: {
                vm_interp_out_arg(interp, instr.out, (vm_std_value_t) {
                    .tag = VM_TAG_TAB,
                    .value.table = vm_table_new(),
                });
                break;
            }
            case VM_IOP_TABLE_LEN: {
                vm_std_value_t v1 = vm_interp_arg(interp, instr.args[0]);
                vm_interp_out_arg(interp, instr.out, VM_STD_VALUE_NUMBER(interp->config, v1.value.table->len));
                break;
            }
            case VM_IOP_STD: {
                vm_interp_out_arg(interp, instr.out, (vm_std_value_t) {
                    .tag = VM_TAG_TAB,
                    .value.table = interp->std,
                });
                break;
            }
        }
    }
    vm_branch_t *branch = &block->branch;
    switch (branch->op) {
        case VM_BOP_JUMP: {
            block = branch->targets[0];
            goto new_block;
        }
        case VM_BOP_BB: {
            vm_std_value_t v1 = vm_interp_arg(interp, branch->args[0]);
            if (v1.tag == VM_TAG_NIL || (v1.tag == VM_TAG_BOOL && !v1.value.b)) {
                block = branch->targets[1];
                goto new_block;
            } else {
                block = branch->targets[0];
                goto new_block;
            }
        }
        case VM_BOP_BLT: {
            vm_std_value_t v1 = vm_interp_arg(interp, branch->args[0]);
            vm_std_value_t v2 = vm_interp_arg(interp, branch->args[1]);
            if (vm_interp_value_lt(v1, v2)) {
                block = branch->targets[0];
                goto new_block;
            } else {
                block = branch->targets[1];
                goto new_block;
            }
        }
        case VM_BOP_BLE: {
            vm_std_value_t v1 = vm_interp_arg(interp, branch->args[0]);
            vm_std_value_t v2 = vm_interp_arg(interp, branch->args[1]);
            if (vm_interp_value_le(v1, v2)) {
                block = branch->targets[0];
                goto new_block;
            } else {
                block = branch->targets[1];
                goto new_block;
            }
        }
        case VM_BOP_BEQ: {
            vm_std_value_t v1 = vm_interp_arg(interp, branch->args[0]);
            vm_std_value_t v2 = vm_interp_arg(interp, branch->args[1]);
            if (vm_value_eq(v1, v2)) {
                block = branch->targets[0];
                goto new_block;
            } else {
                block = branch->targets[1];
                goto new_block;
            }
        }
        case VM_BOP_RET: {
            return vm_interp_arg(interp, branch->args[0]);
        }
        case VM_BOP_LOAD: {
            vm_std_value_t v1 = vm_interp_arg(interp, branch->args[0]);
            vm_std_value_t v2 = vm_interp_arg(interp, branch->args[0]);
            switch (v2.tag) {
                case VM_TAG_I8: {
                    vm_interp_out_arg(interp, branch->out, v1.value.closure[v2.value.i8]);
                    break;
                }
                case VM_TAG_I16: {
                    vm_interp_out_arg(interp, branch->out, v1.value.closure[v2.value.i16]);
                    break;
                }
                case VM_TAG_I32: {
                    vm_interp_out_arg(interp, branch->out, v1.value.closure[v2.value.i32]);
                    break;
                }
                case VM_TAG_I64: {
                    vm_interp_out_arg(interp, branch->out, v1.value.closure[v2.value.i16]);
                    break;
                }
                case VM_TAG_F32: {
                    vm_interp_out_arg(interp, branch->out, v1.value.closure[(int64_t) v2.value.f32]);
                    break;
                }
                case VM_TAG_F64: {
                    vm_interp_out_arg(interp, branch->out, v1.value.closure[(int64_t) v2.value.f64]);
                    break;
                }
            }
            block = branch->targets[0];
            goto new_block;
        }
        case VM_BOP_GET: {
            vm_std_value_t v1 = vm_interp_arg(interp, branch->args[0]);
            vm_std_value_t v2 = vm_interp_arg(interp, branch->args[1]);
            vm_pair_t pair;
            pair.key_tag = v2.tag;
            pair.key_val = v2.value;
            vm_table_get_pair(v1.value.table, &pair);
            vm_std_value_t out = (vm_std_value_t) {
                .tag = pair.val_tag,
                .value = pair.val_val,
            };
            vm_interp_out_arg(interp, branch->out, out);
            block = branch->targets[0];
            goto new_block;
        }
        case VM_BOP_CALL: {
            vm_std_value_t v1 = vm_interp_arg(interp, branch->args[0]);
            switch (v1.tag) {
                case VM_TAG_CLOSURE: {
                    next_regs[0] = v1;
                    size_t i = 1;
                    while (branch->args[i].type != VM_TAG_UNK) {
                        next_regs[i] = vm_interp_arg(interp, branch->args[i]);
                        i += 1;
                    }
                    next_regs[i].tag = VM_TAG_UNK;
                    vm_std_value_t *last_regs = interp->regs;
                    interp->regs = next_regs;
                    vm_std_value_t got = vm_interp_block(interp, interp->blocks->blocks[v1.value.closure[0].value.i32]);
                    interp->regs = last_regs;
                    vm_interp_out_arg(interp, branch->out, got);
                    block = branch->targets[0];
                    goto new_block;
                }
                case VM_TAG_FFI: {
                    size_t i = 0;
                    while (branch->args[i + 1].type != VM_TAG_UNK) {
                        next_regs[i] = vm_interp_arg(interp, branch->args[i + 1]);
                        i += 1;
                    }
                    next_regs[i].tag = VM_TAG_UNK;
                    v1.value.ffi(&interp->closure, next_regs);
                    vm_interp_out_arg(interp, branch->out, next_regs[0]);
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
        .regs = vm_malloc(sizeof(vm_std_value_t) * 65536),
        .closure.config = config,
        .closure.blocks = blocks,
    };

    vm_std_value_t ret = vm_interp_block(&interp, entry);

    vm_free(interp.regs);

    return ret;
}
