
#include "./tb.h"

#include <stdio.h>

#include "../../tb/include/tb.h"
#include "../check.h"
#include "../rblock.h"

#define VM_TB_CC TB_CDECL
// #define VM_TB_CC TB_STDCALL

typedef TB_Node *vm_tb_binary_op_t(TB_Function *fun, TB_Node *lhs, TB_Node *rhs);

void vm_tb_func_print_value(vm_tb_state_t *mod, TB_Function *fun, vm_tag_t tag, TB_Node *value);
TB_Node *vm_tb_func_body_call(vm_tb_state_t *state, TB_Function *fun, TB_Node **args, vm_rblock_t *rblock);
TB_Node *vm_tb_func_body_once(vm_tb_state_t *state, TB_Function *fun, TB_Node **regs, vm_block_t *block);

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
    vm_tag_t tag = xtag;                                       \
    TB_Node *ret = NULL;                                       \
    if (tag != VM_TAG_F64 && tag != VM_TAG_F32) {              \
        ret = onint(__VA_ARGS__, true);          \
    } else {                                                   \
        ret = onfloat(__VA_ARGS__);                            \
    }                                                          \
    ret;                                                       \
})

TB_DataType vm_tb_func_tag(TB_Function *fun, vm_tag_t tag) {
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
            exit(1);
        }
    }
}

TB_Node *vm_tb_func_read_arg(TB_Function *fun, TB_Node **regs, vm_arg_t arg) {
    switch (arg.type) {
        case VM_ARG_NUM: {
            return tb_inst_float64(fun, arg.num.value.f64);
        }
        case VM_ARG_REG: {
            if (regs[arg.reg] == NULL) {
                fprintf(stderr, "use of uninitialized value: r%zu\n", (size_t)arg.reg);
                __builtin_trap();
            }
            return regs[arg.reg];
        }
        case VM_ARG_STR: {
            return tb_inst_string(fun, strlen(arg.str), arg.str);
        }
        default: {
            vm_print_arg(stderr, arg);
            fprintf(stderr, "\n ^ unhandled arg (type#%zu)\n", (size_t)arg.type);
            __builtin_trap();
        }
    }
}

