
#include "./tb.h"

#include "../../tb/include/tb.h"

// typedef void __attribute__((cdecl)) callable_t(double, double);


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
        case VM_BOP_EXIT:
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

TB_Node *vm_tb_func_arg(TB_Function* fun, vm_arg_t arg) {
    switch (arg.type) {
        case VM_ARG_NUM: {
            return tb_inst_float64(fun, arg.num);
        }
        default: {
            vm_print_arg(stderr, arg);
            fprintf(stderr, "\n ^ unhandled arg\n");
            break;
        }
    }
}

TB_DataType vm_tb_func_tag(TB_Function *fun, vm_tag_t tag) {
    switch (tag) {
    case VM_TAG_F64: {
        return TB_TYPE_F64;
    }
    case VM_TAG_I64: {
        return TB_TYPE_I64;
    }
    default: {
        vm_print_tag(stderr, tag);
        fprintf(stderr, "\n ^ unhandled tag\n");
        exit(1);
    }
    }
}

void *vm_tb_func_comp(vm_tb_state_t *state, vm_rblock_t *rblock) {
    if (rblock->cache != NULL) {
        return rblock->cache;
    }
    vm_block_t *block = vm_tb_rblock_version(rblock);
    if (block == NULL) {
        vm_print_block(stderr, rblock->block);
        __builtin_trap();
    }
    fprintf(stdout, "\n--- vmir ---\n");
    vm_print_block(stdout, block);
    TB_FeatureSet features = (TB_FeatureSet) {0};
    TB_Module* module = tb_module_create_for_host(&features, true);
    TB_Function* fun = tb_function_create( module, -1, "block", TB_LINKAGE_PUBLIC);

    TB_PrototypeParam *args = vm_malloc(sizeof(TB_PrototypeParam) * (block->nargs + 1));

    args[0] = (TB_PrototypeParam) { TB_TYPE_PTR };

    for (size_t arg = 0; arg < block->nargs; arg++) {
        args[arg + 1] = (TB_PrototypeParam) { TB_TYPE_F64 };
    }

    TB_FunctionPrototype *proto = tb_prototype_create(module, TB_CDECL, block->nargs + 1, args, 0, NULL, false);
    tb_function_set_prototype(
        fun,
        -1, proto,
        NULL
    );

    for (size_t n = 0; n < block->len; n++) {
        vm_instr_t instr = block->instrs[n];
        switch (instr.op)
        {
        default: {
            vm_print_instr(stderr, instr);
            fprintf(stderr, "\n ^ unhandled instruction\n");
            exit(1);
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
                vm_tb_func_arg(fun, branch.args[0]),
                1,
                false
            );
            tb_inst_ret(fun, 0, NULL);
            break;
        }
        
        default: {
            vm_print_branch(stderr, branch);
            fprintf(stderr, "\n ^ unhandled branch\n");
            exit(1);
        }
    }

    TB_Passes *passes = tb_pass_enter(fun, tb_function_get_arena(fun));
    fprintf(stdout, "\n--- tb ---\n");
    tb_pass_print(passes);

    TB_FunctionOutput *out = tb_pass_codegen(passes, true);

    fprintf(stdout, "\n--- x86asm ---\n");
    tb_output_print_asm(out, stdout);

    tb_pass_exit(passes);

    TB_JIT *jit = tb_jit_begin(module, 1 << 16);
    return tb_jit_place_function(jit, fun);
}

void *vm_tb_full_comp(vm_tb_state_t *state, vm_block_t *block) {
    vm_tags_t *regs = vm_rblock_regs_empty(256);
    block->isfunc = true;
    vm_rblock_t *rblock = vm_rblock_new(block, regs);
    return vm_tb_func_comp(state, rblock);
}

vm_std_value_t vm_tb_run(vm_block_t *block, vm_table_t *std, vm_std_value_t *args) {
    vm_tb_state_t state = (vm_tb_state_t) {
        .std = std,
    };
    void (*fn)(vm_std_value_t *ptr) = vm_tb_full_comp(&state, block);
    vm_std_value_t ret = (vm_std_value_t) {
        .tag = VM_TAG_NIL,
    };
    fn(&ret);
    return ret;
}

vm_std_value_t vm_x64_run(vm_block_t *block, vm_table_t *std, vm_std_value_t *args) {
    return vm_tb_run(block, std, args);
}

