
#include "./repl.h"
#include "../ir.h"
#include "../io.h"
#include "../vm.h"
#include "../obj.h"
#include "../ast/ast.h"
#include "../ast/comp.h"
#include "../save/value.h"
#include "../backend/backend.h"

#include "../../vendor/tree-sitter/lib/include/tree_sitter/api.h"

#include "../../vendor/isocline/include/isocline.h"
#include "../../vendor/isocline/src/completions.h"

const TSLanguage *tree_sitter_lua(void);
vm_ast_node_t vm_lang_lua_parse(vm_t *vm, const char *str, const char *file);

void vm_repl_completer(ic_completion_env_t *cenv, const char *prefix) {
    vm_t *vm = cenv->arg;
    ptrdiff_t len = strlen(prefix);
    ptrdiff_t head = len - 1;
    while (head >= 0 && (iswalnum(prefix[head]) || prefix[head] == '.')) {
        head -= 1;
    }
    head += 1;
    const char *last_word = &prefix[head];
    vm_table_t *std = vm->std.value.table;
with_new_std:;
    for (size_t i = 0; i < ((size_t)1 << std->alloc); i++) {
        vm_table_pair_t *pair = &std->pairs[i];
        if (pair->key.tag == VM_TAG_STR) {
            const char *got = pair->key.value.str->buf;
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
            if (pair->value.tag == VM_TAG_NIL) {
                if (last_word[i] == '.') {
                    std = pair->key.value.table;
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

void vm_repl_highlight_walk(ic_highlight_env_t *henv, size_t *depth, TSNode node) {
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
        "or",
        "and",
        "not",
        "local",
        "function",
        "while",
        "do",
        "if",
        "then",
        "return",
        "else",
        "end",
        "for",
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
        vm_repl_highlight_walk(henv, depth, sub);
    }
}

void vm_repl_highlight(ic_highlight_env_t *henv, const char *input, void *arg) {
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
    vm_repl_highlight_walk(henv, &depth, root_node);
    ts_tree_delete(tree);
    ts_parser_delete(parser);
}

void vm_repl(vm_t *vm) {
    ic_set_history(".minivm-history", 2000);

    while (true) {
        char *input = ic_readline_ex(
            "lua",
            vm_repl_completer,
            vm,
            vm_repl_highlight,
            vm
        );

        if (input == NULL) {
            break;
        }

        if (vm->save_file != NULL) {
            FILE *f = fopen(vm->save_file, "rb");
            if (f != NULL) {
                vm_save_t save = vm_save_load(f);
                fclose(f);
                vm_load_value(vm, save);
            }
        }
        
        ic_history_add(input);

        vm_block_t *entry = vm_compile(vm, input, "__repl__");

        free(input);

        vm_obj_t value = vm_run_repl(vm, entry);

        if (value.tag == VM_TAG_ERROR) {
            vm_error_report(value.value.error, stderr);
        } else if (value.tag != VM_TAG_NIL) {
            vm_io_buffer_t buf = {0};
            vm_io_debug(&buf, 0, "", value, NULL);
            printf("%.*s", (int)buf.len, buf.buf);
        }
        
        if (vm->save_file != NULL) {
            vm_save_t save = vm_save_value(vm);
            FILE *f = fopen(vm->save_file, "wb");
            if (f != NULL) {
                fwrite(save.buf, 1, save.len, f);
                fclose(f);
            }
        }
    }
}