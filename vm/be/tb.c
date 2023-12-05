
#include "./tb.h"

#include "../../cuik/tb/include/tb.h"
#include "../check.h"
#include "../rblock.h"

#define VM_TB_CC TB_CDECL
// #define VM_TB_CC TB_STDCALL

void vm_tb_func_print_value(vm_tb_state_t *mod, TB_Function *fun, vm_tag_t tag, TB_Node *value);
TB_Node *vm_tb_func_body_once(vm_tb_state_t *state, TB_Function *fun, TB_Node **regs, vm_block_t *block);
void vm_tb_func_report_error(vm_tb_state_t *state, TB_Function *fun, const char *str);

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

TB_Node *vm_tb_ptr_name(TB_Module *mod, TB_Function *fun, const char *name, void *value) {
    // char buf[24];
    // snprintf(buf, 23, "<ptr: %p>", value);
    // TB_Symbol *ext = (TB_Symbol *) tb_extern_create(mod, -1, buf, TB_EXTERNAL_SO_LOCAL);
    // tb_symbol_bind_ptr(ext, value);
    // printf("%p\n", value);
    // // return tb_inst_load(fun, TB_TYPE_PTR, tb_inst_get_symbol_address(fun, ext), 1, false);
    // return tb_inst_get_symbol_address(fun, ext);
    // printf("%s: %p\n", name, value);
    return tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t)value);
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
            vm_print_tag(stderr, tag);
            fprintf(stderr, "\n ^ unhandled tag #%zu\n", (size_t)tag);
            __builtin_trap();
        }
    }
}

TB_Node *vm_tb_func_read_arg(TB_Function *fun, TB_Node **regs, vm_arg_t arg) {
    switch (arg.type) {
        case VM_ARG_LIT: {
            switch (arg.lit.tag) {
                case VM_TAG_NIL: {
                    return tb_inst_uint(fun, TB_TYPE_PTR, 0);
                }
                case VM_TAG_BOOL: {
                    return tb_inst_bool(fun, arg.lit.value.b);
                }
                case VM_TAG_I8: {
                    return tb_inst_sint(fun, TB_TYPE_I8, arg.lit.value.i8);
                }
                case VM_TAG_I16: {
                    return tb_inst_sint(fun, TB_TYPE_I16, arg.lit.value.i16);
                }
                case VM_TAG_I32: {
                    return tb_inst_sint(fun, TB_TYPE_I32, arg.lit.value.i32);
                }
                case VM_TAG_I64: {
                    return tb_inst_sint(fun, TB_TYPE_I64, arg.lit.value.i64);
                }
                case VM_TAG_F32: {
                    return tb_inst_float32(fun, arg.lit.value.f32);
                }
                case VM_TAG_F64: {
                    return tb_inst_float64(fun, arg.lit.value.f64);
                }
                case VM_TAG_STR: {
                    return tb_inst_string(fun, strlen(arg.lit.value.str) + 1, arg.lit.value.str);
                }
                case VM_TAG_FFI: {
                    return tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t)arg.lit.value.ffi);
                }
                default: {
                    __builtin_trap();
                }
            }
        }
        case VM_ARG_NONE: {
            return tb_inst_uint(fun, TB_TYPE_PTR, 0);
        }
        case VM_ARG_REG: {
            return tb_inst_load(
                fun,
                vm_tag_to_tb_type(arg.reg_tag),
                regs[arg.reg],
                8,
                false
            );
        }
        case VM_ARG_FUN: {
            return tb_inst_uint(fun, TB_TYPE_I32, (uint64_t)arg.func->id);
        }
        default: {
            vm_print_arg(stderr, arg);
            fprintf(stderr, "\n ^ unhandled arg (type#%zu)\n", (size_t)arg.type);
            __builtin_trap();
        }
    }
}

void vm_tb_func_reset_pass(vm_block_t *block) {
    if (block->pass == NULL) {
        return;
    }
    block->pass = NULL;
    switch (block->branch.op) {
        case VM_BOP_JUMP: {
            vm_tb_func_reset_pass(block->branch.targets[0]);
            break;
        }
        case VM_BOP_BB:
        case VM_BOP_BLT:
        case VM_BOP_BEQ: {
            vm_tb_func_reset_pass(block->branch.targets[0]);
            vm_tb_func_reset_pass(block->branch.targets[1]);
            break;
        }
    }
}

