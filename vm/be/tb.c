
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
            return TB_TYPE_PTR;
        }
        case VM_TAG_TAB: {
            return TB_TYPE_PTR;
        }
        case VM_TAG_FFI: {
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
        case VM_ARG_NUM: {
            switch (arg.num.tag) {
                case VM_TAG_I8: {
                    return tb_inst_sint(fun, TB_TYPE_I8, arg.num.value.i8);
                }
                case VM_TAG_I16: {
                    return tb_inst_sint(fun, TB_TYPE_I16, arg.num.value.i16);
                }
                case VM_TAG_I32: {
                    return tb_inst_sint(fun, TB_TYPE_I32, arg.num.value.i32);
                }
                case VM_TAG_I64: {
                    return tb_inst_sint(fun, TB_TYPE_I64, arg.num.value.i64);
                }
                case VM_TAG_F32: {
                    return tb_inst_float32(fun, arg.num.value.f32);
                }
                case VM_TAG_F64: {
                    return tb_inst_float64(fun, arg.num.value.f64);
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
        case VM_ARG_STR: {
            return tb_inst_string(fun, strlen(arg.str) + 1, arg.str);
        }
        case VM_ARG_NIL: {
            return tb_inst_uint(fun, TB_TYPE_PTR, 0);
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

    block->pass = ret;

    if (state->config->dump_ver) {
        fprintf(stdout, "\n--- vmir ---\n");
        vm_print_block(stdout, block);
    }

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
                if (instr.tag == VM_TAG_F32 || instr.tag == VM_TAG_F64) {
                    __builtin_trap();
                }
                TB_Node *value = tb_inst_mod(
                    fun,
                    vm_tb_func_read_arg(fun, regs, instr.args[0]),
                    vm_tb_func_read_arg(fun, regs, instr.args[1]),
                    true
                );
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
            case VM_IOP_NOP: {
                break;
            }
            case VM_IOP_STD: {
                tb_inst_store(
                    fun,
                    TB_TYPE_PTR,
                    regs[instr.out.reg],
                    tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t)state->std),
                    8,
                    false
                );
                break;
            }
            default: {
                vm_print_instr(stderr, instr);
                fprintf(stderr, "\n ^ unhandled instruction\n");
                asm("int3");
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

        case VM_BOP_CALL: {
            size_t nparams = 0;

            for (size_t i = 1; branch.args[i].type != VM_ARG_NONE; i++) {
                nparams += 1;
            }
            TB_Node *val_val = NULL;
            TB_Node *val_tag = NULL;
            if (branch.args[0].type == VM_ARG_RFUNC) {
                TB_Node **args = vm_malloc(sizeof(TB_Node *) * nparams);

                for (size_t i = 1; branch.args[i].type != VM_ARG_NONE; i++) {
                    args[i - 1] = vm_tb_func_read_arg(fun, regs, branch.args[i]);
                }

                vm_rblock_t *rblock = branch.args[0].rfunc;

                TB_PrototypeParam *call_proto_params = vm_malloc(sizeof(TB_PrototypeParam) * rblock->block->nargs);

                for (size_t arg = 0; arg < rblock->block->nargs; arg++) {
                    call_proto_params[arg] = (TB_PrototypeParam){
                        vm_tag_to_tb_type(rblock->regs->tags[rblock->block->args[arg].reg]),
                    };
                }

                TB_PrototypeParam call_proto_rets[2] = {
                    {TB_TYPE_PTR},
                    {TB_TYPE_I32},
                };

                TB_FunctionPrototype *call_proto = tb_prototype_create(state->module, VM_TB_CC, rblock->block->nargs, call_proto_params, 2, call_proto_rets, false);

                val_val = tb_inst_local(fun, sizeof(vm_value_t), 8);
                val_tag = tb_inst_local(fun, sizeof(uint32_t), 4);

                TB_Node *has_cache = tb_inst_region(fun);
                TB_Node *no_cache = tb_inst_region(fun);
                TB_Node *after = tb_inst_region(fun);

                TB_Node *global_ptr = tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t)&rblock->jit);
                TB_Node *global = tb_inst_load(fun, TB_TYPE_PTR, global_ptr, 1, false);

                tb_inst_if(
                    fun,
                    tb_inst_cmp_ne(fun, global, tb_inst_uint(fun, TB_TYPE_PTR, 0)),
                    has_cache,
                    no_cache
                );

                {
                    tb_inst_set_control(fun, no_cache);

                    TB_PrototypeParam comp_args[2] = {
                        {TB_TYPE_PTR},
                    };

                    TB_PrototypeParam comp_ret[1] = {
                        {TB_TYPE_PTR},
                    };

                    TB_FunctionPrototype *comp_proto = tb_prototype_create(state->module, VM_TB_CC, 1, comp_args, 1, comp_ret, false);

                    TB_Node *comp_params[1];

                    comp_params[0] = tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t)rblock);
                    rblock->state = state;

                    TB_MultiOutput multi = tb_inst_call(
                        fun,
                        comp_proto,
                        tb_inst_get_symbol_address(fun, state->vm_tb_rfunc_comp),
                        1,
                        comp_params
                    );

                    TB_Node **got = tb_inst_call(
                                        fun,
                                        call_proto,
                                        multi.single,
                                        rblock->block->nargs,
                                        args
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
                                        rblock->block->nargs,
                                        args
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

                val_tag = tb_inst_load(
                    fun,
                    TB_TYPE_I32,
                    val_tag,
                    4,
                    false
                );

                val_val = tb_inst_load(
                    fun,
                    TB_TYPE_PTR,
                    val_val,
                    8,
                    false
                );
                // printf("rfunc %p\n", res_ptr);
            } else if (branch.args[0].type == VM_ARG_REG) {
                if (branch.args[0].reg_tag == VM_TAG_FFI) {
                    TB_PrototypeParam call_proto_params[1] = {
                        {TB_TYPE_PTR},
                    };

                    TB_FunctionPrototype *call_proto = tb_prototype_create(state->module, VM_TB_CC, 1, call_proto_params, 0, NULL, false);

                    TB_Node *call_arg = tb_inst_local(fun, sizeof(vm_std_value_t) * (nparams + 1), 8);

                    for (size_t i = 1; branch.args[i].type != VM_ARG_NONE; i++) {
                        vm_arg_t arg = branch.args[i];
                        TB_Node *head = tb_inst_member_access(fun, call_arg, sizeof(vm_std_value_t) * (i - 1));
                        vm_tag_t tag = VM_TAG_NIL;
                        if (arg.type == VM_ARG_REG) {
                            tag = arg.reg_tag;
                        } else if (arg.type == VM_ARG_NUM) {
                            tag = arg.num.tag;
                        } else if (arg.type == VM_ARG_STR) {
                            tag = VM_TAG_STR;
                        } else {
                            fprintf(stderr, "ERROR: ");
                            vm_print_branch(stderr, branch);
                            fprintf(stderr, "\n");
                        }
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
                        tb_inst_uint(fun, TB_TYPE_PTR, 0),
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

                    // printf("ffi %p\n", res_ptr);
                } else {
                    fprintf(stderr, "call of ");
                    vm_print_arg(stderr, branch.args[0]);
                    printf("\n");
                    __builtin_trap();
                }
            } else {
                fprintf(stderr, "call of weird thing; stop it\n");
                __builtin_trap();
            }

            // tb_inst_store(
            //     fun,
            //     TB_TYPE_PTR,
            //     regs[branch.out.reg],
            //     val_val,
            //     8,
            //     false);
            TB_PrototypeParam proto_params[2] = {
                {TB_TYPE_PTR},
                {TB_TYPE_PTR},
            };

            TB_PrototypeParam proto_rets[2] = {
                {TB_TYPE_PTR},
                {TB_TYPE_I32},
            };

            TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, 2, proto_params, 2, proto_rets, false);

            vm_tb_comp_state_t *value_state = vm_malloc(sizeof(vm_tb_comp_state_t) * VM_TAG_MAX);

            for (size_t i = 1; i < VM_TAG_MAX; i++) {
                value_state[i].func = &vm_tb_comp_call;
                branch.rtargets[i]->state = state;
                value_state[i].rblock = branch.rtargets[i];
            }

            TB_Node *ptr_state = tb_inst_array_access(
                fun,
                tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t)value_state),
                val_tag,
                sizeof(vm_tb_comp_state_t)
            );

            TB_Node *args_local;
            if (branch.targets[0]->nargs == 0) {
                args_local = tb_inst_uint(fun, TB_TYPE_PTR, 0);
            } else {
                args_local = tb_inst_local(fun, 8 * branch.targets[0]->nargs, 8);
            }

            TB_Node *call_args[2];

            call_args[0] = ptr_state;
            call_args[1] = args_local;

            for (size_t i = 0; i < branch.targets[0]->nargs; i++) {
                vm_arg_t next_arg = branch.targets[0]->args[i];
                if (next_arg.type == VM_ARG_REG && next_arg.reg == branch.out.reg) {
                    tb_inst_store(
                        fun,
                        TB_TYPE_PTR,
                        tb_inst_member_access(fun, args_local, i * 8),
                        val_val,
                        1,
                        false
                    );
                } else {
                    tb_inst_store(
                        fun,
                        vm_tag_to_tb_type(next_arg.reg_tag),
                        tb_inst_member_access(fun, args_local, i * 8),
                        vm_tb_func_read_arg(fun, regs, next_arg),
                        1,
                        false
                    );
                }
            }

            TB_Node **ret_vals = tb_inst_call(
                                     fun,
                                     proto,
                                     tb_inst_load(
                                         fun,
                                         TB_TYPE_PTR,
                                         tb_inst_member_access(fun, ptr_state, offsetof(vm_tb_comp_state_t, func)),
                                         1,
                                         false
                                     ),
                                     2,
                                     call_args
            )
                                     .multiple;

            tb_inst_ret(fun, 2, ret_vals);
            break;
        }

        case VM_BOP_GET: {
            TB_PrototypeParam get_params[2] = {
                {TB_TYPE_PTR},
                {TB_TYPE_PTR},
            };

            TB_FunctionPrototype *get_proto = tb_prototype_create(state->module, VM_TB_CC, 2, get_params, 0, NULL, false);
            TB_Node *arg2 = tb_inst_local(fun, sizeof(vm_pair_t), 8);
            tb_inst_store(
                fun,
                vm_tag_to_tb_type(branch.tag),
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
                tb_inst_uint(fun, TB_TYPE_I32, branch.tag),
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

            TB_Node *val_tag = tb_inst_load(
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

            TB_Node *val_val = tb_inst_load(
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

            TB_PrototypeParam proto_params[2] = {
                {TB_TYPE_PTR},
                {TB_TYPE_PTR},
            };

            TB_PrototypeParam proto_rets[2] = {
                {TB_TYPE_PTR},
                {TB_TYPE_I32},
            };

            TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, 2, proto_params, 2, proto_rets, false);

            vm_tb_comp_state_t *value_state = vm_malloc(sizeof(vm_tb_comp_state_t) * VM_TAG_MAX);

            for (size_t i = 1; i < VM_TAG_MAX; i++) {
                value_state[i].func = &vm_tb_comp_call;
                branch.rtargets[i]->state = state;
                value_state[i].rblock = branch.rtargets[i];
            }

            TB_Node *ptr_state = tb_inst_array_access(
                fun,
                tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t)value_state),
                val_tag,
                sizeof(vm_tb_comp_state_t)
            );

            TB_Node *args_local;
            if (branch.targets[0]->nargs == 0) {
                args_local = tb_inst_uint(fun, TB_TYPE_PTR, 0);
            } else {
                args_local = tb_inst_local(fun, 8 * branch.targets[0]->nargs, 8);
            }

            TB_Node *call_args[2];

            call_args[0] = ptr_state;
            call_args[1] = args_local;

            for (size_t i = 0; i < branch.targets[0]->nargs; i++) {
                vm_arg_t next_arg = branch.targets[0]->args[i];
                if (next_arg.type == VM_ARG_REG && next_arg.reg == branch.out.reg) {
                    tb_inst_store(
                        fun,
                        TB_TYPE_PTR,
                        tb_inst_member_access(fun, args_local, i * 8),
                        val_val,
                        1,
                        false
                    );
                } else {
                    tb_inst_store(
                        fun,
                        vm_tag_to_tb_type(next_arg.reg_tag),
                        tb_inst_member_access(fun, args_local, i * 8),
                        vm_tb_func_read_arg(fun, regs, next_arg),
                        1,
                        false
                    );
                }
            }

            TB_Node **ret_vals = tb_inst_call(
                                     fun,
                                     proto,
                                     tb_inst_load(
                                         fun,
                                         TB_TYPE_PTR,
                                         tb_inst_member_access(fun, ptr_state, offsetof(vm_tb_comp_state_t, func)),
                                         1,
                                         false
                                     ),
                                     2,
                                     call_args
            )
                                     .multiple;

            tb_inst_ret(fun, 2, ret_vals);
            break;
        }

        default: {
            vm_print_branch(stderr, branch);
            fprintf(stderr, "\n ^ unhandled branch\n");
            asm("int3");
            break;
        }
    }

    tb_inst_set_control(fun, old_ctrl);

    return ret;
}

