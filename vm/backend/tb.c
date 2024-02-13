
#include "./tb.h"

#include "../../vendor/common/arena.h"
#include "../../vendor/tb/include/tb.h"
#include "../../vendor/tcc/libtcc.h"
#include "../ir/check.h"
#include "../ir/rblock.h"

#include "./exec.h"

#define VM_TB_CC TB_CDECL
#define VM_TB_TYPE_VALUE TB_TYPE_I64

void vm_tb_new_module(vm_tb_state_t *state);
void *vm_tb_rfunc_comp(vm_rblock_t *rblock);
void vm_tb_func_print_value(vm_tb_state_t *state, vm_tag_t tag, TB_Node *value);
TB_Node *vm_tb_func_body_once(vm_tb_state_t *state, TB_Node **regs, vm_block_t *block);
void vm_tb_func_report_error(vm_tb_state_t *state, const char *str);

#define vm_tb_select_binary_type(xtag, onint, onfloat, ...) ({ \
    vm_tag_t tag = xtag;                                       \
    TB_Node *ret = NULL;                                       \
    if (tag != VM_TAG_F64 && tag != VM_TAG_F32) {              \
        ret = onint(__VA_ARGS__, TB_ARITHMATIC_NONE);          \
    } else {                                                   \
        ret = onfloat(__VA_ARGS__);                            \
    }                                                          \
    ret;                                                       \
})

#define vm_tb_select_binary_cmp(xtag, onint, onfloat, ...) ({ \
    vm_tag_t tag = xtag;                                      \
    TB_Node *ret = NULL;                                      \
    if (tag != VM_TAG_F64 && tag != VM_TAG_F32) {             \
        ret = onint(__VA_ARGS__, true);                       \
    } else {                                                  \
        ret = onfloat(__VA_ARGS__);                           \
    }                                                         \
    ret;                                                      \
})

TB_Node *vm_tb_ptr_name(vm_tb_state_t *state, const char *name, void *value) {
    return tb_inst_uint(state->fun, TB_TYPE_PTR, (uint64_t)value);
}

TB_Node *vm_tb_bitcast_from_value(vm_tb_state_t *state, TB_Node *src, TB_DataType dt) {
    if (src->dt.type != dt.type || src->dt.data != dt.data) {
        return tb_inst_bitcast(state->fun, src, dt);
    }
    return src;
}

TB_Node *vm_tb_bitcast_to_value(vm_tb_state_t *state, TB_Node *src) {
    if (src->dt.type != VM_TB_TYPE_VALUE.type || src->dt.data != VM_TB_TYPE_VALUE.data) {
        return tb_inst_bitcast(state->fun, src, VM_TB_TYPE_VALUE);
    }
    return src;
}

TB_DataType vm_tag_to_tb_type(vm_tag_t tag) {
    switch (tag) {
        case VM_TAG_NIL: {
            return TB_TYPE_PTR;
        }
        case VM_TAG_BOOL: {
            return TB_TYPE_BOOL;
        }
        case VM_TAG_I8: {
            return TB_TYPE_I8;
        }
        case VM_TAG_I16: {
            return TB_TYPE_I16;
        }
        case VM_TAG_I32: {
            return TB_TYPE_I32;
        }
        case VM_TAG_I64: {
            return TB_TYPE_I64;
        }
        case VM_TAG_F32: {
            return TB_TYPE_F32;
        }
        case VM_TAG_F64: {
            return TB_TYPE_F64;
        }
        case VM_TAG_STR: {
            return TB_TYPE_PTR;
        }
        case VM_TAG_FUN: {
            return TB_TYPE_I32;
        }
        case VM_TAG_CLOSURE: {
            return TB_TYPE_PTR;
        }
        case VM_TAG_TAB: {
            return TB_TYPE_PTR;
        }
        case VM_TAG_FFI: {
            return TB_TYPE_PTR;
        }
        case VM_TAG_ERROR: {
            return TB_TYPE_PTR;
        }
        default: {
            fprintf(stderr, "\nunhandled tag #%zu\n", (size_t)tag);
            __builtin_trap();
        }
    }
}

TB_Node *vm_tb_func_read_arg(vm_tb_state_t *state, TB_Node **regs, vm_arg_t arg) {
    switch (arg.type) {
        case VM_ARG_LIT: {
            switch (arg.lit.tag) {
                case VM_TAG_NIL: {
                    return vm_tb_ptr_name(state, "0", NULL);
                }
                case VM_TAG_BOOL: {
                    return tb_inst_bool(state->fun, arg.lit.value.b);
                }
                case VM_TAG_I8: {
                    return tb_inst_sint(state->fun, TB_TYPE_I8, arg.lit.value.i8);
                }
                case VM_TAG_I16: {
                    return tb_inst_sint(state->fun, TB_TYPE_I16, arg.lit.value.i16);
                }
                case VM_TAG_I32: {
                    return tb_inst_sint(state->fun, TB_TYPE_I32, arg.lit.value.i32);
                }
                case VM_TAG_I64: {
                    return tb_inst_sint(state->fun, TB_TYPE_I64, arg.lit.value.i64);
                }
                case VM_TAG_F32: {
                    return tb_inst_float32(state->fun, arg.lit.value.f32);
                }
                case VM_TAG_F64: {
                    return tb_inst_float64(state->fun, arg.lit.value.f64);
                }
                case VM_TAG_STR: {
                    return tb_inst_uint(state->fun, TB_TYPE_PTR, (uint64_t)(size_t)arg.lit.value.str);
                }
                case VM_TAG_FFI: {
                    return vm_tb_ptr_name(state, NULL, arg.lit.value.ffi);
                }
                default: {
                    __builtin_trap();
                }
            }
            __builtin_trap();
        }
        case VM_ARG_NONE: {
            return vm_tb_ptr_name(state, "0", NULL);
        }
        case VM_ARG_REG: {
            return vm_tb_bitcast_from_value(
                state,
                tb_inst_load(
                    state->fun,
                    VM_TB_TYPE_VALUE,
                    regs[arg.reg],
                    8,
                    false
                ),
                vm_tag_to_tb_type(arg.reg_tag)
            );
        }
        case VM_ARG_FUN: {
            return tb_inst_uint(state->fun, TB_TYPE_I32, (uint64_t)arg.func->id);
        }
        default: {
            fprintf(stderr, "\nunhandled arg (type#%zu)\n", (size_t)arg.type);
            __builtin_trap();
        }
    }
}

static bool vm_tb_str_eq(const char *str1, const char *str2) {
    return strcmp(str1, str2) == 0;
}

static bool vm_tb_str_lt(const char *str1, const char *str2) {
    return strcmp(str1, str2) < 0;
}

static void vm_tb_func_reset_pass(vm_block_t *block);

static void vm_tb_func_reset_pass_internal(vm_block_t *block) {
    if (block->pass == NULL) {
        return;
    }
    vm_tb_func_reset_pass(block);
}

static void vm_tb_func_reset_pass(vm_block_t *block) {
    block->pass = NULL;
    vm_branch_t branch = block->branch;
    switch (branch.op) {
        case VM_BOP_JUMP: {
            vm_tb_func_reset_pass_internal(branch.targets[0]);
            break;
        }
        case VM_BOP_BB:
        case VM_BOP_BLT:
        case VM_BOP_BLE: {
            vm_tb_func_reset_pass_internal(branch.targets[0]);
            vm_tb_func_reset_pass_internal(branch.targets[1]);
            break;
        }
        case VM_BOP_BEQ: {
            if (vm_arg_to_tag(branch.args[0]) != vm_arg_to_tag(branch.args[1])) {
                vm_tb_func_reset_pass_internal(branch.targets[1]);
            } else if (branch.tag == VM_TAG_NIL) {
                vm_tb_func_reset_pass_internal(branch.targets[0]);
            } else {
                vm_tb_func_reset_pass_internal(branch.targets[0]);
                vm_tb_func_reset_pass_internal(branch.targets[1]);
            }
            break;
        }
    }
}

void vm_tb_func_write(vm_tb_state_t *state, vm_tag_t tag, TB_Node **regs, size_t reg, TB_Node *value) {
    tb_inst_store(
        state->fun,
        VM_TB_TYPE_VALUE,
        regs[reg],
        vm_tb_bitcast_to_value(state, value),
        8,
        false
    );
}

TB_MultiOutput vm_tb_inst_call(vm_tb_state_t *state, TB_FunctionPrototype *proto, TB_Node *target, size_t param_count, TB_Node **params) {
    return tb_inst_call(state->fun, proto, target, param_count, params);
}

void vm_tb_func_body_once_as(vm_tb_state_t *state, TB_Node **regs, vm_block_t *block);

