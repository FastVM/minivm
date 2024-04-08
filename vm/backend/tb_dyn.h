
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
    
    TB_Module *mod;
    TB_Function **funcs;

    TB_Symbol *vm_table_new;
    TB_Symbol *vm_table_iset;
    TB_Symbol *vm_table_get_pair;
};

TB_Node *vm_tb_dyn_ptr(TB_Function *func, void *ptr) {
    return tb_inst_uint(func, TB_TYPE_PTR, (uint64_t)(size_t)ptr);
}

vm_tb_dyn_pair_t vm_tb_dyn_arg(vm_tb_dyn_state_t *state, vm_tb_dyn_pair_t *regs, TB_Function *func, vm_arg_t arg) {
    switch (arg.type) {
        case VM_ARG_LIT: {
            switch (vm_type_tag(arg.lit.tag)) {
                case VM_TAG_NIL: {
                    return (vm_tb_dyn_pair_t){
                        .val = tb_inst_uint(func, VM_TB_TYPE_VALUE, 0),
                        .tag = vm_tb_dyn_ptr(func, VM_TYPE_NIL),
                    };
                }
                case VM_TAG_BOOL: {
                    return (vm_tb_dyn_pair_t){
                        .val = tb_inst_bitcast(
                            func,
                            tb_inst_bool(func, arg.lit.value.b),
                            VM_TB_TYPE_VALUE
                        ),
                        .tag = vm_tb_dyn_ptr(func, VM_TYPE_BOOL),
                    };
                }
                case VM_TAG_I8: {
                    return (vm_tb_dyn_pair_t){
                        .val = tb_inst_bitcast(
                            func,
                            tb_inst_sint(func, TB_TYPE_I8, arg.lit.value.i8),
                            VM_TB_TYPE_VALUE
                        ),
                        .tag = vm_tb_dyn_ptr(func, VM_TYPE_I8),
                    };
                }
                case VM_TAG_I16: {
                    return (vm_tb_dyn_pair_t){
                        .val = tb_inst_bitcast(
                            func,
                            tb_inst_sint(func, TB_TYPE_I16, arg.lit.value.i8),
                            VM_TB_TYPE_VALUE
                        ),
                        .tag = vm_tb_dyn_ptr(func, VM_TYPE_I16),
                    };
                }
                case VM_TAG_I32: {
                    return (vm_tb_dyn_pair_t){
                        .val = tb_inst_bitcast(
                            func,
                            tb_inst_sint(func, TB_TYPE_I32, arg.lit.value.i8),
                            VM_TB_TYPE_VALUE
                        ),
                        .tag = vm_tb_dyn_ptr(func, VM_TYPE_I32),
                    };
                }
                case VM_TAG_I64: {
                    return (vm_tb_dyn_pair_t){
                        .val = tb_inst_bitcast(
                            func,
                            tb_inst_sint(func, TB_TYPE_I64, arg.lit.value.i8),
                            VM_TB_TYPE_VALUE
                        ),
                        .tag = vm_tb_dyn_ptr(func, VM_TYPE_I64),
                    };
                }
                case VM_TAG_F32: {
                    return (vm_tb_dyn_pair_t){
                        .val = tb_inst_bitcast(
                            func,
                            tb_inst_sint(func, TB_TYPE_F32, arg.lit.value.i8),
                            VM_TB_TYPE_VALUE
                        ),
                        .tag = vm_tb_dyn_ptr(func, VM_TYPE_F32),
                    };
                }
                case VM_TAG_F64: {
                    return (vm_tb_dyn_pair_t){
                        .val = tb_inst_bitcast(
                            func,
                            tb_inst_sint(func, TB_TYPE_F64, arg.lit.value.i8),
                            VM_TB_TYPE_VALUE
                        ),
                        .tag = vm_tb_dyn_ptr(func, VM_TYPE_F64),
                    };
                }
                case VM_TAG_STR: {
                    return (vm_tb_dyn_pair_t){
                        .val = tb_inst_bitcast(
                            func,
                            tb_inst_uint(func, TB_TYPE_PTR, (uint64_t)(size_t)arg.lit.value.str),
                            VM_TB_TYPE_VALUE
                        ),
                        .tag = vm_tb_dyn_ptr(func, VM_TYPE_FFI),
                    };
                }
                case VM_TAG_FFI: {
                    return (vm_tb_dyn_pair_t){
                        .val = tb_inst_bitcast(
                            func,
                            tb_inst_uint(func, TB_TYPE_PTR, (uint64_t)(size_t)arg.lit.value.str),
                            VM_TB_TYPE_VALUE
                        ),
                        .tag = vm_tb_dyn_ptr(func, VM_TYPE_FFI),
                    };
                }
                default: {
                    __builtin_trap();
                }
            }
            __builtin_trap();
        }
        case VM_ARG_NONE: {
            return (vm_tb_dyn_pair_t){
                .val = tb_inst_uint(func, VM_TB_TYPE_VALUE, 0),
                .tag = vm_tb_dyn_ptr(func, VM_TYPE_NIL),
            };
        }
        case VM_ARG_REG: {
            return (vm_tb_dyn_pair_t){
                .val = tb_inst_load(
                    func,
                    VM_TB_TYPE_VALUE,
                    regs[arg.reg].val,
                    4,
                    false
                ),
                .tag = tb_inst_load(
                    func,
                    TB_TYPE_PTR,
                    regs[arg.reg].tag,
                    4,
                    false
                ),
            };
        }
        case VM_ARG_FUN: {
            return (vm_tb_dyn_pair_t){
                .val = tb_inst_bitcast(
                    func,
                    tb_inst_get_symbol_address(func, (TB_Symbol *) state->funcs[arg.func->id]),
                    VM_TB_TYPE_VALUE
                ),
                .tag = vm_tb_dyn_ptr(func, VM_TYPE_FFI),
            };
        }
        default: {
            fprintf(stderr, "\nunhandled arg (type#%zu)\n", (size_t)arg.type);
            __builtin_trap();
        }
    }
}

