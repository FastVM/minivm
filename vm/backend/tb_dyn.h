
#if !defined(VM_BE_TB_DYN)
#define VM_BE_TB_DYN

#include "../../vendor/cuik/common/arena.h"
#include "../../vendor/tcc/libtcc.h"
#include "../ir/check.h"
#include "./exec.h"

struct vm_tb_dyn_pair_t;
struct vm_tb_dyn_state_t;

typedef struct vm_tb_dyn_pair_t vm_tb_dyn_pair_t;
typedef struct vm_tb_dyn_state_t vm_tb_dyn_state_t;

typedef vm_std_value_t vm_tb_dyn_func_t(vm_std_value_t *values);

struct vm_tb_dyn_pair_t {
    TB_Node *val;
    TB_Node *tag;
};

struct vm_tb_dyn_state_t {
    vm_table_t *std;
    vm_config_t *config;
    vm_blocks_t *blocks;
    vm_tag_t number_type;

    TB_Function *func;
    TB_Module *mod;
    TB_Function **funcs;
    TB_DataType number_dt;
    TB_Node **compiled;

    TB_Symbol *vm_table_new;
    TB_Symbol *vm_table_set_pair;
    TB_Symbol *vm_table_get_pair;

    size_t nlocals;
    TB_Node *locals;
    vm_tb_dyn_pair_t *regs;

    void **codes;
};

TB_Node *vm_tb_dyn_ptr(vm_tb_dyn_state_t *state, const void *ptr) {
    return tb_inst_uint(state->func, TB_TYPE_PTR, (uint64_t)(size_t)ptr);
}

vm_tb_dyn_pair_t vm_tb_dyn_pair_of(vm_tb_dyn_state_t *state, vm_tag_t type, TB_Node *value) {
    if (value->dt.raw != VM_TB_TYPE_VALUE.raw) {
        value = tb_inst_bitcast(state->func, value, VM_TB_TYPE_VALUE);
    }
    return (vm_tb_dyn_pair_t){
        .val = value,
        .tag = tb_inst_uint(state->func, VM_TB_TYPE_TAG, type),
    };
}

vm_tb_dyn_pair_t vm_tb_dyn_arg(vm_tb_dyn_state_t *state, vm_arg_t arg) {
    switch (arg.type) {
        case VM_ARG_LIT: {
            switch (vm_type_tag(arg.lit.tag)) {
                case VM_TAG_NIL: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TAG_NIL,
                        tb_inst_uint(state->func, VM_TB_TYPE_VALUE, 0)
                    );
                }
                case VM_TAG_BOOL: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TAG_BOOL,
                        tb_inst_bool(state->func, arg.lit.value.b)
                    );
                }
                case VM_TAG_I8: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TAG_I8,
                        tb_inst_sint(state->func, TB_TYPE_I8, arg.lit.value.i8)
                    );
                }
                case VM_TAG_I16: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TAG_I16,
                        tb_inst_sint(state->func, TB_TYPE_I16, arg.lit.value.i16)
                    );
                }
                case VM_TAG_I32: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TAG_I32,
                        tb_inst_sint(state->func, TB_TYPE_I32, arg.lit.value.i32)
                    );
                }
                case VM_TAG_I64: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TAG_I64,
                        tb_inst_sint(state->func, TB_TYPE_I64, arg.lit.value.i64)
                    );
                }
                case VM_TAG_F32: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TAG_F32,
                        tb_inst_float32(state->func, arg.lit.value.f32)
                    );
                }
                case VM_TAG_F64: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TAG_F64,
                        tb_inst_float64(state->func, arg.lit.value.f64)
                    );
                }
                case VM_TAG_STR: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TAG_STR,
                        vm_tb_dyn_ptr(state, arg.lit.value.str)
                    );
                }
                case VM_TAG_FFI: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TAG_FFI,
                        vm_tb_dyn_ptr(state, arg.lit.value.ffi)
                    );
                }
                default: {
                    __builtin_trap();
                }
            }
            __builtin_trap();
        }
        case VM_ARG_NONE: {
            return vm_tb_dyn_pair_of(
                state,
                VM_TAG_NIL,
                tb_inst_uint(state->func, VM_TB_TYPE_VALUE, 0)
            );
        }
        case VM_ARG_REG: {
            return (vm_tb_dyn_pair_t) {
                .tag = tb_inst_load(state->func, VM_TB_TYPE_TAG, state->regs[arg.reg].tag, 1, false),
                .val = tb_inst_load(state->func, VM_TB_TYPE_VALUE, state->regs[arg.reg].val, 4, false),
            };
        }
        case VM_ARG_FUN: {
            return vm_tb_dyn_pair_of(
                state,
                VM_TAG_FUN,
                tb_inst_uint(state->func, TB_TYPE_I32, arg.func->id)
            );
        }
        default: {
            fprintf(stderr, "\nunhandled arg (type#%zu)\n", (size_t)arg.type);
            __builtin_trap();
        }
    }
}

TB_Node *vm_tb_dyn_tag_eq(vm_tb_dyn_state_t *state, TB_Node *lhs, TB_Node *rhs) {
    return tb_inst_cmp_eq(
        state->func,
        lhs,
        rhs
    );
}

void vm_tb_dyn_set(vm_tb_dyn_state_t *state, vm_arg_t out, vm_tb_dyn_pair_t pair) {
    if (pair.tag) {
        tb_inst_store(
            state->func,
            VM_TB_TYPE_TAG,
            state->regs[out.reg].tag,
            pair.tag,
            1,
            false
        );
    }
    if (pair.val) {
        TB_Node *val = pair.val;;
        if (val->dt.raw != VM_TB_TYPE_VALUE.raw) {
            val = tb_inst_bitcast(
                state->func,
                val,
                VM_TB_TYPE_VALUE
            );
        }
        tb_inst_store(
            state->func,
            VM_TB_TYPE_VALUE,
            state->regs[out.reg].val,
            val,
            4,
            false
        );
    }
}

TB_Node *vm_tb_dyn_lt(TB_Function *f, TB_Node *a, TB_Node *b) {
    return tb_inst_cmp_ilt(f, a, b, true);
}

TB_Node *vm_tb_dyn_le(TB_Function *f, TB_Node *a, TB_Node *b) {
    return tb_inst_cmp_ile(f, a, b, true);
}

