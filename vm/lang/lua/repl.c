#include "repl.h"
#include "../../std/util.h"
#include "../../be/tb.h"
#include "../../ast/ast.h"
#include "../../ast/print.h"
#include "../../ast/comp.h"
#include "../../ir.h"
#include "../../std/libs/io.h"

vm_ast_node_t vm_lang_lua_parse(vm_config_t *config, const char *str);

vm_std_value_t vm_lang_lua_repl_table_get(vm_table_t *table, const char *key) {
    vm_pair_t pair = (vm_pair_t) {
        .key_tag = VM_TAG_STR,
        .key_val.str = key,
    };
    vm_table_get_pair(table, &pair);
    return (vm_std_value_t) {
        .value = pair.val_val,
        .tag = pair.val_tag,
    };
}

bool vm_lang_lua_repl_table_get_bool(vm_table_t *table, const char *key) {
    vm_std_value_t got = vm_lang_lua_repl_table_get(table, key);
    return got.tag != VM_TAG_NIL && (got.tag != VM_TAG_BOOL || got.value.b);
}

void vm_lang_lua_repl_table_get_config(vm_table_t *table, vm_config_t *config) {
    config->use_tb_opt = vm_table_lookup_str(table, "opt")->val_val.b;
    vm_table_t *dump = vm_table_lookup_str(table, "dump")->val_val.table;
    config->dump_src = vm_table_lookup_str(dump, "src")->val_val.b;
    config->dump_ast = vm_table_lookup_str(dump, "ast")->val_val.b;
    config->dump_ir = vm_table_lookup_str(dump, "ir")->val_val.b;
    config->dump_ver = vm_table_lookup_str(dump, "ver")->val_val.b;
    config->dump_tb = vm_table_lookup_str(dump, "tb")->val_val.b;
    config->dump_tb_opt = vm_table_lookup_str(dump, "tb_opt")->val_val.b;
    config->dump_tb_dot = vm_table_lookup_str(dump, "tb_dot")->val_val.b;
    config->dump_tb_opt_dot = vm_table_lookup_str(dump, "tb_opt_dot")->val_val.b;
    config->dump_x86 = vm_table_lookup_str(dump, "x86")->val_val.b;
    config->dump_args = vm_table_lookup_str(dump, "args")->val_val.b;
    config->dump_time = vm_table_lookup_str(dump, "time")->val_val.b;
}

void vm_lang_lua_repl_table_set_config(vm_table_t *table, vm_config_t *config) {
    VM_STD_SET_BOOL(table, "opt", config->use_tb_opt);
    vm_table_t *dump = vm_table_new();
    VM_STD_SET_TAB(table, "dump", dump);
    VM_STD_SET_BOOL(dump, "src", config->dump_src);
    VM_STD_SET_BOOL(dump, "ast", config->dump_ast);
    VM_STD_SET_BOOL(dump, "ir", config->dump_ir);
    VM_STD_SET_BOOL(dump, "ver", config->dump_ver);
    VM_STD_SET_BOOL(dump, "tb", config->dump_tb);
    VM_STD_SET_BOOL(dump, "tb_opt", config->dump_tb_opt);
    VM_STD_SET_BOOL(dump, "tb_dot", config->dump_tb_dot);
    VM_STD_SET_BOOL(dump, "tb_opt_dot", config->dump_tb_opt_dot);
    VM_STD_SET_BOOL(dump, "x86", config->dump_x86);
    VM_STD_SET_BOOL(dump, "args", config->dump_args);
    VM_STD_SET_BOOL(dump, "time", config->dump_time);
}

void vm_lang_lua_repl_completer(ic_completion_env_t *cenv, const char *prefix) {
    vm_lang_lua_repl_complete_state_t *state = cenv->arg;
    FILE *f = fopen("out.log", "w");
    fprintf(f, "prefix: <<%s>>\n", prefix);
    fclose(f);
}
void vm_lang_lua_repl_highlight(ic_highlight_env_t *henv, const char *input, void *arg) {
    vm_lang_lua_repl_highlight_state_t *state = arg;
}

void vm_lang_lua_repl(vm_config_t *config, vm_table_t *std) {
    config->is_repl = true;

    vm_table_t *repl = vm_table_new();
    vm_lang_lua_repl_table_set_config(repl, config);
    VM_STD_SET_BOOL(repl, "echo", true);
    VM_STD_SET_TAB(std, "config", repl);

    ic_set_history(".minivm-history", 2000);

    vm_lang_lua_repl_complete_state_t complete_state = (vm_lang_lua_repl_complete_state_t){0};
    vm_lang_lua_repl_highlight_state_t highlight_state = (vm_lang_lua_repl_highlight_state_t){0};

    char *input;
    while ((input = ic_readline_ex("lua", vm_lang_lua_repl_completer, &complete_state, vm_lang_lua_repl_highlight, &highlight_state)) != NULL) {
        vm_lang_lua_repl_table_get_config(repl, config);
        ic_history_add(input);
        clock_t start = clock();

        if (config->dump_src) {
            printf("\n--- src ---\n");
            printf("%s\n", input);
        }

        vm_ast_node_t node = vm_lang_lua_parse(config, input);

        if (config->dump_ast) {
            printf("\n--- ast ---\n");
            vm_ast_print_node(stdout, 0, "", node);
        }

        vm_ast_blocks_t blocks = vm_ast_comp(node);

        if (config->dump_ir) {
            vm_print_blocks(stdout, blocks.len, blocks.blocks);
        }

        vm_std_value_t value = vm_tb_run(config, blocks.len, blocks.blocks, std);
        if (vm_lang_lua_repl_table_get_bool(repl, "echo") && value.tag != VM_TAG_NIL) {
            vm_io_debug(stdout, 0, "", value, NULL);
        }

        if (config->dump_time) {
            clock_t end = clock();

            double diff = (double)(end - start) / CLOCKS_PER_SEC * 1000;
            printf("took: %.3fms\n", diff);
        }
        free(input);
    }
}