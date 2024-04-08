
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

typedef vm_std_value_t VM_CDECL vm_tb_dyn_func_t(void);

struct vm_tb_dyn_pair_t {
    TB_Node *val;
    TB_Node *tag;
};

struct vm_tb_dyn_state_t {
    vm_table_t *std;
    vm_config_t *config;
    vm_blocks_t *blocks;
    vm_tb_dyn_pair_t *regs;
    vm_type_t number_type;

    TB_Function *func;
    TB_Module *mod;
    TB_Function **funcs;
    TB_DataType number_dt;
    TB_Node **compiled;

    TB_Symbol *vm_table_new;
    TB_Symbol *vm_table_iset;
    TB_Symbol *vm_table_get_pair;

};

TB_Node *vm_tb_dyn_ptr(vm_tb_dyn_state_t *state, const void *ptr) {
    return tb_inst_uint(state->func, TB_TYPE_PTR, (uint64_t)(size_t)ptr);
}

vm_tb_dyn_pair_t vm_tb_dyn_pair_of(vm_tb_dyn_state_t *state, vm_type_t type, TB_Node *value) {
    return (vm_tb_dyn_pair_t){
        .tag = vm_tb_dyn_ptr(state, type),
        .val = tb_inst_bitcast(state->func, value, VM_TB_TYPE_VALUE),
    };
}

vm_tb_dyn_pair_t vm_tb_dyn_arg(vm_tb_dyn_state_t *state, vm_arg_t arg) {
    switch (arg.type) {
        case VM_ARG_LIT: {
            switch (vm_type_tag(arg.lit.tag)) {
                case VM_TAG_NIL: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TYPE_NIL,
                        tb_inst_uint(state->func, VM_TB_TYPE_VALUE, 0)
                    );
                }
                case VM_TAG_BOOL: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TYPE_BOOL,
                        tb_inst_bool(state->func, arg.lit.value.b)
                    );
                }
                case VM_TAG_I8: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TYPE_I8,
                        tb_inst_sint(state->func, TB_TYPE_I8, arg.lit.value.i8)
                    );
                }
                case VM_TAG_I16: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TYPE_I8,
                        tb_inst_sint(state->func, TB_TYPE_I16, arg.lit.value.i16)
                    );
                }
                case VM_TAG_I32: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TYPE_I32,
                        tb_inst_sint(state->func, TB_TYPE_I32, arg.lit.value.i32)
                    );
                }
                case VM_TAG_I64: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TYPE_I64,
                        tb_inst_sint(state->func, TB_TYPE_I64, arg.lit.value.i64)
                    );
                }
                case VM_TAG_F32: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TYPE_F32,
                        tb_inst_float32(state->func, arg.lit.value.f32)
                    );
                }
                case VM_TAG_F64: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TYPE_F64,
                        tb_inst_float64(state->func, arg.lit.value.f64)
                    );
                }
                case VM_TAG_STR: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TYPE_STR,
                        vm_tb_dyn_ptr(state, arg.lit.value.str)
                    );
                }
                case VM_TAG_FFI: {
                    return vm_tb_dyn_pair_of(
                        state,
                        VM_TYPE_FFI,
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
                VM_TYPE_NIL,
                tb_inst_uint(state->func, VM_TB_TYPE_VALUE, 0)
            );
        }
        case VM_ARG_REG: {
            return (vm_tb_dyn_pair_t){
                .val = tb_inst_load(
                    state->func,
                    VM_TB_TYPE_VALUE,
                    state->regs[arg.reg].val,
                    4,
                    false
                ),
                .tag = tb_inst_load(
                    state->func,
                    TB_TYPE_PTR,
                    state->regs[arg.reg].tag,
                    4,
                    false
                ),
            };
        }
        case VM_ARG_FUN: {
            return vm_tb_dyn_pair_of(
                state,
                VM_TYPE_FFI,
                tb_inst_get_symbol_address(state->func, (TB_Symbol *)state->funcs[arg.func->id])
            );
        }
        default: {
            fprintf(stderr, "\nunhandled arg (type#%zu)\n", (size_t)arg.type);
            __builtin_trap();
        }
    }
}

