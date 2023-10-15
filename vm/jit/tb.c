
#include "./tb.h"

#include "../../tb/include/tb.h"

#define VM_TB_CC TB_CDECL
// #define VM_TB_CC TB_STDCALL

vm_tb_cache_t *vm_tb_cache_new(void) {
    vm_tb_cache_t *cache = vm_malloc(sizeof(vm_tb_cache_t));
    *cache = (vm_tb_cache_t){0};
    return cache;
}

vm_block_t *vm_tb_rblock_version(vm_rblock_t *rblock) {
    void *cache = vm_cache_get(&rblock->block->cache, rblock);
    if (cache != NULL) {
        return cache;
    }
    vm_block_t *ret = vm_malloc(sizeof(vm_block_t));
    vm_cache_set(&rblock->block->cache, rblock, ret);
    vm_tags_t *regs = vm_rblock_regs_dup(rblock->regs, 256);
    *ret = *rblock->block;
    ret->label = -1;
    ret->instrs = vm_malloc(sizeof(vm_instr_t) * rblock->block->len);
    ret->args = vm_malloc(sizeof(vm_arg_t) * ret->nargs);
    ret->mark = false;
    for (size_t i = 0; i < ret->nargs; i++) {
        ret->args[i] = rblock->block->args[i];
        if (ret->args[i].type != VM_ARG_REG) {
            __builtin_trap();
        }
    }
    for (size_t ninstr = 0; ninstr < rblock->block->len; ninstr++) {
        vm_instr_t instr = vm_rblock_type_specialize_instr(regs, rblock->block->instrs[ninstr]);
        if (!vm_rblock_type_check_instr(regs, instr)) return NULL;
        for (size_t i = 0; instr.args[i].type != VM_ARG_NONE; i++) {
            if (instr.args[i].type == VM_ARG_REG) {
                instr.args[i].reg_tag = regs->tags[instr.args[i].reg];
            }
        }
        if (instr.op == VM_IOP_SET) {
            if (instr.args[1].type == VM_ARG_REG) {
                instr.args[3] = (vm_arg_t) {
                    .type = VM_ARG_TAG,
                    .tag = instr.args[1].reg_tag,
                };
            } else if (instr.args[1].type == VM_ARG_NUM) {
                instr.args[3] = (vm_arg_t) {
                    .type = VM_ARG_TAG,
                    .tag = VM_TAG_F64,
                };
            }
            if (instr.args[2].type == VM_ARG_REG) {
                instr.args[4] = (vm_arg_t) {
                    .type = VM_ARG_TAG,
                    .tag = instr.args[2].reg_tag,
                };
            } else if (instr.args[2].type == VM_ARG_NUM) {
                instr.args[4] = (vm_arg_t) {
                    .type = VM_ARG_TAG,
                    .tag = VM_TAG_F64,
                };
            }
        }
        ret->instrs[ninstr] = instr;
        if (instr.out.type == VM_ARG_REG) {
            regs->tags[instr.out.reg] = instr.tag;
        }
    }
    vm_branch_t branch = vm_rblock_type_specialize_branch(regs, rblock->block->branch);
    if (!vm_rblock_type_check_branch(regs, branch)) return NULL;
    for (size_t i = 0; branch.args[i].type != VM_ARG_NONE; i++) {
        if (branch.args[i].type == VM_ARG_REG) {
            branch.args[i].reg_tag = regs->tags[branch.args[i].reg];
        }
    }
    switch (branch.op) {
        case VM_BOP_GET: {
            if (branch.args[1].type == VM_ARG_REG) {
                branch.tag = regs->tags[branch.args[1].reg];
            } else if (branch.args[1].type == VM_ARG_NUM) {
                branch.tag = VM_TAG_F64;
            }
            if (branch.args[1].type == VM_ARG_REG) {
                branch.args[3] = (vm_arg_t) {
                    .type = VM_ARG_TAG,
                    .tag = branch.args[1].reg_tag,
                };
            } else if (branch.args[1].type == VM_ARG_NUM) {
                branch.args[3] = (vm_arg_t) {
                    .type = VM_ARG_TAG,
                    .tag = VM_TAG_F64,
                };
            }
            vm_block_t *from = branch.targets[0];
            for (size_t i = 0; i < from->nargs; i++) {
                vm_arg_t *arg = &from->args[i];
                if (arg->type == VM_ARG_REG) {
                    if (arg->reg != branch.out.reg) {
                        arg->reg_tag = regs->tags[arg->reg];
                    }
                }
            }
            for (size_t i = 1; i < VM_TAG_MAX; i++) {
                regs->tags[branch.out.reg] = i;
                branch.rtargets[i] = vm_rblock_new(from, vm_rblock_regs_dup(regs, 256));
            }
            break;
        }
        case VM_BOP_CALL: {
            vm_tags_t *regs2 = vm_rblock_regs_empty(256);
            for (size_t i = 0; branch.args[i].type != VM_ARG_NONE; i++) {
                if (branch.args[i].type == VM_ARG_REG) {
                    regs2->tags[i] = branch.args[i].reg_tag;
                }
            }
            if (branch.args[0].type == VM_ARG_FUNC) {
                branch.args[0] = (vm_arg_t) {
                    .type = VM_ARG_RFUNC,
                    .rfunc = vm_rblock_new(branch.args[0].func, regs2),
                };
            }
            vm_block_t *from = branch.targets[0];
            for (size_t i = 0; i < from->nargs; i++) {
                vm_arg_t *arg = &from->args[i];
                if (arg->type == VM_ARG_REG) {
                    if (arg->reg != branch.out.reg) {
                        arg->reg_tag = regs->tags[arg->reg];
                    }
                }
            }
            for (size_t i = 1; i < VM_TAG_MAX; i++) {
                regs->tags[branch.out.reg] = i;
                branch.rtargets[i] = vm_rblock_new(from, vm_rblock_regs_dup(regs, 256));
            }
            break;
        }
        case VM_BOP_JUMP: {
            branch.targets[0] = vm_tb_rblock_version(vm_rblock_new(branch.targets[0], vm_rblock_regs_dup(regs, 256)));
            if (branch.targets[0] == NULL) {
                return NULL;
            }
            break;
        }
        case VM_BOP_BB:
        case VM_BOP_BEQ:
        case VM_BOP_BLT: {
            branch.targets[0] = vm_tb_rblock_version(vm_rblock_new(branch.targets[0], vm_rblock_regs_dup(regs, 256)));
            branch.targets[1] = vm_tb_rblock_version(vm_rblock_new(branch.targets[1], vm_rblock_regs_dup(regs, 256)));
            if (branch.targets[0] == NULL) {
                return NULL;
            }
            if (branch.targets[1] == NULL) {
                return NULL;
            }
            break;
        }
        case VM_BOP_RET: {
            break;
        }
        default: {
            __builtin_trap();
        }
    }
    ret->branch = branch;
    for (size_t i = 0; i < ret->nargs; i++) {
        if (ret->args[i].type == VM_ARG_REG) {
            ret->args[i].reg_tag = regs->tags[ret->args[i].reg];
        }
    }
    return ret;
}

