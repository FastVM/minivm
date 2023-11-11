#include "../vm/ast/build.h"
#include "../vm/ast/comp.h"
#include "../vm/ast/print.h"
#include "../vm/be/tb.h"
#include "../vm/ir.h"
#include "../vm/std/libs/io.h"
#include "../vm/std/std.h"
#include "../vm/lang/eb.h"

vm_ast_node_t vm_lang_lua_parse(vm_config_t *config, const char *str);

int main(int argc, char **argv) {
    vm_init_mem();
    vm_config_t val_config = (vm_config_t) {
        .use_tb_opt = true,
    };
    vm_config_t *config = &val_config;
    bool dry_run = false;
    bool echo = false;
    const char *lang = "lua";
    for (size_t i = 1; i < argc; i++) {
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
        } else if (!strncmp(arg, "--dump-", 7)) {
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
            clock_t start = clock();
            
            const char *src = vm_io_read(arg);

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
                vm_std_value_t value = vm_tb_run(config, blocks.blocks[0], std);
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
    return 0;
}