void vm_tb_dyn_set(vm_tb_dyn_state_t *state, vm_arg_t out, vm_tb_dyn_pair_t pair) {
    tb_inst_store(
        state->func,
        VM_TB_TYPE_VALUE,
        state->regs[out.reg].val,
        tb_inst_bitcast(
            state->func,
            pair.val,
            VM_TB_TYPE_VALUE
        ),
        4,
        false
    );
    tb_inst_store(
        state->func,
        TB_TYPE_PTR,
        state->regs[out.reg].tag,
        pair.tag,
        4,
        false
    );
}

TB_Node *vm_tb_dyn_block(vm_tb_dyn_state_t *state, vm_block_t *block) {
    printf("ENTER %zi\n", block->id);

    TB_Node **pret = &state->compiled[block->id];

    if (*pret != NULL) {
        return *pret; 
    }
    
    *pret = tb_inst_region(state->func);

    tb_inst_set_control(state->func, *pret);

    for (size_t instr_num = 0; instr_num < block->len; instr_num++) {
        vm_instr_t instr = block->instrs[instr_num];

        switch (instr.op) {
            case VM_IOP_MOVE: {
                vm_tb_dyn_pair_t arg = vm_tb_dyn_arg(state, instr.args[0]);

                vm_tb_dyn_set(state, instr.out, arg);
                break;
            }

            case VM_IOP_ADD: {
                vm_tb_dyn_pair_t arg1 = vm_tb_dyn_arg(state, instr.args[0]);
                vm_tb_dyn_pair_t arg2 = vm_tb_dyn_arg(state, instr.args[1]);

                TB_Node *is_number1 = tb_inst_region(state->func);
                TB_Node *is_number2 = tb_inst_region(state->func);
                TB_Node *is_error = tb_inst_region(state->func);

                tb_inst_if(state->func, tb_inst_cmp_eq(state->func, arg1.tag, vm_tb_dyn_ptr(state, state->number_type)), is_number1, is_error);

                {
                    tb_inst_set_control(state->func, is_error);

                    TB_Node *returns[2] = {
                        tb_inst_bitcast(
                            state->func,
                            vm_tb_dyn_ptr(state, "type error: add"),
                            VM_TB_TYPE_VALUE
                        ),
                        vm_tb_dyn_ptr(state, VM_TYPE_ERROR),
                    };

                    tb_inst_ret(state->func, 2, returns);
                }

                {
                    tb_inst_set_control(state->func, is_number1);

                    tb_inst_if(state->func, tb_inst_cmp_eq(state->func, arg2.tag, vm_tb_dyn_ptr(state, state->number_type)), is_number2, is_error);
                }

                {
                    tb_inst_set_control(state->func, is_number2);

                    vm_tb_dyn_set(
                        state,
                        instr.out,
                        vm_tb_dyn_pair_of(
                            state,
                            state->number_type,
                            tb_inst_add(
                                state->func,
                                tb_inst_bitcast(
                                    state->func,
                                    arg1.val,
                                    state->number_dt
                                ),
                                tb_inst_bitcast(
                                    state->func,
                                    arg2.val,
                                    state->number_dt
                                ),
                                TB_ARITHMATIC_NONE
                            )
                        )
                    );
                }

                break;
            }

            case VM_IOP_STD: {
                tb_inst_store(
                    state->func,
                    VM_TB_TYPE_VALUE,
                    state->regs[instr.out.reg].val,
                    tb_inst_bitcast(
                        state->func,
                        tb_inst_uint(state->func, TB_TYPE_PTR, (uint64_t)(size_t)state->std),
                        VM_TB_TYPE_VALUE
                    ),
                    4,
                    false
                );
                tb_inst_store(
                    state->func,
                    TB_TYPE_PTR,
                    state->regs[instr.out.reg].tag,
                    vm_tb_dyn_ptr(state, VM_TYPE_TAB),
                    4,
                    false
                );
                break;
            }

            default: {
                __builtin_trap();
                break;
            }
        }
    }

    switch (block->branch.op) {
        case VM_BOP_RET: {
            vm_tb_dyn_pair_t arg = vm_tb_dyn_arg(state, block->branch.args[0]);
            TB_Node *returns[2] = {
                arg.val,
                arg.tag,
            };
            tb_inst_ret(state->func, 2, returns);
            break;
        }

        case VM_BOP_JUMP: {
            tb_inst_goto(state->func, vm_tb_dyn_block(state, block->branch.targets[0]));
            break;
        }

        case VM_BOP_INDEX: {
            vm_tb_dyn_pair_t table = vm_tb_dyn_arg(state, block->branch.args[0]);
            vm_tb_dyn_pair_t index = vm_tb_dyn_arg(state, block->branch.args[1]);

            TB_Node *is_table = tb_inst_region(state->func);
            TB_Node *is_result = tb_inst_region(state->func);
            TB_Node *is_error = tb_inst_region(state->func);

            tb_inst_if(state->func, tb_inst_cmp_eq(state->func, table.tag, vm_tb_dyn_ptr(state, VM_TYPE_TAB)), is_table, is_error);

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
                    TB_TYPE_PTR,
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
                    table.val,
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
                    TB_TYPE_PTR,
                    tb_inst_member_access(
                        state->func,
                        arg2,
                        offsetof(vm_pair_t, val_tag)
                    ),
                    4,
                    false
                );

                tb_inst_store(
                    state->func,
                    VM_TB_TYPE_VALUE,
                    state->regs[block->branch.out.reg].val,
                    val,
                    4,
                    false
                );


                tb_inst_store(
                    state->func,
                    TB_TYPE_PTR,
                    state->regs[block->branch.out.reg].tag,
                    tag,
                    4,
                    false
                );

                tb_inst_if(state->func, tb_inst_cmp_eq(state->func, tag, vm_tb_dyn_ptr(state, VM_TYPE_ERROR)), is_error, is_result);
            }

            {
                tb_inst_set_control(state->func, is_error);

                TB_Node *returns[2] = {
                    tb_inst_bitcast(
                        state->func,
                        vm_tb_dyn_ptr(state, "type error: only tables can be indexed"),
                        VM_TB_TYPE_VALUE
                    ),
                    vm_tb_dyn_ptr(state, VM_TYPE_ERROR),
                };

                tb_inst_ret(state->func, 2, returns);
            }

            {
                tb_inst_set_control(state->func, is_result);

                tb_inst_goto(state->func, vm_tb_dyn_block(state, block->branch.targets[0]));
            }

            break;
        }

        case VM_BOP_CALL: {
            vm_tb_dyn_pair_t run = vm_tb_dyn_arg(state, block->branch.args[0]);

            TB_Node *is_ffi = tb_inst_region(state->func);
            TB_Node *is_not_ffi = tb_inst_region(state->func);
            TB_Node *is_closure = tb_inst_region(state->func);
            TB_Node *is_error = tb_inst_region(state->func);
            TB_Node *is_result = tb_inst_region(state->func);

            tb_inst_if(state->func, tb_inst_cmp_eq(state->func, run.tag, vm_tb_dyn_ptr(state, VM_TYPE_FFI)), is_ffi, is_not_ffi);
            tb_inst_set_control(state->func, is_not_ffi);
            tb_inst_if(state->func, tb_inst_cmp_eq(state->func, run.tag, vm_tb_dyn_ptr(state, VM_TYPE_FFI)), is_closure, is_error);

            size_t num_args = 0;
            for (size_t arg = 0; block->branch.args[arg].type != VM_ARG_NONE; arg++) {
                num_args += 2;
            }

            {
                tb_inst_set_control(state->func, is_ffi);

                TB_Node *call_arg = tb_inst_local(state->func, sizeof(vm_std_value_t) * (num_args / 2), 8);

                for (size_t i = 1; block->branch.args[i].type != VM_ARG_NONE; i++) {
                    TB_Node *head = tb_inst_member_access(state->func, call_arg, sizeof(vm_std_value_t) * (i - 1));
                    vm_tb_dyn_pair_t pair = vm_tb_dyn_arg(state, block->branch.args[i]);
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
                        TB_TYPE_PTR,
                        tb_inst_member_access(state->func, head, offsetof(vm_std_value_t, tag)),
                        pair.tag,
                        4,
                        false
                    );
                }

                TB_Node *end_head = tb_inst_member_access(state->func, call_arg, sizeof(vm_std_value_t) * (num_args / 2 - 1));

                tb_inst_store(
                    state->func,
                    TB_TYPE_PTR,
                    tb_inst_member_access(state->func, end_head, offsetof(vm_std_value_t, tag)),
                    vm_tb_dyn_ptr(state, VM_TYPE_UNK),
                    4,
                    false
                );

                TB_PrototypeParam call_proto_params[2] = {
                    {TB_TYPE_PTR},
                    {TB_TYPE_PTR},
                };

                vm_std_closure_t *closure = vm_malloc(sizeof(vm_std_closure_t));

                TB_FunctionPrototype *call_proto = tb_prototype_create(state->mod, VM_TB_CC, 2, call_proto_params, 0, NULL, false);

                TB_Node *call_args[2] = {
                    vm_tb_dyn_ptr(state, VM_TYPE_FFI),
                    call_arg,
                };

                TB_MultiOutput out = tb_inst_call(
                    state->func,
                    call_proto,
                    tb_inst_bitcast(state->func, run.val, TB_TYPE_PTR),
                    2,
                    call_args
                );

                tb_inst_store(
                    state->func,
                    VM_TB_TYPE_VALUE,
                    state->regs[block->branch.out.reg].val,
                    tb_inst_load(
                        state->func,
                        VM_TB_TYPE_VALUE,
                        tb_inst_member_access(
                            state->func,
                            call_arg,
                            offsetof(vm_std_value_t, value)
                        ),
                        4,
                        false
                    ),
                    4,
                    false
                );
                TB_Node *tag = tb_inst_load(
                    state->func,
                    TB_TYPE_PTR,
                    tb_inst_member_access(
                        state->func,
                        call_arg,
                        offsetof(vm_std_value_t, tag)
                    ),
                    4,
                    false
                );
                tb_inst_store(
                    state->func,
                    TB_TYPE_PTR,
                    state->regs[block->branch.out.reg].tag,
                    tag,
                    4,
                    false
                );

                tb_inst_if(state->func, tb_inst_cmp_eq(state->func, tag, vm_tb_dyn_ptr(state, VM_TYPE_ERROR)), is_error, is_result);
            }

            {
                tb_inst_set_control(state->func, is_closure);

                TB_PrototypeParam *call_proto_params = vm_malloc(sizeof(TB_PrototypeParam) * num_args);

                for (size_t arg = 0; block->branch.args[arg].type != VM_ARG_NONE; arg++) {
                    call_proto_params[arg * 2 + 0] = (TB_PrototypeParam){VM_TB_TYPE_VALUE};
                    call_proto_params[arg * 2 + 1] = (TB_PrototypeParam){TB_TYPE_PTR};
                }

                TB_PrototypeParam call_proto_rets[2] = {
                    {VM_TB_TYPE_VALUE},
                    {TB_TYPE_PTR},
                };

                TB_FunctionPrototype *call_proto = tb_prototype_create(state->mod, VM_TB_CC, num_args, call_proto_params, 2, call_proto_rets, false);

                TB_Node **call_args = vm_malloc(sizeof(TB_Node *) * num_args);
                for (size_t arg = 0; block->branch.args[arg].type != VM_ARG_NONE; arg++) {
                    vm_tb_dyn_pair_t run = vm_tb_dyn_arg(state, block->branch.args[0]);
                    call_args[arg * 2 + 0] = run.val;
                    call_args[arg * 2 + 1] = run.tag;
                }

                TB_MultiOutput out = tb_inst_call(
                    state->func,
                    call_proto,
                    tb_inst_bitcast(state->func, run.val, TB_TYPE_PTR),
                    num_args,
                    call_args
                );

                tb_inst_store(
                    state->func,
                    VM_TB_TYPE_VALUE,
                    state->regs[block->branch.out.reg].val,
                    out.multiple[0],
                    4,
                    false
                );

                tb_inst_store(
                    state->func,
                    TB_TYPE_PTR,
                    state->regs[block->branch.out.reg].tag,
                    out.multiple[1],
                    4,
                    false
                );

                tb_inst_if(state->func, tb_inst_cmp_eq(state->func, out.multiple[1], vm_tb_dyn_ptr(state, VM_TYPE_ERROR)), is_error, is_result);
            }

            {
                tb_inst_set_control(state->func, is_error);

                TB_Node *returns[2] = {
                    tb_inst_bitcast(
                        state->func,
                        vm_tb_dyn_ptr(state, "type error: only functions can be called"),
                        VM_TB_TYPE_VALUE
                    ),
                    vm_tb_dyn_ptr(state, VM_TYPE_ERROR),
                };

                tb_inst_ret(state->func, 2, returns);
            }

            {
                tb_inst_set_control(state->func, is_result);

                tb_inst_goto(state->func, vm_tb_dyn_block(state, block->branch.targets[0]));
            }

            break;
        }

        case VM_BOP_BLT: {
            break;
            
        }

        default: {
            __builtin_trap();
            break;
        }
    }

    printf("EXIT %zi\n", block->id);

    return *pret;
}