TB_Node *vm_tb_dyn_block(vm_tb_dyn_state_t *state, vm_block_t *block) {

    TB_Node **pret = &state->compiled[block->id];

    if (*pret != NULL) {
        return *pret;
    }

    *pret = tb_inst_region(state->func);

    char ret_name[32];

    snprintf(ret_name, 31, "vm_block_%zi", block->id);

    tb_inst_set_region_name(state->func, *pret, -1, ret_name);

    tb_inst_set_control(state->func, *pret);

    for (size_t instr_num = 0; instr_num < block->len; instr_num++) {
        vm_instr_t instr = block->instrs[instr_num];

        switch (instr.op) {
            case VM_IOP_MOVE: {
                vm_tb_dyn_pair_t arg = vm_tb_dyn_arg(state, instr.args[0]);

                vm_tb_dyn_set(state, instr.out, arg);
                break;
            }

            case VM_IOP_ADD:
            case VM_IOP_SUB:
            case VM_IOP_MUL:
            case VM_IOP_DIV:
            case VM_IOP_MOD: {
                vm_tb_dyn_pair_t arg1 = vm_tb_dyn_arg(state, instr.args[0]);
                vm_tb_dyn_pair_t arg2 = vm_tb_dyn_arg(state, instr.args[1]);

                TB_Node *is_number1 = tb_inst_region(state->func);
                TB_Node *is_number2 = tb_inst_region(state->func);
                TB_Node *is_error = tb_inst_region(state->func);

                tb_inst_if(state->func, vm_tb_dyn_tag_eq(state, arg1.tag, tb_inst_uint(state->func, VM_TB_TYPE_TAG, state->number_type)), is_number1, is_error);

                {
                    tb_inst_set_control(state->func, is_error);

                    TB_Node *returns[2] = {
                        tb_inst_bitcast(
                            state->func,
                            vm_tb_dyn_ptr(state, "type error: math"),
                            VM_TB_TYPE_VALUE
                        ),
                        tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_ERROR),
                    };

                    tb_inst_ret(state->func, 2, returns);
                }

                {
                    tb_inst_set_control(state->func, is_number1);

                    tb_inst_if(state->func, vm_tb_dyn_tag_eq(state, arg2.tag, tb_inst_uint(state->func, VM_TB_TYPE_TAG, state->number_type)), is_number2, is_error);
                }

                {
                    tb_inst_set_control(state->func, is_number2);

                    TB_Node *lhs = tb_inst_bitcast(
                        state->func,
                        arg1.val,
                        state->number_dt
                    );
                    TB_Node *rhs = tb_inst_bitcast(
                        state->func,
                        arg2.val,
                        state->number_dt
                    );

                    TB_Node *val = NULL;
                    if (vm_type_eq(state->number_type, VM_TAG_F32) || vm_type_eq(state->number_type, VM_TAG_F64)) {
                        if (instr.op == VM_IOP_ADD) {
                            val = tb_inst_fadd(state->func, lhs, rhs);
                        }
                        if (instr.op == VM_IOP_SUB) {
                            val = tb_inst_fsub(state->func, lhs, rhs);
                        }
                        if (instr.op == VM_IOP_MUL) {
                            val = tb_inst_fmul(state->func, lhs, rhs);
                        }
                        if (instr.op == VM_IOP_DIV) {
                            val = tb_inst_fdiv(state->func, lhs, rhs);
                        }
                        if (instr.op == VM_IOP_MOD) {
                            if (state->number_type == VM_TAG_F64) {

                                TB_Node *bad = tb_inst_region(state->func);
                                tb_inst_set_region_name(state->func, bad, -1, "bad_mod");
                                TB_Node *good = tb_inst_region(state->func);
                                tb_inst_set_region_name(state->func, good, -1, "good_mod");
                                TB_Node *after = tb_inst_region(state->func);
                                tb_inst_set_region_name(state->func, after, -1, "after_mod");
                                TB_Node *raw_div = tb_inst_fdiv(state->func, lhs, rhs);
                                TB_Node *too_low = tb_inst_cmp_flt(state->func, raw_div, tb_inst_float64(state->func, (double)INT64_MIN));
                                TB_Node *too_high = tb_inst_cmp_fgt(state->func, raw_div, tb_inst_float64(state->func, (double)INT64_MAX));
                                TB_Node *is_bad = tb_inst_or(state->func, too_low, too_high);
                                tb_inst_if(state->func, is_bad, bad, good);
                                TB_Node *v1 = NULL;
                                TB_Node *v2 = NULL;
                                {
                                    tb_inst_set_control(state->func, good);
                                    TB_Node *int_div = tb_inst_float2int(state->func, raw_div, TB_TYPE_I64, true);
                                    TB_Node *float_div = tb_inst_int2float(state->func, int_div, TB_TYPE_F64, true);
                                    TB_Node *mul = tb_inst_fmul(state->func, float_div, rhs);
                                    TB_Node *sub = tb_inst_fsub(state->func, lhs, mul);
                                    v1 = sub;
                                    tb_inst_goto(state->func, after);
                                }

                                {
                                    tb_inst_set_control(state->func, bad);
                                    TB_Node *mul = tb_inst_fmul(state->func, raw_div, rhs);
                                    TB_Node *sub = tb_inst_fsub(state->func, lhs, mul);
                                    v2 = sub;
                                    tb_inst_goto(state->func, after);
                                }

                                tb_inst_set_control(state->func, after);

                                val = tb_inst_phi2(state->func, after, v1, v2);
                            }
                            if (state->number_type == VM_TAG_F32) {
                                TB_Node *bad = tb_inst_region(state->func);
                                tb_inst_set_region_name(state->func, bad, -1, "bad_mod");
                                TB_Node *good = tb_inst_region(state->func);
                                tb_inst_set_region_name(state->func, bad, -1, "good_mod");
                                TB_Node *after = tb_inst_region(state->func);
                                tb_inst_set_region_name(state->func, bad, -1, "after_mod");
                                TB_Node *raw_div = tb_inst_fdiv(state->func, lhs, rhs);
                                TB_Node *too_low = tb_inst_cmp_flt(state->func, raw_div, tb_inst_float32(state->func, (float)INT32_MIN));
                                TB_Node *too_high = tb_inst_cmp_fgt(state->func, raw_div, tb_inst_float32(state->func, (float)INT32_MAX));
                                TB_Node *is_bad = tb_inst_or(state->func, too_low, too_high);
                                tb_inst_if(state->func, is_bad, bad, good);
                                TB_Node *v1 = NULL;
                                TB_Node *v2 = NULL;
                                {
                                    tb_inst_set_control(state->func, good);
                                    TB_Node *int_div = tb_inst_float2int(state->func, raw_div, TB_TYPE_I32, true);
                                    TB_Node *float_div = tb_inst_int2float(state->func, int_div, TB_TYPE_F32, true);
                                    TB_Node *mul = tb_inst_fmul(state->func, float_div, rhs);
                                    TB_Node *sub = tb_inst_fsub(state->func, lhs, mul);
                                    v1 = sub;
                                    tb_inst_goto(state->func, after);
                                }

                                {
                                    tb_inst_set_control(state->func, bad);
                                    TB_Node *mul = tb_inst_fmul(state->func, raw_div, rhs);
                                    TB_Node *sub = tb_inst_fsub(state->func, lhs, mul);
                                    v2 = sub;
                                    tb_inst_goto(state->func, after);
                                }

                                tb_inst_set_control(state->func, after);

                                val = tb_inst_phi2(state->func, after, v1, v2);
                            }
                        }
                    } else {
                        if (instr.op == VM_IOP_ADD) {
                            val = tb_inst_add(state->func, lhs, rhs, TB_ARITHMATIC_NONE);
                        }
                        if (instr.op == VM_IOP_SUB) {
                            val = tb_inst_sub(state->func, lhs, rhs, TB_ARITHMATIC_NONE);
                        }
                        if (instr.op == VM_IOP_MUL) {
                            val = tb_inst_mul(state->func, lhs, rhs, TB_ARITHMATIC_NONE);
                        }
                        if (instr.op == VM_IOP_DIV) {
                            val = tb_inst_div(state->func, lhs, rhs, true);
                        }
                        if (instr.op == VM_IOP_MOD) {
                            TB_Node *div = tb_inst_div(state->func, lhs, rhs, true);
                            TB_Node *mul = tb_inst_mul(state->func, div, rhs, true);
                            TB_Node *sub = tb_inst_sub(state->func, lhs, mul, true);
                            val = sub;
                        }
                    }

                    vm_tb_dyn_set(
                        state,
                        instr.out,
                        vm_tb_dyn_pair_of(
                            state,
                            state->number_type,
                            val
                        )
                    );
                }

                break;
            }

            case VM_IOP_STD: {
                vm_tb_dyn_set(
                    state,
                    instr.out,
                    (vm_tb_dyn_pair_t){
                        .val = vm_tb_dyn_ptr(state, state->std),
                        .tag = tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_TAB),
                    }
                );
                break;
            }

            case VM_IOP_TABLE_LEN: {
                vm_tb_dyn_pair_t table = vm_tb_dyn_arg(state, instr.args[0]);

                TB_Node *is_table = tb_inst_region(state->func);
                TB_Node *is_error = tb_inst_region(state->func);

                tb_inst_if(state->func, vm_tb_dyn_tag_eq(state, table.tag, tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_TAB)), is_table, is_error);

                {
                    tb_inst_set_control(state->func, is_error);

                    TB_Node *returns[2] = {
                        tb_inst_bitcast(
                            state->func,
                            vm_tb_dyn_ptr(state, "type error: can get the length of only tables"),
                            VM_TB_TYPE_VALUE
                        ),
                        tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_ERROR),
                    };

                    tb_inst_ret(state->func, 2, returns);
                }

                {
                    tb_inst_set_control(state->func, is_table);

                    TB_Node *len = tb_inst_load(
                        state->func,
                        TB_TYPE_I32,
                        tb_inst_member_access(
                            state->func,
                            table.val,
                            offsetof(vm_table_t, len)
                        ),
                        1,
                        false
                    );

                    if (state->config->use_num == VM_USE_NUM_F32) {
                        len = tb_inst_float2int(state->func, len, TB_TYPE_F32, true);
                    } else if (state->config->use_num == VM_USE_NUM_F64) {
                        len = tb_inst_float2int(state->func, len, TB_TYPE_F64, true);
                    } else if (state->number_dt.raw != TB_TYPE_I32.raw) {
                        len = tb_inst_bitcast(state->func, len, state->number_dt);
                    }

                    vm_tb_dyn_set(
                        state,
                        instr.out,
                        vm_tb_dyn_pair_of(
                            state,
                            state->number_type,
                            len
                        )
                    );
                }

                break;
            }

            case VM_IOP_TABLE_NEW: {
                TB_PrototypeParam proto_ret[1] = {
                    {TB_TYPE_PTR},
                };
                TB_FunctionPrototype *proto = tb_prototype_create(state->mod, VM_TB_CC, 0, NULL, 1, proto_ret, false);
                TB_MultiOutput output = tb_inst_call(
                    state->func,
                    proto,
                    tb_inst_get_symbol_address(state->func, state->vm_table_new),
                    0,
                    NULL
                );
                vm_tb_dyn_set(
                    state,
                    instr.out,
                    vm_tb_dyn_pair_of(
                        state,
                        VM_TAG_TAB,
                        output.single
                    )
                );
                break;
            }

            case VM_IOP_TABLE_SET: {
                vm_tb_dyn_pair_t table = vm_tb_dyn_arg(state, instr.args[0]);
                vm_tb_dyn_pair_t index = vm_tb_dyn_arg(state, instr.args[1]);
                vm_tb_dyn_pair_t value = vm_tb_dyn_arg(state, instr.args[2]);

                TB_Node *is_table = tb_inst_region(state->func);
                TB_Node *is_error = tb_inst_region(state->func);

                tb_inst_if(state->func, vm_tb_dyn_tag_eq(state, table.tag, tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_TAB)), is_table, is_error);

                {
                    tb_inst_set_control(state->func, is_error);

                    TB_Node *returns[2] = {
                        tb_inst_bitcast(
                            state->func,
                            vm_tb_dyn_ptr(state, "type error: only tables can be set"),
                            VM_TB_TYPE_VALUE
                        ),
                        tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_ERROR),
                    };

                    tb_inst_ret(state->func, 2, returns);
                }

                {
                    tb_inst_set_control(state->func, is_table);

                    TB_Node *res = tb_inst_local(state->func, sizeof(vm_pair_t), 8);

                    tb_inst_store(
                        state->func,
                        VM_TB_TYPE_VALUE,
                        tb_inst_member_access(
                            state->func,
                            res,
                            offsetof(vm_pair_t, key_val)
                        ),
                        index.val,
                        4,
                        false
                    );
                    tb_inst_store(
                        state->func,
                        VM_TB_TYPE_TAG,
                        tb_inst_member_access(
                            state->func,
                            res,
                            offsetof(vm_pair_t, key_tag)
                        ),
                        index.tag,
                        4,
                        false
                    );
                    tb_inst_store(
                        state->func,
                        VM_TB_TYPE_VALUE,
                        tb_inst_member_access(
                            state->func,
                            res,
                            offsetof(vm_pair_t, val_val)
                        ),
                        value.val,
                        4,
                        false
                    );
                    tb_inst_store(
                        state->func,
                        VM_TB_TYPE_TAG,
                        tb_inst_member_access(
                            state->func,
                            res,
                            offsetof(vm_pair_t, val_tag)
                        ),
                        value.tag,
                        4,
                        false
                    );

                    TB_PrototypeParam params[2] = {
                        {TB_TYPE_PTR},
                        {TB_TYPE_PTR},
                    };

                    TB_FunctionPrototype *proto = tb_prototype_create(state->mod, VM_TB_CC, 2, params, 0, NULL, false);

                    TB_Node *args[2] = {
                        tb_inst_bitcast(
                            state->func,
                            table.val,
                            TB_TYPE_PTR
                        ),
                        res,
                    };

                    tb_inst_call(
                        state->func,
                        proto,
                        tb_inst_get_symbol_address(state->func, state->vm_table_set_pair),
                        2,
                        args
                    );
                }

                break;
            }

            default: {
                __builtin_trap();
                break;
            }
        }
    }

    vm_branch_t branch = block->branch;

    switch (branch.op) {
        case VM_BOP_RET: {
            vm_tb_dyn_pair_t arg = vm_tb_dyn_arg(state, branch.args[0]);
            TB_Node *returns[2] = {
                arg.val,
                arg.tag,
            };
            tb_inst_ret(state->func, 2, returns);
            break;
        }

        case VM_BOP_JUMP: {
            TB_Node *more = tb_inst_region(state->func);
            tb_inst_goto(state->func, more);

            TB_Node *then = vm_tb_dyn_block(state, branch.targets[0]);
            tb_inst_set_control(state->func, more);
            tb_inst_goto(state->func, then);
            break;
        }

        case VM_BOP_LOAD: {
            TB_Node *more = tb_inst_region(state->func);

            tb_inst_goto(state->func, more);
            TB_Node *then = vm_tb_dyn_block(state, branch.targets[0]);
            tb_inst_set_control(state->func, more);

            TB_Node *closure = vm_tb_dyn_arg(state, branch.args[0]).val;
            TB_Node *index = vm_tb_dyn_arg(state, branch.args[1]).val;

            TB_Node *obj = tb_inst_array_access(
                state->func,
                closure,
                index,
                sizeof(vm_std_value_t)
            );

            TB_Node *val = tb_inst_member_access(
                state->func,
                obj,
                offsetof(vm_std_value_t, value)
            );

            TB_Node *tag = tb_inst_member_access(
                state->func,
                obj,
                offsetof(vm_std_value_t, tag)
            );

            vm_tb_dyn_set(
                state,
                branch.out,
                (vm_tb_dyn_pair_t){
                    .val = tb_inst_load(state->func, VM_TB_TYPE_VALUE, val, 4, false),
                    .tag = tb_inst_load(state->func, VM_TB_TYPE_TAG, tag, 4, false),
                }
            );

            tb_inst_goto(state->func, then);
            break;
        }

        case VM_BOP_GET: {
            vm_tb_dyn_pair_t table = vm_tb_dyn_arg(state, branch.args[0]);
            vm_tb_dyn_pair_t index = vm_tb_dyn_arg(state, branch.args[1]);

            TB_Node *is_table = tb_inst_region(state->func);
            TB_Node *is_error = tb_inst_region(state->func);

            tb_inst_if(state->func, vm_tb_dyn_tag_eq(state, table.tag, tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_TAB)), is_table, is_error);

            TB_Node *next = vm_tb_dyn_block(state, branch.targets[0]);

            {
                tb_inst_set_control(state->func, is_table);

                TB_PrototypeParam get_params[2] = {
                    {TB_TYPE_PTR},
                    {TB_TYPE_PTR},
                };

                TB_FunctionPrototype *get_proto = tb_prototype_create(state->mod, VM_TB_CC, 2, get_params, 0, NULL, false);
                TB_Node *arg2 = tb_inst_local(state->func, sizeof(vm_pair_t), 8);
                tb_inst_store(
                    state->func,
                    VM_TB_TYPE_VALUE,
                    tb_inst_member_access(
                        state->func,
                        arg2,
                        offsetof(vm_pair_t, key_val)
                    ),
                    index.val,
                    4,
                    false
                );
                tb_inst_store(
                    state->func,
                    VM_TB_TYPE_TAG,
                    tb_inst_member_access(
                        state->func,
                        arg2,
                        offsetof(vm_pair_t, key_tag)
                    ),
                    index.tag,
                    4,
                    false
                );

                TB_Node *get_args[2] = {
                    tb_inst_bitcast(
                        state->func,
                        table.val,
                        TB_TYPE_PTR
                    ),
                    arg2,
                };

                tb_inst_call(
                    state->func,
                    get_proto,
                    tb_inst_get_symbol_address(state->func, state->vm_table_get_pair),
                    2,
                    get_args
                );

                TB_Node *val = tb_inst_load(
                    state->func,
                    VM_TB_TYPE_VALUE,
                    tb_inst_member_access(
                        state->func,
                        arg2,
                        offsetof(vm_pair_t, val_val)
                    ),
                    4,
                    false
                );

                TB_Node *tag = tb_inst_load(
                    state->func,
                    VM_TB_TYPE_TAG,
                    tb_inst_member_access(
                        state->func,
                        arg2,
                        offsetof(vm_pair_t, val_tag)
                    ),
                    4,
                    false
                );

                vm_tb_dyn_set(
                    state,
                    branch.out,
                    (vm_tb_dyn_pair_t){
                        .val = val,
                        .tag = tag,
                    }
                );

                tb_inst_if(state->func, vm_tb_dyn_tag_eq(state, tag, tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_ERROR)), is_error, next);
            }

            {
                tb_inst_set_control(state->func, is_error);

                TB_Node *returns[2] = {
                    tb_inst_bitcast(
                        state->func,
                        vm_tb_dyn_ptr(state, "type error: only tables can be indexed"),
                        VM_TB_TYPE_VALUE
                    ),
                    tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_ERROR),
                };

                tb_inst_ret(state->func, 2, returns);
            }

            break;
        }

        case VM_BOP_BB: {
            vm_tb_dyn_pair_t arg = vm_tb_dyn_arg(state, branch.args[0]);

            TB_Node *not_bool = tb_inst_region(state->func);
            TB_Node *is_bool = tb_inst_region(state->func);

            tb_inst_if(state->func, vm_tb_dyn_tag_eq(state, arg.tag, tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_BOOL)), is_bool, not_bool);

            TB_Node *then = vm_tb_dyn_block(state, branch.targets[0]);
            TB_Node *els = vm_tb_dyn_block(state, branch.targets[1]);

            {
                tb_inst_set_control(state->func, not_bool);

                tb_inst_if(state->func, tb_inst_cmp_ne(state->func, arg.tag, tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_NIL)), then, els);
            }

            {
                tb_inst_set_control(state->func, is_bool);

                tb_inst_if(state->func, tb_inst_bitcast(state->func, arg.val, TB_TYPE_BOOL), then, els);
            }

            break;
        }
        case VM_BOP_BEQ: {
            TB_Node *is_same_type = tb_inst_region(state->func);
            TB_Node *is_not_same_type = tb_inst_region(state->func);

            vm_tb_dyn_pair_t arg1 = vm_tb_dyn_arg(state, branch.args[0]);
            vm_tb_dyn_pair_t arg2 = vm_tb_dyn_arg(state, branch.args[1]);
            
            tb_inst_if(state->func, vm_tb_dyn_tag_eq(state, arg1.tag, arg2.tag), is_same_type, is_not_same_type);

            TB_Node *then = vm_tb_dyn_block(state, branch.targets[0]);
            TB_Node *els = vm_tb_dyn_block(state, branch.targets[1]);
            
            {
                TB_Node *is_boolean = tb_inst_region(state->func);
                TB_Node *is_number = tb_inst_region(state->func);
                TB_Node *is_string = tb_inst_region(state->func);
                TB_Node *is_ptr_cmp = tb_inst_region(state->func);
                
                tb_inst_set_control(state->func, is_same_type);

                TB_SwitchEntry keys[6];

                // for (size_t i = 0; i < VM_TAG_MAX; i++) {
                    // keys[i] = (TB_SwitchEntry) {
                    //     .key = i,
                    //     .value = is_not_same_type,
                    // };
                // }

                keys[0].key = VM_TAG_NIL;
                keys[0].value = then;
                keys[1].key = VM_TAG_BOOL;
                keys[1].value = is_boolean;
                keys[2].key = state->number_type;
                keys[2].value = is_number;
                keys[3].key = VM_TAG_STR;
                keys[3].value = is_string;
                keys[4].key = VM_TAG_TAB;
                keys[4].value = is_ptr_cmp;
                keys[5].key = VM_TAG_CLOSURE;
                keys[5].value = is_ptr_cmp;

                tb_inst_branch(state->func, VM_TB_TYPE_TAG, arg1.tag, is_not_same_type, 6, &keys[0]);

                {
                    tb_inst_set_control(state->func, is_boolean);

                    tb_inst_if(
                        state->func,
                        tb_inst_cmp_eq(
                            state->func,
                            tb_inst_bitcast(state->func, arg1.val, TB_TYPE_BOOL),
                            tb_inst_bitcast(state->func, arg2.val, TB_TYPE_BOOL)
                        ),
                        then,
                        els
                    );
                }

                {
                    tb_inst_set_control(state->func, is_number);

                    tb_inst_if(
                        state->func,
                        tb_inst_cmp_eq(
                            state->func,
                            tb_inst_bitcast(state->func, arg1.val, state->number_dt),
                            tb_inst_bitcast(state->func, arg2.val, state->number_dt)
                        ),
                        then,
                        els
                    );
                }

                {
                    tb_inst_set_control(state->func, is_string);

                    TB_PrototypeParam params[2] = {
                        {TB_TYPE_PTR},
                        {TB_TYPE_PTR},
                    };

                    TB_PrototypeParam rets[1] = {
                        {TB_TYPE_BOOL},
                    };

                    TB_FunctionPrototype *proto = tb_prototype_create(state->mod, VM_TB_CC, 2, params, 1, rets, false);

                    TB_Node *args[2] = {
                        tb_inst_bitcast(state->func, arg1.val, TB_TYPE_PTR),
                        tb_inst_bitcast(state->func, arg2.val, TB_TYPE_PTR),
                    };

                    tb_inst_if(
                        state->func,
                        tb_inst_call(
                            state->func,
                            proto,
                            vm_tb_dyn_ptr(state, &vm_tb_str_eq),
                            2,
                            args
                        ).single,
                        then,
                        els
                    );
                }

                {
                    tb_inst_set_control(state->func, is_ptr_cmp);

                    tb_inst_if(
                        state->func,
                        tb_inst_cmp_eq(
                            state->func,
                            tb_inst_bitcast(state->func, arg1.val, TB_TYPE_PTR),
                            tb_inst_bitcast(state->func, arg2.val, TB_TYPE_PTR)
                        ),
                        then,
                        els
                    );
                }
            }

            {
                tb_inst_set_control(state->func, is_not_same_type);
                
                tb_inst_goto(state->func, els);
            }

            break;
        }
        case VM_BOP_BLT:
        case VM_BOP_BLE: {
            TB_Node *is_number1 = tb_inst_region(state->func);
            TB_Node *is_number2 = tb_inst_region(state->func);
            TB_Node *is_error = tb_inst_region(state->func);

            vm_tb_dyn_pair_t arg1 = vm_tb_dyn_arg(state, branch.args[0]);
            vm_tb_dyn_pair_t arg2 = vm_tb_dyn_arg(state, branch.args[1]);

            tb_inst_if(state->func, vm_tb_dyn_tag_eq(state, arg1.tag, tb_inst_uint(state->func, VM_TB_TYPE_TAG, state->number_type)), is_number1, is_error);

            {
                tb_inst_set_control(state->func, is_error);

                TB_Node *returns[2] = {
                    tb_inst_bitcast(
                        state->func,
                        vm_tb_dyn_ptr(state, "type error: branch"),
                        VM_TB_TYPE_VALUE
                    ),
                    tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_ERROR),
                };

                tb_inst_ret(state->func, 2, returns);
            }

            {
                tb_inst_set_control(state->func, is_number1);

                tb_inst_if(state->func, vm_tb_dyn_tag_eq(state, arg2.tag, tb_inst_uint(state->func, VM_TB_TYPE_TAG, state->number_type)), is_number2, is_error);
            }

            {
                TB_Node *then = vm_tb_dyn_block(state, branch.targets[0]);
                TB_Node *els = vm_tb_dyn_block(state, branch.targets[1]);

                tb_inst_set_control(state->func, is_number2);

                TB_Node *(*func)(TB_Function *func, TB_Node *lhs, TB_Node *rhs) = NULL;

                if (vm_type_eq(state->number_type, VM_TAG_F32) || vm_type_eq(state->number_type, VM_TAG_F64)) {
                    if (branch.op == VM_BOP_BEQ) {
                        func = tb_inst_cmp_eq;
                    }
                    if (branch.op == VM_BOP_BLT) {
                        func = tb_inst_cmp_flt;
                    }
                    if (branch.op == VM_BOP_BLE) {
                        func = tb_inst_cmp_fle;
                    }
                } else {
                    if (branch.op == VM_BOP_BEQ) {
                        func = tb_inst_cmp_eq;
                    }
                    if (branch.op == VM_BOP_BLT) {
                        func = vm_tb_dyn_lt;
                    }
                    if (branch.op == VM_BOP_BLE) {
                        func = vm_tb_dyn_le;
                    }
                }

                tb_inst_if(state->func, func(state->func, arg1.val, arg2.val), then, els);
            }

            break;
        }

        case VM_BOP_CALL: {
            vm_tb_dyn_pair_t run = vm_tb_dyn_arg(state, branch.args[0]);

            TB_Node *is_ffi = tb_inst_region(state->func);
            TB_Node *is_not_ffi = tb_inst_region(state->func);
            TB_Node *is_closure = tb_inst_region(state->func);
            TB_Node *is_error = tb_inst_region(state->func);

            tb_inst_if(state->func, vm_tb_dyn_tag_eq(state, run.tag, tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_CLOSURE)), is_closure, is_not_ffi);
            tb_inst_set_control(state->func, is_not_ffi);
            tb_inst_if(state->func, vm_tb_dyn_tag_eq(state, run.tag, tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_FFI)), is_ffi, is_error);

            TB_Node *then = vm_tb_dyn_block(state, branch.targets[0]);

            size_t num_args = 0;
            for (size_t arg = 0; branch.args[arg].type != VM_ARG_NONE; arg++) {
                num_args += 2;
            }

            {
                tb_inst_set_control(state->func, is_ffi);

                TB_Node *call_arg;
                if (num_args == 0) {
                    call_arg = tb_inst_local(state->func, sizeof(vm_std_value_t) * (num_args / 2), 8);
                } else {
                    call_arg = tb_inst_local(state->func, sizeof(vm_std_value_t), 8);
                }

                for (size_t i = 1; branch.args[i].type != VM_ARG_NONE; i++) {
                    TB_Node *head = tb_inst_member_access(state->func, call_arg, sizeof(vm_std_value_t) * (i - 1));
                    vm_tb_dyn_pair_t pair = vm_tb_dyn_arg(state, branch.args[i]);
                    tb_inst_store(
                        state->func,
                        VM_TB_TYPE_VALUE,
                        tb_inst_member_access(state->func, head, offsetof(vm_std_value_t, value)),
                        pair.val,
                        4,
                        false
                    );
                    tb_inst_store(
                        state->func,
                        VM_TB_TYPE_TAG,
                        tb_inst_member_access(state->func, head, offsetof(vm_std_value_t, tag)),
                        pair.tag,
                        4,
                        false
                    );
                }

                TB_Node *end_head = tb_inst_member_access(state->func, call_arg, sizeof(vm_std_value_t) * (num_args / 2 - 1));

                tb_inst_store(
                    state->func,
                    VM_TB_TYPE_TAG,
                    tb_inst_member_access(state->func, end_head, offsetof(vm_std_value_t, tag)),
                    tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_UNK),
                    4,
                    false
                );

                TB_PrototypeParam call_proto_params[2] = {
                    {TB_TYPE_PTR},
                    {TB_TYPE_PTR},
                };

                vm_std_closure_t *closure = vm_malloc(sizeof(vm_std_closure_t));

                closure->config = state->config;
                closure->blocks = state->blocks;

                TB_FunctionPrototype *call_proto = tb_prototype_create(state->mod, VM_TB_CC, 2, call_proto_params, 0, NULL, false);

                TB_Node *call_args[2] = {
                    vm_tb_dyn_ptr(state, closure),
                    call_arg,
                };

                tb_inst_call(
                    state->func,
                    call_proto,
                    run.val,
                    2,
                    call_args
                );

                TB_Node *val = tb_inst_load(
                    state->func,
                    VM_TB_TYPE_VALUE,
                    tb_inst_member_access(
                        state->func,
                        call_arg,
                        offsetof(vm_std_value_t, value)
                    ),
                    4,
                    false
                );

                TB_Node *tag = tb_inst_load(
                    state->func,
                    VM_TB_TYPE_TAG,
                    tb_inst_member_access(
                        state->func,
                        call_arg,
                        offsetof(vm_std_value_t, tag)
                    ),
                    4,
                    false
                );

                vm_tb_dyn_set(
                    state,
                    branch.out,
                    (vm_tb_dyn_pair_t){
                        .val = val,
                        .tag = tag,
                    }
                );

                TB_Node *is_failed_call = tb_inst_region(state->func);

                tb_inst_if(state->func, vm_tb_dyn_tag_eq(state, tag, tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_ERROR)), is_failed_call, then);

                {
                    tb_inst_set_control(state->func, is_failed_call);

                    TB_Node *returns[2] = {
                        val,
                        tag,
                    };

                    tb_inst_ret(state->func, 2, returns);
                }
            }

            {
                tb_inst_set_control(state->func, is_closure);

                TB_PrototypeParam call_proto_params[1] = {
                    {TB_TYPE_PTR},
                };

                TB_PrototypeParam call_proto_rets[2] = {
                    {VM_TB_TYPE_VALUE},
                    {VM_TB_TYPE_TAG},
                };

                TB_FunctionPrototype *call_proto = tb_prototype_create(state->mod, VM_TB_CC, 1, call_proto_params, 2, call_proto_rets, false);

                TB_Node *call_arg = tb_inst_local(state->func, sizeof(vm_std_value_t) * (num_args + 1), 4);
                for (size_t arg = 0; branch.args[arg].type != VM_ARG_NONE; arg++) {
                    vm_tb_dyn_pair_t pair = vm_tb_dyn_arg(state, branch.args[arg]);
                    size_t local = sizeof(vm_std_value_t) * arg;
                    tb_inst_store(
                        state->func,
                        VM_TB_TYPE_VALUE,
                        tb_inst_member_access(state->func, call_arg, local + offsetof(vm_std_value_t, value)),
                        pair.val,
                        4,
                        false
                    );
                    tb_inst_store(
                        state->func,
                        VM_TB_TYPE_TAG,
                        tb_inst_member_access(state->func, call_arg, local + offsetof(vm_std_value_t, tag)),
                        pair.tag,
                        4,
                        false
                    );
                }

                size_t local = sizeof(vm_std_value_t) * num_args;
                tb_inst_store(
                    state->func,
                    VM_TB_TYPE_TAG,
                    tb_inst_member_access(state->func, call_arg, local + offsetof(vm_std_value_t, tag)),
                    tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_UNK),
                    4,
                    false
                );

                TB_MultiOutput out = tb_inst_call(
                    state->func,
                    call_proto,
                    tb_inst_load(
                        state->func,
                        TB_TYPE_PTR,
                        tb_inst_array_access(
                            state->func,
                            vm_tb_dyn_ptr(state, state->codes),
                            tb_inst_load(
                                state->func,
                                TB_TYPE_I32,
                                tb_inst_member_access(
                                    state->func,
                                    run.val,
                                    offsetof(vm_std_value_t, value)
                                ),
                                4,
                                false
                            ),
                            sizeof(void *)
                        ),
                        4,
                        false
                    ),
                    1,
                    &call_arg
                );

                vm_tb_dyn_set(
                    state,
                    branch.out,
                    (vm_tb_dyn_pair_t){
                        .val = out.multiple[0],
                        .tag = out.multiple[1],
                    }
                );

                TB_Node *is_failed_call = tb_inst_region(state->func);

                tb_inst_if(state->func, vm_tb_dyn_tag_eq(state, out.multiple[1], tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_ERROR)), is_failed_call, then);

                {
                    tb_inst_set_control(state->func, is_failed_call);

                    TB_Node *returns[2] = {
                        out.multiple[0],
                        out.multiple[1],
                    };

                    tb_inst_ret(state->func, 2, returns);
                }
            }

            {
                tb_inst_set_control(state->func, is_error);

                TB_Node *returns[2] = {
                    tb_inst_bitcast(
                        state->func,
                        vm_tb_dyn_ptr(state, "type error: only functions can be called"),
                        VM_TB_TYPE_VALUE
                    ),
                    tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_ERROR),
                };

                tb_inst_ret(state->func, 2, returns);
            }

            break;
        }

        default: {
            __builtin_trap();
            break;
        }
    }

    return *pret;
}