TB_Node *vm_tb_func_body(vm_tb_state_t *state, TB_Function *fun, TB_Node **args, vm_rblock_t *rblock) {
    vm_block_t *block = vm_rblock_version(rblock);

    if (!vm_check_block(block)) {
        return NULL;
    }

    TB_Module *module = state->module;

    TB_Node *ctrl = tb_inst_get_control(fun);
    
    TB_Node *ret = tb_inst_region(fun);

    tb_inst_set_control(fun, ret);

    if (rblock->cache != NULL) {
        TB_Node *func = tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t) rblock->cache);
        TB_PrototypeParam *call_params = vm_malloc(sizeof(TB_PrototypeParam) * (rblock->block->nargs + 1));

        call_params[0] = (TB_PrototypeParam){TB_TYPE_PTR};

        for (size_t arg = 0; arg < rblock->block->nargs; arg++) {
            call_params[arg + 1] = (TB_PrototypeParam){
                vm_tb_func_tag(fun, rblock->regs->tags[rblock->block->args[arg].reg]),
            };
        }

        TB_FunctionPrototype *call_proto = tb_prototype_create(state->module, VM_TB_CC, rblock->block->nargs + 1, call_params, 0, NULL, false);

        TB_Node **call_args = vm_malloc(sizeof(TB_Node *) * (rblock->block->nargs + 1));

        call_args[0] = tb_inst_param(fun, 0);

        for (size_t i = 0; i < rblock->block->nargs; i++) {
            call_args[i + 1] = args[i];
        }
        
        tb_inst_call(
            fun,
            call_proto,
            func,
            rblock->block->nargs + 1,
            call_args);

        tb_inst_ret(fun, 0, NULL);
    } else {
        TB_Node *global_addr = tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t) &rblock->cache);
        TB_Node *global = tb_inst_load( fun, TB_TYPE_PTR, global_addr, 1, false);
        TB_Node *cond = tb_inst_cmp_eq(fun, global, tb_inst_uint(fun, TB_TYPE_PTR, 0));

        TB_Node *has_global = tb_inst_region(fun);
        TB_Node *no_global = tb_inst_region(fun);

        tb_inst_if(fun, cond, no_global, has_global);

        tb_inst_set_control(fun, no_global);

        TB_PrototypeParam comp_args[2] = {
            {TB_TYPE_PTR},
            {TB_TYPE_PTR},
        };

        TB_PrototypeParam comp_ret[1] = {
            {TB_TYPE_PTR},
        };

        TB_FunctionPrototype *comp_proto = tb_prototype_create(state->module, VM_TB_CC, 2, comp_args, 1, comp_ret, false);

        TB_Node *comp_params[2];

        GC_add_roots(rblock, rblock + 1);

        comp_params[0] = tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t)state);
        comp_params[1] = tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t)rblock);

        TB_MultiOutput multi = tb_inst_call(
            fun,
            comp_proto,
            tb_inst_get_symbol_address(fun, state->vm_tb_rfunc_comp),
            2,
            comp_params);

        {
            TB_PrototypeParam *call_params = vm_malloc(sizeof(TB_PrototypeParam) * (rblock->block->nargs + 1));

            call_params[0] = (TB_PrototypeParam){TB_TYPE_PTR};

            for (size_t arg = 0; arg < rblock->block->nargs; arg++) {
                call_params[arg + 1] = (TB_PrototypeParam){
                    vm_tb_func_tag(fun, rblock->regs->tags[rblock->block->args[arg].reg]),
                };
            }

            TB_FunctionPrototype *call_proto = tb_prototype_create(state->module, VM_TB_CC, rblock->block->nargs + 1, call_params, 0, NULL, false);

            TB_Node **call_args = vm_malloc(sizeof(TB_Node *) * (rblock->block->nargs + 1));

            call_args[0] = tb_inst_param(fun, 0);

            for (size_t i = 0; i < rblock->block->nargs; i++) {
                call_args[i + 1] = args[i];
            }

            tb_inst_call(
                fun,
                call_proto,
                multi.single,
                rblock->block->nargs + 1,
                call_args);

            tb_inst_ret(fun, 0, NULL);
        }

        tb_inst_set_control(fun, has_global);

        {
            TB_PrototypeParam *call_params = vm_malloc(sizeof(TB_PrototypeParam) * (rblock->block->nargs + 1));

            call_params[0] = (TB_PrototypeParam){TB_TYPE_PTR};

            for (size_t arg = 0; arg < rblock->block->nargs; arg++) {
                call_params[arg + 1] = (TB_PrototypeParam){
                    vm_tb_func_tag(fun, rblock->regs->tags[rblock->block->args[arg].reg]),
                };
            }

            TB_FunctionPrototype *call_proto = tb_prototype_create(state->module, VM_TB_CC, rblock->block->nargs + 1, call_params, 0, NULL, false);

            TB_Node **call_args = vm_malloc(sizeof(TB_Node *) * (rblock->block->nargs + 1));

            call_args[0] = tb_inst_param(fun, 0);

            for (size_t i = 0; i < rblock->block->nargs; i++) {
                call_args[i + 1] = args[i];
            }
            
            tb_inst_call(
                fun,
                call_proto,
                global,
                rblock->block->nargs + 1,
                call_args);

            tb_inst_ret(fun, 0, NULL);
        }
    }

    tb_inst_set_control(fun, ctrl);

    return ret;
}