TB_Node *vm_tb_func_body_once(vm_tb_state_t *state, TB_Node **regs, vm_block_t *block) {
    if (block->pass != NULL) {
        return block->pass;
    }
    TB_Trace old_ctrl = tb_inst_get_trace(state->fun);
    TB_Node *ret = tb_inst_region(state->fun);
    block->pass = ret;
    char name[24];
    snprintf(name, 23, "vm.%zi", block->id);
    tb_inst_set_region_name(state->fun, ret, -1, name);
    tb_inst_set_control(state->fun, ret);
    vm_tb_func_body_once_as(state, regs, block);
    tb_inst_set_trace(state->fun, old_ctrl);
    return ret;
}

void vm_tb_func_body_once_as(vm_tb_state_t *state, TB_Node **regs, vm_block_t *block) {
    {
        const char *block_err = vm_check_block(block);
        if (block_err != NULL) {
            vm_tb_func_report_error(state, block_err);
            return;
        }
    }

#if VM_USE_DUMP
    if (state->config->dump_ver) {
        vm_io_buffer_t buf = {0};
        vm_io_format_block(&buf, block);
        fprintf(stdout, "\n--- vmir ---\n%.*s", (int)buf.len, buf.buf);
    }
#endif

    for (size_t n = 0; n < block->len; n++) {
        vm_instr_t instr = block->instrs[n];
        switch (instr.op) {
            case VM_IOP_MOVE: {
                vm_tb_func_write(state, instr.tag, regs, instr.out.reg, vm_tb_func_read_arg(state, regs, instr.args[0]));
                break;
            }
            case VM_IOP_ADD: {
                TB_Node *value = NULL;
                if (instr.tag == VM_TAG_F32 || instr.tag == VM_TAG_F64) {
                    value = tb_inst_fadd(
                        state->fun,
                        vm_tb_func_read_arg(state, regs, instr.args[0]),
                        vm_tb_func_read_arg(state, regs, instr.args[1])
                    );
                } else {
                    value = tb_inst_add(
                        state->fun,
                        vm_tb_func_read_arg(state, regs, instr.args[0]),
                        vm_tb_func_read_arg(state, regs, instr.args[1]),
                        TB_ARITHMATIC_NONE
                    );
                }
                vm_tb_func_write(state, instr.tag, regs, instr.out.reg, value);
                break;
            }
            case VM_IOP_SUB: {
                TB_Node *value = NULL;
                if (instr.tag == VM_TAG_F32 || instr.tag == VM_TAG_F64) {
                    value = tb_inst_fsub(
                        state->fun,
                        vm_tb_func_read_arg(state, regs, instr.args[0]),
                        vm_tb_func_read_arg(state, regs, instr.args[1])
                    );
                } else {
                    value = tb_inst_sub(
                        state->fun,
                        vm_tb_func_read_arg(state, regs, instr.args[0]),
                        vm_tb_func_read_arg(state, regs, instr.args[1]),
                        TB_ARITHMATIC_NONE
                    );
                }
                vm_tb_func_write(state, instr.tag, regs, instr.out.reg, value);
                break;
            }
            case VM_IOP_MUL: {
                TB_Node *value = NULL;
                if (instr.tag == VM_TAG_F32 || instr.tag == VM_TAG_F64) {
                    value = tb_inst_fmul(
                        state->fun,
                        vm_tb_func_read_arg(state, regs, instr.args[0]),
                        vm_tb_func_read_arg(state, regs, instr.args[1])
                    );
                } else {
                    value = tb_inst_mul(
                        state->fun,
                        vm_tb_func_read_arg(state, regs, instr.args[0]),
                        vm_tb_func_read_arg(state, regs, instr.args[1]),
                        TB_ARITHMATIC_NONE
                    );
                }
                vm_tb_func_write(state, instr.tag, regs, instr.out.reg, value);
                break;
            }
            case VM_IOP_DIV: {
                TB_Node *value = NULL;
                if (instr.tag == VM_TAG_F32 || instr.tag == VM_TAG_F64) {
                    value = tb_inst_fdiv(
                        state->fun,
                        vm_tb_func_read_arg(state, regs, instr.args[0]),
                        vm_tb_func_read_arg(state, regs, instr.args[1])
                    );
                } else {
                    value = tb_inst_div(
                        state->fun,
                        vm_tb_func_read_arg(state, regs, instr.args[0]),
                        vm_tb_func_read_arg(state, regs, instr.args[1]),
                        true
                    );
                }
                vm_tb_func_write(state, instr.tag, regs, instr.out.reg, value);
                break;
            }
            case VM_IOP_IDIV: {
                if (instr.tag == VM_TAG_F32) {
                    TB_Node *raw_div = tb_inst_fdiv(
                        state->fun,
                        vm_tb_func_read_arg(state, regs, instr.args[0]),
                        vm_tb_func_read_arg(state, regs, instr.args[1])
                    );
                    TB_PrototypeParam f32p = {TB_TYPE_F32};
                    TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, 1, &f32p, 1, &f32p, false);
                    TB_MultiOutput multi = tb_inst_call(
                        state->fun,
                        proto,
                        vm_tb_ptr_name(state, "floor", &floorf),
                        1,
                        &raw_div
                    );
                    vm_tb_func_write(
                        state,
                        instr.tag,
                        regs,
                        instr.out.reg,
                        multi.single
                    );
                } else if (instr.tag == VM_TAG_F64) {
                    TB_Node *raw_div = tb_inst_fdiv(
                        state->fun,
                        vm_tb_func_read_arg(state, regs, instr.args[0]),
                        vm_tb_func_read_arg(state, regs, instr.args[1])
                    );
                    TB_PrototypeParam f64p = {TB_TYPE_F64};
                    TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, 1, &f64p, 1, &f64p, false);
                    TB_MultiOutput multi = tb_inst_call(
                        state->fun,
                        proto,
                        vm_tb_ptr_name(state, "floor", &floor),
                        1,
                        &raw_div
                    );
                    vm_tb_func_write(
                        state,
                        instr.tag,
                        regs,
                        instr.out.reg,
                        multi.single
                    );
                } else {
                    TB_Node *value = tb_inst_div(
                        state->fun,
                        vm_tb_func_read_arg(state, regs, instr.args[0]),
                        vm_tb_func_read_arg(state, regs, instr.args[1]),
                        true
                    );
                    vm_tb_func_write(state, instr.tag, regs, instr.out.reg, value);
                }
                break;
            }
            case VM_IOP_MOD: {
                if (instr.tag == VM_TAG_F64) {
                    TB_Node *bad = tb_inst_region(state->fun);
                    tb_inst_set_region_name(state->fun, bad, -1, "bad_mod");
                    TB_Node *good = tb_inst_region(state->fun);
                    tb_inst_set_region_name(state->fun, good, -1, "good_mod");
                    TB_Node *after = tb_inst_region(state->fun);
                    tb_inst_set_region_name(state->fun, after, -1, "after_mod");
                    TB_Node *lhs = vm_tb_func_read_arg(state, regs, instr.args[0]);
                    TB_Node *rhs = vm_tb_func_read_arg(state, regs, instr.args[1]);
                    TB_Node *raw_div = tb_inst_fdiv(state->fun, lhs, rhs);
                    TB_Node *too_low = tb_inst_cmp_flt(state->fun, raw_div, tb_inst_float64(state->fun, (double)INT64_MIN));
                    TB_Node *too_high = tb_inst_cmp_fgt(state->fun, raw_div, tb_inst_float64(state->fun, (double)INT64_MAX));
                    TB_Node *is_bad = tb_inst_or(state->fun, too_low, too_high);
                    tb_inst_if(state->fun, is_bad, bad, good);
                    {
                        tb_inst_set_control(state->fun, good);
                        TB_Node *int_div = tb_inst_float2int(state->fun, raw_div, TB_TYPE_I64, true);
                        TB_Node *float_div = tb_inst_int2float(state->fun, int_div, TB_TYPE_F64, true);
                        TB_Node *mul = tb_inst_fmul(state->fun, float_div, rhs);
                        TB_Node *sub = tb_inst_fsub(state->fun, lhs, mul);
                        vm_tb_func_write(
                            state,
                            instr.tag,
                            regs,
                            instr.out.reg,
                            sub
                        );
                        tb_inst_goto(state->fun, after);
                    }
                    {
                        tb_inst_set_control(state->fun, bad);
                        TB_Node *mul = tb_inst_fmul(state->fun, raw_div, rhs);
                        TB_Node *sub = tb_inst_fsub(state->fun, lhs, mul);
                        vm_tb_func_write(
                            state,
                            instr.tag,
                            regs,
                            instr.out.reg,
                            sub
                        );
                        tb_inst_goto(state->fun, after);
                    }
                    tb_inst_set_control(state->fun, after);
                } else if (instr.tag == VM_TAG_F32) {
                    TB_Node *bad = tb_inst_region(state->fun);
                    tb_inst_set_region_name(state->fun, bad, -1, "bad_mod");
                    TB_Node *good = tb_inst_region(state->fun);
                    tb_inst_set_region_name(state->fun, bad, -1, "good_mod");
                    TB_Node *after = tb_inst_region(state->fun);
                    tb_inst_set_region_name(state->fun, bad, -1, "after_mod");
                    TB_Node *lhs = vm_tb_func_read_arg(state, regs, instr.args[0]);
                    TB_Node *rhs = vm_tb_func_read_arg(state, regs, instr.args[1]);
                    TB_Node *raw_div = tb_inst_fdiv(state->fun, lhs, rhs);
                    TB_Node *too_low = tb_inst_cmp_flt(state->fun, raw_div, tb_inst_float32(state->fun, (float)INT32_MIN));
                    TB_Node *too_high = tb_inst_cmp_fgt(state->fun, raw_div, tb_inst_float32(state->fun, (float)INT32_MAX));
                    TB_Node *is_bad = tb_inst_or(state->fun, too_low, too_high);
                    tb_inst_if(state->fun, is_bad, bad, good);
                    {
                        tb_inst_set_control(state->fun, good);
                        TB_Node *int_div = tb_inst_float2int(state->fun, raw_div, TB_TYPE_I32, true);
                        TB_Node *float_div = tb_inst_int2float(state->fun, int_div, TB_TYPE_F32, true);
                        TB_Node *mul = tb_inst_fmul(state->fun, float_div, rhs);
                        TB_Node *sub = tb_inst_fsub(state->fun, lhs, mul);
                        vm_tb_func_write(
                            state,
                            instr.tag,
                            regs,
                            instr.out.reg,
                            sub
                        );
                        tb_inst_goto(state->fun, after);
                    }
                    {
                        tb_inst_set_control(state->fun, bad);
                        TB_Node *mul = tb_inst_fmul(state->fun, raw_div, rhs);
                        TB_Node *sub = tb_inst_fsub(state->fun, lhs, mul);
                        vm_tb_func_write(
                            state,
                            instr.tag,
                            regs,
                            instr.out.reg,
                            sub
                        );
                        tb_inst_goto(state->fun, after);
                    }
                    tb_inst_set_control(state->fun, after);
                } else {
                    TB_Node *value = tb_inst_mod(
                        state->fun,
                        vm_tb_func_read_arg(state, regs, instr.args[0]),
                        vm_tb_func_read_arg(state, regs, instr.args[1]),
                        true
                    );
                    vm_tb_func_write(state, instr.tag, regs, instr.out.reg, value);
                }
                break;
            }
            case VM_IOP_NOP: {
                break;
            }
            case VM_IOP_STD: {
                vm_tb_func_write(
                    state,
                    instr.tag,
                    regs,
                    instr.out.reg,
                    vm_tb_ptr_name(state, "<data>", state->std)
                );
                break;
            }
            case VM_IOP_LEN: {
                vm_tag_t table_tag = vm_arg_to_tag(instr.args[0]);
                if (table_tag == VM_TAG_TAB) {
                    TB_Node *len = tb_inst_load(
                        state->fun,
                        TB_TYPE_I32,
                        tb_inst_member_access(
                            state->fun,
                            vm_tb_func_read_arg(state, regs, instr.args[0]),
                            offsetof(vm_table_t, len)
                        ),
                        1,
                        false
                    );
                    if (instr.tag == VM_TAG_I32) {
                        // defaults to i32
                    } else if (instr.tag == VM_TAG_I64) {
                        len = tb_inst_zxt(state->fun, len, TB_TYPE_I64);
                    } else if (instr.tag == VM_TAG_F32) {
                        len = tb_inst_int2float(state->fun, len, TB_TYPE_F32, false);
                    } else if (instr.tag == VM_TAG_F64) {
                        len = tb_inst_int2float(state->fun, len, TB_TYPE_F64, false);
                    }
                    vm_tb_func_write(
                        state,
                        instr.tag,
                        regs,
                        instr.out.reg,
                        len
                    );
                } else {
                    fprintf(stderr, "\n ^ unhandled instruction\n");
                }
                break;
            }
            case VM_IOP_SET: {
                vm_tag_t key_tag = vm_arg_to_tag(instr.args[1]);
                vm_tag_t val_tag = vm_arg_to_tag(instr.args[2]);
                TB_PrototypeParam proto_params[5] = {
                    {TB_TYPE_PTR},
                    {VM_TB_TYPE_VALUE},
                    {VM_TB_TYPE_VALUE},
                    {TB_TYPE_I32},
                    {TB_TYPE_I32},
                };
                TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, 5, proto_params, 0, NULL, false);
                TB_Node *args[5] = {
                    vm_tb_func_read_arg(state, regs, instr.args[0]),
                    vm_tb_bitcast_to_value(state, vm_tb_func_read_arg(state, regs, instr.args[1])),
                    vm_tb_bitcast_to_value(state, vm_tb_func_read_arg(state, regs, instr.args[2])),
                    tb_inst_uint(state->fun, TB_TYPE_I32, key_tag),
                    tb_inst_uint(state->fun, TB_TYPE_I32, val_tag),
                };
                vm_tb_inst_call(
                    state,
                    proto,
                    tb_inst_get_symbol_address(state->fun, state->vm_table_iset),
                    5,
                    args
                );
                break;
            }
            case VM_IOP_NEW: {
                TB_PrototypeParam proto_ret[1] = {
                    {TB_TYPE_PTR},
                };
                TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, 0, NULL, 1, proto_ret, false);
                tb_inst_store(
                    state->fun,
                    TB_TYPE_PTR,
                    regs[instr.out.reg],
                    vm_tb_inst_call(
                        state,
                        proto,
                        tb_inst_get_symbol_address(state->fun, state->vm_table_new),
                        0,
                        NULL
                    )
                        .single,
                    8,
                    false
                );
                break;
            }
            default: {
                fprintf(stderr, "\nunhandled instruction #%zu\n", (size_t)instr.op);
                exit(1);
            }
        }
    }

    vm_branch_t branch = block->branch;

    switch (branch.op) {
        case VM_BOP_JUMP: {
            tb_inst_goto(
                state->fun,
                vm_tb_func_body_once(state, regs, branch.targets[0])
            );
            break;
        }

        case VM_BOP_BB: {
            if (branch.tag == VM_TAG_BOOL) {
                tb_inst_if(
                    state->fun,
                    vm_tb_func_read_arg(state, regs, branch.args[0]),
                    vm_tb_func_body_once(state, regs, branch.targets[0]),
                    vm_tb_func_body_once(state, regs, branch.targets[1])
                );
            } else if (branch.tag == VM_TAG_NIL) {
                tb_inst_goto(
                    state->fun,
                    vm_tb_func_body_once(state, regs, branch.targets[1])
                );
            } else {
                tb_inst_goto(
                    state->fun,
                    vm_tb_func_body_once(state, regs, branch.targets[0])
                );
            }
            break;
        }

        case VM_BOP_BLT: {
            tb_inst_if(
                state->fun,
                vm_tb_select_binary_cmp(
                    branch.tag,
                    tb_inst_cmp_ilt, tb_inst_cmp_flt,
                    state->fun,
                    vm_tb_func_read_arg(state, regs, branch.args[0]),
                    vm_tb_func_read_arg(state, regs, branch.args[1])
                ),
                vm_tb_func_body_once(state, regs, branch.targets[0]),
                vm_tb_func_body_once(state, regs, branch.targets[1])
            );
            break;
        }

        case VM_BOP_BLE: {
            tb_inst_if(
                state->fun,
                vm_tb_select_binary_cmp(
                    branch.tag,
                    tb_inst_cmp_ile, tb_inst_cmp_fle,
                    state->fun,
                    vm_tb_func_read_arg(state, regs, branch.args[0]),
                    vm_tb_func_read_arg(state, regs, branch.args[1])
                ),
                vm_tb_func_body_once(state, regs, branch.targets[0]),
                vm_tb_func_body_once(state, regs, branch.targets[1])
            );
            break;
        }

        case VM_BOP_BEQ: {
            if (vm_arg_to_tag(branch.args[0]) != vm_arg_to_tag(branch.args[1])) {
                tb_inst_goto(
                    state->fun,
                    vm_tb_func_body_once(state, regs, branch.targets[1])
                );
            } else if (branch.tag == VM_TAG_NIL) {
                tb_inst_goto(
                    state->fun,
                    vm_tb_func_body_once(state, regs, branch.targets[0])
                );
            } else if (branch.tag == VM_TAG_STR) {
                TB_PrototypeParam params[2] = {
                    {TB_TYPE_PTR},
                    {TB_TYPE_PTR},
                };

                TB_PrototypeParam rets[1] = {
                    {TB_TYPE_BOOL},
                };

                TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, 2, params, 1, rets, false);

                TB_Node *args[2] = {
                    vm_tb_func_read_arg(state, regs, branch.args[0]),
                    vm_tb_func_read_arg(state, regs, branch.args[1]),
                };

                TB_MultiOutput out = vm_tb_inst_call(
                    state,
                    proto,
                    vm_tb_ptr_name(state, "vm_tb_str_eq", &vm_tb_str_eq),
                    2,
                    args
                );

                tb_inst_if(
                    state->fun,
                    out.single,
                    vm_tb_func_body_once(state, regs, branch.targets[0]),
                    vm_tb_func_body_once(state, regs, branch.targets[1])
                );
            } else {
                tb_inst_if(
                    state->fun,
                    tb_inst_cmp_eq(
                        state->fun,
                        vm_tb_func_read_arg(state, regs, branch.args[0]),
                        vm_tb_func_read_arg(state, regs, branch.args[1])
                    ),
                    vm_tb_func_body_once(state, regs, branch.targets[0]),
                    vm_tb_func_body_once(state, regs, branch.targets[1])
                );
            }
            break;
        }

        case VM_BOP_RET: {
            TB_Node *ret[2];

            ret[0] = vm_tb_bitcast_to_value(state, vm_tb_func_read_arg(state, regs, branch.args[0]));

            ret[1] = tb_inst_uint(state->fun, TB_TYPE_I32, branch.tag);

            tb_inst_ret(state->fun, 2, ret);
            break;
        }

        case VM_BOP_GET:
        case VM_BOP_CALL: {
            TB_Node *val_val = NULL;
            TB_Node *val_tag = NULL;

            if (branch.op == VM_BOP_CALL) {
                size_t nparams = 0;

                for (size_t i = 1; branch.args[i].type != VM_ARG_NONE; i++) {
                    nparams += 1;
                }
                if (vm_arg_to_tag(branch.args[0]) == VM_TAG_FFI) {
                    TB_PrototypeParam call_proto_params[2] = {
                        {TB_TYPE_PTR},
                        {TB_TYPE_PTR},
                    };

                    vm_std_closure_t *closure = vm_malloc(sizeof(vm_std_closure_t));

                    closure->config = state->config;
                    closure->blocks = state->blocks;

                    TB_FunctionPrototype *call_proto = tb_prototype_create(state->module, VM_TB_CC, 2, call_proto_params, 0, NULL, false);

                    TB_Node *call_arg = tb_inst_local(state->fun, sizeof(vm_std_value_t) * (nparams + 1), 8);

                    for (size_t i = 1; branch.args[i].type != VM_ARG_NONE; i++) {
                        vm_arg_t arg = branch.args[i];
                        TB_Node *head = tb_inst_member_access(state->fun, call_arg, sizeof(vm_std_value_t) * (i - 1));
                        vm_tag_t tag = vm_arg_to_tag(arg);
                        if (tag != VM_TAG_NIL) {
                            tb_inst_store(
                                state->fun,
                                VM_TB_TYPE_VALUE,
                                tb_inst_member_access(state->fun, head, offsetof(vm_std_value_t, value)),
                                vm_tb_bitcast_to_value(state, vm_tb_func_read_arg(state, regs, arg)),
                                8,
                                false
                            );
                        }
                        tb_inst_store(
                            state->fun,
                            TB_TYPE_I32,
                            tb_inst_member_access(state->fun, head, offsetof(vm_std_value_t, tag)),
                            tb_inst_uint(state->fun, TB_TYPE_I32, tag),
                            4,
                            false
                        );
                    }

                    TB_Node *end_head = tb_inst_member_access(state->fun, call_arg, sizeof(vm_std_value_t) * nparams);

                    tb_inst_store(
                        state->fun,
                        TB_TYPE_I32,
                        tb_inst_member_access(state->fun, end_head, offsetof(vm_std_value_t, tag)),
                        tb_inst_uint(state->fun, TB_TYPE_I32, 0),
                        4,
                        false
                    );

                    TB_Node *call_func = vm_tb_func_read_arg(state, regs, branch.args[0]);

                    TB_Node *call_args[2] = {
                        vm_tb_ptr_name(state, "closure", closure),
                        call_arg,
                    };

                    vm_tb_inst_call(
                        state,
                        call_proto,
                        call_func,
                        2,
                        call_args
                    );

                    val_tag = tb_inst_load(
                        state->fun,
                        TB_TYPE_I32,
                        tb_inst_member_access(
                            state->fun,
                            call_arg,
                            offsetof(vm_std_value_t, tag)
                        ),
                        4,
                        false
                    );

                    val_val = tb_inst_load(
                        state->fun,
                        VM_TB_TYPE_VALUE,
                        tb_inst_member_access(
                            state->fun,
                            call_arg,
                            offsetof(vm_std_value_t, value)
                        ),
                        4,
                        false
                    );
                } else if (vm_arg_to_tag(branch.args[0]) == VM_TAG_CLOSURE) {
                    TB_Node *closure = vm_tb_func_read_arg(state, regs, branch.args[0]);
                    TB_Node *block_num = vm_tb_bitcast_from_value(
                        state, 
                        tb_inst_load(
                            state->fun,
                            VM_TB_TYPE_VALUE,
                            tb_inst_member_access(
                                state->fun,
                                closure,
                                offsetof(vm_std_value_t, value)
                            ),
                            1,
                            false
                        ),
                        TB_TYPE_I32
                    );

                    void **cache = vm_malloc(sizeof(void *) * state->blocks->len);
                    memset(cache, 0, sizeof(void *) * state->blocks->len);

                    TB_Node *has_cache = tb_inst_region(state->fun);
                    tb_inst_set_region_name(state->fun, has_cache, -1, "has_cache");
                    TB_Node *no_cache = tb_inst_region(state->fun);
                    tb_inst_set_region_name(state->fun, no_cache, -1, "no_cache");
                    TB_Node *after = tb_inst_region(state->fun);
                    tb_inst_set_region_name(state->fun, after, -1, "after_cache");

                    val_val = tb_inst_local(state->fun, 8, 8);
                    val_tag = tb_inst_local(state->fun, 4, 4);

                    size_t nargs = 1;
                    for (size_t arg = 1; branch.args[arg].type != VM_ARG_NONE; arg++) {
                        nargs += 1;
                    }

                    TB_PrototypeParam *call_proto_params = vm_malloc(sizeof(TB_PrototypeParam) * nargs);

                    call_proto_params[0] = (TB_PrototypeParam){VM_TB_TYPE_VALUE};
                    for (size_t arg = 1; branch.args[arg].type != VM_ARG_NONE; arg++) {
                        call_proto_params[arg] = (TB_PrototypeParam){
                            VM_TB_TYPE_VALUE,
                        };
                    }

                    TB_PrototypeParam call_proto_rets[2] = {
                        {VM_TB_TYPE_VALUE},
                        {TB_TYPE_I32},
                    };

                    TB_FunctionPrototype *call_proto = tb_prototype_create(state->module, VM_TB_CC, nargs, call_proto_params, 2, call_proto_rets, false);
#if VM_USE_CACHE
                    TB_Node *global_ptr = tb_inst_array_access(
                        state->fun,
                        vm_tb_ptr_name(state, "<code_cache>", cache),
                        block_num,
                        sizeof(void *)
                    );
                    TB_Node *global = tb_inst_load(state->fun, TB_TYPE_PTR, global_ptr, 1, false);

                    tb_inst_if(
                        state->fun,
                        tb_inst_cmp_eq(state->fun, global, vm_tb_ptr_name(state, "0", NULL)),
                        no_cache,
                        has_cache
                    );

                    {
                        tb_inst_set_control(state->fun, no_cache);

                        TB_Node *rblock_call_table = vm_tb_ptr_name(state, "<call_table>", branch.call_table);
                        TB_Node *rblock_ref = tb_inst_array_access(
                            state->fun,
                            rblock_call_table,
                            block_num,
                            sizeof(vm_rblock_t *)
                        );
                        TB_Node *rblock = tb_inst_load(state->fun, TB_TYPE_PTR, rblock_ref, 1, false);

                        TB_PrototypeParam comp_params[1] = {
                            {TB_TYPE_PTR},
                        };

                        TB_PrototypeParam comp_ret[1] = {
                            {TB_TYPE_PTR},
                        };

                        TB_FunctionPrototype *comp_proto = tb_prototype_create(state->module, VM_TB_CC, 1, comp_params, 1, comp_ret, false);

                        for (size_t i = 0; i < state->blocks->len; i++) {
                            vm_rblock_t *rblock = branch.call_table[i];
                            if (rblock == NULL) {
                                continue;
                            }
                            rblock->state = state;
                        }

                        TB_Node *call_func = vm_tb_inst_call(
                                                 state,
                                                 comp_proto,
                                                 tb_inst_get_symbol_address(state->fun, state->vm_tb_rfunc_comp),
                                                 1,
                                                 &rblock
                        )
                                                 .single;

                        tb_inst_store(
                            state->fun,
                            TB_TYPE_PTR,
                            global_ptr,
                            call_func,
                            1,
                            false
                        );

                        TB_Node **call_args = vm_malloc(sizeof(TB_Node *) * nargs);
                        call_args[0] = vm_tb_bitcast_to_value(state, closure);
                        for (size_t arg = 1; branch.args[arg].type != VM_ARG_NONE; arg++) {
                            call_args[arg] = vm_tb_bitcast_to_value(state, vm_tb_func_read_arg(state, regs, branch.args[arg]));
                        }

                        TB_Node **got = vm_tb_inst_call(
                                            state,
                                            call_proto,
                                            call_func,
                                            nargs,
                                            call_args
                        )
                                            .multiple;

                        vm_free(call_args);

                        tb_inst_store(
                            state->fun,
                            VM_TB_TYPE_VALUE,
                            val_val,
                            got[0],
                            8,
                            false
                        );

                        tb_inst_store(
                            state->fun,
                            TB_TYPE_I32,
                            val_tag,
                            got[1],
                            4,
                            false
                        );

                        tb_inst_goto(state->fun, after);
                    }

                    {
                        tb_inst_set_control(state->fun, has_cache);

                        TB_Node **call_args = vm_malloc(sizeof(TB_Node *) * nargs);
                        call_args[0] = vm_tb_bitcast_to_value(state, closure);
                        for (size_t arg = 1; branch.args[arg].type != VM_ARG_NONE; arg++) {
                            call_args[arg] = vm_tb_bitcast_to_value(state, vm_tb_func_read_arg(state, regs, branch.args[arg]));
                        }

                        TB_Node **got = vm_tb_inst_call(
                                            state,
                                            call_proto,
                                            global,
                                            nargs,
                                            call_args
                        )
                                            .multiple;

                        vm_free(call_args);

                        tb_inst_store(
                            state->fun,
                            VM_TB_TYPE_VALUE,
                            val_val,
                            got[0],
                            8,
                            false
                        );
                        tb_inst_store(
                            state->fun,
                            TB_TYPE_I32,
                            val_tag,
                            got[1],
                            4,
                            false
                        );

                        tb_inst_goto(state->fun, after);
                    }

                    tb_inst_set_control(state->fun, after);
#else

                        TB_Node *rblock_call_table = vm_tb_ptr_name(state, "<call_table>", branch.call_table);
                        TB_Node *rblock_ref = tb_inst_array_access(
                            state->fun,
                            rblock_call_table,
                            block_num,
                            sizeof(vm_rblock_t *)
                        );
                        TB_Node *rblock = tb_inst_load(state->fun, TB_TYPE_PTR, rblock_ref, 1, false);

                        TB_PrototypeParam comp_params[1] = {
                            {TB_TYPE_PTR},
                        };

                        TB_PrototypeParam comp_ret[1] = {
                            {TB_TYPE_PTR},
                        };

                        TB_FunctionPrototype *comp_proto = tb_prototype_create(state->module, VM_TB_CC, 1, comp_params, 1, comp_ret, false);

                        for (size_t i = 0; i < state->blocks->len; i++) {
                            vm_rblock_t *rblock = branch.call_table[i];
                            if (rblock == NULL) {
                                continue;
                            }
                            rblock->state = state;
                        }

                        TB_Node *call_func = vm_tb_inst_call(
                                                 state,
                                                 comp_proto,
                                                 tb_inst_get_symbol_address(state->fun, state->vm_tb_rfunc_comp),
                                                 1,
                                                 &rblock
                        )
                                                 .single;

                        TB_Node **call_args = vm_malloc(sizeof(TB_Node *) * nargs);
                        call_args[0] = vm_tb_bitcast_to_value(state, closure);
                        for (size_t arg = 1; branch.args[arg].type != VM_ARG_NONE; arg++) {
                            call_args[arg] = vm_tb_bitcast_to_value(state, vm_tb_func_read_arg(state, regs, branch.args[arg]));
                        }

                        TB_Node **got = vm_tb_inst_call(
                                            state,
                                            call_proto,
                                            call_func,
                                            nargs,
                                            call_args
                        )
                                            .multiple;

                        vm_free(call_args);

                        tb_inst_store(
                            state->fun,
                            VM_TB_TYPE_VALUE,
                            val_val,
                            got[0],
                            8,
                            false
                        );

                        tb_inst_store(
                            state->fun,
                            TB_TYPE_I32,
                            val_tag,
                            got[1],
                            4,
                            false
                        );

#endif

                    val_val = tb_inst_load(
                        state->fun,
                        VM_TB_TYPE_VALUE,
                        val_val,
                        8,
                        false
                    );

                    val_tag = tb_inst_load(
                        state->fun,
                        TB_TYPE_I32,
                        val_tag,
                        4,
                        false
                    );
                } else {
                    vm_tb_func_report_error(state, "bad call");
                    break;
                }
            } else if (branch.op == VM_BOP_GET) {
                vm_tag_t arg0tag = vm_arg_to_tag(branch.args[0]);
                if (arg0tag == VM_TAG_TAB) {
                    TB_PrototypeParam get_params[2] = {
                        {TB_TYPE_PTR},
                        {TB_TYPE_PTR},
                    };

                    TB_FunctionPrototype *get_proto = tb_prototype_create(state->module, VM_TB_CC, 2, get_params, 0, NULL, false);
                    TB_Node *arg2 = tb_inst_local(state->fun, sizeof(vm_pair_t), 8);
                    vm_tag_t tag = vm_arg_to_tag(branch.args[1]);
                    tb_inst_store(
                        state->fun,
                        VM_TB_TYPE_VALUE,
                        tb_inst_member_access(
                            state->fun,
                            arg2,
                            offsetof(vm_pair_t, key_val)
                        ),
                        vm_tb_bitcast_to_value(state, vm_tb_func_read_arg(state, regs, branch.args[1])),
                        8,
                        false
                    );
                    tb_inst_store(
                        state->fun,
                        TB_TYPE_I32,
                        tb_inst_member_access(
                            state->fun,
                            arg2,
                            offsetof(vm_pair_t, key_tag)
                        ),
                        tb_inst_uint(state->fun, TB_TYPE_I32, tag),
                        4,
                        false
                    );
                    TB_Node *get_args[2] = {
                        vm_tb_func_read_arg(state, regs, branch.args[0]),
                        arg2,
                    };
                    vm_tb_inst_call(
                        state,
                        get_proto,
                        tb_inst_get_symbol_address(state->fun, state->vm_table_get_pair),
                        2,
                        get_args
                    );

                    val_tag = tb_inst_load(
                        state->fun,
                        TB_TYPE_I32,
                        tb_inst_member_access(
                            state->fun,
                            arg2,
                            offsetof(vm_pair_t, val_tag)
                        ),
                        1,
                        false
                    );

                    val_val = tb_inst_load(
                        state->fun,
                        VM_TB_TYPE_VALUE,
                        tb_inst_member_access(
                            state->fun,
                            arg2,
                            offsetof(vm_pair_t, val_val)
                        ),
                        1,
                        false
                    );
                } else if (arg0tag == VM_TAG_CLOSURE) {
                    TB_Node *std_val_ref = tb_inst_array_access(
                        state->fun,
                        vm_tb_func_read_arg(state, regs, branch.args[0]),
                        vm_tb_func_read_arg(state, regs, branch.args[1]),
                        sizeof(vm_std_value_t)
                    );

                    val_tag = tb_inst_load(
                        state->fun,
                        TB_TYPE_I32,
                        tb_inst_member_access(
                            state->fun,
                            std_val_ref,
                            offsetof(vm_std_value_t, tag)
                        ),
                        1,
                        false
                    );

                    val_val = tb_inst_load(
                        state->fun,
                        VM_TB_TYPE_VALUE,
                        tb_inst_member_access(
                            state->fun,
                            std_val_ref,
                            offsetof(vm_std_value_t, value)
                        ),
                        1,
                        false
                    );
                } else {
                    vm_tb_func_report_error(state, "bad get");
                    break;
                }
            }

            size_t next_nargs = branch.targets[0]->nargs;

            // for (size_t argno = 0; argno < next_nargs; argno++) {
            //     if (branch.targets[0]->args[argno].reg == branch.out.reg) {
            //         goto branch_uses_reg;
            //     }
            // }

            {
                TB_Node *report_err = tb_inst_region(state->fun);
                TB_Node *after_err_check = tb_inst_region(state->fun);
                TB_Node *is_err = tb_inst_cmp_eq(state->fun, val_tag, tb_inst_uint(state->fun, TB_TYPE_I32, VM_TAG_ERROR));
                tb_inst_if(state->fun, is_err, report_err, after_err_check);
                tb_inst_set_control(state->fun, report_err);
                TB_Node *ret_args[2] = {
                    val_val,
                    val_tag,
                };
                tb_inst_ret(state->fun, 2, ret_args);
                tb_inst_set_control(state->fun, after_err_check);
            }

        // branch_uses_reg:;

            TB_PrototypeParam next_rets[2] = {
                {VM_TB_TYPE_VALUE},
                {TB_TYPE_I32},
            };

            TB_PrototypeParam *next_params = vm_malloc(sizeof(TB_PrototypeParam) * next_nargs);

            for (size_t argno = 0; argno < next_nargs; argno++) {
                next_params[argno] = (TB_PrototypeParam){VM_TB_TYPE_VALUE};
            }

            TB_FunctionPrototype *next_proto = tb_prototype_create(
                state->module,
                VM_TB_CC,
                next_nargs,
                next_params,
                2,
                next_rets,
                false
            );

            vm_free(next_params);

#if VM_USE_CACHE
            void **mem = vm_malloc(sizeof(void *) * VM_TAG_MAX);

            memset(mem, 0, sizeof(void *) * VM_TAG_MAX);

            TB_Node *func_ref = tb_inst_array_access(
                state->fun,
                vm_tb_ptr_name(state, "<rtargets>", mem),
                val_tag,
                sizeof(void *)
            );
            TB_Node *func = tb_inst_load(
                state->fun,
                TB_TYPE_PTR,
                func_ref,
                1,
                false
            );
            TB_Node *func_zero = tb_inst_cmp_eq(state->fun, func, vm_tb_ptr_name(state, "0", NULL));
            TB_Node *has_cache_region = tb_inst_region(state->fun);
            tb_inst_set_region_name(state->fun, has_cache_region, -1, "has_cache_typecheck");
            TB_Node *no_cache_region = tb_inst_region(state->fun);
            tb_inst_set_region_name(state->fun, no_cache_region, -1, "no_cache_typecheck");

            tb_inst_if(state->fun, func_zero, no_cache_region, has_cache_region);

            {
                tb_inst_set_control(state->fun, no_cache_region);

                TB_PrototypeParam comp_params[1] = {
                    {TB_TYPE_PTR},
                };

                TB_PrototypeParam comp_ret[1] = {
                    {TB_TYPE_PTR},
                };

                TB_FunctionPrototype *comp_proto = tb_prototype_create(state->module, VM_TB_CC, 1, comp_params, 1, comp_ret, false);

                for (size_t i = 1; i < VM_TAG_MAX; i++) {
                    branch.rtargets[i]->state = state;
                }

                TB_Node *comp_args[1];
                comp_args[0] = tb_inst_load(
                    state->fun,
                    TB_TYPE_PTR,
                    tb_inst_array_access(
                        state->fun,
                        vm_tb_ptr_name(state, "<rtargets>", &block->branch.rtargets[0]),
                        val_tag,
                        sizeof(vm_rblock_t *)
                    ),
                    1,
                    false
                );

                TB_MultiOutput new_func_multi = vm_tb_inst_call(
                    state,
                    comp_proto,
                    tb_inst_get_symbol_address(state->fun, state->vm_tb_rfunc_comp),
                    1,
                    comp_args
                );

                tb_inst_store(
                    state->fun,
                    TB_TYPE_PTR,
                    func_ref,
                    new_func_multi.single,
                    1,
                    false
                );

                TB_Node **next_args = vm_malloc(sizeof(TB_Node *) * next_nargs);
                for (size_t argno = 0; argno < next_nargs; argno++) {
                    vm_arg_t arg = branch.targets[0]->args[argno];
                    if (arg.type == VM_ARG_REG && arg.reg == branch.out.reg) {
                        next_args[argno] = val_val;
                    } else {
                        next_args[argno] = vm_tb_bitcast_to_value(state, vm_tb_func_read_arg(state, regs, arg));
                    }
                }

                if (VM_NO_TAILCALL) {
                    TB_MultiOutput out = vm_tb_inst_call(
                        state,
                        next_proto,
                        new_func_multi.single,
                        next_nargs,
                        next_args
                    );

                    tb_inst_ret(state->fun, 2, out.multiple);
                } else {
                    tb_inst_tailcall(
                        state->fun,
                        next_proto,
                        new_func_multi.single,
                        next_nargs,
                        next_args
                    );
                }
            }

            {
                tb_inst_set_control(state->fun, has_cache_region);

                TB_Node **next_args = vm_malloc(sizeof(TB_Node *) * next_nargs);
                for (size_t argno = 0; argno < next_nargs; argno++) {
                    vm_arg_t arg = branch.targets[0]->args[argno];
                    if (arg.type == VM_ARG_REG && arg.reg == branch.out.reg) {
                        next_args[argno] = val_val;
                    } else {
                        next_args[argno] = vm_tb_bitcast_to_value(state, vm_tb_func_read_arg(state, regs, arg));
                    }
                }

                if (next_nargs > 6 || VM_NO_TAILCALL) {
                    TB_MultiOutput out = vm_tb_inst_call(
                        state,
                        next_proto,
                        func,
                        next_nargs,
                        next_args
                    );

                    tb_inst_ret(state->fun, 2, out.multiple);
                } else {
                    tb_inst_tailcall(
                        state->fun,
                        next_proto,
                        func,
                        next_nargs,
                        next_args
                    );
                }
            }
#else
                TB_PrototypeParam comp_params[1] = {
                    {TB_TYPE_PTR},
                };

                TB_PrototypeParam comp_ret[1] = {
                    {TB_TYPE_PTR},
                };

                TB_FunctionPrototype *comp_proto = tb_prototype_create(state->module, VM_TB_CC, 1, comp_params, 1, comp_ret, false);

                for (size_t i = 1; i < VM_TAG_MAX; i++) {
                    branch.rtargets[i]->state = state;
                }

                TB_Node *comp_args[1];
                comp_args[0] = tb_inst_load(
                    state->fun,
                    TB_TYPE_PTR,
                    tb_inst_array_access(
                        state->fun,
                        vm_tb_ptr_name(state, "<rtargets>", &block->branch.rtargets[0]),
                        val_tag,
                        sizeof(vm_rblock_t *)
                    ),
                    1,
                    false
                );

                TB_MultiOutput new_func_multi = vm_tb_inst_call(
                    state,
                    comp_proto,
                    tb_inst_get_symbol_address(state->fun, state->vm_tb_rfunc_comp),
                    1,
                    comp_args
                );

                TB_Node **next_args = vm_malloc(sizeof(TB_Node *) * next_nargs);
                for (size_t argno = 0; argno < next_nargs; argno++) {
                    vm_arg_t arg = branch.targets[0]->args[argno];
                    if (arg.type == VM_ARG_REG && arg.reg == branch.out.reg) {
                        next_args[argno] = val_val;
                    } else {
                        next_args[argno] = vm_tb_bitcast_to_value(state, vm_tb_func_read_arg(state, regs, arg));
                    }
                }

                if (VM_NO_TAILCALL) {
                    TB_MultiOutput out = vm_tb_inst_call(
                        state,
                        next_proto,
                        new_func_multi.single,
                        next_nargs,
                        next_args
                    );

                    tb_inst_ret(state->fun, 2, out.multiple);
                } else {
                    tb_inst_tailcall(
                        state->fun,
                        next_proto,
                        new_func_multi.single,
                        next_nargs,
                        next_args
                    );
                }
#endif

            break;
        }

        default: {
            fprintf(stderr, "\nunhandled branch #%zu\n", (size_t)branch.op);
            exit(1);
        }
    }

    return;
}