TB_DataType vm_tb_func_tag(TB_Function *fun, vm_tag_t tag) {
    switch (tag) {
    case VM_TAG_UNK: {
        return TB_TYPE_PTR;
    }
    case VM_TAG_NIL: {
        return TB_TYPE_PTR;
    }
    case VM_TAG_BOOL: {
        return TB_TYPE_BOOL;
    }
    case VM_TAG_F64: {
        return TB_TYPE_F64;
    }
    case VM_TAG_I64: {
        return TB_TYPE_I64;
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
        fprintf(stderr, "\n ^ unhandled tag #%zu\n", (size_t) tag);
        asm("int3");
    }
    }
}

TB_Node *vm_tb_func_write_arg(TB_Function* fun, TB_Node **regs, vm_arg_t arg) {
    switch (arg.type) {
        case VM_ARG_REG: {
            return regs[arg.reg];
        }
        default: {
            vm_print_arg(stderr, arg);
            fprintf(stderr, "\n ^ unhandled arg (type#%zu)\n", (size_t) arg.type);
            asm("int3");
        }
    }
}

TB_Node *vm_tb_func_read_arg(TB_Function* fun, TB_Node **regs, vm_arg_t arg) {
    switch (arg.type) {
        case VM_ARG_NUM: {
            return tb_inst_float64(fun, arg.num);
        }
        case VM_ARG_REG: {
            return tb_inst_load(fun, vm_tb_func_tag(fun, arg.reg_tag), regs[arg.reg], 1, false);
        }
        case VM_ARG_STR: {
            return tb_inst_string(fun, strlen(arg.str), arg.str);
        }
        default: {
            vm_print_arg(stderr, arg);
            fprintf(stderr, "\n ^ unhandled arg (type#%zu)\n", (size_t) arg.type);
            asm("int3");
        }
    }
}