TB_Node *vm_tb_func_body_call(vm_tb_state_t *state, TB_Function *fun, TB_Node **args, vm_rblock_t *rblock) {
    if (!vm_check_block(vm_rblock_version(rblock))) {
        return NULL;
    }

    TB_Node *func = NULL;
    if (rblock->cache != NULL) {
        func = tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t)rblock->cache);
    } else {
        TB_PrototypeParam comp_args[2] = {
            {TB_TYPE_PTR},
            {TB_TYPE_PTR},
        };

        TB_PrototypeParam comp_ret[1] = {
            {TB_TYPE_PTR},
        };

        TB_FunctionPrototype *comp_proto = tb_prototype_create(state->module, VM_TB_CC, 2, comp_args, 1, comp_ret, false);

        TB_Node *comp_params[2];

        GC_add_roots(rblock, rblock + 1);

        comp_params[0] = tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t)state);
        comp_params[1] = tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t)rblock);

        TB_MultiOutput multi = tb_inst_call(
            fun,
            comp_proto,
            tb_inst_get_symbol_address(fun, state->vm_tb_rfunc_comp),
            2,
            comp_params);

        func = multi.single;
    }

    TB_PrototypeParam *call_params = vm_malloc(sizeof(TB_PrototypeParam) * (rblock->block->nargs + 1));

    call_params[0] = (TB_PrototypeParam){TB_TYPE_PTR};

    for (size_t arg = 0; arg < rblock->block->nargs; arg++) {
        call_params[arg + 1] = (TB_PrototypeParam){
            vm_tb_func_tag(fun, rblock->regs->tags[rblock->block->args[arg].reg]),
        };
    }

    TB_FunctionPrototype *call_proto = tb_prototype_create(state->module, VM_TB_CC, rblock->block->nargs + 1, call_params, 0, NULL, false);

    TB_Node **call_args = vm_malloc(sizeof(TB_Node *) * (rblock->block->nargs + 1));

    call_args[0] = tb_inst_local(fun, 8, 8);

    for (size_t i = 0; i < rblock->block->nargs; i++) {
        call_args[i + 1] = args[i];
    }

    tb_inst_call(
        fun,
        call_proto,
        func,
        rblock->block->nargs + 1,
        call_args);

    return call_args[0];
}