void vm_tb_func_report_error(vm_tb_state_t *state, const char *str) {
    TB_Node *ret_vals[2];
    ret_vals[0] = vm_tb_bitcast_to_value(state, vm_tb_ptr_name(state, "<error>", (void *)str));
    ret_vals[1] = tb_inst_uint(state->fun, TB_TYPE_I32, VM_TAG_ERROR);
    tb_inst_ret(state->fun, 2, ret_vals);
}

void vm_tb_print(uint32_t tag, uint64_t ivalue) {
    vm_std_value_t val = (vm_std_value_t){
        .tag = tag,
        .value = *(vm_value_t *) &ivalue,
    };
    vm_io_buffer_t buf = {0};
    vm_io_debug(&buf, 0, "debug: ", val, NULL);
    fprintf(stdout, "%.*s", (int)buf.len, buf.buf);
}

void vm_tb_func_print_value(vm_tb_state_t *state, vm_tag_t tag, TB_Node *value) {
    TB_PrototypeParam proto_args[2] = {
        {TB_TYPE_I32},
        {VM_TB_TYPE_VALUE},
    };

    TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, 2, proto_args, 0, NULL, false);

    TB_Node *params[2] = {
        tb_inst_uint(state->fun, TB_TYPE_I32, (uint64_t)tag),
        vm_tb_bitcast_to_value(state, value),
    };

    vm_tb_inst_call(
        state,
        proto,
        tb_inst_get_symbol_address(state->fun, state->vm_tb_print),
        2,
        params
    );
}

