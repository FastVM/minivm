
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

#if defined(EMSCRIPTEN)
#include "../save/value.h"

#include <emscripten.h>
#endif


const TSLanguage *tree_sitter_lua(void);
vm_ast_node_t vm_lang_lua_parse(vm_t *vm, const char *str);

vm_std_value_t vm_repl_table_get(vm_table_t *table, const char *key) {
    vm_table_pair_t pair = (vm_table_pair_t){
        .key_tag = VM_TAG_STR,
        .key_val.str = key,
    };
    vm_table_get_pair(table, &pair);
    return (vm_std_value_t){
        .value = pair.val_val,
        .tag = pair.val_tag,
    };
}

bool vm_repl_table_get_bool(vm_table_t *table, const char *key) {
    vm_std_value_t got = vm_repl_table_get(table, key);
    return got.tag != VM_TAG_NIL && (got.tag != VM_TAG_BOOL || got.value.b);
}

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
        if (pair->key_tag == VM_TAG_STR) {
            const char *got = pair->key_val.str;
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
            if (pair->val_tag == VM_TAG_NIL) {
                if (last_word[i] == '.') {
                    std = pair->val_val.table;
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
    // FILE *out = fopen("out.log", "w");
    // fprintf(out, "%s\n", input);
    // fclose(out);
    // ic_highlight(henv, 1, strlen(input) - 2, "keyword");
}

#if defined(EMSCRIPTEN)
EM_JS(void, vm_repl_sync, (void), {
    Module._vm_repl_sync();
});
#endif

#include "../../vendor/c11threads/threads.h"

void vm_repl(vm_t *vm) {
    ic_set_history(".minivm-history", 2000);

    while (true) {
#if defined(EMSCRIPTEN)
        vm_repl_sync();
#endif

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

        mtx_lock(vm->mutex);

        if (vm->save_file != NULL) {
            FILE *f = fopen(vm->save_file, "rb");
            if (f != NULL) {
                vm_save_t save = vm_save_load(f);
                fclose(f);
                vm_load_value(vm, save);
            }
        }
        
        ic_history_add(input);

        vm_block_t *entry = vm_compile(vm, input);

        vm_std_value_t value = vm_run_repl(vm, entry);

        if (value.tag == VM_TAG_ERROR) {
            fprintf(stderr, "error: %s\n", value.value.str);
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

        mtx_unlock(vm->mutex);
    }
}