void vm_tb_dyn_func(vm_tb_dyn_state_t *state, TB_Function *xfunc, vm_block_t *entry) {
    state->func = xfunc;
    TB_PrototypeParam param_types[1] = {
        {TB_TYPE_PTR},
    };

    TB_PrototypeParam return_types[2] = {
        {VM_TB_TYPE_VALUE},
        {VM_TB_TYPE_TAG},
    };
    TB_FunctionPrototype *proto = tb_prototype_create(state->mod, VM_TB_CC, 1, param_types, 2, return_types, false);
    tb_function_set_prototype(state->func, -1, proto);

    TB_Node *compiled_start = tb_inst_region(state->func);

    state->regs = vm_malloc(sizeof(vm_tb_dyn_pair_t) * entry->nregs);

    for (size_t i = 0; i < entry->nregs; i++) {
        state->regs[i] = (vm_tb_dyn_pair_t) {
            .tag = tb_inst_local(state->func, sizeof(vm_tag_t), 1),
            .val = tb_inst_local(state->func, sizeof(vm_value_t), 4),
        };
    }

    TB_Node *head = tb_inst_param(state->func, 0);
    TB_Node *nil_tag = tb_inst_uint(state->func, VM_TB_TYPE_TAG, VM_TAG_UNK);

    for (size_t i = 0; i < entry->nargs; i++) {
        TB_Node *on_nil = tb_inst_region(state->func);
        TB_Node *on_nonnil = tb_inst_region(state->func);
        TB_Node *on_end = tb_inst_region(state->func);

        vm_arg_t arg = entry->args[i];

        TB_Node *tag = tb_inst_load(
            state->func, 
            VM_TB_TYPE_TAG,
            tb_inst_member_access(state->func, head, offsetof(vm_std_value_t, tag)),
            4,
            false
        );

        tb_inst_if(state->func, tb_inst_cmp_eq(state->func, tag, nil_tag), on_nil, on_nonnil);

        {
            tb_inst_set_control(state->func, on_nil);
            
            vm_tb_dyn_set(
                state,
                arg,
                (vm_tb_dyn_pair_t){
                    .val = NULL,
                    .tag = nil_tag,
                }
            );
        
            tb_inst_goto(state->func, on_end);
        }

        {
            tb_inst_set_control(state->func, on_nonnil);

            vm_tb_dyn_set(
                state,
                arg,
                (vm_tb_dyn_pair_t){
                    .val = tb_inst_load(
                        state->func, 
                        VM_TB_TYPE_VALUE,
                        tb_inst_member_access(state->func, head, offsetof(vm_std_value_t, value)),
                        4,
                        false
                    ),
                    .tag = tag,
                }
            );

            head = tb_inst_member_access(state->func, head, sizeof(vm_std_value_t));
            tb_inst_goto(state->func, on_end);
        }

        tb_inst_set_control(state->func, on_end);
    }

    tb_inst_goto(state->func, compiled_start);

    TB_Node *compiled_first = vm_tb_dyn_block(state, entry);

    tb_inst_set_control(state->func, compiled_start);

    tb_inst_goto(state->func, compiled_first);
}