void vm_tb_new_module(vm_tb_state_t *state) {
    if (state->module != NULL) {
#if !defined(EMSCRIPTEN)
        tb_module_destroy(state->module);
#endif
    }

    TB_Module *mod = tb_module_create_for_host(true);

    state->module = mod;

    state->vm_tb_rfunc_comp = tb_extern_create(mod, -1, "vm_tb_rfunc_comp", TB_EXTERNAL_SO_LOCAL);
    state->vm_table_new = tb_extern_create(mod, -1, "vm_table_new", TB_EXTERNAL_SO_LOCAL);
    state->vm_table_iset = tb_extern_create(mod, -1, "vm_table_iset", TB_EXTERNAL_SO_LOCAL);
    state->vm_table_get_pair = tb_extern_create(mod, -1, "vm_table_get_pair", TB_EXTERNAL_SO_LOCAL);
    state->vm_tb_print = tb_extern_create(mod, -1, "vm_tb_print", TB_EXTERNAL_SO_LOCAL);
    tb_symbol_bind_ptr(state->vm_tb_rfunc_comp, (void *)&vm_tb_rfunc_comp);
    tb_symbol_bind_ptr(state->vm_table_new, (void *)&vm_table_new);
    tb_symbol_bind_ptr(state->vm_table_iset, (void *)&vm_table_iset);
    tb_symbol_bind_ptr(state->vm_table_get_pair, (void *)&vm_table_get_pair);
    tb_symbol_bind_ptr(state->vm_tb_print, (void *)&vm_tb_print);

    state->arena = tb_arena_create(1 << 16);
    state->jit = tb_jit_begin(state->module, 1 << 16);

    // on windows we don't have access to multiple returns from C so we'll
    // just make a dumb caller for such a pattern
#if defined(_WIN32)
    TB_PrototypeParam call_proto_rets[2] = {{VM_TB_TYPE_VALUE}, {TB_TYPE_I32}};
    TB_FunctionPrototype *call_proto = tb_prototype_create(state->module, VM_TB_CC, 0, NULL, 2, call_proto_rets, false);

    TB_PrototypeParam proto_params[2] = {{TB_TYPE_PTR}, {TB_TYPE_PTR}};
    TB_Function *fun = tb_function_create(state->module, -1, "caller", TB_LINKAGE_PUBLIC);
    TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, 2, proto_params, 0, NULL, false);
    tb_function_set_prototype(fun, -1, proto, NULL);

    // tb_inst_debugbreak(fun);
    TB_MultiOutput out = tb_inst_call(fun, call_proto, tb_inst_param(fun, 1), 0, NULL);

    // store into struct
    TB_Node *dst = tb_inst_param(fun, 0);
    tb_inst_store(fun, VM_TB_TYPE_VALUE, dst, out.multiple[0], _Alignof(vm_value_t), false);
    TB_Node *dst2 = tb_inst_member_access(fun, dst, offsetof(vm_std_value_t, tag));
    tb_inst_store(fun, TB_TYPE_I32, dst2, out.multiple[1], _Alignof(uint32_t), false);

    tb_inst_ret(fun, 0, NULL);

    // compile it
    TB_FeatureSet features = (TB_FeatureSet){0};
    TB_Passes *passes = tb_pass_enter(fun, tb_function_get_arena(fun));
    tb_pass_codegen(passes, state->arena, &features, false);
    tb_pass_exit(passes);

    state->vm_caller = tb_jit_place_function(state->jit, fun);
    tb_arena_clear(state->arena);
}
#endif
}