TB_Node *vm_tb_func_body_once(vm_tb_state_t *state, TB_Function *fun, TB_Node **regs, vm_block_t *block) {
    if (block->pass != NULL) {
        return block->pass;
    }
    TB_Node *old_ctrl = tb_inst_get_control(fun);

    TB_Node *ret = tb_inst_region(fun);

    tb_inst_set_control(fun, ret);

    {
        const char *block_err = vm_check_block(block);
        if (block_err != NULL) {
            vm_tb_func_report_error(state, fun, block_err);
            tb_inst_set_control(fun, old_ctrl);
            return ret;
        }
    }

    block->pass = ret;

#if VM_USE_DUMP
    if (state->config->dump_ver) {
        fprintf(stdout, "\n--- vmir ---\n");
        vm_print_block(stdout, block);
    }
#endif

    for (size_t n = 0; n < block->len; n++) {
        vm_instr_t instr = block->instrs[n];
        switch (instr.op) {
            case VM_IOP_MOVE: {
                tb_inst_store(
                    fun,
                    vm_tag_to_tb_type(instr.tag),
                    regs[instr.out.reg],
                    vm_tb_func_read_arg(fun, regs, instr.args[0]),
                    8,
                    false
                );
                break;
            }
            case VM_IOP_ADD: {
                TB_Node *value = NULL;
                if (instr.tag == VM_TAG_F32 || instr.tag == VM_TAG_F64) {
                    value = tb_inst_fadd(
                        fun,
                        vm_tb_func_read_arg(fun, regs, instr.args[0]),
                        vm_tb_func_read_arg(fun, regs, instr.args[1])
                    );
                } else {
                    value = tb_inst_add(
                        fun,
                        vm_tb_func_read_arg(fun, regs, instr.args[0]),
                        vm_tb_func_read_arg(fun, regs, instr.args[1]),
                        TB_ARITHMATIC_NONE
                    );
                }
                tb_inst_store(
                    fun,
                    vm_tag_to_tb_type(instr.tag),
                    regs[instr.out.reg],
                    value,
                    8,
                    false
                );
                break;
            }
            case VM_IOP_SUB: {
                TB_Node *value = NULL;
                if (instr.tag == VM_TAG_F32 || instr.tag == VM_TAG_F64) {
                    value = tb_inst_fsub(
                        fun,
                        vm_tb_func_read_arg(fun, regs, instr.args[0]),
                        vm_tb_func_read_arg(fun, regs, instr.args[1])
                    );
                } else {
                    value = tb_inst_sub(
                        fun,
                        vm_tb_func_read_arg(fun, regs, instr.args[0]),
                        vm_tb_func_read_arg(fun, regs, instr.args[1]),
                        TB_ARITHMATIC_NONE
                    );
                }
                tb_inst_store(
                    fun,
                    vm_tag_to_tb_type(instr.tag),
                    regs[instr.out.reg],
                    value,
                    8,
                    false
                );
                break;
            }
            case VM_IOP_MUL: {
                TB_Node *value = NULL;
                if (instr.tag == VM_TAG_F32 || instr.tag == VM_TAG_F64) {
                    value = tb_inst_fmul(
                        fun,
                        vm_tb_func_read_arg(fun, regs, instr.args[0]),
                        vm_tb_func_read_arg(fun, regs, instr.args[1])
                    );
                } else {
                    value = tb_inst_mul(
                        fun,
                        vm_tb_func_read_arg(fun, regs, instr.args[0]),
                        vm_tb_func_read_arg(fun, regs, instr.args[1]),
                        TB_ARITHMATIC_NONE
                    );
                }
                tb_inst_store(
                    fun,
                    vm_tag_to_tb_type(instr.tag),
                    regs[instr.out.reg],
                    value,
                    8,
                    false
                );
                break;
            }
            case VM_IOP_DIV: {
                TB_Node *value = NULL;
                if (instr.tag == VM_TAG_F32 || instr.tag == VM_TAG_F64) {
                    value = tb_inst_fdiv(
                        fun,
                        vm_tb_func_read_arg(fun, regs, instr.args[0]),
                        vm_tb_func_read_arg(fun, regs, instr.args[1])
                    );
                } else {
                    value = tb_inst_div(
                        fun,
                        vm_tb_func_read_arg(fun, regs, instr.args[0]),
                        vm_tb_func_read_arg(fun, regs, instr.args[1]),
                        true
                    );
                }
                tb_inst_store(
                    fun,
                    vm_tag_to_tb_type(instr.tag),
                    regs[instr.out.reg],
                    value,
                    8,
                    false
                );
                break;
            }
            case VM_IOP_MOD: {
                if (instr.tag == VM_TAG_F64) {
                    TB_Node *bad = tb_inst_region(fun);
                    TB_Node *good = tb_inst_region(fun);
                    TB_Node *after = tb_inst_region(fun);
                    TB_Node *lhs = vm_tb_func_read_arg(fun, regs, instr.args[0]);
                    TB_Node *rhs = vm_tb_func_read_arg(fun, regs, instr.args[1]);
                    TB_Node *raw_div = tb_inst_fdiv(fun, lhs, rhs);
                    TB_Node *too_low = tb_inst_cmp_flt(fun, raw_div, tb_inst_float64(fun, (double)INT64_MIN));
                    TB_Node *too_high = tb_inst_cmp_fgt(fun, raw_div, tb_inst_float64(fun, (double)INT64_MAX));
                    TB_Node *is_bad = tb_inst_or(fun, too_low, too_high);
                    tb_inst_if(fun, is_bad, bad, good);
                    {
                        tb_inst_set_control(fun, good);
                        TB_Node *int_div = tb_inst_float2int(fun, raw_div, TB_TYPE_I64, true);
                        TB_Node *float_div = tb_inst_int2float(fun, int_div, TB_TYPE_F64, true);
                        TB_Node *mul = tb_inst_fmul(fun, float_div, rhs);
                        TB_Node *sub = tb_inst_fsub(fun, lhs, mul);
                        tb_inst_store(
                            fun,
                            vm_tag_to_tb_type(instr.tag),
                            regs[instr.out.reg],
                            sub, 8, false
                        );
                        tb_inst_goto(fun, after);
                    }
                    {
                        tb_inst_set_control(fun, bad);
                        TB_Node *mul = tb_inst_fmul(fun, raw_div, rhs);
                        TB_Node *sub = tb_inst_fsub(fun, lhs, mul);
                        tb_inst_store(
                            fun,
                            vm_tag_to_tb_type(instr.tag),
                            regs[instr.out.reg],
                            sub, 8, false
                        );
                        tb_inst_goto(fun, after);
                    }
                    tb_inst_set_control(fun, after);
                } else if (instr.tag == VM_TAG_F32) {
                    TB_Node *bad = tb_inst_region(fun);
                    TB_Node *good = tb_inst_region(fun);
                    TB_Node *after = tb_inst_region(fun);
                    TB_Node *lhs = vm_tb_func_read_arg(fun, regs, instr.args[0]);
                    TB_Node *rhs = vm_tb_func_read_arg(fun, regs, instr.args[1]);
                    TB_Node *raw_div = tb_inst_fdiv(fun, lhs, rhs);
                    TB_Node *too_low = tb_inst_cmp_flt(fun, raw_div, tb_inst_float32(fun, (float)INT32_MIN));
                    TB_Node *too_high = tb_inst_cmp_fgt(fun, raw_div, tb_inst_float32(fun, (float)INT32_MAX));
                    TB_Node *is_bad = tb_inst_or(fun, too_low, too_high);
                    tb_inst_if(fun, is_bad, bad, good);
                    {
                        tb_inst_set_control(fun, good);
                        TB_Node *int_div = tb_inst_float2int(fun, raw_div, TB_TYPE_I32, true);
                        TB_Node *float_div = tb_inst_int2float(fun, int_div, TB_TYPE_F32, true);
                        TB_Node *mul = tb_inst_fmul(fun, float_div, rhs);
                        TB_Node *sub = tb_inst_fsub(fun, lhs, mul);
                        tb_inst_store(
                            fun,
                            vm_tag_to_tb_type(instr.tag),
                            regs[instr.out.reg],
                            sub, 8, false
                        );
                        tb_inst_goto(fun, after);
                    }
                    {
                        tb_inst_set_control(fun, bad);
                        TB_Node *mul = tb_inst_fmul(fun, raw_div, rhs);
                        TB_Node *sub = tb_inst_fsub(fun, lhs, mul);
                        tb_inst_store(
                            fun,
                            vm_tag_to_tb_type(instr.tag),
                            regs[instr.out.reg],
                            sub, 8, false
                        );
                        tb_inst_goto(fun, after);
                    }
                    tb_inst_set_control(fun, after);
                } else {
                    tb_inst_store(
                        fun,
                        vm_tag_to_tb_type(instr.tag),
                        regs[instr.out.reg],
                        tb_inst_mod(
                            fun,
                            vm_tb_func_read_arg(fun, regs, instr.args[0]),
                            vm_tb_func_read_arg(fun, regs, instr.args[1]),
                            true
                        ),
                        8,
                        false
                    );
                }
                break;
            }
            case VM_IOP_NOP: {
                break;
            }
            case VM_IOP_STD: {
                tb_inst_store(
                    fun,
                    TB_TYPE_PTR,
                    regs[instr.out.reg],
                    vm_tb_ptr_name(state->module, fun, "<data>", state->std),
                    8,
                    false
                );
                break;
            }
            case VM_IOP_LEN: {
                vm_tag_t table_tag = vm_arg_to_tag(instr.args[0]);
                if (table_tag == VM_TAG_TAB) {
                    TB_Node *len = tb_inst_load(
                        fun,
                        TB_TYPE_I32,
                        tb_inst_member_access(
                            fun,
                            vm_tb_func_read_arg(fun, regs, instr.args[0]),
                            offsetof(vm_table_t, len)
                        ),
                        1,
                        false
                    );
                    if (instr.tag == VM_TAG_I32) {
                        // defaults to i32
                    } else if (instr.tag == VM_TAG_I64) {
                        len = tb_inst_zxt(fun, len, TB_TYPE_I64);
                    } else if (instr.tag == VM_TAG_F32) {
                        len = tb_inst_int2float(fun, len, TB_TYPE_F32, false);
                    } else if (instr.tag == VM_TAG_F64) {
                        len = tb_inst_int2float(fun, len, TB_TYPE_F64, false);
                    }
                    tb_inst_store(
                        fun,
                        vm_tag_to_tb_type(instr.tag),
                        regs[instr.out.reg],
                        len,
                        1,
                        false
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
                    {vm_tag_to_tb_type(key_tag)},
                    {vm_tag_to_tb_type(val_tag)},
                    {TB_TYPE_I32},
                    {TB_TYPE_I32},
                };
                TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, 5, proto_params, 0, NULL, false);
                TB_Node *args[5] = {
                    vm_tb_func_read_arg(fun, regs, instr.args[0]),
                    vm_tb_func_read_arg(fun, regs, instr.args[1]),
                    vm_tb_func_read_arg(fun, regs, instr.args[2]),
                    tb_inst_uint(fun, TB_TYPE_I32, key_tag),
                    tb_inst_uint(fun, TB_TYPE_I32, val_tag),
                };
                tb_inst_call(
                    fun,
                    proto,
                    tb_inst_get_symbol_address(fun, state->vm_table_set),
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
                    fun,
                    TB_TYPE_PTR,
                    regs[instr.out.reg],
                    tb_inst_call(
                        fun,
                        proto,
                        tb_inst_get_symbol_address(fun, state->vm_table_new),
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
                vm_print_instr(stderr, instr);
                fprintf(stderr, "\n ^ unhandled instruction\n");
                exit(1);
            }
        }
    }

    vm_branch_t branch = block->branch;

    switch (branch.op) {
        case VM_BOP_JUMP: {
            tb_inst_goto(
                fun,
                vm_tb_func_body_once(state, fun, regs, branch.targets[0])
            );
            break;
        }

        case VM_BOP_BB: {
            if (branch.tag == VM_TAG_BOOL) {
                tb_inst_if(
                    fun,
                    vm_tb_func_read_arg(fun, regs, branch.args[0]),
                    vm_tb_func_body_once(state, fun, regs, branch.targets[0]),
                    vm_tb_func_body_once(state, fun, regs, branch.targets[1])
                );
            } else if (branch.tag == VM_TAG_NIL) {
                tb_inst_goto(
                    fun,
                    vm_tb_func_body_once(state, fun, regs, branch.targets[1])
                );
            } else {
                tb_inst_goto(
                    fun,
                    vm_tb_func_body_once(state, fun, regs, branch.targets[0])
                );
            }
            break;
        }

        case VM_BOP_BLT: {
            tb_inst_if(
                fun,
                vm_tb_select_binary_cmp(
                    branch.tag,
                    tb_inst_cmp_ilt, tb_inst_cmp_flt,
                    fun,
                    vm_tb_func_read_arg(fun, regs, branch.args[0]),
                    vm_tb_func_read_arg(fun, regs, branch.args[1])
                ),
                vm_tb_func_body_once(state, fun, regs, branch.targets[0]),
                vm_tb_func_body_once(state, fun, regs, branch.targets[1])
            );
            break;
        }

        case VM_BOP_BEQ: {
            tb_inst_if(
                fun,
                tb_inst_cmp_eq(
                    fun,
                    vm_tb_func_read_arg(fun, regs, branch.args[0]),
                    vm_tb_func_read_arg(fun, regs, branch.args[1])
                ),
                vm_tb_func_body_once(state, fun, regs, branch.targets[0]),
                vm_tb_func_body_once(state, fun, regs, branch.targets[1])
            );
            break;
        }

        case VM_BOP_RET: {
            TB_Node *ret[2];

            ret[0] = tb_inst_bitcast(fun, vm_tb_func_read_arg(fun, regs, branch.args[0]), TB_TYPE_PTR);

            ret[1] = tb_inst_uint(fun, TB_TYPE_I32, branch.tag);

            tb_inst_ret(fun, 2, ret);
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
                    TB_PrototypeParam call_proto_params[1] = {
                        {TB_TYPE_PTR},
                    };

                    TB_FunctionPrototype *call_proto = tb_prototype_create(state->module, VM_TB_CC, 1, call_proto_params, 0, NULL, false);

                    TB_Node *call_arg = tb_inst_local(fun, sizeof(vm_std_value_t) * (nparams + 1), 8);

                    for (size_t i = 1; branch.args[i].type != VM_ARG_NONE; i++) {
                        vm_arg_t arg = branch.args[i];
                        TB_Node *head = tb_inst_member_access(fun, call_arg, sizeof(vm_std_value_t) * (i - 1));
                        vm_tag_t tag = vm_arg_to_tag(arg);
                        tb_inst_store(
                            fun,
                            vm_tag_to_tb_type(tag),
                            tb_inst_member_access(fun, head, offsetof(vm_std_value_t, value)),
                            vm_tb_func_read_arg(fun, regs, arg),
                            8,
                            false
                        );
                        tb_inst_store(
                            fun,
                            TB_TYPE_I32,
                            tb_inst_member_access(fun, head, offsetof(vm_std_value_t, tag)),
                            tb_inst_uint(fun, TB_TYPE_I32, tag),
                            4,
                            false
                        );
                    }

                    TB_Node *end_head = tb_inst_member_access(fun, call_arg, sizeof(vm_std_value_t) * nparams);

                    tb_inst_store(
                        fun,
                        TB_TYPE_PTR,
                        tb_inst_member_access(fun, end_head, offsetof(vm_std_value_t, value)),
                        vm_tb_ptr_name(state->module, fun, "<data>", 0),
                        8,
                        false
                    );
                    tb_inst_store(
                        fun,
                        TB_TYPE_I32,
                        tb_inst_member_access(fun, end_head, offsetof(vm_std_value_t, tag)),
                        tb_inst_uint(fun, TB_TYPE_I32, 0),
                        4,
                        false
                    );

                    TB_Node *call_func = vm_tb_func_read_arg(fun, regs, branch.args[0]);

                    tb_inst_call(
                        fun,
                        call_proto,
                        call_func,
                        1,
                        &call_arg
                    );

                    val_tag = tb_inst_load(
                        fun,
                        TB_TYPE_I32,
                        tb_inst_member_access(
                            fun,
                            call_arg,
                            offsetof(vm_std_value_t, tag)
                        ),
                        4,
                        false
                    );

                    val_val = tb_inst_load(
                        fun,
                        TB_TYPE_PTR,
                        tb_inst_member_access(
                            fun,
                            call_arg,
                            offsetof(vm_std_value_t, value)
                        ),
                        1,
                        false
                    );
                } else if (vm_arg_to_tag(branch.args[0]) == VM_TAG_CLOSURE) {
                    TB_Node *closure = vm_tb_func_read_arg(fun, regs, branch.args[0]);
                    TB_Node *block_num = tb_inst_load(
                        fun,
                        vm_tag_to_tb_type(VM_TAG_FUN),
                        tb_inst_member_access(
                            fun,
                            closure,
                            offsetof(vm_std_value_t, value)
                        ),
                        1,
                        false
                    );

                    void **cache = vm_malloc(sizeof(void *) * state->nblocks);
                    memset(cache, 0, sizeof(void *) * state->nblocks);

                    TB_Node *has_cache = tb_inst_region(fun);
                    TB_Node *no_cache = tb_inst_region(fun);
                    TB_Node *after = tb_inst_region(fun);

                    TB_Node *global_ptr = tb_inst_array_access(
                        fun,
                        vm_tb_ptr_name(state->module, fun, "<code_cache>", cache),
                        block_num,
                        sizeof(void *)
                    );
                    TB_Node *global = tb_inst_load(fun, TB_TYPE_PTR, global_ptr, 1, false);

                    val_val = tb_inst_local(fun, 8, 8);
                    val_tag = tb_inst_local(fun, 4, 4);

                    size_t nargs = 1;
                    for (size_t arg = 1; branch.args[arg].type != VM_ARG_NONE; arg++) {
                        nargs += 1;
                    }

                    TB_PrototypeParam *call_proto_params = vm_malloc(sizeof(TB_PrototypeParam) * nargs);
                    TB_Node **call_args = vm_malloc(sizeof(TB_Node *) * nargs);

                    call_proto_params[0] = (TB_PrototypeParam){TB_TYPE_PTR};
                    call_args[0] = closure;
                    for (size_t arg = 1; branch.args[arg].type != VM_ARG_NONE; arg++) {
                        call_proto_params[arg] = (TB_PrototypeParam){
                            vm_tag_to_tb_type(vm_arg_to_tag(branch.args[arg])),
                        };
                        call_args[arg] = vm_tb_func_read_arg(fun, regs, branch.args[arg]);
                    }

                    TB_PrototypeParam call_proto_rets[2] = {
                        {TB_TYPE_PTR},
                        {TB_TYPE_I32},
                    };

                    TB_FunctionPrototype *call_proto = tb_prototype_create(state->module, VM_TB_CC, nargs, call_proto_params, 2, call_proto_rets, false);

                    tb_inst_if(
                        fun,
                        tb_inst_cmp_eq(fun, global, tb_inst_uint(fun, TB_TYPE_PTR, 0)),
                        no_cache,
                        has_cache
                    );

                    {
                        tb_inst_set_control(fun, no_cache);

                        TB_Node *rblock_call_table = vm_tb_ptr_name(state->module, fun, "<call_table>", branch.call_table);
                        // tb_inst_debugbreak(fun);
                        TB_Node *rblock_ref = tb_inst_array_access(
                            fun,
                            rblock_call_table,
                            block_num,
                            sizeof(vm_rblock_t *)
                        );
                        TB_Node *rblock = tb_inst_load(fun, TB_TYPE_PTR, rblock_ref, 1, false);

                        TB_PrototypeParam comp_params[2] = {
                            {TB_TYPE_PTR},
                        };

                        TB_PrototypeParam comp_ret[1] = {
                            {TB_TYPE_PTR},
                        };

                        TB_FunctionPrototype *comp_proto = tb_prototype_create(state->module, VM_TB_CC, 1, comp_params, 1, comp_ret, false);

                        for (size_t i = 0; i < state->nblocks; i++) {
                            vm_rblock_t *rblock = branch.call_table[i];
                            if (rblock == NULL) {
                                continue;
                            }
                            rblock->state = state;
                        }

                        TB_Node *call_func = tb_inst_call(
                                                fun,
                                                comp_proto,
                                                tb_inst_get_symbol_address(fun, state->vm_tb_rfunc_comp),
                                                1,
                                                &rblock
                        )
                                                .single;

                        tb_inst_store(
                            fun,
                            TB_TYPE_PTR,
                            global_ptr,
                            call_func,
                            1,
                            false
                        );

                        TB_Node **got = tb_inst_call(
                                            fun,
                                            call_proto,
                                            call_func,
                                            nargs,
                                            call_args
                        )
                                            .multiple;

                        tb_inst_store(
                            fun,
                            TB_TYPE_PTR,
                            val_val,
                            got[0],
                            8,
                            false
                        );

                        tb_inst_store(
                            fun,
                            TB_TYPE_I32,
                            val_tag,
                            got[1],
                            4,
                            false
                        );

                        tb_inst_goto(fun, after);
                    }

                    {
                        tb_inst_set_control(fun, has_cache);

                        TB_Node **got = tb_inst_call(
                                            fun,
                                            call_proto,
                                            global,
                                            nargs,
                                            call_args
                        )
                                            .multiple;

                        tb_inst_store(
                            fun,
                            TB_TYPE_PTR,
                            val_val,
                            got[0],
                            8,
                            false
                        );
                        tb_inst_store(
                            fun,
                            TB_TYPE_I32,
                            val_tag,
                            got[1],
                            4,
                            false
                        );

                        tb_inst_goto(fun, after);
                    }

                    tb_inst_set_control(fun, after);

                    val_val = tb_inst_load(
                        fun,
                        TB_TYPE_PTR,
                        val_val,
                        8,
                        false
                    );

                    val_tag = tb_inst_load(
                        fun,
                        TB_TYPE_I32,
                        val_tag,
                        4,
                        false
                    );
                } else {
                    vm_tb_func_report_error(state, fun, "bad call");
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
                    TB_Node *arg2 = tb_inst_local(fun, sizeof(vm_pair_t), 8);
                    vm_tag_t tag = vm_arg_to_tag(branch.args[1]);
                    tb_inst_store(
                        fun,
                        vm_tag_to_tb_type(tag),
                        tb_inst_member_access(
                            fun,
                            arg2,
                            offsetof(vm_pair_t, key_val)
                        ),
                        vm_tb_func_read_arg(fun, regs, branch.args[1]),
                        8,
                        false
                    );
                    tb_inst_store(
                        fun,
                        TB_TYPE_I32,
                        tb_inst_member_access(
                            fun,
                            arg2,
                            offsetof(vm_pair_t, key_tag)
                        ),
                        tb_inst_uint(fun, TB_TYPE_I32, tag),
                        4,
                        false
                    );
                    TB_Node *get_args[2] = {
                        vm_tb_func_read_arg(fun, regs, branch.args[0]),
                        arg2,
                    };
                    tb_inst_call(
                        fun,
                        get_proto,
                        tb_inst_get_symbol_address(fun, state->vm_table_get_pair),
                        2,
                        get_args
                    );

                    val_tag = tb_inst_load(
                        fun,
                        TB_TYPE_I32,
                        tb_inst_member_access(
                            fun,
                            arg2,
                            offsetof(vm_pair_t, val_tag)
                        ),
                        1,
                        false
                    );

                    val_val = tb_inst_load(
                        fun,
                        TB_TYPE_PTR,
                        tb_inst_member_access(
                            fun,
                            arg2,
                            offsetof(vm_pair_t, val_val)
                        ),
                        1,
                        false
                    );
                } else if (arg0tag == VM_TAG_CLOSURE) {
                    TB_Node *std_val_ref = tb_inst_array_access(
                        fun,
                        vm_tb_func_read_arg(fun, regs, branch.args[0]),
                        vm_tb_func_read_arg(fun, regs, branch.args[1]),
                        sizeof(vm_std_value_t)
                    );

                    val_tag = tb_inst_load(
                        fun,
                        TB_TYPE_I32,
                        tb_inst_member_access(
                            fun,
                            std_val_ref,
                            offsetof(vm_std_value_t, tag)
                        ),
                        1,
                        false
                    );

                    val_val = tb_inst_load(
                        fun,
                        TB_TYPE_PTR,
                        tb_inst_member_access(
                            fun,
                            std_val_ref,
                            offsetof(vm_std_value_t, value)
                        ),
                        1,
                        false
                    );
                } else {
                    vm_tb_func_report_error(state, fun, "bad get");
                    break;
                }

            }
            
            TB_PrototypeParam next_rets[2] = {
                {TB_TYPE_PTR},
                {TB_TYPE_I32},
            };

            size_t next_nargs = branch.targets[0]->nargs;
            TB_PrototypeParam *next_params = vm_malloc(sizeof(TB_PrototypeParam) * next_nargs);
            TB_Node **next_args = vm_malloc(sizeof(TB_Node *) * next_nargs);

            for (size_t argno = 0; argno < next_nargs; argno++) {
                vm_arg_t arg = branch.targets[0]->args[argno];
                if (arg.type == VM_ARG_REG && arg.reg == branch.out.reg) {
                    next_params[argno] = (TB_PrototypeParam){ TB_TYPE_PTR };
                    next_args[argno] = val_val;
                } else {
                    next_params[argno] = (TB_PrototypeParam){vm_tag_to_tb_type(vm_arg_to_tag(arg))};
                    next_args[argno] = vm_tb_func_read_arg(fun, regs, arg);
                }
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
                fun,
                vm_tb_ptr_name(state->module, fun, "<rtargets>", mem),
                val_tag,
                sizeof(vm_rblock_t *)
            );
            TB_Node *func = tb_inst_load(
                fun,
                TB_TYPE_PTR,
                func_ref,
                1,
                false
            );
            TB_Node *func_nonzero = tb_inst_cmp_ne(fun, func, tb_inst_uint(fun, TB_TYPE_PTR, 0));
            TB_Node *has_cache_region = tb_inst_region(fun);
            TB_Node *no_cache_region = tb_inst_region(fun);

            tb_inst_if(fun, func_nonzero, has_cache_region, no_cache_region);
            {
                tb_inst_set_control(fun, no_cache_region);

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
                    fun,
                    TB_TYPE_PTR,
                    tb_inst_array_access(
                        fun,
                        vm_tb_ptr_name(state->module, fun, "<rtargets>", &block->branch.rtargets[0]),
                        val_tag,
                        sizeof(vm_rblock_t *)
                    ),
                    1,
                    false
                );

                TB_MultiOutput new_func_multi = tb_inst_call(
                    fun,
                    comp_proto,
                    tb_inst_get_symbol_address(fun, state->vm_tb_rfunc_comp),
                    1,
                    comp_args
                );

                tb_inst_store(
                    fun,
                    TB_TYPE_PTR,
                    func_ref,
                    new_func_multi.single,
                    1,
                    false
                );
                
                tb_inst_tailcall(
                    fun,
                    next_proto,
                    new_func_multi.single,
                    next_nargs,
                    next_args
                );
            }

            {
                tb_inst_set_control(fun, has_cache_region);
                
                tb_inst_tailcall(
                    fun,
                    next_proto,
                    func,
                    next_nargs,
                    next_args
                );
            }

            // TB_Node *old_region = tb_inst_get_control(fun);

            // TB_Node *bad_region = tb_inst_region(fun);

            // TB_Node *type_fail_region = NULL;

            // TB_SwitchEntry entries[VM_TAG_MAX];

            // entries[0].key = 0;
            // entries[0].value = bad_region;

            // for (size_t i = 1; i < VM_TAG_MAX; i++) {
            //     if (i == VM_TAG_ERROR) {
            //         TB_Node *i_region = tb_inst_region(fun);
            //         tb_inst_set_control(fun, i_region);
            //         TB_Node *ret_vals[2];
            //         ret_vals[0] = val_val;
            //         ret_vals[1] = tb_inst_uint(fun, TB_TYPE_I32, VM_TAG_ERROR);
            //         tb_inst_ret(fun, 2, ret_vals);
            //         entries[i].key = i;
            //         entries[i].value = i_region;
            //     } else {
            //         TB_Node *i_region = tb_inst_region(fun);
            //         tb_inst_set_control(fun, i_region);
            //         void **mem = vm_malloc(sizeof(void *));
            //         *mem = NULL;
            //         TB_Node *func_ref = vm_tb_ptr_name(state->module, fun, "<funcs>", mem);
            //         TB_Node *func = tb_inst_load(
            //             fun,
            //             TB_TYPE_PTR,
            //             func_ref,
            //             1,
            //             false
            //         );
            //         TB_Node *func_nonzero = tb_inst_cmp_ne(fun, func, tb_inst_uint(fun, TB_TYPE_PTR, 0));
            //         TB_Node *has_cache_region = tb_inst_region(fun);
            //         TB_Node *no_cache_region = tb_inst_region(fun);

            //         size_t next_nargs = branch.targets[0]->nargs;
            //         TB_PrototypeParam *next_params = vm_malloc(sizeof(TB_PrototypeParam) * next_nargs);
            //         TB_Node **next_args = vm_malloc(sizeof(TB_Node *) * next_nargs);

            //         for (size_t argno = 0; argno < next_nargs; argno++) {
            //             vm_arg_t arg = branch.targets[0]->args[argno];
            //             if (arg.type == VM_ARG_REG && arg.reg == branch.out.reg) {
            //                 next_params[argno] = (TB_PrototypeParam){vm_tag_to_tb_type(i)};
            //                 next_args[argno] = val_val;
            //             } else {
            //                 next_params[argno] = (TB_PrototypeParam){vm_tag_to_tb_type(vm_arg_to_tag(arg))};
            //                 next_args[argno] = vm_tb_func_read_arg(fun, regs, arg);
            //             }
            //         }

            //         TB_PrototypeParam next_rets[2] = {
            //             {TB_TYPE_PTR},
            //             {TB_TYPE_I32},
            //         };

            //         TB_FunctionPrototype *next_proto = tb_prototype_create(
            //             state->module,
            //             VM_TB_CC,
            //             next_nargs,
            //             next_params,
            //             2,
            //             next_rets,
            //             false
            //         );

            //         tb_inst_if(fun, func_nonzero, has_cache_region, no_cache_region);
            //         {
            //             tb_inst_set_control(fun, has_cache_region);

            //             tb_inst_tailcall(
            //                 fun,
            //                 next_proto,
            //                 func,
            //                 next_nargs,
            //                 next_args
            //             );
            //         }
            //         {
            //             tb_inst_set_control(fun, no_cache_region);

            //             TB_PrototypeParam comp_params[1] = {
            //                 {TB_TYPE_PTR},
            //             };

            //             TB_PrototypeParam comp_ret[1] = {
            //                 {TB_TYPE_PTR},
            //             };

            //             TB_FunctionPrototype *comp_proto = tb_prototype_create(state->module, VM_TB_CC, 1, comp_params, 1, comp_ret, false);

            //             branch.rtargets[i]->state = state;

            //             TB_Node *comp_param = vm_tb_ptr_name(state->module, fun, "<data>", branch.rtargets[i]);

            //             TB_MultiOutput new_func_multi = tb_inst_call(
            //                 fun,
            //                 comp_proto,
            //                 tb_inst_get_symbol_address(fun, state->vm_tb_rfunc_comp),
            //                 1,
            //                 &comp_param
            //             );

            //             TB_Node *new_func = new_func_multi.single;

            //             tb_inst_store(
            //                 fun,
            //                 TB_TYPE_PTR,
            //                 func_ref,
            //                 new_func,
            //                 1,
            //                 false
            //             );

            //             tb_inst_tailcall(
            //                 fun,
            //                 next_proto,
            //                 new_func,
            //                 next_nargs,
            //                 next_args
            //             );
            //         }
            //         entries[i].key = i;
            //         entries[i].value = i_region;
            //     }
            // }

            // tb_inst_set_control(fun, old_region);

            // tb_inst_branch(fun, TB_TYPE_I32, val_tag, bad_region, VM_TAG_MAX, entries);

            // tb_inst_set_control(fun, bad_region);
            // tb_inst_unreachable(fun);

            break;
        }

        // case VM_BOP_GET: {
        //     TB_Node *val_tag;
        //     TB_Node *val_val;
        //     TB_Node *old_region = tb_inst_get_control(fun);

        //     TB_Node *bad_region = tb_inst_region(fun);

        //     TB_Node *type_fail_region = NULL;

        //     TB_SwitchEntry entries[VM_TAG_MAX];

        //     entries[0].key = 0;
        //     entries[0].value = bad_region;

        //     for (size_t i = 1; i < VM_TAG_MAX; i++) {
        //         if (i == VM_TAG_ERROR) {
        //             TB_Node *i_region = tb_inst_region(fun);
        //             tb_inst_set_control(fun, i_region);
        //             TB_Node *ret_vals[2];
        //             ret_vals[0] = val_val;
        //             ret_vals[1] = tb_inst_uint(fun, TB_TYPE_I32, i);
        //             tb_inst_ret(fun, 2, ret_vals);
        //             entries[i].key = i;
        //             entries[i].value = i_region;
        //         } else {
        //             TB_Node *i_region = tb_inst_region(fun);
        //             tb_inst_set_control(fun, i_region);
        //             void **mem = vm_malloc(sizeof(void *));
        //             *mem = NULL;
        //             TB_Node *func_ref = vm_tb_ptr_name(state->module, fun, "<funcs>", mem);
        //             TB_Node *func = tb_inst_load(
        //                 fun,
        //                 TB_TYPE_PTR,
        //                 func_ref,
        //                 1,
        //                 false
        //             );
        //             TB_Node *func_nonzero = tb_inst_cmp_ne(fun, func, tb_inst_uint(fun, TB_TYPE_PTR, 0));
        //             TB_Node *has_cache_region = tb_inst_region(fun);
        //             TB_Node *no_cache_region = tb_inst_region(fun);

        //             size_t next_nargs = branch.targets[0]->nargs;
        //             TB_PrototypeParam *next_params = vm_malloc(sizeof(TB_PrototypeParam) * next_nargs);
        //             TB_Node **next_args = vm_malloc(sizeof(TB_Node *) * next_nargs);

        //             for (size_t argno = 0; argno < next_nargs; argno++) {
        //                 vm_arg_t arg = branch.targets[0]->args[argno];
        //                 if (arg.type == VM_ARG_REG && arg.reg == branch.out.reg) {
        //                     next_params[argno] = (TB_PrototypeParam){vm_tag_to_tb_type(i)};
        //                     next_args[argno] = val_val;
        //                 } else {
        //                     next_params[argno] = (TB_PrototypeParam){vm_tag_to_tb_type(vm_arg_to_tag(arg))};
        //                     next_args[argno] = vm_tb_func_read_arg(fun, regs, arg);
        //                 }
        //             }

        //             TB_PrototypeParam next_rets[2] = {
        //                 {TB_TYPE_PTR},
        //                 {TB_TYPE_I32},
        //             };

        //             TB_FunctionPrototype *next_proto = tb_prototype_create(
        //                 state->module,
        //                 VM_TB_CC,
        //                 next_nargs,
        //                 next_params,
        //                 2,
        //                 next_rets,
        //                 false
        //             );

        //             tb_inst_if(fun, func_nonzero, has_cache_region, no_cache_region);
        //             {
        //                 tb_inst_set_control(fun, has_cache_region);

        //                 tb_inst_tailcall(
        //                     fun,
        //                     next_proto,
        //                     func,
        //                     next_nargs,
        //                     next_args
        //                 );
        //             }
        //             {
        //                 tb_inst_set_control(fun, no_cache_region);

        //                 TB_PrototypeParam comp_params[1] = {
        //                     {TB_TYPE_PTR},
        //                 };

        //                 TB_PrototypeParam comp_ret[1] = {
        //                     {TB_TYPE_PTR},
        //                 };

        //                 TB_FunctionPrototype *comp_proto = tb_prototype_create(state->module, VM_TB_CC, 1, comp_params, 1, comp_ret, false);

        //                 branch.rtargets[i]->state = state;

        //                 TB_Node *comp_param = vm_tb_ptr_name(state->module, fun, "<data>", branch.rtargets[i]);

        //                 TB_MultiOutput new_func_multi = tb_inst_call(
        //                     fun,
        //                     comp_proto,
        //                     tb_inst_get_symbol_address(fun, state->vm_tb_rfunc_comp),
        //                     1,
        //                     &comp_param
        //                 );

        //                 TB_Node *new_func = new_func_multi.single;

        //                 tb_inst_store(
        //                     fun,
        //                     TB_TYPE_PTR,
        //                     func_ref,
        //                     new_func,
        //                     1,
        //                     false
        //                 );

        //                 tb_inst_tailcall(
        //                     fun,
        //                     next_proto,
        //                     new_func,
        //                     next_nargs,
        //                     next_args
        //                 );
        //             }
        //             entries[i].key = i;
        //             entries[i].value = i_region;
        //         }
        //     }

        //     tb_inst_set_control(fun, old_region);

        //     tb_inst_branch(fun, TB_TYPE_I32, val_tag, bad_region, VM_TAG_MAX, entries);

        //     tb_inst_set_control(fun, bad_region);
        //     tb_inst_unreachable(fun);

        //     break;
        // }

        default: {
            vm_print_branch(stderr, branch);
            fprintf(stderr, "\n ^ unhandled branch\n");
            exit(1);
        }
    }

    tb_inst_set_control(fun, old_ctrl);

    return ret;
}