vm_tb_dyn_func_t *vm_tb_dyn_comp(vm_tb_dyn_state_t *state, vm_block_t *entry) {
    TB_Arena *ir_arena = tb_arena_create(1 << 20);
    TB_Arena *tmp_arena = tb_arena_create(1 << 20);
    TB_Arena *code_arena = tb_arena_create(1 << 20);

    switch (state->config->use_num) {
        case VM_USE_NUM_I8: {
            state->number_dt = TB_TYPE_I8;
            state->number_type = VM_TAG_I8;
            break;
        }
        case VM_USE_NUM_I16: {
            state->number_dt = TB_TYPE_I16;
            state->number_type = VM_TAG_I16;
            break;
        }
        case VM_USE_NUM_I32: {
            state->number_dt = TB_TYPE_I32;
            state->number_type = VM_TAG_I32;
            break;
        }
        case VM_USE_NUM_I64: {
            state->number_dt = TB_TYPE_I64;
            state->number_type = VM_TAG_I64;
            break;
        }
        case VM_USE_NUM_F32: {
            state->number_dt = TB_TYPE_F32;
            state->number_type = VM_TAG_F32;
            break;
        }
        case VM_USE_NUM_F64: {
            state->number_dt = TB_TYPE_F64;
            state->number_type = VM_TAG_F64;
            break;
        }
    }

    state->mod = tb_module_create_for_host(true);

    size_t max = 0;
    for (size_t block_num = 0; block_num < state->blocks->len; block_num++) {
        vm_block_t *block = state->blocks->blocks[block_num];
        if (block->id >= (ptrdiff_t) max) {
            max = block->id + 1;
        }
    }

    state->funcs = vm_malloc(sizeof(TB_Function *) * max);
    memset(state->funcs, 0, sizeof(TB_Function *) * max);

    state->codes = vm_malloc(sizeof(void *) * max);

    state->compiled = vm_malloc(sizeof(TB_Node *) * max);
    memset(state->compiled, 0, sizeof(TB_Node *) * max);

    state->vm_table_new = (TB_Symbol *)tb_extern_create(state->mod, -1, "vm_table_new", TB_EXTERNAL_SO_LOCAL);
    state->vm_table_set_pair = (TB_Symbol *)tb_extern_create(state->mod, -1, "vm_table_set_pair", TB_EXTERNAL_SO_LOCAL);
    state->vm_table_get_pair = (TB_Symbol *)tb_extern_create(state->mod, -1, "vm_table_get_pair", TB_EXTERNAL_SO_LOCAL);
    tb_symbol_bind_ptr(state->vm_table_new, (void *)&vm_table_new);
    tb_symbol_bind_ptr(state->vm_table_set_pair, (void *)&vm_table_set_pair);
    tb_symbol_bind_ptr(state->vm_table_get_pair, (void *)&vm_table_get_pair);

    char entry_buf[64];
    TB_Function *entry_func = NULL;

    for (size_t block_num = 0; block_num < state->blocks->len; block_num++) {
        vm_block_t *block = state->blocks->blocks[block_num];

        if (block->isfunc || block == entry) {
            char buf[64];
            snprintf(buf, 63, "vm_block_func_%zu", block_num);
            TB_Function *cur = tb_function_create(state->mod, -1, buf, TB_LINKAGE_PUBLIC);
            tb_function_set_arenas(cur, tmp_arena, ir_arena);
            if (block == entry) {
                snprintf(entry_buf, 63, "%s", buf);
                entry_func = cur;
            }
            state->funcs[block_num] = cur;
        }
    }

    for (size_t block_num = 0; block_num < state->blocks->len; block_num++) {
        vm_block_t *block = state->blocks->blocks[block_num];

        if (block->isfunc) {
            vm_tb_dyn_func(state, state->funcs[block_num], block);
        }
    }

    TB_Worklist *worklist = tb_worklist_alloc();

    for (size_t block_num = 0; block_num < state->blocks->len; block_num++) {
        vm_block_t *block = state->blocks->blocks[block_num];

        if (block->isfunc) {
            if (state->config->dump_tb) {
                fprintf(stdout, "\n--- tb ---\n");
                tb_print(state->funcs[block_num], tmp_arena);
                fflush(stdout);
            }

            if (state->config->tb_opt) {
                tb_opt(state->funcs[block_num], worklist, false);

                if (state->config->dump_tb_opt) {
                    fprintf(stdout, "\n--- opt tb ---\n");
                    tb_print(state->funcs[block_num], tmp_arena);
                    fflush(stdout);
                }
            }
        }
    }

    void *ret = NULL;
    switch (state->config->target) {
#if defined(EMSCRIPTEN)
        case VM_TARGET_TB: {
            TB_Worklist *worklist = tb_worklist_alloc();
            TB_JIT *jit = tb_jit_begin(state->mod, 1 << 16);

            if (state->config->dump_asm) {
                fprintf(stdout, "\n--- x86asm ---\n");
            }

            TB_FeatureSet features = (TB_FeatureSet){};
            for (size_t block_num = 0; block_num < state->blocks->len; block_num++) {
                TB_Function *func = state->funcs[block_num];
                if (func != NULL) {
                    if (state->config->dump_asm) {
                        TB_FunctionOutput *out = tb_codegen(func, worklist, code_arena, &features, true);
                        tb_output_print_asm(out, stdout);
                    } else {
                        TB_FunctionOutput *out = tb_codegen(func, worklist, code_arena, &features, false);
                        size_t len = 0;
                        uint8_t *code = tb_output_get_code(out, &len);
                        printf("%.*s", (int) len, (char *) code);
                    }
                }
            }
            void *code = tb_jit_place_function(jit, entry_func);

            ret = code;
            break;
        }
        case VM_TARGET_TB_EMCC: {
            TB_CBuffer *cbuf = tb_c_buf_new();
            tb_c_print_prelude(cbuf, state->mod);
            TB_Worklist *worklist = tb_worklist_alloc();
            for (size_t block_num = 0; block_num < state->blocks->len; block_num++) {
                TB_Function *func = state->funcs[block_num];
                if (func != NULL) {
                    tb_c_print_function(cbuf, func, worklist, tmp_arena);
                }
            }
            const char *buf = tb_c_buf_to_data(cbuf);
            void *handle = vm_cache_comp("emcc", state->config->cflags, buf);
            tb_c_data_free(buf);
            ret = vm_cache_dlsym(handle, entry_buf);
            break;
        }
#else
#if defined(VM_USE_TCC)
        case VM_TARGET_TB_TCC: {
            TB_CBuffer *cbuf = tb_c_buf_new();
            tb_c_print_prelude(cbuf, state->mod);
            TB_Worklist *worklist = tb_worklist_alloc();
            for (size_t block_num = 0; block_num < state->blocks->len; block_num++) {
                TB_Function *func = state->funcs[block_num];
                if (func != NULL) {
                    tb_c_print_function(cbuf, func, worklist, tmp_arena);
                }
            }
            const char *buf = tb_c_buf_to_data(cbuf);
            if (state->config->dump_c) {
                printf("\n--- c ---\n%s", buf);
            }
            TCCState *tcc = tcc_new();
            tcc_set_error_func(tcc, 0, vm_tb_tcc_error_func);
            tcc_set_options(tcc, "-nostdlib");
            tcc_set_output_type(tcc, TCC_OUTPUT_MEMORY);
            tcc_compile_string(tcc, buf);
            tcc_add_symbol(tcc, "memmove", &memmove);
            tcc_relocate(tcc);
            tb_c_data_free(buf);
            for (size_t block_num = 0; block_num < state->blocks->len; block_num++) {
                vm_block_t *block = state->blocks->blocks[block_num];
                TB_Function *func = state->funcs[block_num];
                if (block->isfunc) {
                    state->codes[block->id] = tcc_get_symbol(tcc, ((TB_Symbol *)func)->name);
                }
            }
            ret = tcc_get_symbol(tcc, entry_buf);
            break;
        }
#endif
        case VM_TARGET_TB_CC:
        case VM_TARGET_TB_GCC:
        case VM_TARGET_TB_CLANG: {
            TB_CBuffer *cbuf = tb_c_buf_new();
            tb_c_print_prelude(cbuf, state->mod);
            TB_Worklist *worklist = tb_worklist_alloc();
            for (size_t block_num = 0; block_num < state->blocks->len; block_num++) {
                TB_Function *func = state->funcs[block_num];
                if (func != NULL) {
                    tb_c_print_function(cbuf, func, worklist, tmp_arena);
                }
            }
            const char *buf = tb_c_buf_to_data(cbuf);
            if (state->config->dump_c) {
                printf("\n--- c ---\n%s", buf);
            }
            const char *cc_name = NULL;
            switch (state->config->target) {
                case VM_TARGET_TB_CC:
                    cc_name = "cc";
                    break;
                case VM_TARGET_TB_CLANG:
                    cc_name = "clang";
                    break;
                case VM_TARGET_TB_GCC:
                    cc_name = "gcc";
                    break;
                default:
                    break;
            }
            void *handle = vm_cache_comp(cc_name, state->config->cflags, buf);
            tb_c_data_free(buf);

            for (size_t block_num = 0; block_num < state->blocks->len; block_num++) {
                vm_block_t *block = state->blocks->blocks[block_num];
                TB_Function *func = state->funcs[block_num];
                if (block->isfunc) {
                    state->codes[block->id] = vm_cache_dlsym(handle, ((TB_Symbol *)func)->name);
                }
            }

            ret = vm_cache_dlsym(handle, entry_buf);
            break;
        }

        case VM_TARGET_TB: {
            TB_Worklist *worklist = tb_worklist_alloc();
            TB_JIT *jit = tb_jit_begin(state->mod, 1 << 16);

            if (state->config->dump_asm) {
                fprintf(stdout, "\n--- x86asm ---\n");
            }

            TB_FeatureSet features = (TB_FeatureSet){};
            for (size_t block_num = 0; block_num < state->blocks->len; block_num++) {
                TB_Function *func = state->funcs[block_num];
                if (func != NULL) {
                    if (state->config->dump_asm) {
                        TB_FunctionOutput *out = tb_codegen(func, worklist, code_arena, &features, true);
                        tb_output_print_asm(out, stdout);
                    } else {
                        tb_codegen(func, worklist, code_arena, &features, false);
                    }
                }
            }
            for (size_t block_num = 0; block_num < state->blocks->len; block_num++) {
                vm_block_t *block = state->blocks->blocks[block_num];
                TB_Function *func = state->funcs[block_num];
                if (block->isfunc) {
                    state->codes[block->id] = tb_jit_place_function(jit, func);
                }
            }

            ret = tb_jit_place_function(jit, entry_func);
            break;
        }
#endif
        default: {
            break;
        }
    }

    tb_arena_destroy(ir_arena);
    tb_arena_destroy(tmp_arena);
    tb_arena_destroy(code_arena);

    if (ret == NULL) {
        fprintf(stderr, "FAIL!\n");
        __builtin_trap();
    }

    return ret;
}

#endif