TB_Node *vm_tb_func_body(vm_tb_state_t *state, TB_Module *module, TB_Function* fun, TB_Node **args, vm_rblock_t *rblock) {
    TB_Node *ctrl = tb_inst_get_control(fun);
    
    TB_Node *ret = tb_inst_region(fun);

    tb_inst_set_control(fun, ret);
    
    TB_PrototypeParam comp_args[2] = {
        { TB_TYPE_PTR },
        { TB_TYPE_PTR },
    };

    TB_PrototypeParam comp_rets[1] = {
        { TB_TYPE_PTR },
    };

    TB_FunctionPrototype *comp_proto = tb_prototype_create(module, VM_TB_CC, 2, comp_args, 1, comp_rets, false);

    TB_Node *comp_params[2];

    comp_params[0] = tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t) state); 
    comp_params[1] = tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t) rblock); 

    TB_MultiOutput multi = tb_inst_call(
        fun,
        comp_proto,
        tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t) &vm_tb_rfunc_comp),
        2,
        comp_params
    );

    TB_Node *node = multi.single; 

    TB_PrototypeParam *call_params = vm_malloc(sizeof(TB_PrototypeParam) * (rblock->block->nargs + 1));

    call_params[0] = (TB_PrototypeParam) { TB_TYPE_PTR };

    for (size_t arg = 0; arg < rblock->block->nargs; arg++) {
        // vm_print_arg(stdout, rblock->block->args[arg]);
        // printf(" <-- arg#%zu of ", arg);
        // vm_print_tag(stdout, rblock->regs->tags[rblock->block->args[arg].reg]);
        // printf("\n");
        call_params[arg + 1] = (TB_PrototypeParam) {
            vm_tb_func_tag(fun, rblock->regs->tags[rblock->block->args[arg].reg]),
        };
    }

    TB_FunctionPrototype *call_proto = tb_prototype_create(module, VM_TB_CC, rblock->block->nargs + 1, call_params, 0, NULL, false);

    TB_Node **call_args = vm_malloc(sizeof(TB_Node *) * (rblock->block->nargs + 1));

    call_args[0] = tb_inst_param(fun, 0);

    for (size_t i = 0; i < rblock->block->nargs; i++) {
        call_args[i + 1] = args[i];
    }

    tb_inst_call(
        fun,
        call_proto,
        node,
        rblock->block->nargs + 1,
        call_args
    );

    tb_inst_ret(fun, 0, NULL);

    tb_inst_set_control(fun, ctrl);

    return ret;
}