void vm_tb_func_report_error(vm_tb_state_t *state, TB_Function *fun, const char *str) {
    TB_Node *ret_vals[2];
    ret_vals[0] = vm_tb_ptr_name(state->module, fun, "<error>", (void *)str);
    ret_vals[1] = tb_inst_uint(fun, TB_TYPE_I32, VM_TAG_ERROR);
    tb_inst_ret(fun, 2, ret_vals);
}

void vm_tb_print(uint32_t tag, void *value) {
    vm_std_value_t val = (vm_std_value_t){
        .tag = tag,
    };
    switch (tag) {
        case VM_TAG_I8: {
            val.value.i8 = *(int8_t *)value;
            break;
        }
        case VM_TAG_I16: {
            val.value.i16 = *(int16_t *)value;
            break;
        }
        case VM_TAG_I32: {
            val.value.i32 = *(int32_t *)value;
            break;
        }
        case VM_TAG_I64: {
            val.value.i64 = *(int64_t *)value;
            break;
        }
        case VM_TAG_F32: {
            val.value.f32 = *(float *)value;
            break;
        }
        case VM_TAG_F64: {
            val.value.f64 = *(double *)value;
            break;
        }
        case VM_TAG_STR: {
            val.value.str = *(const char **)value;
            break;
        }
        case VM_TAG_CLOSURE: {
            val.value.closure = *(vm_std_value_t **)value;
            break;
        }
        case VM_TAG_TAB: {
            val.value.table = *(vm_table_t **)value;
            break;
        }
        case VM_TAG_FFI: {
            val.value.ffi = *(void (*)(vm_std_value_t *))value;
            break;
        }
        default: {
            printf("bad tag: %zu\n", (size_t)tag);
            exit(1);
        }
    }
    vm_io_debug(stdout, 0, "debug: ", val, NULL);
}