void vm_tb_rblock_del(vm_rblock_t *rblock);

void *vm_tb_rfunc_comp(vm_rblock_t *rblock) {
    void *cache = rblock->code;
    if (cache != NULL) {
        return cache;
    }

    vm_tb_state_t *state = rblock->state;

    vm_tb_new_module(state);
    
    if (state->config->use_ver_count >= VM_USE_VERSION_COUNT_GLOBAL) {
        vm_table_t *vm_tab = vm_table_lookup(state->std, (vm_value_t){.str = "vm"}, VM_TAG_STR)->val_val.table;
        vm_table_t *vm_ver_tab = vm_table_lookup(vm_tab, (vm_value_t){.str = "version"}, VM_TAG_STR)->val_val.table;
        vm_pair_t *global_pair = vm_table_lookup(vm_ver_tab, (vm_value_t){.str = "global"}, VM_TAG_STR);
        int64_t global_count;
        if (global_pair != NULL) {
            vm_std_value_t global_val = (vm_std_value_t){
                .tag = global_pair->val_tag,
                .value = global_pair->val_val,
            };
            global_count = vm_value_to_i64(global_val);
        } else {
            global_count = 0;
        }
        global_count += 1;
        vm_tag_t tag;
        vm_value_t res;
        switch (state->config->use_num) {
            case VM_USE_NUM_I8: {
                tag = VM_TAG_I8;
                res.i8 = global_count;
                break;
            }
            case VM_USE_NUM_I16: {
                tag = VM_TAG_I16;
                res.i16 = global_count;
                break;
            }
            case VM_USE_NUM_I32: {
                tag = VM_TAG_I32;
                res.i32 = global_count;
                break;
            }
            case VM_USE_NUM_I64: {
                tag = VM_TAG_I64;
                res.i64 = global_count;
                break;
            }
            case VM_USE_NUM_F32: {
                tag = VM_TAG_F32;
                res.f32 = global_count;
                break;
            }
            case VM_USE_NUM_F64: {
                tag = VM_TAG_F64;
                res.f64 = global_count;
                break;
            }
        }
        vm_table_set(vm_ver_tab, (vm_value_t){.str = "global"}, res, VM_TAG_STR, tag);
    }

    vm_block_t *block = vm_rblock_version(state->blocks, rblock);

    static size_t comps = 0;
    char name[64];
    if (block) {
        snprintf(name, 23, "block_%zi_%zu", block->id, comps++);
    } else {
        snprintf(name, 23, "block_unk_%zu", comps++);
    }
    state->fun = tb_function_create(state->module, -1, name, TB_LINKAGE_PUBLIC);
    
    if (block == NULL) {
        TB_PrototypeParam proto_rets[2] = {
            {VM_TB_TYPE_VALUE},
            {TB_TYPE_I32},
        };

        TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, 0, NULL, 2, proto_rets, false);

        tb_function_set_prototype(
            state->fun,
            -1,
            proto,
            NULL
        );

        vm_tb_func_report_error(state, "internal: block == NULL");
    } else {
        TB_PrototypeParam *proto_args = vm_malloc(sizeof(TB_PrototypeParam) * block->nargs);

        for (size_t arg = 0; arg < block->nargs; arg++) {
            proto_args[arg] = (TB_PrototypeParam){
                VM_TB_TYPE_VALUE,
            };
        }

        TB_PrototypeParam proto_rets[2] = {
            {VM_TB_TYPE_VALUE},
            {TB_TYPE_I32},
        };

        TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, block->nargs, proto_args, 2, proto_rets, false);

        vm_free(proto_args);

        tb_function_set_prototype(
            state->fun,
            -1,
            proto,
            NULL
        );

        if (block != NULL) {
            for (size_t i = 0; i < block->nargs; i++) {
                vm_arg_t arg = block->args[i];
                if (arg.type == VM_ARG_REG && rblock->regs->tags[arg.reg] == VM_TAG_ERROR) {
                    TB_Node *rets[2];
                    rets[0] = tb_inst_param(state->fun, i);
                    rets[1] = tb_inst_uint(state->fun, TB_TYPE_I32, VM_TAG_ERROR);
                    tb_inst_ret(state->fun, 2, rets);
                    block = NULL;
                }
            }
        }

        if (block != NULL) {
            TB_Node **regs = vm_malloc(sizeof(TB_Node *) * block->nregs);

            for (size_t i = 0; i < block->nregs; i++) {
                regs[i] = tb_inst_local(state->fun, 8, 8);
                // tb_inst_store(state->fun, VM_TB_TYPE_VALUE, regs[i], tb_inst_uint(state->fun, VM_TB_TYPE_VALUE, 0), 8, false);
                // tb_inst_store(state->fun, VM_TB_TYPE_VALUE, regs[i], tb_inst_load(state->fun, VM_TB_TYPE_VALUE, regs[i], 8, false), 8, false);
            }

            for (size_t i = 0; i < block->nargs; i++) {
                tb_inst_store(
                    state->fun,
                    VM_TB_TYPE_VALUE,
                    regs[block->args[i].reg],
                    tb_inst_param(state->fun, i),
                    8,
                    false
                );
                if (state->config->dump_args) {
                    vm_tb_func_print_value(
                        state,
                        rblock->regs->tags[block->args[i].reg],
                        tb_inst_param(state->fun, i)
                    );
                }
            }
            TB_Node *main = tb_inst_region(state->fun);
            tb_inst_set_region_name(state->fun, main, -1, "entry");
            tb_inst_goto(state->fun, main);
            tb_inst_set_control(state->fun, main);

            vm_tb_func_reset_pass(block);
            vm_tb_func_body_once_as(state, regs, block);
            vm_tb_func_reset_pass(block);

            vm_free(regs);
        }
    }

    TB_Arena *parena = state->arena;
    TB_Passes *passes = tb_pass_enter(state->fun, tb_function_get_arena(state->fun));