void vm_tb_report_err(const char *str) {
    fprintf(stderr, "error: %s\n", str);
    __builtin_trap();
}

void vm_tb_func_report_error(vm_tb_state_t *state, TB_Function *fun, const char *str) {
    TB_PrototypeParam proto_args[1] = {
        {TB_TYPE_PTR},
    };

    TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, 1, proto_args, 0, NULL, false);

    size_t len = strlen(str) + 3;
    char *str2 = vm_malloc(sizeof(char) * len);
    snprintf(str2, len, "\"%s\"", str);

    void *ext = tb_extern_create(state->module, -1, str2, TB_EXTERNAL_SO_LOCAL);

    tb_symbol_bind_ptr(ext, (void *)str);

    TB_Node *params[1] = {
        tb_inst_load(fun, TB_TYPE_PTR, tb_inst_get_symbol_address(fun, ext), 1, false),
    };

    tb_inst_call(
        fun,
        proto,
        tb_inst_get_symbol_address(fun, state->vm_tb_report_err),
        1,
        params
    );
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
        case VM_TAG_TAB: {
            val.value.table = *(vm_table_t **)value;
            break;
        }
        case VM_TAG_FFI: {
            val.value.all = *(void **)value;
            break;
        }
        default: {
            printf("bad tag: %zu\n", (size_t)tag);
            asm("int3");
            break;
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

    state->state_self = tb_extern_create(mod, -1, "<state>", TB_EXTERNAL_SO_LOCAL);
    state->vm_tb_rfunc_comp = tb_extern_create(mod, -1, "vm_tb_rfunc_comp", TB_EXTERNAL_SO_LOCAL);
    state->vm_table_get_pair = tb_extern_create(mod, -1, "vm_table_get_pair", TB_EXTERNAL_SO_LOCAL);
    state->vm_tb_print = tb_extern_create(mod, -1, "vm_tb_print", TB_EXTERNAL_SO_LOCAL);
    state->vm_tb_report_err = tb_extern_create(mod, -1, "vm_tb_report_err", TB_EXTERNAL_SO_LOCAL);
    tb_symbol_bind_ptr(state->state_self, &state);
    tb_symbol_bind_ptr(state->vm_tb_rfunc_comp, &vm_tb_rfunc_comp);
    tb_symbol_bind_ptr(state->vm_table_get_pair, &vm_table_get_pair);
    tb_symbol_bind_ptr(state->vm_tb_print, &vm_tb_print);
    tb_symbol_bind_ptr(state->vm_tb_report_err, &vm_tb_report_err);
}

