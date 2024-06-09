
#include "./tb.h"
#include "./exec.h"
#include "../../ir/check.h"
#include "../../ir/rblock.h"

#include "../../../vendor/cuik/tb/include/tb.h"
#include "../../../vendor/cuik/common/arena.h"

#define VM_TB_CC TB_CDECL
#define VM_TB_TYPE_TAG TB_TYPE_I8
#define VM_TB_TYPE_VALUE TB_TYPE_I64

#if defined(VM_USE_TCC)
#include "../../../vendor/tcc/libtcc.h"

void vm_tb_tcc_error_func(void *user, const char *msg) {
    (void)user;
    printf("%s\n", msg);
    exit(1);
}

static void *vm_tcc_new(void) {
    unsigned long long __fixunsdfdi(double a1);

    TCCState *state = tcc_new();
    tcc_set_error_func(state, 0, vm_tb_tcc_error_func);
    tcc_set_options(state, "-nostdlib");
    tcc_add_symbol(state, "__fixunsdfdi", (const void *)&__fixunsdfdi);
    return state;
}
#endif

static bool vm_tb_str_eq(const char *str1, const char *str2) {
    return strcmp(str1, str2) == 0;
}

#include "dynamic.h"
#include "lbbv.h"

typedef void vm_tb_caller_t(vm_std_value_t *, void *);

vm_std_value_t vm_tb_run(vm_config_t *config, vm_block_t *entry, vm_blocks_t *blocks, vm_table_t *std) {
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
        tb_function_set_arenas(fun, tmp_arena, tmp_arena);
        TB_FunctionPrototype *proto = tb_prototype_create(mod, VM_TB_CC, 2, proto_params, 0, NULL, false);
        tb_function_set_prototype(fun, -1, proto);

        // tb_inst_debugbreak(fun);
        TB_MultiOutput out = tb_inst_call(fun, call_proto, tb_inst_param(fun, 1), 0, NULL);

        // store into struct
        TB_Node *dst = tb_inst_param(fun, 0);
        tb_inst_store(fun, VM_TB_TYPE_VALUE, dst, out.multiple[0], _Alignof(vm_value_t), false);
        TB_Node *dst2 = tb_inst_member_access(fun, dst, offsetof(vm_std_value_t, tag));
        tb_inst_store(fun, TB_TYPE_PTR, dst2, out.multiple[1], _Alignof(uint32_t), false);

        tb_inst_ret(fun, 0, NULL);

        // compile it
#if defined(VM_USE_TCC)
        TB_CBuffer *cbuf = tb_c_buf_new();
        tb_c_print_prelude(cbuf, mod);
        tb_c_print_function(cbuf, fun, worklist, tmp_arena);
        const char *buf = tb_c_buf_to_data(cbuf);
        if (config->dump_c) {
            printf("\n--- c ---\n%s", buf);
        }
        TCCState *state = vm_tcc_new();
        tcc_set_output_type(state, TCC_OUTPUT_MEMORY);
        tcc_compile_string(state, buf);
        tcc_add_symbol(state, "memmove", &memmove);
        tcc_relocate(state);
        tb_c_data_free(buf);
        caller = tcc_get_symbol(state, "caller");
#else
        tb_codegen(fun, worklist, code_arena, NULL, false);
        caller = tb_jit_place_function(jit, fun);
#endif

        tb_worklist_free(worklist);
    }
#endif

    if (config->lbbv) {
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

        vm_std_value_t values[1] = {
            [0].tag = VM_TAG_UNK,
        };

#if defined(_WIN32)
        caller(&value, fn);
#else
        value = fn(&values[0]);
#endif

        // blocks->len = 0;

        vm_free(state);
    }

    for (size_t i = 0; i < blocks->len; i++) {
        vm_block_t *block = blocks->blocks[i];
        block->cache.len = 0;
    }

    return *(vm_std_value_t *)&value;
}