#if VM_USE_DUMP
    if (state->config->dump_tb) {
        fprintf(stdout, "\n--- tb ---\n");
        tb_pass_print(passes);
    }
    if (state->config->dump_tb_dot) {
        fprintf(stdout, "\n--- tb dot ---\n");
        tb_pass_print_dot(passes, tb_default_print_callback, stdout);
    }
    if (state->config->use_tb_opt) {
        tb_pass_optimize(passes);
        if (state->config->dump_tb_opt) {
            fprintf(stdout, "\n--- opt tb ---\n");
            tb_pass_print(passes);
        }
        if (state->config->dump_tb_dot) {
            fprintf(stdout, "\n--- opt dot ---\n");
            tb_pass_print_dot(passes, tb_default_print_callback, stdout);
        }
    }
#endif
#if defined(EMSCRIPTEN)
    if (state->config->target == VM_TARGET_TB_EMCC) {
        const char *cs[] = {
            tb_pass_c_prelude(state->module),
            tb_pass_c_fmt(passes),
            NULL,
        };
        void *code = vm_cache_comp("emcc", cs, name);
        rblock->code = code;
        return code;
    // } else if (state->config->target == VM_TARGET_TB_JS) {
        // const char *js1 = tb_pass_js_prelude(state->module);
        // const char *js2 = tb_pass_js_fmt(passes);
        // size_t len1 = strlen(js1);
        // size_t len2 = strlen(js2);
        // char *buf = vm_malloc(len1 + len2 + 1);
        // memcpy(buf, js1, len1);
        // memcpy(buf + len1, js2, len2);
        // buf[len1 + len2] = '\0';
        // rblock->code = buf;
        // return buf;
    } else {
        __builtin_trap();
    }