void vm_tb_dyn_block(vm_tb_dyn_state_t *state, vm_tb_dyn_pair_t *regs, TB_Function *func, vm_block_t *block) {
    printf("ENTER %zi\n", block->id);

    for (size_t instr_num = 0; instr_num < block->len; instr_num++) {
        vm_instr_t instr = block->instrs[instr_num];

        switch (instr.op) {
            case VM_IOP_MOVE: {
                vm_tb_dyn_pair_t arg = vm_tb_dyn_arg(state, regs, func, instr.args[0]);
                tb_inst_store(
                    func,
                    VM_TB_TYPE_VALUE,
                    regs[instr.out.reg].val,
                    arg.val,
                    4,
                    false
                );
                tb_inst_store(
                    func,
                    TB_TYPE_PTR,
                    regs[instr.out.reg].tag,
                    arg.tag,
                    4,
                    false
                );
                break;
            }

            case VM_IOP_STD: {
                tb_inst_store(
                    func,
                    VM_TB_TYPE_VALUE,
                    regs[instr.out.reg].val,
                    tb_inst_bitcast(
                        func,
                        tb_inst_uint(func, TB_TYPE_PTR, (uint64_t)(size_t)state->std),
                        VM_TB_TYPE_VALUE
                    ),
                    4,
                    false
                );
                tb_inst_store(
                    func,
                    TB_TYPE_PTR,
                    regs[instr.out.reg].tag,
                    vm_tb_dyn_ptr(func, VM_TYPE_TAB),
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
            vm_tb_dyn_pair_t arg = vm_tb_dyn_arg(state, regs, func, block->branch.args[0]);
            TB_Node *returns[2] = {
                arg.val,
                arg.tag,
            };
            tb_inst_ret(func, 2, returns);
            break;
        }

        case VM_BOP_JUMP: {
            vm_tb_dyn_block(state, regs, func, block->branch.targets[0]);
            break;
        }

        case VM_BOP_INDEX: {
            vm_tb_dyn_pair_t table = vm_tb_dyn_arg(state, regs, func, block->branch.args[0]);
            vm_tb_dyn_pair_t index = vm_tb_dyn_arg(state, regs, func, block->branch.args[1]);

            TB_Node *is_table = tb_inst_region(func);
            TB_Node *is_error = tb_inst_region(func);

            tb_inst_if(func, tb_inst_cmp_eq(func, table.tag, vm_tb_dyn_ptr(func, VM_TYPE_FFI)), is_table, is_error);

            {
                tb_inst_set_control(func, is_table);

                TB_PrototypeParam get_params[2] = {
                    {TB_TYPE_PTR},
                    {TB_TYPE_PTR},
                };

                TB_FunctionPrototype *get_proto = tb_prototype_create(state->mod, VM_TB_CC, 2, get_params, 0, NULL, false);
                TB_Node *arg2 = tb_inst_local(func, sizeof(vm_pair_t), 8);
                vm_tb_dyn_pair_t pair = vm_tb_dyn_arg(state, regs, func, block->branch.args[1]);
                tb_inst_store(
                    func,
                    VM_TB_TYPE_VALUE,
                    tb_inst_member_access(
                        func,
                        arg2,
                        offsetof(vm_pair_t, key_val)
                    ),
                    pair.val,
                    4,
                    false
                );
                tb_inst_store(
                    func,
                    TB_TYPE_PTR,
                    tb_inst_member_access(
                        func,
                        arg2,
                        offsetof(vm_pair_t, key_tag)
                    ),
                    pair.tag,
                    4,
                    false
                );
                TB_Node *get_args[2] = {
                    vm_tb_ver_func_read_arg(state, block->branch.args[0]),
                    arg2,
                };

                vm_tb_ver_inst_call(
                    state,
                    get_proto,
                    tb_inst_get_symbol_address(func, state->vm_table_get_pair),
                    2,
                    get_args
                );


                tb_inst_store(
                    func,
                    VM_TB_TYPE_VALUE,
                    regs[block->branch.out.reg].val,
                    tb_inst_load(
                        func,
                        VM_TB_TYPE_VALUE,
                        tb_inst_member_access(
                            func,
                            arg2,
                            offsetof(vm_pair_t, val_val)
                        ),
                        4,
                        false
                    ),
                    4,
                    false
                );

                tb_inst_store(
                    func,
                    TB_TYPE_PTR,
                    regs[block->branch.out.reg].tag,
                    tb_inst_load(
                        func,
                        TB_TYPE_PTR,
                        tb_inst_member_access(
                            func,
                            arg2,
                            offsetof(vm_pair_t, val_tag)
                        ),
                        4,
                        false
                    ),
                    4,
                    false
                );

                vm_tb_dyn_block(state, regs, func, block->branch.targets[0]);
            }

            {
                tb_inst_set_control(func, is_error);

                TB_Node *returns[2] = {
                    tb_inst_bitcast(
                        func,
                        vm_tb_dyn_ptr(func, "type error"),
                        VM_TB_TYPE_VALUE
                    ),
                    vm_tb_dyn_ptr(func, VM_TYPE_ERROR),
                };

                tb_inst_ret(func, 2, returns);
            }

            break;
        }

        case VM_BOP_CALL: {
            vm_tb_dyn_pair_t run = vm_tb_dyn_arg(state, regs, func, block->branch.args[0]);

            TB_Node *is_ffi = tb_inst_region(func);
            TB_Node *is_not_ffi = tb_inst_region(func);
            TB_Node *is_closure = tb_inst_region(func);
            TB_Node *is_error = tb_inst_region(func);
            TB_Node *is_result = tb_inst_region(func);
            
            tb_inst_if(func, tb_inst_cmp_eq(func, run.tag, vm_tb_dyn_ptr(func, VM_TYPE_FFI)), is_ffi, is_not_ffi);
            tb_inst_set_control(func, is_not_ffi);
            tb_inst_if(func, tb_inst_cmp_eq(func, run.tag, vm_tb_dyn_ptr(func, VM_TYPE_FFI)), is_closure, is_error);
        
            size_t num_args = 0;
            for (size_t arg = 0; block->branch.args[arg].type != VM_ARG_NONE; arg++) {
                num_args += 2;
            }

            {
                tb_inst_set_control(func, is_ffi);

                TB_Node *call_arg = tb_inst_local(func, sizeof(vm_std_value_t) * (num_args / 2 + 1), 8);

                for (size_t i = 1; block->branch.args[i].type != VM_ARG_NONE; i++) {
                    TB_Node *head = tb_inst_member_access(func, call_arg, sizeof(vm_std_value_t) * (i - 1));
                    vm_tb_dyn_pair_t pair = vm_tb_dyn_arg(state, regs, func, block->branch.args[i]);
                    tb_inst_store(
                        func,
                        VM_TB_TYPE_VALUE,
                        tb_inst_member_access(func, head, offsetof(vm_std_value_t, value)),
                        pair.val,
                        4,
                        false
                    );
                    tb_inst_store(
                        func,
                        TB_TYPE_PTR,
                        tb_inst_member_access(func, head, offsetof(vm_std_value_t, tag)),
                        pair.tag,
                        1,
                        false
                    );
                }

                TB_Node *end_head = tb_inst_member_access(func, call_arg, sizeof(vm_std_value_t) * (num_args / 2 + 1));

                tb_inst_store(
                    func,
                    TB_TYPE_PTR,
                    tb_inst_member_access(func,
                     end_head, offsetof(vm_std_value_t, tag)),
                    vm_tb_dyn_ptr(func, VM_TYPE_UNK),
                    1,
                    false
                );

                TB_PrototypeParam call_proto_params[2] = {
                    {TB_TYPE_PTR},
                    {TB_TYPE_PTR},
                };

                vm_std_closure_t *closure = vm_malloc(sizeof(vm_std_closure_t));

                TB_FunctionPrototype *call_proto = tb_prototype_create(state->mod, VM_TB_CC, 2, call_proto_params, 0, NULL, false);

                TB_Node *call_args[2] = {
                    vm_tb_dyn_ptr(func, VM_TYPE_FFI),
                    call_arg,
                };

                TB_MultiOutput out = tb_inst_call(
                    func,
                    call_proto,
                    tb_inst_bitcast(func, run.val, TB_TYPE_PTR),
                    2,
                    call_args
                );

                tb_inst_store(
                    func,
                    VM_TB_TYPE_VALUE,
                    regs[block->branch.out.reg].val,
                    tb_inst_load(
                        func,
                        VM_TB_TYPE_VALUE,
                        tb_inst_member_access(
                            func,
                            call_arg,
                            offsetof(vm_std_value_t, value)
                        ),
                        4,
                        false
                    ),
                    4,
                    false
                );
                tb_inst_store(
                    func,
                    TB_TYPE_PTR,
                    regs[block->branch.out.reg].tag,
                    tb_inst_load(
                        func,
                        TB_TYPE_PTR,
                        tb_inst_member_access(
                            func,
                            call_arg,
                            offsetof(vm_std_value_t, tag)
                        ),
                        4,
                        false
                    ),
                    4,
                    false
                );

                tb_inst_if(func, tb_inst_cmp_eq(func, out.multiple[1], vm_tb_dyn_ptr(func, VM_TYPE_ERROR)), is_error, is_result);
            }

            {
                tb_inst_set_control(func, is_closure);

                TB_PrototypeParam *call_proto_params = vm_malloc(sizeof(TB_PrototypeParam) * num_args);

                for (size_t arg = 0; block->branch.args[arg].type != VM_ARG_NONE; arg++) {
                    call_proto_params[arg * 2 + 0] = (TB_PrototypeParam){ VM_TB_TYPE_VALUE };
                    call_proto_params[arg * 2 + 1] = (TB_PrototypeParam){ TB_TYPE_PTR };
                }

                TB_PrototypeParam call_proto_rets[2] = {
                    {VM_TB_TYPE_VALUE},
                    {TB_TYPE_PTR},
                };

                TB_FunctionPrototype *call_proto = tb_prototype_create(state->mod, VM_TB_CC, num_args, call_proto_params, 2, call_proto_rets, false);
                
                TB_Node **call_args = vm_malloc(sizeof(TB_Node *) * num_args);
                for (size_t arg = 0; block->branch.args[arg].type != VM_ARG_NONE; arg++) {
                    vm_tb_dyn_pair_t run = vm_tb_dyn_arg(state, regs, func, block->branch.args[0]);
                    call_args[arg * 2 + 0] = run.val;
                    call_args[arg * 2 + 1] = run.tag;
                }

                TB_MultiOutput out = tb_inst_call(
                    func,
                    call_proto,
                    tb_inst_bitcast(func, run.val, TB_TYPE_PTR),
                    num_args,
                    call_args
                );

                tb_inst_store(
                    func,
                    VM_TB_TYPE_VALUE,
                    regs[block->branch.out.reg].val,
                    out.multiple[1],
                    4,
                    false
                );
                tb_inst_store(
                    func,
                    TB_TYPE_PTR,
                    regs[block->branch.out.reg].tag,
                    out.multiple[1],
                    4,
                    false
                );

                tb_inst_if(func, tb_inst_cmp_eq(func, out.multiple[1], vm_tb_dyn_ptr(func, VM_TYPE_ERROR)), is_error, is_result);
            }

            {
                tb_inst_set_control(func, is_error);

                TB_Node *returns[2] = {
                    tb_inst_bitcast(
                        func,
                        vm_tb_dyn_ptr(func, "type error"),
                        VM_TB_TYPE_VALUE
                    ),
                    vm_tb_dyn_ptr(func, VM_TYPE_ERROR),
                };

                tb_inst_ret(func, 2, returns);
            }

            {
                tb_inst_set_control(func, is_result);

                vm_tb_dyn_block(state, regs, func, block->branch.targets[0]);
            }

            break;
        }

        // case VM_BOP_BLT: {
            
        //     vm_tb_dyn_block(state, regs, func, block);
            
        //     tb_inst_if(func, cond, then, els);
        //     break;
        // }

        default: {
            __builtin_trap();
            break;
        }
    }
 
    printf("EXIT %zi\n", block->id);
}

void vm_tb_dyn_func(vm_tb_dyn_state_t *state, TB_Function *func, vm_block_t *entry) {
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
    tb_function_set_prototype(func, -1, proto, NULL);

    vm_tb_dyn_pair_t *regs = vm_malloc(sizeof(vm_tb_dyn_pair_t) * entry->nregs);

    for (size_t i = 0; i < entry->nregs; i++) {
        regs[i] = (vm_tb_dyn_pair_t){
            .val = tb_inst_local(func, sizeof(vm_value_t), 4),
            .tag = tb_inst_local(func, sizeof(void *), 4),
        };
    }

    for (size_t i = 0; i < entry->nargs; i++) {
        tb_inst_store(
            func,
            VM_TB_TYPE_VALUE,
            regs[i].val,
            tb_inst_param(func, i * 2 + 0),
            4,
            false
        );
        tb_inst_store(
            func,
            TB_TYPE_PTR,
            regs[i].tag,
            tb_inst_param(func, i * 2 + 1),
            4,
            false
        );
    }

    vm_tb_dyn_block(state, regs, func, entry);

    // TB_Node *returns[2] = {
    //     tb_inst_uint(func, VM_TB_TYPE_VALUE, 0),
    //     tb_inst_uint(func, TB_TYPE_PTR, (uint64_t) (size_t) VM_TYPE_NIL),
    // };
    // tb_inst_ret(func, num_returns, returns);
}

vm_tb_dyn_func_t *vm_tb_dyn_comp(vm_tb_dyn_state_t *state, vm_block_t *entry) {
    TB_Arena *tmp_arena = tb_arena_create(1 << 16);

    state->mod = tb_module_create_for_host(true);

    state->funcs = vm_malloc(sizeof(TB_Function *) * state->blocks->len);

    state->vm_table_new = (TB_Symbol *)tb_extern_create(state->mod, -1, "vm_table_new", TB_EXTERNAL_SO_LOCAL);
    state->vm_table_iset = (TB_Symbol *)tb_extern_create(state->mod, -1, "vm_table_iset", TB_EXTERNAL_SO_LOCAL);
    state->vm_table_get_pair = (TB_Symbol *)tb_extern_create(state->mod, -1, "vm_table_get_pair", TB_EXTERNAL_SO_LOCAL);

    memset(state->funcs, 0, sizeof(TB_Function *) * state->blocks->len);

    TB_Function *entry_func = NULL;

    for (size_t block_num = 0; block_num < state->blocks->len; block_num++) {
        vm_block_t *block = state->blocks->blocks[block_num];

        if (block->isfunc || block == entry) {
            TB_Function *cur = tb_function_create(state->mod, -1, "__main__", TB_LINKAGE_PUBLIC);
            if (block == entry) {
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

    void *ret = NULL;
    switch (state->config->target) {
#if defined(VM_USE_GCCJIT)
        case VM_TARGET_TB_GCCJIT: {
            TB_GCCJIT_Module *mod = tb_gcc_module_new(state->mod);
            for (size_t block_num = 0; block_num < state->blocks->len; block_num++) {
                TB_Function *func = state->funcs[block_num];
                if (func != NULL && func != entry_func) {
                    tb_gcc_module_function(mod, entry_func, worklist, tmp_arena);
                }
            }
            TB_GCCJIT_Function *func = tb_gcc_module_function(mod, entry_func, worklist, tmp_arena);
            ret = tb_gcc_function_ptr(func);
            break;
        }
#endif
        default: {
            break;
        }
    }

    if (ret == NULL) {
        fprintf(stderr, "FAIL!\n");
        __builtin_trap();
    }

    return ret;
}

#endif