void vm_tb_func_print_value(vm_tb_state_t *state, TB_Function *fun, vm_tag_t tag, TB_Node *value) {
    TB_PrototypeParam proto_args[2] = {
        {TB_TYPE_I32},
        {TB_TYPE_PTR},
    };

    TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, 2, proto_args, 0, NULL, false);

    TB_Node *local = tb_inst_local(fun, 8, 8);

    tb_inst_store(
        fun,
        vm_tag_to_tb_type(tag),
        local,
        value,
        8,
        false
    );

    TB_Node *params[2] = {
        tb_inst_uint(fun, TB_TYPE_I32, (uint64_t)tag),
        local,
    };

    tb_inst_call(
        fun,
        proto,
        tb_inst_get_symbol_address(fun, state->vm_tb_print),
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

void *vm_tb_rfunc_comp(vm_rblock_t *rblock) {
    void *cache = rblock->jit;
    // printf("%zi\n", rblock->block->id);
    if (cache != NULL) {
        return cache;
    }

    rblock->count += 1;

    vm_tb_state_t *state = rblock->state;
    state->faults = 0;

    vm_block_t *block = vm_rblock_version(state->nblocks, state->blocks, rblock);
    TB_Function *fun = tb_function_create(state->module, -1, "block", TB_LINKAGE_PRIVATE);

    if (block == NULL) {
        TB_PrototypeParam proto_rets[2] = {
            {TB_TYPE_PTR},
            {TB_TYPE_I32},
        };

        TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, 0, NULL, 2, proto_rets, false);

        tb_function_set_prototype(
            fun,
            -1,
            proto,
            NULL
        );

        vm_tb_func_report_error(state, fun, "internal: block == NULL");
    } else {
        TB_PrototypeParam *proto_args = vm_malloc(sizeof(TB_PrototypeParam) * block->nargs);

        for (size_t arg = 0; arg < block->nargs; arg++) {
            size_t reg = block->args[arg].reg;
            proto_args[arg] = (TB_PrototypeParam){
                vm_tag_to_tb_type(rblock->regs->tags[reg]),
            };
        }

        TB_PrototypeParam proto_rets[2] = {
            {TB_TYPE_PTR},
            {TB_TYPE_I32},
        };

        TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, block->nargs, proto_args, 2, proto_rets, false);
        tb_function_set_prototype(
            fun,
            -1,
            proto,
            NULL
        );

        TB_Node **regs = vm_malloc(sizeof(TB_Node *) * block->nregs);

        for (size_t i = 0; i < block->nregs; i++) {
            regs[i] = tb_inst_local(fun, 8, 8);
        }

        for (size_t i = 0; i < block->nargs; i++) {
            tb_inst_store(
                fun,
                vm_tag_to_tb_type(block->args[i].reg_tag),
                regs[block->args[i].reg],
                tb_inst_param(fun, i),
                8,
                false
            );
            if (state->config->dump_args) {
                vm_tb_func_print_value(state, fun, rblock->regs->tags[block->args[i].reg], regs[block->args[i].reg]);
            }
        }

        vm_tb_func_reset_pass(block);
        TB_Node *main = vm_tb_func_body_once(state, fun, regs, block);
        vm_tb_func_reset_pass(block);
        tb_inst_goto(fun, main);
    }

    TB_Passes *passes = tb_pass_enter(fun, tb_function_get_arena(fun));
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
    void *ret = tb_jit_place_function(jit, fun);

    rblock->jit = ret;

    if (state->faults < rblock->least_faults) {
        rblock->least_faults = state->faults;
    } else {
        if (state->faults == 0) {
            rblock->base_redo = SIZE_MAX;
        } else {
            rblock->base_redo *= 256;
        }
        rblock->redo = rblock->base_redo;
    }

    // printf("block #%zi with %zu faults\n", rblock->block->id, state->faults);

    // printf("code buf: %p\n", ret);

    return ret;
}

