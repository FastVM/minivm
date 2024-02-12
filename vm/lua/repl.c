#include "./repl.h"
#include "./parser.h"
#include "../ir/ir.h"
#include "../ast/ast.h"
#include "../ast/comp.h"
#include "../ast/print.h"
#include "../backend/tb.h"
#include "../std/io.h"
#include "../std/util.h"
#include "../../vendor/trees/api.h"

const TSLanguage *tree_sitter_lua(void);
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
    config->use_tb_opt = vm_lang_lua_repl_table_get_bool(table, "opt");
    vm_table_t *dump = vm_table_lookup_str(table, "dump")->val_val.table;
    config->dump_src = vm_lang_lua_repl_table_get_bool(dump, "src");
    config->dump_ast = vm_lang_lua_repl_table_get_bool(dump, "ast");
    config->dump_ir = vm_lang_lua_repl_table_get_bool(dump, "ir");
    config->dump_ver = vm_lang_lua_repl_table_get_bool(dump, "ver");
    config->dump_tb = vm_lang_lua_repl_table_get_bool(dump, "tb");
    config->dump_tb_opt = vm_lang_lua_repl_table_get_bool(dump, "tb_opt");
    config->dump_tb_dot = vm_lang_lua_repl_table_get_bool(dump, "tb_dot");
    config->dump_tb_opt_dot = vm_lang_lua_repl_table_get_bool(dump, "tb_opt_dot");
    config->dump_asm = vm_lang_lua_repl_table_get_bool(dump, "asm");
    config->dump_args = vm_lang_lua_repl_table_get_bool(dump, "args");
    config->dump_time = vm_lang_lua_repl_table_get_bool(dump, "time");
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
    VM_STD_SET_BOOL(dump, "asm", config->dump_asm);
    VM_STD_SET_BOOL(dump, "args", config->dump_args);
    VM_STD_SET_BOOL(dump, "time", config->dump_time);
}

void vm_lang_lua_repl_completer(ic_completion_env_t *cenv, const char *prefix) {
    vm_lang_lua_repl_complete_state_t *state = cenv->arg;
    ptrdiff_t len = strlen(prefix);
    ptrdiff_t head = len - 1;
    while (head >= 0 &&(iswalnum(prefix[head]) || prefix[head] == '.')) {
        head -= 1;
    }
    head += 1;
    const char *last_word = &prefix[head];
    vm_table_t *std = state->std;
with_new_std:;
    for (size_t i = 0; i < (1 << std->alloc); i++) {
        vm_pair_t *pair = &std->pairs[i];
        if (pair->key_tag == VM_TAG_STR) {
            const char *got = pair->key_val.str;
            size_t i = 0;
            while (got[i] != '\0') {
                if (last_word[i] == '\0') {
                    const char *completions[2];
                    completions[0] = got+i;
                    completions[1] = NULL;
                    if (!ic_add_completions(cenv, "", completions)) {
                        goto ret;
                    }
                    break;
                } else if (got[i] != last_word[i]) {
                    break;
                } else {
                    i += 1;
                    continue;
                }
            }
            if (pair->val_tag == VM_TAG_TAB) {
                if (last_word[i] == '.') {
                    std = pair->val_val.table;
                    last_word = &last_word[i+1];
                    goto with_new_std;
                }
                if (!strcmp(last_word, got)) {
                    const char *completions[2];
                    completions[0] = ".";
                    completions[1] = NULL;
                    if (!ic_add_completions(cenv, "", completions)) {
                        goto ret;
                    }
                }
            }
        }
    }
ret:;
}

const char *vm_lang_lua_repl_highlight_bracket_color(vm_table_t *repl, size_t depth) {
    if (repl == NULL) {
        return "";
    }
    vm_pair_t *value = vm_table_lookup_str(repl, "parens");
    if (value == NULL) {
        return "";
    }
    if (value->val_tag != VM_TAG_TAB) {
        return "";
    }
    if (value->val_val.table->len == 0) {
        return "";
    }
    vm_table_t *tab = value->val_val.table;
    vm_pair_t *sub = vm_table_lookup(tab, (vm_value_t) {.i32 = (int32_t) depth % (int32_t) tab->len + 1}, VM_TAG_I32);
    if (sub == NULL || sub->val_tag != VM_TAG_STR) {
        return "";
    }
    return sub->val_val.str;
}

void vm_lang_lua_repl_highlight_walk(ic_highlight_env_t *henv, vm_table_t *repl, size_t *depth, TSNode node) {
    const char *type = ts_node_type(node);
    size_t start = ts_node_start_byte(node);
    size_t end = ts_node_end_byte(node);
    size_t len = end - start;
    if (!strcmp("string", type)) {
        ic_highlight(henv, start, len, "orange");
    }
    if (!strcmp("number", type)) {
        ic_highlight(henv, start, len, "lightgreen");
    }
    if (!strcmp("identifier", type)) {
        ic_highlight(henv, start, len, "white");
    }
    // if (!strcmp("binary_expression", type)) {
    //     TSNode node2 = ts_node_child(node, 1);
    //     size_t start2 = ts_node_start_byte(node2);
    //     size_t end2 = ts_node_end_byte(node2);
    //     size_t len2 = end2 - start2;
    //     ic_highlight(henv, start2, len2, "white");
    // }
    if (!strcmp("(", type)) {
        ic_highlight(henv, start, len, vm_lang_lua_repl_highlight_bracket_color(repl, *depth));
        *depth += 1;
    }
    if (!strcmp(")", type)) {
        *depth -= 1;
        ic_highlight(henv, start, len, vm_lang_lua_repl_highlight_bracket_color(repl, *depth));
    }
    const char *keywords[] = {
        "local",
        "function",
        "while",
        "do",
        "if",
        "then",
        "return",
        "else",
        "end",
        NULL,
    };
    for (size_t i = 0; keywords[i] != NULL; i++) {
        if (!strcmp(keywords[i], type)) {
            ic_highlight(henv, start, len, "keyword");
        }
    }
    size_t num_children = ts_node_child_count(node);
    for (size_t i = 0; i < num_children; i++) {
        TSNode sub = ts_node_child(node, i);
        vm_lang_lua_repl_highlight_walk(henv, repl, depth, sub);
    }
}

