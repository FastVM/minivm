
#include "./tb.h"

#include "../../vendor/cuik/tb/include/tb.h"

#define VM_TB_CC TB_CDECL
#define VM_TB_TYPE_TAG TB_TYPE_I8
#define VM_TB_TYPE_VALUE TB_TYPE_I64

#if defined(VM_USE_TCC)
void vm_tb_tcc_error_func(void *user, const char *msg) {
    printf("%s\n", msg);
    exit(1);
}
#endif

#include "tb_dyn.h"
#include "tb_ver.h"

vm_std_value_t vm_tb_run_main(vm_config_t *config, vm_block_t *entry, vm_blocks_t *blocks, vm_table_t *std) {
    vm_std_value_t val = vm_tb_run_repl(config, entry, blocks, std);
    if (vm_type_eq(val.tag, VM_TAG_ERROR)) {
        fprintf(stderr, "error: %s\n", val.value.str);
    }
    return val;
}

typedef void vm_tb_caller_t(vm_std_value_t *, void *);

vm_std_value_t vm_tb_run_repl(vm_config_t *config, vm_block_t *entry, vm_blocks_t *blocks, vm_table_t *std) {
    vm_std_value_t value;
    // on windows we don't have access to multiple returns from C so we'll
    // just make a dumb caller for such a pattern
#if defined(_WIN32)
    static vm_tb_caller_t *caller = NULL;

    if (caller == NULL) {
        TB_Module *mod = tb_module_create_for_host(true);

        TB_Arena *tmp_arena = tb_arena_create(TB_ARENA_SMALL_CHUNK_SIZE);
        TB_Arena *code_arena = tb_arena_create(TB_ARENA_MEDIUM_CHUNK_SIZE);
        TB_Worklist *worklist = tb_worklist_alloc();
        TB_JIT *jit = tb_jit_begin(mod, 1 << 16);

        TB_PrototypeParam call_proto_rets[2] = {{VM_TB_TYPE_VALUE}, {TB_TYPE_PTR}};
        TB_FunctionPrototype *call_proto = tb_prototype_create(mod, VM_TB_CC, 0, NULL, 2, call_proto_rets, false);

        TB_PrototypeParam proto_params[2] = {{TB_TYPE_PTR}, {TB_TYPE_PTR}};
        TB_Function *fun = tb_function_create(mod, -1, "caller", TB_LINKAGE_PUBLIC);
        TB_FunctionPrototype *proto = tb_prototype_create(mod, VM_TB_CC, 2, proto_params, 0, NULL, false);
        tb_function_set_prototype(fun, -1, proto, NULL);

        // tb_inst_debugbreak(fun);
        TB_MultiOutput out = tb_inst_call(fun, call_proto, tb_inst_param(fun, 1), 0, NULL);

        // store into struct
        TB_Node *dst = tb_inst_param(fun, 0);
        tb_inst_store(fun, VM_TB_TYPE_VALUE, dst, out.multiple[0], _Alignof(vm_value_t), false);
        TB_Node *dst2 = tb_inst_member_access(fun, dst, offsetof(vm_std_value_t, tag));
        tb_inst_store(fun, TB_TYPE_PTR, dst2, out.multiple[1], _Alignof(uint32_t), false);

        tb_inst_ret(fun, 0, NULL);

        // compile it
        tb_codegen(fun, worklist, tmp_arena, code_arena, tmp_arena, NULL, false);

        caller = tb_jit_place_function(jit, fun);

        tb_worklist_free(worklist);
    }
#endif

    if (config->tb_lbbv) {
        vm_tb_ver_state_t *state = vm_malloc(sizeof(vm_tb_ver_state_t));

        *state = (vm_tb_ver_state_t){
            .std = std,
            .config = config,
            .blocks = blocks,
        };

        vm_tb_ver_func_t *fn = (vm_tb_ver_func_t *)vm_tb_ver_full_comp(state, entry);

#if defined(_WIN32)
        caller(&value, fn);
#else
        value = fn();
#endif

        tb_arena_destroy(state->ir_arena);
        tb_arena_destroy(state->tmp_arena);
        tb_arena_destroy(state->code_arena);

        vm_free(state);
    } else {
        vm_tb_dyn_state_t *state = vm_malloc(sizeof(vm_tb_dyn_state_t));

        *state = (vm_tb_dyn_state_t){
            .std = std,
            .config = config,
            .blocks = blocks,
        };

        vm_tb_dyn_func_t *fn = (vm_tb_dyn_func_t *)vm_tb_dyn_comp(state, entry);

#if defined(_WIN32)
        caller(&value, fn);
#else
        value = fn();
#endif

        blocks->len = 0;

        vm_free(state);
    }

    for (size_t i = 0; i < blocks->len; i++) {
        vm_block_t *block = blocks->blocks[i];
        // for (size_t j = 0; j < block->cache.len; j++) {
        //     vm_rblock_reset(block->cache.keys[j]);
        //     vm_free_block_sub(block->cache.values[j]);
        // }
        block->cache.len = 0;
    }

    return *(vm_std_value_t *)&value;
}