TB_Node *vm_tb_func_body_once(vm_tb_state_t *state, TB_Function *fun, TB_Node **regs, vm_block_t *block) {
    TB_Module *module = state->module;

    if (block->pass) {
        return block->pass;
    }

    TB_Node *old_ctrl = tb_inst_get_control(fun);

    TB_Node *ret = tb_inst_region(fun);

    tb_inst_set_control(fun, ret);

    block->pass = ret;

#if defined(VM_DUMP_IR)
    fprintf(stdout, "\n--- vmir ---\n");
    vm_print_block(stdout, block);
    fflush(stdout);
#endif

    for (size_t n = 0; n < block->len; n++) {
        vm_instr_t instr = block->instrs[n];
        switch (instr.op) {
            case VM_IOP_MOVE: {
                regs[instr.out.reg] = vm_tb_func_read_arg(fun, regs, instr.args[0]);
                break;
            }
            case VM_IOP_ADD: {
                regs[instr.out.reg] = vm_tb_select_binary_type(
                    instr.tag,
                    tb_inst_add, tb_inst_fadd,
                    fun,
                    vm_tb_func_read_arg(fun, regs, instr.args[0]),
                    vm_tb_func_read_arg(fun, regs, instr.args[1]));
                break;
            }
            case VM_IOP_SUB: {
                regs[instr.out.reg] = vm_tb_select_binary_type(
                    instr.tag,
                    tb_inst_sub, tb_inst_fsub,
                    fun,
                    vm_tb_func_read_arg(fun, regs, instr.args[0]),
                    vm_tb_func_read_arg(fun, regs, instr.args[1]));
                break;
            }
            case VM_IOP_MUL: {
                regs[instr.out.reg] = vm_tb_select_binary_type(
                    instr.tag,
                    tb_inst_mul, tb_inst_fmul,
                    fun,
                    vm_tb_func_read_arg(fun, regs, instr.args[0]),
                    vm_tb_func_read_arg(fun, regs, instr.args[1]));
                break;
            }
            case VM_IOP_DIV: {
                regs[instr.out.reg] = vm_tb_select_binary_type(
                    instr.tag,
                    tb_inst_div, tb_inst_fdiv,
                    fun,
                    vm_tb_func_read_arg(fun, regs, instr.args[0]),
                    vm_tb_func_read_arg(fun, regs, instr.args[1]));
                break;
            }
            case VM_IOP_NOP: {
                break;
            }
            case VM_IOP_STD: {
                regs[instr.out.reg] = tb_inst_uint(fun, TB_TYPE_PTR, (uint64_t)state->std);
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
                vm_tb_func_body_once(state, fun, regs, branch.targets[0]));
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
                    vm_tb_func_read_arg(fun, regs, branch.args[1])),
                vm_tb_func_body_once(state, fun, regs, branch.targets[0]),
                vm_tb_func_body_once(state, fun, regs, branch.targets[1]));
            break;
        }

        case VM_BOP_RET: {
            tb_inst_store(
                fun,
                TB_TYPE_I32,
                tb_inst_member_access(
                    fun,
                    tb_inst_param(fun, 0),
                    offsetof(vm_std_value_t, tag)),
                tb_inst_sint(fun, TB_TYPE_I32, branch.tag),
                1,
                false);
            tb_inst_store(
                fun,
                vm_tb_func_tag(fun, branch.tag),
                tb_inst_member_access(
                    fun,
                    tb_inst_param(fun, 0),
                    offsetof(vm_std_value_t, value)),
                vm_tb_func_read_arg(fun, regs, branch.args[0]),
                1,
                false);

            tb_inst_ret(fun, 0, NULL);
            break;
        }

        case VM_BOP_CALL: {
            size_t nparams = 0;

            for (size_t i = 1; branch.args[i].type != VM_ARG_NONE; i++) {
                nparams += 1;
            }
            TB_Node *res_ptr = NULL;
            if (branch.args[0].type == VM_ARG_RFUNC) {
                TB_Node **call_args = vm_malloc(sizeof(TB_Node *) * nparams);

                for (size_t i = 1; branch.args[i].type != VM_ARG_NONE; i++) {
                    call_args[i - 1] = vm_tb_func_read_arg(fun, regs, branch.args[i]);
                }

                res_ptr = vm_tb_func_body_call(state, fun, call_args, branch.args[0].rfunc);
            } else if (branch.args[0].type == VM_ARG_REG) {
                if (branch.args[0].reg_tag == VM_TAG_FFI) {
                    TB_PrototypeParam call_proto_params[1] = {
                        {TB_TYPE_PTR},
                    };

                    TB_FunctionPrototype *call_proto = tb_prototype_create(state->module, VM_TB_CC, 1, call_proto_params, 0, NULL, false);

                    TB_Node *call_arg = tb_inst_local(fun, sizeof(vm_std_value_t) * (nparams + 1), 8);

                    for (size_t i = 1; branch.args[i].type != VM_ARG_NONE; i++) {
                        TB_Node *head = tb_inst_member_access(fun, call_arg, sizeof(vm_std_value_t) * (i - 1));
                        tb_inst_store(
                            fun,
                            vm_tb_func_tag(fun, branch.args[i].reg_tag),
                            tb_inst_member_access(fun, head, offsetof(vm_std_value_t, value)),
                            vm_tb_func_read_arg(fun, regs, branch.args[i]),
                            8,
                            false);
                        tb_inst_store(
                            fun,
                            TB_TYPE_I32,
                            tb_inst_member_access(fun, head, offsetof(vm_std_value_t, tag)),
                            tb_inst_uint(fun, TB_TYPE_I32, branch.args[i].reg_tag),
                            4,
                            false);
                    }

                    TB_Node *end_head = tb_inst_member_access(fun, call_arg, sizeof(vm_std_value_t) * nparams);

                    tb_inst_store(
                        fun,
                        TB_TYPE_PTR,
                        tb_inst_member_access(fun, end_head, offsetof(vm_std_value_t, value)),
                        tb_inst_uint(fun, TB_TYPE_PTR, 0),
                        8,
                        false);
                    tb_inst_store(
                        fun,
                        TB_TYPE_I32,
                        tb_inst_member_access(fun, end_head, offsetof(vm_std_value_t, tag)),
                        tb_inst_uint(fun, TB_TYPE_I32, 0),
                        4,
                        false);

                    TB_Node *call_func = vm_tb_func_read_arg(fun, regs, branch.args[0]);

                    tb_inst_call(
                        fun,
                        call_proto,
                        call_func,
                        1,
                        &call_arg);

                    res_ptr = call_arg;
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

            TB_Node *val_tag = tb_inst_load(
                fun,
                TB_TYPE_I32,
                tb_inst_member_access(
                    fun,
                    res_ptr,
                    offsetof(vm_std_value_t, tag)),
                4,
                false);

            TB_Node *val_val = tb_inst_member_access(
                fun,
                res_ptr,
                offsetof(vm_std_value_t, value));

            TB_SwitchEntry keys[VM_TAG_MAX - 1];
            size_t write = 0;
            for (size_t i = 1; i < VM_TAG_MAX; i++) {
                TB_Node **next_args = vm_malloc(sizeof(TB_Node *) * branch.targets[0]->nargs);
                for (size_t j = 0; j < branch.targets[0]->nargs; j++) {
                    vm_arg_t next_arg = branch.targets[0]->args[j];
                    if (next_arg.type == VM_ARG_REG && next_arg.reg == branch.out.reg) {
                        next_args[j] = tb_inst_load(
                            fun,
                            vm_tb_func_tag(fun, (vm_tag_t)i),
                            val_val,
                            8,
                            false);
                    } else {
                        next_args[j] = vm_tb_func_read_arg(fun, regs, next_arg);
                    }
                }
                TB_Node *value = vm_tb_func_body(state, fun, next_args, branch.rtargets[i]);
                if (value != NULL) {
                    keys[write].key = i;
                    keys[write].value = value;
                    write += 1;
                }
            }

            if (write == 0) {
                tb_inst_ret(fun, 0, NULL);
            } else {
                tb_inst_branch(
                    fun,
                    TB_TYPE_I32,
                    val_tag,
                    keys[0].value,
                    write,
                    keys);
            }
            break;
        }

        case VM_BOP_GET: {
            TB_PrototypeParam get_params[2] = {
                {TB_TYPE_PTR},
                {TB_TYPE_PTR},
            };

            TB_PrototypeParam get_returns[1] = {
                {TB_TYPE_PTR},
            };

            TB_FunctionPrototype *get_proto = tb_prototype_create(state->module, VM_TB_CC, 2, get_params, 1, get_returns, false);
            TB_Node *arg2 = tb_inst_local(fun, sizeof(vm_pair_t), 8);
            tb_inst_store(
                fun,
                vm_tb_func_tag(fun, branch.tag),
                tb_inst_member_access(
                    fun,
                    arg2,
                    offsetof(vm_pair_t, key_val)),
                vm_tb_func_read_arg(fun, regs, branch.args[1]),
                8,
                false);
            tb_inst_store(
                fun,
                TB_TYPE_I32,
                tb_inst_member_access(
                    fun,
                    arg2,
                    offsetof(vm_pair_t, key_tag)),
                tb_inst_uint(fun, TB_TYPE_I32, branch.tag),
                4,
                false);
            TB_Node *get_args[2] = {
                vm_tb_func_read_arg(fun, regs, branch.args[0]),
                arg2,
            };
            tb_inst_call(
                fun,
                get_proto,
                tb_inst_get_symbol_address(fun, state->vm_table_get_pair),
                2,
                get_args);
            TB_Node *val_tag = tb_inst_load(
                fun,
                TB_TYPE_I32,
                tb_inst_member_access(
                    fun,
                    arg2,
                    offsetof(vm_pair_t, val_tag)),
                4,
                false);

            TB_SwitchEntry keys[VM_TAG_MAX - 1];
            size_t write = 0;
            for (size_t i = 1; i < VM_TAG_MAX; i++) {
                TB_Node **next_args = vm_malloc(sizeof(TB_Node *) * branch.targets[0]->nargs);
                for (size_t j = 0; j < branch.targets[0]->nargs; j++) {
                    vm_arg_t next_arg = branch.targets[0]->args[j];
                    if (next_arg.type == VM_ARG_REG && next_arg.reg == branch.out.reg) {
                        next_args[j] = tb_inst_load(
                            fun,
                            vm_tb_func_tag(fun, (vm_tag_t)i),
                            tb_inst_member_access(
                                fun,
                                arg2,
                                offsetof(vm_pair_t, val_val)),
                            8,
                            false);
                    } else {
                        next_args[j] = vm_tb_func_read_arg(fun, regs, next_arg);
                    }
                }
                TB_Node *value = vm_tb_func_body(state, fun, next_args, branch.rtargets[i]);
                if (value != NULL) {
                    keys[write].key = i;
                    keys[write].value = value;
                    write += 1;
                }
            }

            if (write == 0) {
                tb_inst_ret(fun, 0, NULL);
            } else {
                tb_inst_branch(
                    fun,
                    TB_TYPE_I32,
                    val_tag,
                    keys[0].value,
                    write,
                    keys);
            }

            break;
        }

        default: {
            vm_print_branch(stderr, branch);
            fprintf(stderr, "\n ^ unhandled branch\n");
            asm("int3");
            break;
        }
    }

    block->pass = NULL;

    tb_inst_set_control(fun, old_ctrl);

    return ret;
}