void vm_tb_func_body_once(vm_tb_state_t *state, TB_Module *module, TB_Function* fun, TB_Node **args, vm_block_t *block) {
    fprintf(stdout, "\n--- vmir ---\n");
    vm_print_block(stdout, block);

    TB_Node **regs = vm_malloc(sizeof(TB_Node *) * block->nregs);

    for (size_t i = 0; i < block->nregs; i++) {
        regs[i] = tb_inst_local(fun, 8, 8);
    }

    for (size_t i = 0; i < block->nargs; i++) {
        tb_inst_store(
            fun,
            TB_TYPE_PTR,
            regs[i + 1],
            tb_inst_load(
                fun,
                TB_TYPE_PTR,
                args[i],
                8,
                false
            ),
            8,
            false
        );
    }

    for (size_t n = 0; n < block->len; n++) {
        vm_instr_t instr = block->instrs[n];
        switch (instr.op)
        {
        case VM_IOP_MOVE: {
            tb_inst_store(
                fun,
                vm_tb_func_tag(fun, instr.tag),
                vm_tb_func_write_arg(fun, regs, instr.out),
                vm_tb_func_read_arg(fun, regs, instr.args[0]),
                8,
                false
            );
            break;
        }
        case VM_IOP_STD: {
            tb_inst_store(
                fun,
                vm_tb_func_tag(fun, instr.tag),
                vm_tb_func_write_arg(fun, regs, instr.out),
                tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t) state->std),
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
        case VM_BOP_RET: {
            tb_inst_store(
                fun,
                TB_TYPE_I32,
                tb_inst_member_access(
                    fun,
                    tb_inst_param(fun, 0),
                    offsetof(vm_std_value_t, tag)
                ),
                tb_inst_sint(fun, TB_TYPE_I32, branch.tag),
                1,
                false
            );
            tb_inst_store(
                fun,
                vm_tb_func_tag(fun, branch.tag),
                tb_inst_member_access(
                    fun,
                    tb_inst_param(fun, 0),
                    offsetof(vm_std_value_t, value)
                ),
                vm_tb_func_read_arg(fun, regs, branch.args[0]),
                1,
                false
            );
            
            tb_inst_ret(fun, 0, NULL);
            break;
        }

        case VM_BOP_CALL: {
            TB_PrototypeParam *call_args = vm_malloc(sizeof(TB_PrototypeParam) * (block->nargs + 1));

            call_args[0] = (TB_PrototypeParam) { TB_TYPE_PTR };

            for (size_t arg = 0; arg < block->nargs; arg++) {
                call_args[arg + 1] = (TB_PrototypeParam) {
                    vm_tb_func_tag(fun, block->args[arg].tag),
                };
            }

            TB_FunctionPrototype *call_proto = tb_prototype_create(module, VM_TB_CC, block->nargs + 1, call_args, 0, NULL, false);

            TB_Node **params = vm_malloc(sizeof(TB_Node *) * (block->nargs + 1));

            params[0] = tb_inst_local(fun, sizeof(vm_std_value_t), 8);

            for (size_t i = 1; branch.args[i].type != VM_ARG_NONE; i++) {
                params[i - 1] = vm_tb_func_read_arg(fun, regs, branch.args[i]);
            }

            TB_Node *call_func = NULL;
            if (branch.args[0].type == VM_ARG_RFUNC) {
                call_func = tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t) vm_tb_rfunc_comp(state, branch.args[0].rfunc));
            } else {
                call_func = vm_tb_func_read_arg(fun, regs, branch.args[0]);
            }

            tb_inst_call(
                fun,
                call_proto,
                call_func,
                block->nargs,
                params
            );
            
            TB_Node *val_tag = tb_inst_load(
                fun,
                TB_TYPE_I32,
                tb_inst_member_access(
                    fun,
                    params[0],
                    offsetof(vm_std_value_t, tag)
                ),
                4,
                false
            );

            TB_SwitchEntry keys[VM_TAG_MAX - 1];
            for (size_t i = 1; i < VM_TAG_MAX; i++) {
                keys[i - 1].key = i;
                TB_Node **next_args = vm_malloc(sizeof(TB_Node *) * branch.targets[0]->nargs);
                for (size_t j = 0; j < branch.targets[0]->nargs; j++) {
                    vm_arg_t next_arg = branch.targets[0]->args[j];
                    if (next_arg.type == VM_ARG_REG && next_arg.reg == branch.out.reg) {
                        next_args[j] = tb_inst_load(
                            fun,
                            vm_tb_func_tag(fun, (vm_tag_t) i),
                            tb_inst_member_access(
                                fun,
                                params[0],
                                offsetof(vm_std_value_t, value)
                            ),
                            8,
                            false
                        );
                    } else {
                        next_args[j] = vm_tb_func_read_arg(fun, regs, next_arg);
                    }
                }
                keys[i - 1].value = vm_tb_func_body(state, module, fun, next_args, branch.rtargets[i]);
            }

            tb_inst_branch(
                fun,
                TB_TYPE_I32,
                val_tag,
                keys[0].value,
                VM_TAG_MAX - 1,
                keys
            );
            
            break;
        }

        case VM_BOP_GET: {
            TB_PrototypeParam get_params[2] = {
                { TB_TYPE_PTR },
                { TB_TYPE_PTR },
            };
            TB_FunctionPrototype *get_proto = tb_prototype_create(module, VM_TB_CC, 2, get_params, 0, NULL, false);
            TB_Node *arg2 = tb_inst_local(fun, sizeof(vm_pair_t), 8);
            tb_inst_store(
                fun,
                vm_tb_func_tag(fun, branch.tag),
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
                tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t) &vm_table_get_pair),
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
                4,
                false
            );
            
            TB_SwitchEntry keys[VM_TAG_MAX - 1];
            for (size_t i = 1; i < VM_TAG_MAX; i++) {
                keys[i - 1].key = i;
                // vm_block_t *next_block = vm_tb_rblock_version(branch.rtargets[i]);
                TB_Node **next_args = vm_malloc(sizeof(TB_Node *) * branch.targets[0]->nargs);
                for (size_t j = 0; j < branch.targets[0]->nargs; j++) {
                    vm_arg_t next_arg = branch.targets[0]->args[j];
                    if (next_arg.type == VM_ARG_REG && next_arg.reg == branch.out.reg) {
                        next_args[j] = tb_inst_load(
                            fun,
                            vm_tb_func_tag(fun, (vm_tag_t) i),
                            tb_inst_member_access(
                                fun,
                                arg2,
                                offsetof(vm_pair_t, val_val)
                            ),
                            8,
                            false
                        );
                    } else {
                        next_args[j] = vm_tb_func_read_arg(fun, regs, next_arg);
                    }
                }
                keys[i - 1].value = vm_tb_func_body(state, module, fun, next_args, branch.rtargets[i]);
            }
            tb_inst_branch(
                fun,
                TB_TYPE_I32,
                val_tag,
                keys[0].value,
                VM_TAG_MAX - 1,
                keys
            );
            break;
        }
        
        default: {
            vm_print_branch(stderr, branch);
            fprintf(stderr, "\n ^ unhandled branch\n");
            asm("int3");
        }
    }
}