#else
    if (state->config->target == VM_TARGET_TB_TCC) {
        const char *c_header = tb_pass_c_prelude(state->module);
        const char *c_src = tb_pass_c_fmt(passes);
        int c_header_size = strlen(c_header);
        int c_src_size = strlen(c_src);
        char *buf = vm_malloc(c_header_size + c_src_size + 1);
        strcpy(buf, c_header);
        strcpy(buf + c_header_size, c_src);
        tb_pass_exit(passes);
        if (state->config->dump_asm) {
            printf("\n--- c ---\n%s", buf);
        }
        TCCState *state = tcc_new();
        tcc_set_error_func(state, 0, vm_tcc_error_func);
        tcc_set_options(state, "-nostdlib");
        tcc_set_output_type(state, TCC_OUTPUT_MEMORY);
        tcc_compile_string(state, buf);
        tcc_relocate(state, TCC_RELOCATE_AUTO);
        vm_free(buf);
        void *code = tcc_get_symbol(state, name);
        rblock->code = code;
        return code;
    } else if (state->config->target == VM_TARGET_TB_GCC) {
        const char *cs[] = {
            tb_pass_c_prelude(state->module),
            tb_pass_c_fmt(passes),
            NULL
        };
        void *code = vm_cache_comp("gcc", cs, name);
        rblock->code = code;
        return code;
    } else if (state->config->target == VM_TARGET_TB_CLANG) {
        const char *cs[] = {
            tb_pass_c_prelude(state->module),
            tb_pass_c_fmt(passes),
            NULL
        };
        void *code = vm_cache_comp("clang", cs, name);
        rblock->code = code;
        return code;
    } else if (state->config->target == VM_TARGET_TB_CC) {
        const char *cs[] = {
            tb_pass_c_prelude(state->module),
            tb_pass_c_fmt(passes),
            NULL
        };
        void *code = vm_cache_comp("cc", cs, name);
        rblock->code = code;
        return code;
    } else if (state->config->target == VM_TARGET_TB) {
        TB_FeatureSet features = (TB_FeatureSet){0};
#if VM_USE_DUMP
        if (state->config->dump_asm) {
            TB_FunctionOutput *out = tb_pass_codegen(passes, state->arena, &features, true);
            fprintf(stdout, "\n--- x86asm ---\n");
            tb_output_print_asm(out, stdout);
        } else {
            tb_pass_codegen(passes, state->arena, &features, false);
        }
#else
        tb_pass_codegen(passes, state->arena, &features, false);
#endif
        tb_pass_exit(passes);

        void *code = tb_jit_place_function(state->jit, state->fun);
        tb_arena_clear(state->arena);

        rblock->code = code;
        rblock->jit = state->jit;
        rblock->del = &vm_tb_rblock_del;

        return code;
    } else {
        __builtin_trap();
    }
