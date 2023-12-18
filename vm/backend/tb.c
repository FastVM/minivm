
#include "./tb.h"

#include "../../vendor/tb/include/tb.h"
#include "../ir/check.h"
#include "../ir/rblock.h"

#define VM_TB_CC TB_CDECL
#define VM_TB_TYPE_VALUE TB_TYPE_I64

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

size_t vm_tb_ptr_len = 0;
void **vm_tb_ptr_globals = NULL;
size_t vm_tb_ptr_alloc = 0;

TB_Node *vm_tb_ptr_name(vm_tb_state_t *state, const char *name, void *value) {
    if (value != NULL) {
        if (vm_tb_ptr_len + 1 >= vm_tb_ptr_alloc) {
            vm_tb_ptr_alloc = (vm_tb_ptr_len + 1) * 2;
            vm_tb_ptr_globals = vm_realloc(vm_tb_ptr_globals, sizeof(void *) * vm_tb_ptr_alloc);
        }
        vm_tb_ptr_globals[vm_tb_ptr_len++] = value;
    }
    return tb_inst_uint(state->fun, TB_TYPE_PTR, (uint64_t)value);
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
            // vm_io_format_tag(stderr, tag);
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
                    return tb_inst_string(state->fun, strlen(arg.lit.value.str) + 1, arg.lit.value.str);
                }
                case VM_TAG_FFI: {
                    return vm_tb_ptr_name(state, "<ffi>", arg.lit.value.ffi);
                }
                default: {
                    __builtin_trap();
                }
            }
        }
        case VM_ARG_NONE: {
            return vm_tb_ptr_name(state, "0", NULL);
        }
        case VM_ARG_REG: {
            return tb_inst_bitcast(
                state->fun,
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
            // printf("ref.func %zu\n", arg.func->id);
            return tb_inst_uint(state->fun, TB_TYPE_I32, (uint64_t)arg.func->id);
        }
        default: {
            // vm_io_format_arg(stderr, arg);
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

void vm_tb_func_reset_pass(vm_block_t *block) {
    if (block->pass == NULL) {
        return;
    }
    block->pass = NULL;
    vm_branch_t branch = block->branch;
    switch (branch.op) {
        case VM_BOP_JUMP: {
            vm_tb_func_reset_pass(branch.targets[0]);
            break;
        }
        case VM_BOP_BB:
        case VM_BOP_BLT: {
            vm_tb_func_reset_pass(branch.targets[0]);
            vm_tb_func_reset_pass(branch.targets[1]);
            break;
        }
        case VM_BOP_BEQ: {
            if (vm_arg_to_tag(branch.args[0]) != vm_arg_to_tag(branch.args[1])) {
                vm_tb_func_reset_pass(branch.targets[1]);
            } else if (branch.tag == VM_TAG_NIL) {
                vm_tb_func_reset_pass(branch.targets[0]);
            } else {
                vm_tb_func_reset_pass(branch.targets[0]);
                vm_tb_func_reset_pass(branch.targets[1]);
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
        tb_inst_bitcast(
            state->fun,
            value,
            VM_TB_TYPE_VALUE
        ),
        8,
        false
    );
}

TB_MultiOutput vm_tb_inst_call(vm_tb_state_t *state, TB_FunctionPrototype* proto, TB_Node* target, size_t param_count, TB_Node** params) {
    // TB_Node *local = tb_inst_local(state->fun, sizeof(void *), 1);
    // tb_inst_store(state->fun, TB_TYPE_PTR, local, target, 1, false);
    // target = tb_inst_load(state->fun, TB_TYPE_PTR, local, 1, false);
    return tb_inst_call(state->fun, proto, target, param_count, params);
}

TB_Node *vm_tb_func_body_once(vm_tb_state_t *state, TB_Node **regs, vm_block_t *block) {
    if (block->pass != NULL) {
        return block->pass;
    }
    TB_Node *old_ctrl = tb_inst_get_control(state->fun);

    TB_Node *ret = tb_inst_region(state->fun);

    char name[24];
    snprintf(name, 23, "vm.%zi", block->id);

    tb_inst_set_region_name(state->fun, ret, -1, name);

    tb_inst_set_control(state->fun, ret);

    {
        const char *block_err = vm_check_block(block);
        if (block_err != NULL) {
            vm_tb_func_report_error(state, block_err);
            tb_inst_set_control(state->fun, old_ctrl);
            return ret;
        }
    }

    block->pass = ret;

#if VM_USE_DUMP
    if (state->config->dump_ver) {
        vm_io_buffer_t buf = {0};
        vm_io_format_block(&buf, block);
        fprintf(stdout, "\n--- vmir ---\n%.*s", (int) buf.len, buf.buf);
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
                    tb_inst_bitcast(state->fun, vm_tb_func_read_arg(state, regs, instr.args[1]), VM_TB_TYPE_VALUE),
                    tb_inst_bitcast(state->fun, vm_tb_func_read_arg(state, regs, instr.args[2]), VM_TB_TYPE_VALUE),
                    tb_inst_uint(state->fun, TB_TYPE_I32, key_tag),
                    tb_inst_uint(state->fun, TB_TYPE_I32, val_tag),
                };
                vm_tb_inst_call(
                    state,
                    proto,
                    tb_inst_get_symbol_address(state->fun, state->vm_table_set),
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
                // vm_io_format_instr(stderr, instr);
                fprintf(stderr, "\nunhandled instruction #%zu\n", (size_t) instr.op);
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

            ret[0] = tb_inst_bitcast(state->fun, vm_tb_func_read_arg(state, regs, branch.args[0]), VM_TB_TYPE_VALUE);

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
                                tb_inst_bitcast(state->fun, vm_tb_func_read_arg(state, regs, arg), VM_TB_TYPE_VALUE),
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

                    // tb_inst_store(
                    //     state->fun,
                    //     TB_TYPE_PTR,
                    //     tb_inst_member_access(state->fun, end_head, offsetof(vm_std_value_t, value)),
                    //     vm_tb_ptr_name(state, "<data>", 0),
                    //     8,
                    //     false
                    // );
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
                    // tb_inst_debugbreak(state->fun);
                    
                    TB_Node *closure = vm_tb_func_read_arg(state, regs, branch.args[0]);
                    TB_Node *block_num = tb_inst_load(
                        state->fun,
                        vm_tag_to_tb_type(VM_TAG_FUN),
                        tb_inst_member_access(
                            state->fun,
                            closure,
                            offsetof(vm_std_value_t, value)
                        ),
                        1,
                        false
                    );


                    void **cache = vm_malloc(sizeof(void *) * state->blocks->len);
                    memset(cache, 0, sizeof(void *) * state->blocks->len);

                    TB_Node *has_cache = tb_inst_region(state->fun);
                    tb_inst_set_region_name(state->fun, has_cache, -1, "has_cache");
                    TB_Node *no_cache = tb_inst_region(state->fun);
                    tb_inst_set_region_name(state->fun, no_cache, -1, "no_cache");
                    TB_Node *after = tb_inst_region(state->fun);
                    tb_inst_set_region_name(state->fun, after, -1, "after_cache");

                    TB_Node *global_ptr = tb_inst_array_access(
                        state->fun,
                        vm_tb_ptr_name(state, "<code_cache>", cache),
                        block_num,
                        sizeof(void *)
                    );
                    TB_Node *global = tb_inst_load(state->fun, TB_TYPE_PTR, global_ptr, 1, false);

                    val_val = tb_inst_local(state->fun, 8, 8);
                    val_tag = tb_inst_local(state->fun, 4, 4);

                    size_t nargs = 1;
                    for (size_t arg = 1; branch.args[arg].type != VM_ARG_NONE; arg++) {
                        nargs += 1;
                    }

                    TB_PrototypeParam *call_proto_params = vm_malloc(sizeof(TB_PrototypeParam) * nargs);

                    call_proto_params[0] = (TB_PrototypeParam){TB_TYPE_PTR};
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

                        TB_PrototypeParam comp_params[2] = {
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
                        call_args[0] = closure;
                        for (size_t arg = 1; branch.args[arg].type != VM_ARG_NONE; arg++) {
                            call_args[arg] = tb_inst_bitcast(state->fun, vm_tb_func_read_arg(state, regs, branch.args[arg]), VM_TB_TYPE_VALUE);
                        }

                        TB_Node **got = vm_tb_inst_call(
                                            state,
                                            call_proto,
                                            call_func,
                                            nargs,
                                            call_args
                        )
                                            .multiple;

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
                        call_args[0] = closure;
                        for (size_t arg = 1; branch.args[arg].type != VM_ARG_NONE; arg++) {
                            call_args[arg] = tb_inst_bitcast(state->fun, vm_tb_func_read_arg(state, regs, branch.args[arg]), VM_TB_TYPE_VALUE);
                        }
                        
                        TB_Node **got = vm_tb_inst_call(
                                            state,
                                            call_proto,
                                            global,
                                            nargs,
                                            call_args
                        )
                                            .multiple;

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
                        tb_inst_bitcast(state->fun, vm_tb_func_read_arg(state, regs, branch.args[1]), VM_TB_TYPE_VALUE),
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
                        TB_TYPE_PTR,
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
                        TB_TYPE_PTR,
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

            for (size_t argno = 0; argno < next_nargs; argno++) {
                if (branch.targets[0]->args[argno].reg == branch.out.reg) {
                    goto branch_uses_reg;
                }
            }

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

        branch_uses_reg:;
            
            TB_PrototypeParam next_rets[2] = {
                {TB_TYPE_PTR},
                {TB_TYPE_I32},
            };

            TB_PrototypeParam *next_params = vm_malloc(sizeof(TB_PrototypeParam) * next_nargs);

            for (size_t argno = 0; argno < next_nargs; argno++) {
                next_params[argno] = (TB_PrototypeParam){ TB_TYPE_PTR };
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
            TB_Node *func_nonzero = tb_inst_cmp_ne(state->fun, func, vm_tb_ptr_name(state, "0", NULL));
            TB_Node *has_cache_region = tb_inst_region(state->fun);
            tb_inst_set_region_name(state->fun, has_cache_region, -1, "has_cache_typecheck");
            TB_Node *no_cache_region = tb_inst_region(state->fun);
            tb_inst_set_region_name(state->fun, has_cache_region, -1, "no_cache_typecheck");

            tb_inst_if(state->fun, func_nonzero, has_cache_region, no_cache_region);
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
                        next_args[argno] = tb_inst_bitcast(state->fun, vm_tb_func_read_arg(state, regs, arg), VM_TB_TYPE_VALUE);
                    }
                }
                
                if (next_nargs > 6 || VM_NO_TAILCALL) {
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
                        next_args[argno] = tb_inst_bitcast(state->fun, vm_tb_func_read_arg(state, regs, arg), VM_TB_TYPE_VALUE);
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

            break;
        }

        default: {
            fprintf(stderr, "\nunhandled branch #%zu\n", (size_t) branch.op);
            exit(1);
        }
    }

    tb_inst_set_control(state->fun, old_ctrl);

    return ret;
}

void vm_tb_func_report_error(vm_tb_state_t *state, const char *str) {
    TB_Node *ret_vals[2];
    ret_vals[0] = vm_tb_ptr_name(state, "<error>", (void *)str);
    ret_vals[1] = tb_inst_uint(state->fun, TB_TYPE_I32, VM_TAG_ERROR);
    tb_inst_ret(state->fun, 2, ret_vals);
}

void vm_tb_print(uint32_t tag, void *value) {
    vm_std_value_t val = (vm_std_value_t){
        .tag = tag,
    };
    val.value = *(vm_value_t *)value;
    // switch (tag) {
    //     case VM_TAG_I8: {
    //         val.value.i8 = *(int8_t *)value;
    //         break;
    //     }
    //     case VM_TAG_I16: {
    //         val.value.i16 = *(int16_t *)value;
    //         break;
    //     }
    //     case VM_TAG_I32: {
    //         val.value.i32 = *(int32_t *)value;
    //         break;
    //     }
    //     case VM_TAG_I64: {
    //         val.value.i64 = *(int64_t *)value;
    //         break;
    //     }
    //     case VM_TAG_F32: {
    //         val.value.f32 = *(float *)value;
    //         break;
    //     }
    //     case VM_TAG_F64: {
    //         val.value.f64 = *(double *)value;
    //         break;
    //     }
    //     case VM_TAG_STR: {
    //         val.value.str = *(const char **)value;
    //         break;
    //     }
    //     case VM_TAG_CLOSURE: {
    //         val.value.closure = *(vm_std_value_t **)value;
    //         break;
    //     }
    //     case VM_TAG_TAB: {
    //         val.value.table = *(vm_table_t **)value;
    //         break;
    //     }
    //     case VM_TAG_FFI: {
    //         val.value.ffi = *(void (*)(vm_std_closure_t *closure, vm_std_value_t *))value;
    //         break;
    //     }
    //     default: {
    //         printf("bad tag: %zu\n", (size_t)tag);
    //         exit(1);
    //     }
    // }
    vm_io_buffer_t buf = {0};
    vm_io_debug(&buf, 0, "debug: ", val, NULL);
    fprintf(stdout, "%.*s", (int)buf.len, buf.buf);
}

void vm_tb_func_print_value(vm_tb_state_t *state, vm_tag_t tag, TB_Node *value) {
    TB_PrototypeParam proto_args[2] = {
        {TB_TYPE_I32},
        {TB_TYPE_PTR},
    };

    TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, 2, proto_args, 0, NULL, false);

    TB_Node *local = tb_inst_local(state->fun, 8, 8);

    tb_inst_store(
        state->fun,
        VM_TB_TYPE_VALUE,
        local,
        tb_inst_bitcast(state->fun, value, VM_TB_TYPE_VALUE),
        8,
        false
    );

    TB_Node *params[2] = {
        tb_inst_uint(state->fun, TB_TYPE_I32, (uint64_t)tag),
        local,
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
    TB_FeatureSet features = (TB_FeatureSet){0};

    TB_Module *mod = tb_module_create_for_host(&features, true);

    state->module = mod;

    state->vm_tb_rfunc_comp = tb_extern_create(mod, -1, "vm_tb_rfunc_comp", TB_EXTERNAL_SO_LOCAL);
    state->vm_table_new = tb_extern_create(mod, -1, "vm_table_new", TB_EXTERNAL_SO_LOCAL);
    state->vm_table_set = tb_extern_create(mod, -1, "vm_table_set", TB_EXTERNAL_SO_LOCAL);
    state->vm_table_get_pair = tb_extern_create(mod, -1, "vm_table_get_pair", TB_EXTERNAL_SO_LOCAL);
    state->vm_tb_print = tb_extern_create(mod, -1, "vm_tb_print", TB_EXTERNAL_SO_LOCAL);
    tb_symbol_bind_ptr(state->vm_tb_rfunc_comp, (void *)&vm_tb_rfunc_comp);
    tb_symbol_bind_ptr(state->vm_table_new, (void *)&vm_table_new);
    tb_symbol_bind_ptr(state->vm_table_set, (void *)&vm_table_set);
    tb_symbol_bind_ptr(state->vm_table_get_pair, (void *)&vm_table_get_pair);
    tb_symbol_bind_ptr(state->vm_tb_print, (void *)&vm_tb_print);
}

vm_block_t *vm_tb_handle_upvalues(vm_block_t *input) {
    vm_block_t *output = vm_malloc(sizeof(vm_block_t));
    memcpy(output, input, sizeof(vm_block_t));
    return output;
}

void *vm_tb_rfunc_comp(vm_rblock_t *rblock) {
    void *cache = rblock->jit;
    if (cache != NULL) {
        return cache;
    }

    vm_tb_state_t *state = rblock->state;

    vm_block_t *block_pre = vm_rblock_version(state->blocks, rblock);
    vm_block_t *block = vm_tb_handle_upvalues(block_pre);
    state->fun  = tb_function_create(state->module, -1, "block", TB_LINKAGE_PRIVATE);

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
                    vm_tb_func_print_value(state, rblock->regs->tags[block->args[i].reg], regs[block->args[i].reg]);
                }
            }

            vm_tb_func_reset_pass(block);
            TB_Node *main = vm_tb_func_body_once(state, regs, block);
            vm_tb_func_reset_pass(block);
            tb_inst_goto(state->fun, main);
        }
    }

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
#if VM_USE_DUMP
    if (state->config->dump_x86) {
        TB_FunctionOutput *out = tb_pass_codegen(passes, true);
        fprintf(stdout, "\n--- x86asm ---\n");
        tb_output_print_asm(out, stdout);
    } else {
        tb_pass_codegen(passes, false);
    }
#else
    tb_pass_codegen(passes, false);
#endif
    tb_pass_exit(passes);

    TB_JIT *jit = tb_jit_begin(state->module, 1 << 16);
    void *ret = tb_jit_place_function(jit, state->fun);

    rblock->jit = ret;

    // printf("%zi -> %p\n", block->id, rblock->jit);

    // printf("code buf: %p\n", ret);

    return ret;
}

void *vm_tb_full_comp(vm_tb_state_t *state, vm_block_t *block) {
    vm_tags_t *regs = vm_rblock_regs_empty(block->nregs);
    vm_rblock_t *rblock = vm_rblock_new(block, regs);
    rblock->state = state;
    return vm_tb_rfunc_comp(rblock);
}

typedef vm_std_value_t VM_CDECL vm_tb_func_t(void);

vm_std_value_t vm_tb_run_main(vm_config_t *config, vm_block_t *entry, vm_blocks_t *blocks, vm_table_t *std) {
    vm_std_value_t val = vm_tb_run_repl(config, entry, blocks, std);
    if (val.tag == VM_TAG_ERROR) {
        printf("error: %s\n", val.value.str);
    }
    return val;
}

vm_std_value_t vm_tb_run_repl(vm_config_t *config, vm_block_t *entry, vm_blocks_t *blocks, vm_table_t *std) {
    vm_tb_state_t *state = vm_malloc(sizeof(vm_tb_state_t));
    state->std = std;
    state->config = config;
    state->blocks = blocks;

    vm_tb_new_module(state);

    vm_tb_func_t *fn = (vm_tb_func_t *)vm_tb_full_comp(state, entry);
    return fn();
}
