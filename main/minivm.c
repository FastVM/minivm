#include "../vm/ast/build.h"
#include "../vm/ast/comp.h"
#include "../vm/ast/print.h"
#include "../vm/backend/tb.h"
#include "../vm/ir/ir.h"
#include "../vm/std/util.h"
#include "../vm/std/io.h"
#include "../vm/std/std.h"
#include "../vm/config.h"

vm_ast_node_t vm_lang_lua_parse(vm_config_t *config, const char *str);
void vm_lang_lua_repl(vm_config_t *config, vm_table_t *std);

int main(int argc, char **argv) {
    vm_init_mem();
    vm_config_t val_config = (vm_config_t){
        .use_tb_opt = false,
        .use_num = VM_USE_NUM_I64,
    };
    vm_config_t *config = &val_config;
    bool dry_run = false;
    bool echo = false;
    bool isrepl = true;
    vm_table_t *std = vm_std_new();
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
        } else if (!strcmp(arg, "--repl")) {
            vm_lang_lua_repl(config, std);
            isrepl = false;
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
            bool last_isrepl = isrepl;
            isrepl = false;

            clock_t start = clock();

            const char *src;
            // if (!strcmp(arg, "-f")) {
            //     isrepl = last_isrepl;
            //     arg = argv[i + 1];
            //     i += 1;
            // }
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

            vm_ast_node_t node = vm_lang_lua_parse(config, src);

            if (config->dump_ast) {
                vm_io_buffer_t buf = {0};
                vm_ast_print_node(&buf, 0, "", node);
                printf("\n--- ast ---\n%.*s", (int) buf.len, buf.buf);
            }

            vm_ast_blocks_t blocks = vm_ast_comp(node);

            if (config->dump_ir) {
                vm_io_buffer_t buf = {0};
                vm_io_format_blocks(&buf, blocks.len, blocks.blocks);
                printf("%.*s", (int) buf.len, buf.buf);
            }

            if (!dry_run) {
                vm_std_value_t value = vm_tb_run_main(config, blocks.entry, blocks.len, blocks.blocks, std);
                if (echo) {
                    vm_io_buffer_t buf = {0};
                    vm_io_debug(&buf, 0, "", value, NULL);
                    printf("%.*s", (int) buf.len, buf.buf);
                }
            }

            if (config->dump_time) {
                clock_t end = clock();

                double diff = (double)(end - start) / CLOCKS_PER_SEC * 1000;
                printf("took: %.3fms\n", diff);
            }
        }
    }

    if (isrepl) {
        vm_lang_lua_repl(config, std);
    }

    return 0;
}
