#include "../vm/ast/comp.h"
#include "../vm/ast/print.h"
#include "../vm/backend/tb.h"
#include "../vm/ir/ir.h"
#include "../vm/std/io.h"
#include "../vm/std/std.h"
#include "../vm/config.h"

vm_ast_node_t vm_lang_lua_parse(vm_config_t *config, const char *str);
void vm_lang_lua_repl(vm_config_t *config, vm_table_t *std, vm_blocks_t *blocks);

// void GC_disable(void);

int main(int argc, char **argv) {
    vm_init_mem();
    // GC_disable();
    vm_config_t val_config = (vm_config_t){
        .use_tb_opt = false,
        .use_num = VM_USE_NUM_I64,
#if defined(EMSCRIPTEN)
        .target = VM_TARGET_TB_EMCC,
#else
        .target = VM_TARGET_TB,
#endif
    };
    vm_blocks_t val_blocks = {0};
    vm_blocks_t *blocks = &val_blocks;
    vm_config_t *config = &val_config;
    bool echo = false;
#if defined(EMSCRIPTEN)
    bool isrepl = false;
#else
    bool isrepl = true;
#endif
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
        } else if (!strcmp(arg, "--ver-count=none")) {
            config->use_ver_count = VM_USE_VERSION_COUNT_NONE;
        } else if (!strcmp(arg, "--ver-count=global")) {
            config->use_ver_count = VM_USE_VERSION_COUNT_GLOBAL;
        } else if (!strcmp(arg, "--ver-count=fine")) {
            config->use_ver_count = VM_USE_VERSION_COUNT_FINE;
        } else if (!strcmp(arg, "--profile")) {
            config->use_profile = true;
        } else if (!strcmp(arg, "--no-profile")) {
            config->use_profile = false;
        } else if (!strcmp(arg, "--echo")) {
            echo = true;
        } else if (!strcmp(arg, "--no-echo")) {
            echo = false;
        } else if (!strcmp(arg, "--repl")) {
            vm_lang_lua_repl(config, std, blocks);
            isrepl = false;
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
            } else if (!strcmp(arg, "asm")) {
                config->dump_asm = true;
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
                fprintf(stderr, "error: no such file: %s\n", arg);
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

            vm_ast_comp_more(node, blocks);

            if (config->dump_ir) {
                vm_io_buffer_t buf = {0};
                vm_io_format_blocks(&buf, blocks);
                printf("\n--- ir ---\n%.*s", (int) buf.len, buf.buf);
            }

            vm_std_value_t value = vm_tb_run_main(config, blocks->entry, blocks, std);
            if (echo) {
                printf("tag = %zu\n", (size_t) value.tag);
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

    if (isrepl) {
        // vm_lang_lua_repl(config, std, blocks);
    }

    return 0;
}