void *vm_tb_func_comp(vm_tb_state_t *state, vm_block_t *block) {
    TB_FeatureSet features = (TB_FeatureSet) {0};
    TB_Module* module = tb_module_create_for_host(&features, true);
    TB_Function* fun = tb_function_create( module, -1, "block", TB_LINKAGE_PUBLIC);

    TB_PrototypeParam *proto_args = vm_malloc(sizeof(TB_PrototypeParam) * (block->nargs + 1));

    proto_args[0] = (TB_PrototypeParam) { TB_TYPE_PTR };

    for (size_t arg = 0; arg < block->nargs; arg++) {
        proto_args[arg + 1] = (TB_PrototypeParam) { TB_TYPE_F64 };
    }

    TB_FunctionPrototype *proto = tb_prototype_create(module, VM_TB_CC, block->nargs + 1, proto_args, 0, NULL, false);
    tb_function_set_prototype(
        fun,
        -1, proto,
        NULL
    );

    TB_Node **args = vm_malloc(sizeof(TB_Node *) * (block->nargs));

    for (size_t i = 0; i < block->nargs; i++) {
        args[i] = tb_inst_param(fun, i);
    }

    vm_tb_func_body_once(state, module, fun, args, block);

    TB_Passes *passes = tb_pass_enter(fun, tb_function_get_arena(fun));
    tb_pass_optimize(passes);
    fprintf(stdout, "\n--- tb ---\n");
    tb_pass_print(passes);

    TB_FunctionOutput *out = tb_pass_codegen(passes, true);

    fprintf(stdout, "\n--- x86asm ---\n");
    tb_output_print_asm(out, stdout);

    tb_pass_exit(passes);

    TB_JIT *jit = tb_jit_begin(module, 1 << 16);
    return tb_jit_place_function(jit, fun);
}

void *vm_tb_rfunc_comp(vm_tb_state_t *state, vm_rblock_t *rblock) {
    if (rblock->cache != NULL) {
        return rblock->cache;
    }
    vm_block_t *block = vm_tb_rblock_version(rblock);
    if (block == NULL) {
        vm_print_block(stderr, rblock->block);
        __builtin_trap();
    }
    return vm_tb_func_comp(state, block);
}

void *vm_tb_full_comp(vm_tb_state_t *state, vm_block_t *block) {
    vm_tags_t *regs = vm_rblock_regs_empty(256);
    block->isfunc = true;
    vm_rblock_t *rblock = vm_rblock_new(block, regs);
    return vm_tb_rfunc_comp(state, rblock);
}

typedef void *vm_tb_func_t(vm_std_value_t *ptr);

vm_std_value_t vm_tb_run(vm_block_t *block, vm_table_t *std, vm_std_value_t *args) {
    vm_tb_state_t state = (vm_tb_state_t) {
        .std = std,
    };
    vm_tb_func_t *fn = vm_tb_full_comp(&state, block);
    vm_std_value_t ret = (vm_std_value_t) {
        .tag = VM_TAG_NIL,
    };
    fn(&ret);
    return ret;
}

vm_std_value_t vm_x64_run(vm_block_t *block, vm_table_t *std, vm_std_value_t *args) {
    return vm_tb_run(block, std, args);
}