void vm_tb_print(uint32_t tag, void *value) {
    vm_std_value_t val = (vm_std_value_t){
        .tag = tag,
    };
    switch (tag) {
        // case VM_TAG_I64: {
        //     val.value.i64 = *(int64_t *)value;
        //     break;
        // }
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
    TB_PrototypeParam proot_args[2] = {
        {TB_TYPE_I32},
        {TB_TYPE_PTR},
    };

    TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, 2, proot_args, 0, NULL, false);

    TB_Node *local = tb_inst_local(fun, 8, 8);

    tb_inst_store(
        fun,
        vm_tb_func_tag(fun, tag),
        local,
        value,
        8,
        false);

    TB_Node *params[2] = {
        tb_inst_uint(fun, TB_TYPE_I32, (uint64_t)tag),
        local,
    };

    tb_inst_call(
        fun,
        proto,
        tb_inst_get_symbol_address(fun, state->vm_tb_print),
        2,
        params);
}

void *vm_tb_rfunc_comp(vm_tb_state_t *state, vm_rblock_t *rblock) {
    void *test = rblock->cache;
    if (test != NULL) {
        return test;
    }

    vm_block_t *block = vm_rblock_version(rblock);
    if (block == NULL) {
        vm_print_block(stderr, rblock->block);
        __builtin_trap();
    }
    TB_Module *module = state->module;
    TB_Function *fun = tb_function_create(state->module, -1, "block", TB_LINKAGE_PRIVATE);

    TB_PrototypeParam *proto_args = vm_malloc(sizeof(TB_PrototypeParam) * (block->nargs + 1));

    proto_args[0] = (TB_PrototypeParam){TB_TYPE_PTR};

    for (size_t arg = 0; arg < block->nargs; arg++) {
        size_t reg = block->args[arg].reg;
        proto_args[arg + 1] = (TB_PrototypeParam){
            vm_tb_func_tag(fun, rblock->regs->tags[reg]),
        };
    }

    TB_FunctionPrototype *proto = tb_prototype_create(state->module, VM_TB_CC, block->nargs + 1, proto_args, 0, NULL, false);
    tb_function_set_prototype(
        fun,
        -1, proto,
        NULL);

    TB_Node **args = vm_malloc(sizeof(TB_Node *) * (block->nargs));

    for (size_t i = 0; i < block->nargs; i++) {
        args[i] = tb_inst_param(fun, i + 1);
    }

    TB_Node **regs = vm_malloc(sizeof(TB_Node *) * block->nregs);

    for (size_t i = 0; i < block->nregs; i++) {
        regs[i] = NULL;
    }

    for (size_t i = 0; i < block->nargs; i++) {
        regs[block->args[i].reg] = args[i];
    }

    TB_Node *main = vm_tb_func_body_once(state, fun, regs, block);

    tb_inst_goto(fun, main);

    TB_Passes *passes = tb_pass_enter(fun, tb_function_get_arena(fun));
#if defined(VM_DUMP_TB)
    fprintf(stdout, "\n--- tb ---\n");
    tb_pass_print(passes);
#endif
    // tb_pass_optimize(passes);
#if defined(VM_DUMP_TB_OPT)
    fprintf(stdout, "\n--- opt tb ---\n");
    tb_pass_print(passes);
#endif

#if defined(VM_DUMP_X86)
    TB_FunctionOutput *out = tb_pass_codegen(passes, true);
    fprintf(stdout, "\n--- x86asm ---\n");
    tb_output_print_asm(out, stdout);
#else
    TB_FunctionOutput *out = tb_pass_codegen(passes, false);
#endif

    tb_pass_exit(passes);

    TB_JIT *jit = tb_jit_begin(state->module, 1 << 24);
    void *ret = tb_jit_place_function(jit, fun);

    rblock->cache = ret;

    return ret;
}

