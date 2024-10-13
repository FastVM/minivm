
#include "repl.h"
#include "../ir.h"
#include "../io.h"
#include "../vm.h"
#include "../obj.h"
#include "../ast/ast.h"
#include "../ast/comp.h"
#include "../backend/backend.h"
#include "../primes.inc"

#include "../../vendor/tree-sitter/lib/include/tree_sitter/api.h"

#include "../../vendor/isocline/include/isocline.h"
#include "../../vendor/isocline/src/completions.h"

const TSLanguage *tree_sitter_lua(void);
vm_ast_node_t vm_lang_lua_parse(vm_t *vm, const char *str, const char *file);

void vm_lang_lua_repl_completer(ic_completion_env_t *cenv, const char *prefix) {
    vm_t *vm = cenv->arg;
    const char *last_word;
    {
        ptrdiff_t len = strlen(prefix);
        ptrdiff_t head = len - 1;
        while (head >= 0 && (isalnum(prefix[head]) || prefix[head] == '.')) {
            head -= 1;
        }
        head += 1;
        last_word = &prefix[head];
    }
    vm_obj_table_t *std = vm_obj_get_table(vm->std);
with_new_std:;
    uint64_t len = vm_primes_table[std->size];
    for (size_t i = 0; i < len; i++) {
        vm_table_pair_t *pair = &std->pairs[i];
        if (vm_obj_is_table(pair->key)) {
            const char *got = vm_obj_get_string(pair->key)->buf;
            size_t i = 0;
            while (got[i] != '\0') {
                if (last_word[i] == '\0') {
                    const char *completions[2];
                    completions[0] = got + i;
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
            if (vm_obj_is_nil(pair->value)) {
                if (last_word[i] == '.') {
                    std = vm_obj_get_table(pair->key);
                    last_word = &last_word[i + 1];
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

void vm_lang_lua_repl_highlight_walk(ic_highlight_env_t *henv, size_t *depth, TSNode node) {
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
    const char *keywords[] = {
        "and",
        "do",
        "else",
        "end",
        "for",
        "function",
        "if",
        "local",
        "not",
        "or",
        "return",
        "then",
        "while",
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
        vm_lang_lua_repl_highlight_walk(henv, depth, sub);
    }
}

void vm_lang_lua_repl_highlight(ic_highlight_env_t *henv, const char *input, void *arg) {
    (void)arg;
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
    vm_lang_lua_repl_highlight_walk(henv, &depth, root_node);
    ts_tree_delete(tree);
    ts_parser_delete(parser);
}

void vm_lang_lua_repl(vm_t *vm) {
    ic_set_history(".minivm-history", 2000);

    while (true) {
        char *input = ic_readline_ex(
            "lua",
            vm_lang_lua_repl_completer,
            vm,
            vm_lang_lua_repl_highlight,
            vm
        );

        if (input == NULL) {
            break;
        }
        
        ic_history_add(input);

        vm_ir_block_t *entry = vm_lang_lua_compile(vm, input, "__repl__");

        free(input);

        vm_obj_t value = vm_run_repl(vm, entry);

        if (vm_obj_is_error(value)) {
            vm_error_report(vm_obj_get_error(value), stderr);
        } else if (!vm_obj_is_nil(value)) {
            vm_io_buffer_t *buf = vm_io_buffer_new();
            vm_io_buffer_obj_debug(buf, 0, "", value, NULL);
            printf("%.*s", (int)buf->len, buf->buf);
            vm_free(buf->buf);
            vm_free(buf);
        }
    }
}