void *vm_tb_full_comp(vm_tb_state_t *state, vm_block_t *block) {
    vm_tags_t *regs = vm_rblock_regs_empty(block->nregs);
    block->isfunc = true;
    vm_rblock_t *rblock = vm_rblock_new(block, regs);
    rblock->state = state;
    return vm_tb_rfunc_comp(rblock);
}

typedef vm_std_value_t VM_CDECL vm_tb_func_t(void);

vm_std_value_t vm_tb_run_main(vm_config_t *config, vm_block_t *entry, size_t nblocks, vm_block_t **blocks, vm_table_t *std) {
    vm_std_value_t val = vm_tb_run_repl(config, entry, nblocks, blocks, std);
    if (val.tag == VM_TAG_ERROR) {
        printf("error: %s\n", val.value.str);
    }
    return val;
}

vm_std_value_t vm_tb_run_repl(vm_config_t *config, vm_block_t *entry, size_t nblocks, vm_block_t **blocks, vm_table_t *std) {
    vm_tb_state_t *state = vm_malloc(sizeof(vm_tb_state_t));
    state->std = std;
    state->config = config;
    state->nblocks = nblocks;
    state->blocks = blocks;

    vm_tb_new_module(state);

    vm_tb_func_t *fn = (vm_tb_func_t *)vm_tb_full_comp(state, entry);
    return fn();
}