void vm_tb_dyn_func(vm_tb_dyn_state_t *state, TB_Function *xfunc, vm_block_t *entry) {
    state->func = xfunc;
    size_t num_params = entry->nargs * 2;
    TB_PrototypeParam *param_types = vm_malloc(sizeof(TB_Node *) * num_params);
    for (size_t i = 0; i < entry->nargs; i++) {
        param_types[i * 2 + 0] = (TB_PrototypeParam){VM_TB_TYPE_VALUE};
        param_types[i * 2 + 1] = (TB_PrototypeParam){TB_TYPE_PTR};
    }

    size_t num_returns = 2;
    TB_PrototypeParam return_types[2] = {
        {VM_TB_TYPE_VALUE},
        {TB_TYPE_PTR},
    };
    TB_FunctionPrototype *proto = tb_prototype_create(state->mod, VM_TB_CC, num_params, param_types, num_returns, return_types, false);
    tb_function_set_prototype(state->func, -1, proto, NULL);

    TB_Node *compiled_start = tb_inst_region(state->func);
    tb_inst_goto(state->func, compiled_start);

    state->regs = vm_malloc(sizeof(vm_tb_dyn_pair_t) * entry->nregs);

    for (size_t i = 0; i < entry->nregs; i++) {
        state->regs[i] = (vm_tb_dyn_pair_t){
            .val = tb_inst_local(state->func, sizeof(vm_value_t), 4),
            .tag = tb_inst_local(state->func, sizeof(void *), 4),
        };
    }

    for (size_t i = 0; i < entry->nargs; i++) {
        tb_inst_store(
            state->func,
            VM_TB_TYPE_VALUE,
            state->regs[i].val,
            tb_inst_param(state->func, i * 2 + 0),
            4,
            false
        );
        tb_inst_store(
            state->func,
            TB_TYPE_PTR,
            state->regs[i].tag,
            tb_inst_param(state->func, i * 2 + 1),
            4,
            false
        );
    }

    TB_Node *compiled_first = vm_tb_dyn_block(state, entry);

    tb_inst_set_control(state->func, compiled_start);

    tb_inst_goto(state->func, compiled_first);

    // TB_Node *returns[2] = {
    //     tb_inst_uint(state->func, VM_TB_TYPE_VALUE, 0),
    //     tb_inst_uint(state->func, TB_TYPE_PTR, (uint64_t) (size_t) VM_TYPE_NIL),
    // };
    // tb_inst_ret(state->func, num_returns, returns);
}