#endif
}

#if !defined(EMSCRIPTEN)
void vm_tcc_error_func(void *user, const char *msg) {
    printf("%s\n", msg);
    exit(1);
}
#endif

void vm_tb_rblock_del(vm_rblock_t *rblock) {
    TB_JIT *jit = rblock->jit;
}

void *vm_tb_full_comp(vm_tb_state_t *state, vm_block_t *block) {
    vm_tags_t *regs = vm_rblock_regs_empty(block->nregs);
    vm_rblock_t *rblock = vm_rblock_new(block, regs);
    rblock->state = state;
    return vm_tb_rfunc_comp(rblock);
}

typedef vm_std_value_t VM_CDECL vm_tb_func_t();

vm_std_value_t vm_tb_run_main(vm_config_t *config, vm_block_t *entry, vm_blocks_t *blocks, vm_table_t *std) {
    vm_std_value_t val = vm_tb_run_repl(config, entry, blocks, std);
    if (val.tag == VM_TAG_ERROR) {
        fprintf(stderr, "error: %s\n", val.value.str);
    }
    return val;
}

vm_std_value_t vm_tb_run_repl(vm_config_t *config, vm_block_t *entry, vm_blocks_t *blocks, vm_table_t *std) {
    vm_tb_state_t *state = vm_malloc(sizeof(vm_tb_state_t));
    state->std = std;
    state->config = config;
    state->blocks = blocks;
    state->module = NULL;

    vm_tb_func_t *fn = (vm_tb_func_t *)vm_tb_full_comp(state, entry);

#if defined(_WIN32)
    vm_std_value_t value;
    state->vm_caller(&value, fn);
#else
    vm_std_value_t value = fn();
#endif

    for (size_t i = 0; i < blocks->len; i++) {
        vm_block_t *block = blocks->blocks[i];
        // for (size_t j = 0; j < block->cache.len; j++) {
        //     vm_rblock_reset(block->cache.keys[j]);
        //     vm_free_block_sub(block->cache.values[j]);
        // }
        block->cache.len = 0;
    }

    return *(vm_std_value_t*) &value;
}