vm_std_value_t vm_tb_comp_call(vm_tb_comp_state_t *comp, vm_value_t *args) {
    vm_rblock_t *rblock = comp->rblock;

    // vm_tb_state_t *last_state = rblock->state;

    // vm_tb_state_t *state = vm_malloc(sizeof(vm_tb_state_t));
    // state->std = last_state->std;
    // state->config = last_state->config;

    vm_tb_state_t *state = rblock->state;

    vm_tb_new_module(state);

    state->faults = 0;

    vm_block_t *block = vm_rblock_version(rblock);
    if (block == NULL) {
        vm_print_block(stderr, rblock->block);
        __builtin_trap();
    }
    TB_Function *fun = tb_function_create(state->module, -1, "block", TB_LINKAGE_PRIVATE);

    TB_PrototypeParam proto_params[2] = {
        {TB_TYPE_PTR},
        {TB_TYPE_PTR},
    };

    TB_PrototypeParam proto_rets[2] = {
        {TB_TYPE_PTR},
        {TB_TYPE_I32},
    };

    TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, 2, proto_params, 2, proto_rets, false);
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
            tb_inst_load(
                fun,
                vm_tag_to_tb_type(block->args[i].reg_tag),
                tb_inst_member_access(fun, tb_inst_param(fun, 1), sizeof(vm_value_t) * i),
                1,
                false
            ),
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

    TB_Passes *passes = tb_pass_enter(fun, tb_function_get_arena(fun));
    if (state->config->dump_tb) {
        fprintf(stdout, "\n--- tb ---\n");
        tb_pass_print(passes);
    }
    if (state->config->use_tb_opt) {
        tb_pass_optimize(passes);
        if (state->config->dump_tb_opt) {
            fprintf(stdout, "\n--- opt tb ---\n");
            tb_pass_print(passes);
            fflush(stdout);
        }
    }
    if (state->config->dump_x86) {
        TB_FunctionOutput *out = tb_pass_codegen(passes, true);
        fprintf(stdout, "\n--- x86asm ---\n");
        tb_output_print_asm(out, stdout);
    } else {
        tb_pass_codegen(passes, false);
    }

    tb_pass_exit(passes);

    TB_JIT *jit = tb_jit_begin(state->module, 1 << 16);
    vm_tb_comp_t *new_func = tb_jit_place_function(jit, fun);

    comp->func = new_func;

    // printf("code buf: %p\n", new_func);

    return new_func(NULL, args);
}

