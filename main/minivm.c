#include "../vm/ast/build.h"
#include "../vm/ast/comp.h"
#include "../vm/ast/print.h"
#include "../vm/be/tb.h"
#include "../vm/ir.h"
#include "../vm/lang/eb.h"
#include "../vm/std/util.h"
#include "../vm/std/libs/io.h"
#include "../vm/std/std.h"
#include "isocline/isocline.h"

vm_ast_node_t vm_lang_lua_parse(vm_config_t *config, const char *str);

vm_std_value_t vm_main_table_get(vm_table_t *table, const char *key) {
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

void vm_main_table_set_config(vm_table_t *table, vm_config_t *config) {
    VM_STD_SET_BOOL(table, "opt", config->use_tb_opt);
    vm_table_t *dump = vm_table_new();
    VM_STD_SET_BOOL(table, "dump", dump);
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

bool vm_main_table_get_bool(vm_table_t *table, const char *key) {
    vm_std_value_t got = vm_main_table_get(table, key);
    return got.tag != VM_TAG_NIL && (got.tag != VM_TAG_BOOL || got.value.b);
}

int main(int argc, char **argv) {
    vm_init_mem();
    vm_config_t val_config = (vm_config_t){
        .use_tb_opt = true,
        .use_num = VM_USE_NUM_I32,
    };
    vm_config_t *config = &val_config;
    bool dry_run = false;
    bool echo = false;
    const char *lang = "lua";
    bool isrepl = true;
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (!strcmp(arg, "--")) {
            break;
        }
        if (!strcmp(arg, "--opt")) {
            config->use_tb_opt = true;
        } else if (!strcmp(arg, "--no-opt")) {
            config->use_tb_opt = false;
        } else if (!strcmp(arg, "--echo")) {
            echo = true;
        } else if (!strcmp(arg, "--no-echo")) {
            echo = false;
        } else if (!strcmp(arg, "--dry-run")) {
            dry_run = true;
        } else if (!strncmp(arg, "--number=", 9)) {
            arg += 9;
            if (!strcmp(arg, "i8")) {
                config->use_num = VM_USE_NUM_I8;
            } else if (!strcmp(arg, "i16")) {
                config->use_num = VM_USE_NUM_I16;
            } else if (!strcmp(arg, "i32")) {
                config->use_num = VM_USE_NUM_I32;
            } else if (!strcmp(arg, "i64")) {
                config->use_num = VM_USE_NUM_I64;
            } else if (!strcmp(arg, "f32")) {
                config->use_num = VM_USE_NUM_F32;
            } else if (!strcmp(arg, "f64")) {
                config->use_num = VM_USE_NUM_F64;
            } else {
                fprintf(stderr, "cannot use have as a number type: %s\n", arg);
                return 1;
            }
        } else if (!strncmp(arg, "--dump-", 7) || !strncmp(arg, "--dump=", 7)) {
            arg += 7;
            if (!strcmp(arg, "src")) {
                config->dump_src = true;
            } else if (!strcmp(arg, "ast")) {
                config->dump_ast = true;
            } else if (!strcmp(arg, "ir")) {
                config->dump_ir = true;
            } else if (!strcmp(arg, "ver")) {
                config->dump_ver = true;
            } else if (!strcmp(arg, "tb")) {
                config->dump_tb = true;
            } else if (!strcmp(arg, "opt")) {
                config->dump_tb_opt = true;
            } else if (!strcmp(arg, "tb-dot")) {
                config->dump_tb_dot = true;
            } else if (!strcmp(arg, "opt-dot")) {
                config->dump_tb_opt_dot = true;
            } else if (!strcmp(arg, "x86")) {
                config->dump_x86 = true;
            } else if (!strcmp(arg, "args")) {
                config->dump_args = true;
            } else if (!strcmp(arg, "time")) {
                config->dump_time = true;
            } else {
                fprintf(stderr, "cannot dump: %s\n", arg);
                return 1;
            }
        } else {
            isrepl = false;

            clock_t start = clock();

            const char *src;
            if (!strcmp(arg, "-e")) {
                src = argv[i + 1];
                i += 1;
            } else {
                src = vm_io_read(arg);
            }

            if (src == NULL) {
                fprintf(stderr, "error: no such file: %s\n", src);
            }

            if (config->dump_src) {
                printf("\n--- src ---\n");
                printf("%s\n", src);
            }

            vm_ast_node_t node;
            if (!strcmp(lang, "lua")) {
                node = vm_lang_lua_parse(config, src);
            } else if (!strcmp(lang, "ast")) {
                node = vm_lang_eb_parse(config, src);
            } else {
                fprintf(stderr, "not supported: lang %s\n", lang);
                return 1;
            }

            if (config->dump_ast) {
                printf("\n--- ast ---\n");
                vm_ast_print_node(stdout, 0, "", node);
            }

            vm_ast_blocks_t blocks = vm_ast_comp(node);

            if (config->dump_ir) {
                vm_print_blocks(stdout, blocks.len, blocks.blocks);
            }

            if (!dry_run) {
                vm_table_t *std = vm_std_new();
                // vm_io_debug(stdout, 0, "std = ", (vm_std_value_t) {.tag = VM_TAG_TAB, .value.table = std,}, NULL);
                vm_std_value_t value = vm_tb_run(config, blocks.len, blocks.blocks, std);
                if (echo) {
                    vm_io_debug(stdout, 0, "", value, NULL);
                }
                // vm_io_debug(stdout, 0, "std = ", (vm_std_value_t) {.tag = VM_TAG_TAB, .value.table = std,}, NULL);
            }

            if (config->dump_time) {
                clock_t end = clock();

                double diff = (double)(end - start) / CLOCKS_PER_SEC * 1000;
                printf("took: %.3fms\n", diff);
            }
        }
    }

    vm_table_t *std = vm_std_new();

    vm_table_t *repl = vm_table_new();
    vm_main_table_set_config(repl, config);
    VM_STD_SET_BOOL(repl, "echo", true);
    VM_STD_SET_TAB(std, "repl", repl);

    char *input;
    while ((input = ic_readline("lua")) != NULL) {
        clock_t start = clock();

        if (config->dump_src) {
            printf("\n--- src ---\n");
            printf("%s\n", input);
        }

        vm_ast_node_t node;
        if (!strcmp(lang, "lua")) {
            node = vm_lang_lua_parse(config, input);
        } else if (!strcmp(lang, "ast")) {
            node = vm_lang_eb_parse(config, input);
        } else {
            fprintf(stderr, "not supported: lang %s\n", lang);
            return 1;
        }

        if (config->dump_ast) {
            printf("\n--- ast ---\n");
            vm_ast_print_node(stdout, 0, "", node);
        }

        vm_ast_blocks_t blocks = vm_ast_comp(node);

        if (config->dump_ir) {
            vm_print_blocks(stdout, blocks.len, blocks.blocks);
        }

        if (!dry_run) {
            // vm_io_debug(stdout, 0, "std = ", (vm_std_value_t) {.tag = VM_TAG_TAB, .value.table = std,}, NULL);
            vm_std_value_t value = vm_tb_run(config, blocks.len, blocks.blocks, std);
            if (vm_main_table_get_bool(repl, "echo") && value.tag != VM_TAG_NIL) {
                vm_io_debug(stdout, 0, "", value, NULL);
            }
            // vm_io_debug(stdout, 0, "std = ", (vm_std_value_t) {.tag = VM_TAG_TAB, .value.table = std,}, NULL);
        }

        if (config->dump_time) {
            clock_t end = clock();

            double diff = (double)(end - start) / CLOCKS_PER_SEC * 1000;
            printf("took: %.3fms\n", diff);
        }
        free(input);
    }

    return 0;
}