vm_tb_dyn_func_t *vm_tb_dyn_comp(vm_tb_dyn_state_t *state, vm_block_t *entry) {
    TB_Arena *tmp_arena = tb_arena_create(1 << 16);

    switch (state->config->use_num) {
        case VM_USE_NUM_I8: {
            state->number_dt = TB_TYPE_I8;
            state->number_type = VM_TYPE_I8;
            break;
        }
        case VM_USE_NUM_I16: {
            state->number_dt = TB_TYPE_I16;
            state->number_type = VM_TYPE_I16;
            break;
        }
        case VM_USE_NUM_I32: {
            state->number_dt = TB_TYPE_I32;
            state->number_type = VM_TYPE_I32;
            break;
        }
        case VM_USE_NUM_I64: {
            state->number_dt = TB_TYPE_I64;
            state->number_type = VM_TYPE_I64;
            break;
        }
        case VM_USE_NUM_F32: {
            state->number_dt = TB_TYPE_F32;
            state->number_type = VM_TYPE_F32;
            break;
        }
        case VM_USE_NUM_F64: {
            state->number_dt = TB_TYPE_F64;
            state->number_type = VM_TYPE_F64;
            break;
        }
    }

    state->mod = tb_module_create_for_host(true);

    state->funcs = vm_malloc(sizeof(TB_Function *) * state->blocks->len);
    state->compiled = vm_malloc(sizeof(TB_Node *) * state->blocks->len);

    state->vm_table_new = (TB_Symbol *)tb_extern_create(state->mod, -1, "vm_table_new", TB_EXTERNAL_SO_LOCAL);
    state->vm_table_iset = (TB_Symbol *)tb_extern_create(state->mod, -1, "vm_table_iset", TB_EXTERNAL_SO_LOCAL);
    state->vm_table_get_pair = (TB_Symbol *)tb_extern_create(state->mod, -1, "vm_table_get_pair", TB_EXTERNAL_SO_LOCAL);
    tb_symbol_bind_ptr(state->vm_table_new, (void *)&vm_table_new);
    tb_symbol_bind_ptr(state->vm_table_iset, (void *)&vm_table_iset);
    tb_symbol_bind_ptr(state->vm_table_get_pair, (void *)&vm_table_get_pair);

    memset(state->funcs, 0, sizeof(TB_Function *) * state->blocks->len);

    char entry_buf[64];
    TB_Function *entry_func = NULL;

    for (size_t block_num = 0; block_num < state->blocks->len; block_num++) {
        vm_block_t *block = state->blocks->blocks[block_num];

        if (block->isfunc || block == entry) {
            char buf[64];
            snprintf(buf, 63, "vm_block_func_%zu", block_num);
            TB_Function *cur = tb_function_create(state->mod, -1, buf, TB_LINKAGE_PUBLIC);
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

    void *ret = NULL;
    switch (state->config->target) {
#if defined(EMSCRIPTEN)
#else
#if defined(VM_USE_GCCJIT)
        case VM_TARGET_TB_GCCJIT: {
            TB_GCCJIT_Module *mod = tb_gcc_module_new(state->mod);
            for (size_t block_num = 0; block_num < state->blocks->len; block_num++) {
                TB_Function *func = state->funcs[block_num];
                if (state->func != NULL && func != entry_func) {
                    TB_Worklist *worklist = tb_worklist_alloc();
                    tb_gcc_module_function(mod, entry_func, worklist, tmp_arena);
                }
            }
            TB_Worklist *worklist = tb_worklist_alloc();
            TB_GCCJIT_Function *func = tb_gcc_module_function(mod, entry_func, worklist, tmp_arena);
            ret = tb_gcc_function_ptr(func);
            break;
        }
#endif
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
            if (state->config->dump_asm) {
                printf("\n--- c ---\n%s", buf);
            }
            TCCState *state = tcc_new();
            tcc_set_error_func(state, 0, vm_tb_tcc_error_func);
            tcc_set_options(state, "-nostdlib");
            tcc_set_output_type(state, TCC_OUTPUT_MEMORY);
            tcc_compile_string(state, buf);
            tcc_relocate(state);
            tb_c_data_free(buf);
            ret = tcc_get_symbol(state, entry_buf);
            break;
        }
#endif
        case VM_TARGET_TB_CC:
        case VM_TARGET_TB_GCC:
        case VM_TARGET_TB_CLANG: {
            TB_CBuffer *cbuf = tb_c_buf_new();
            tb_c_print_prelude(cbuf, state->mod);
            for (size_t block_num = 0; block_num < state->blocks->len; block_num++) {
                TB_Function *func = state->funcs[block_num];
                if (state->func != NULL) {
                    TB_Worklist *worklist = tb_worklist_alloc();
                    tb_c_print_function(cbuf, state->func, worklist, tmp_arena);
                }
            }
            const char *buf = tb_c_buf_to_data(cbuf);
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
            void *code = vm_cache_comp(cc_name, buf, entry_buf);
            tb_c_data_free(buf);
            ret = code;
            break;
        }
#endif
        default: {
            break;
        }
    }

    // printf("%zu\n", ret);

    if (ret == NULL) {
        fprintf(stderr, "FAIL!\n");
        __builtin_trap();
    }

    return ret;
}


#endif