void *vm_tb_rfunc_comp(vm_rblock_t *rblock) {
    void *cache = rblock->jit;
    if (cache != NULL) {
#if VM_USE_RECOMPILE
        size_t redo = rblock->redo--;
        if (__builtin_expect(redo != 0, true)) {
            return cache;
        }
#else
        return cache;
#endif
    }

    rblock->count += 1;

    vm_tb_state_t *state = rblock->state;
    state->faults = 0;

    vm_block_t *block = vm_rblock_version(rblock);
    if (block == NULL) {
        vm_print_block(stderr, rblock->block);
        __builtin_trap();
    }
    TB_Function *fun = tb_function_create(state->module, -1, "block", TB_LINKAGE_PRIVATE);

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

    TB_Passes *passes = tb_pass_enter(fun, tb_function_get_arena(fun));
    if (state->config->dump_tb) {
        fprintf(stdout, "\n--- tb ---\n");
        tb_pass_print(passes);
    }
    if (state->config->use_tb_opt) {
        tb_pass_optimize(passes);
        if (state->config->dump_tb_opt) {
            fprintf(stdout, "\n--- opt tb ---\n");
            tb_pass_print(passes);
            fflush(stdout);
        }
    }
    if (state->config->dump_x86) {
        TB_FunctionOutput *out = tb_pass_codegen(passes, true);
        fprintf(stdout, "\n--- x86asm ---\n");
        tb_output_print_asm(out, stdout);
    } else {
        tb_pass_codegen(passes, false);
    }

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

vm_std_value_t vm_tb_run(vm_config_t *config, vm_block_t *block, vm_table_t *std) {
    vm_tb_state_t *state = vm_malloc(sizeof(vm_tb_state_t));
    state->std = std;
    state->config = config;

    vm_tb_new_module(state);

    vm_tb_func_t *fn = (vm_tb_func_t *)vm_tb_full_comp(state, block);
    return fn();
}