void vm_lang_lua_repl_highlight(ic_highlight_env_t *henv, const char *input, void *arg) {
    vm_lang_lua_repl_highlight_state_t *state = arg;
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_lua());
    TSTree *tree = ts_parser_parse_string(
        parser,
        NULL,
        input,
        strlen(input)
    );
    TSNode root_node = ts_tree_root_node(tree);
    size_t depth = 0;
    vm_pair_t *value = vm_table_lookup_str(state->std, "config");
    vm_table_t *repl = NULL;
    if (value != NULL && value->val_tag == VM_TAG_TAB) {
        repl = value->val_val.table;
    }
    vm_lang_lua_repl_highlight_walk(henv, repl, &depth, root_node);
    ts_tree_delete(tree);
    ts_parser_delete(parser);
    // FILE *out = fopen("out.log", "w");
    // fprintf(out, "%s\n", input);
    // fclose(out);
    // ic_highlight(henv, 1, strlen(input) - 2, "keyword");
}

void vm_lang_lua_repl(vm_config_t *config, vm_table_t *std, vm_blocks_t *blocks) {
    config->is_repl = true;

    vm_table_t *repl = vm_table_new();
    vm_lang_lua_repl_table_set_config(repl, config);
    VM_STD_SET_BOOL(repl, "echo", true);
    vm_table_t *parens = vm_table_new();
    vm_table_set(parens, (vm_value_t) {.i32 = 1}, (vm_value_t) {.str = "yellow"}, VM_TAG_I32, VM_TAG_STR);
    vm_table_set(parens, (vm_value_t) {.i32 = 2}, (vm_value_t) {.str = "magenta"}, VM_TAG_I32, VM_TAG_STR);
    vm_table_set(parens, (vm_value_t) {.i32 = 3}, (vm_value_t) {.str = "blue"}, VM_TAG_I32, VM_TAG_STR);
    VM_STD_SET_TAB(repl, "parens", parens);
    VM_STD_SET_TAB(std, "config", repl);

    ic_set_history(".minivm-history", 2000);

    vm_lang_lua_repl_complete_state_t complete_state = (vm_lang_lua_repl_complete_state_t){
        .config = config,
        .std = std,
    };
    vm_lang_lua_repl_highlight_state_t highlight_state = (vm_lang_lua_repl_highlight_state_t){
        .config = config,
        .std = std,
    };

    #if defined(EMSCRIPTEN)
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    #endif

    while (true) {
        #if !defined(EMSCRIPTEN)
        char *input = ic_readline_ex(
            "lua", 
            vm_lang_lua_repl_completer,
            &complete_state,
            vm_lang_lua_repl_highlight, 
            &highlight_state
        );
        if (input == NULL) {
            break;
        }
        #else
        printf("lua> ");
        char input[256];
        size_t head = 0;
        while (true) {
            char c = fgetc(stdin);
            if (c == '\n' || c == '\r') {
                c = '\0';
                printf("\n");
            } else if (c == 127) {
                printf("\x1B[D \x1B[D");
                head -= 1;
            } else {
                printf("%c", (int) c);
            }
            input[head++] = c;
            if (c == '\0') {
                break;
            }
        }
        #endif
        vm_lang_lua_repl_table_get_config(repl, config);
        ic_history_add(input);
        clock_t start = clock();

        if (config->dump_src) {
            printf("\n--- src ---\n");
            printf("%s\n", input);
        }

        vm_ast_node_t node = vm_lang_lua_parse(config, input);
        #if !defined(EMSCRIPTEN)
        free(input);
        #endif

        if (config->dump_ast) {
            vm_io_buffer_t buf = {0};
            vm_ast_print_node(&buf, 0, "", node);
            printf("\n--- ast ---\n%.*s", (int) buf.len, buf.buf);
        }

        vm_ast_comp_more(node, blocks);
        vm_ast_free_node(node);

        if (config->dump_ir) {
            vm_io_buffer_t buf = {0};
            vm_io_format_blocks(&buf, blocks);
            printf("%.*s", (int) buf.len, buf.buf);
        }

        vm_std_value_t value = vm_tb_run_repl(config, blocks->entry, blocks, std);
        if (value.tag == VM_TAG_ERROR) {
            printf("error: %s\n", value.value.str);
        } else if (vm_lang_lua_repl_table_get_bool(repl, "echo") && value.tag != VM_TAG_NIL) {
            vm_io_buffer_t buf = {0};
            vm_io_debug(&buf, 0, "", value, NULL);
            printf("%.*s", (int) buf.len, buf.buf);
        }

        if (config->dump_time) {
            clock_t end = clock();

            double diff = (double)(end - start) / CLOCKS_PER_SEC * 1000;
            printf("took: %.3fms\n", diff);
        }
    }
}