void *vm_tb_full_comp(vm_tb_state_t *state, vm_block_t *block) {
    vm_tags_t *regs = vm_rblock_regs_empty(256);
    block->isfunc = true;
    vm_rblock_t *rblock = vm_rblock_new(block, regs);
    return vm_tb_rfunc_comp(state, rblock);
}

typedef void *__attribute__((cdecl)) vm_tb_func_t(vm_std_value_t *ptr);

vm_std_value_t vm_tb_run(vm_block_t *block, vm_table_t *std, vm_std_value_t *args) {
    TB_FeatureSet features = (TB_FeatureSet){0};
    TB_Module *module = tb_module_create_for_host(&features, true);

    vm_tb_state_t state = (vm_tb_state_t){
        .std = std,
        .module = module,
    };

    // bind externals
    state.state_self = tb_extern_create(module, -1, "<state>", TB_EXTERNAL_SO_LOCAL);
    state.vm_tb_rfunc_comp = tb_extern_create(module, -1, "vm_tb_rfunc_comp", TB_EXTERNAL_SO_LOCAL);
    state.vm_table_get_pair = tb_extern_create(module, -1, "vm_table_get_pair", TB_EXTERNAL_SO_LOCAL);
    state.vm_tb_print = tb_extern_create(module, -1, "vm_tb_print", TB_EXTERNAL_SO_LOCAL);
    tb_symbol_bind_ptr(state.state_self, &state);
    tb_symbol_bind_ptr(state.vm_tb_rfunc_comp, &vm_tb_rfunc_comp);
    tb_symbol_bind_ptr(state.vm_table_get_pair, &vm_table_get_pair);
    tb_symbol_bind_ptr(state.vm_tb_print, &vm_tb_print);

    vm_tb_func_t *fn = vm_tb_full_comp(&state, block);
    vm_std_value_t ret = (vm_std_value_t){
        .tag = VM_TAG_NIL,
    };
    fn(&ret);
    return ret;
}

vm_std_value_t vm_x64_run(vm_block_t *block, vm_table_t *std, vm_std_value_t *args) {
    return vm_tb_run(block, std, args);
}
