#include "../vm/ast/comp.h"
#include "../vm/ast/print.h"
#include "../vm/backend/backend.h"
#include "../vm/config.h"
#include "../vm/ir/ir.h"
#include "../vm/save/value.h"
#include "../vm/std/io.h"
#include "../vm/std/std.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

vm_ast_node_t vm_lang_lua_parse(vm_config_t *config, const char *str);
void vm_lang_lua_repl(vm_config_t *config, vm_table_t *std, vm_blocks_t *blocks);

#if defined(EMSCRIPTEN)
#include <emscripten.h>
#endif

int main(int argc, char **argv) {
    vm_config_t val_config = (vm_config_t) {
        .use_num = VM_USE_NUM_F64,
        .lbbv = false,
        .tb_regs_cast = true,
#if defined(EMSCRIPTEN)
        .target = VM_TARGET_TB_EMCC,
#elif defined(__FreeBSD__) || defined(__APPLE__)
        .target = VM_TARGET_TB_CLANG,
#else
        .target = VM_TARGET_TB_TCC,
#endif
        .cflags = NULL,
    };
    vm_blocks_t val_blocks = {0};
    vm_blocks_t *blocks = &val_blocks;
    vm_config_t *config = &val_config;
    bool echo = false;
    bool isrepl = true;
    vm_table_t *std = vm_std_new(config);
    int i = 1;
    while (i < argc) {
        char *arg = argv[i++];
        if (!strcmp(arg, "--")) {
            break;
        }
        if (!strcmp(arg, "--opt")) {
            config->tb_opt = true;
        } else if (!strcmp(arg, "--no-opt")) {
            config->tb_opt = false;
        } else if (!strcmp(arg, "--ver-count=none")) {
            config->use_ver_count = VM_USE_VERSION_COUNT_NONE;
        } else if (!strcmp(arg, "--ver-count=global")) {
            config->use_ver_count = VM_USE_VERSION_COUNT_GLOBAL;
        } else if (!strcmp(arg, "--ver-count=fine")) {
            config->use_ver_count = VM_USE_VERSION_COUNT_FINE;
        } else if (!strcmp(arg, "--echo")) {
            echo = true;
        } else if (!strcmp(arg, "--no-echo")) {
            echo = false;
        } else if (!strcmp(arg, "--repl")) {
            vm_lang_lua_repl(config, std, blocks);
            isrepl = false;
        } else if (!strncmp(arg, "--file=", 7)) {
            arg += 7;
            isrepl = false;
            config->save_file = arg;
            vm_lang_lua_repl(config, std, blocks);
        } else if (!strncmp(arg, "--load=", 7)) {
            arg += 7;
            FILE *f = fopen(arg, "rb");
            if (f != NULL) {
                vm_save_t save = vm_save_load(f);
                fclose(f);
                vm_save_loaded_t ld = vm_load_value(config, save);
                if (ld.blocks != NULL) {
                    *blocks = *ld.blocks;
                    *std = *ld.env.value.table;
                    vm_io_buffer_t *buf = vm_io_buffer_new();
                    vm_io_format_blocks(buf, blocks);
                }
            }
        } else if (!strncmp(arg, "--save=", 7)) {
            arg += 7;
            vm_save_t save = vm_save_value(config, blocks, (vm_std_value_t){.tag = VM_TAG_TAB, .value.table = std});
            FILE *f = fopen(arg, "wb");
            if (f != NULL) {
                fwrite(save.buf, 1, save.len, f);
                fclose(f);
            }
        } else if (!strncmp(arg, "--cflag=", 8)) {
            arg += 8;
            const char *last = config->cflags;
            if (last == NULL) {
                size_t alloc = strlen(arg) + 1;
                char *buf = vm_malloc(sizeof(char) * alloc);
                snprintf(buf, alloc, "%s", arg);
                config->cflags = buf;
            } else {
                size_t alloc = strlen(last) + 1 + strlen(arg) + 1;
                char *buf = vm_malloc(sizeof(char) * alloc);
                snprintf(buf, alloc, "%s %s", last, arg);
                config->cflags = buf;
                vm_free(last);
            }
        } else if (!strcmp(arg, "--lbbv")) {
            config->lbbv = true;
        } else if (!strcmp(arg, "--no-lbbv")) {
            config->lbbv = false;
        } else if (!strncmp(arg, "--tb-", 5)) {
            arg += 5;
            bool enable = true;
            if (!strncmp(arg, "no-", 3)) {
                arg += 3;
                enable = false;
            }
            if (!strcmp(arg, "recompile")) {
                config->tb_recompile = enable;
            } else if (!strcmp(arg, "cast-regs")) {
                config->tb_regs_cast = enable;
            } else if (!strcmp(arg, "force-bitcast")) {
                config->tb_force_bitcast = enable;
            } else {
                fprintf(stderr, "error: unknown flag --tb-%s want --tb-[recompile/cast-regs/force-bitcast]", arg);
                return 1;
            }
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
                fprintf(stderr, "error: cannot use have as a number type: %s\n", arg);
                return 1;
            }
        } else if (!strncmp(arg, "--target-", 9) || !strncmp(arg, "--target=", 9)) {
            arg += 9;
#if defined(EMSCRIPTEN)
            if (!strcmp(arg, "tb-emcc")) {
                config->target = VM_TARGET_TB_EMCC;
            } else {
                fprintf(stderr, "cannot target: %s\n", arg);
                return 1;
            }
#else
            if (!strcmp(arg, "interp")) {
                config->target = VM_TARGET_INTERP;
            } else if (!strcmp(arg, "tb")) {
                config->target = VM_TARGET_TB;
#if defined(VM_USE_TCC)
            } else if (!strcmp(arg, "tb-tcc")) {
                config->target = VM_TARGET_TB_TCC;
#endif
            } else if (!strcmp(arg, "tb-cc")) {
                config->target = VM_TARGET_TB_CC;
            } else if (!strcmp(arg, "tb-gcc")) {
                config->target = VM_TARGET_TB_GCC;
            } else if (!strcmp(arg, "tb-clang")) {
                config->target = VM_TARGET_TB_CLANG;
            } else {
                fprintf(stderr, "cannot target: %s\n", arg);
                return 1;
            }
#endif
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
            } else if (!strcmp(arg, "asm")) {
                config->dump_asm = true;
            } else if (!strcmp(arg, "c")) {
                config->dump_c = true;
            } else if (!strcmp(arg, "time")) {
                config->dump_time = true;
            } else {
                fprintf(stderr, "cannot dump: %s\n", arg);
                return 1;
            }
        } else {
            isrepl = false;

            clock_t start = clock();

            const char *name = NULL;
            const char *src;
            if (!strcmp(arg, "-e")) {
                src = argv[i++];
            } else {
                src = vm_io_read(arg);
                name = arg;
                vm_std_set_arg(config, std, argv[0], name, argc - i, &argv[i]);
            }


            if (src == NULL) {
                fprintf(stderr, "error: no such file: %s\n", arg);
            }

            if (config->dump_src) {
                printf("\n--- src ---\n");
                printf("%s\n", src);
            }

            vm_ast_node_t node = vm_lang_lua_parse(config, src);
            vm_blocks_add_src(blocks, src);

            if (config->dump_ast) {
                vm_io_buffer_t buf = {0};
                vm_ast_print_node(&buf, 0, "", node);
                printf("\n--- ast ---\n%.*s", (int)buf.len, buf.buf);
            }

            vm_ast_comp_more(node, blocks);

            if (config->dump_ir) {
                vm_io_buffer_t buf = {0};
                vm_io_format_blocks(&buf, blocks);
                printf("\n--- ir ---\n%.*s", (int)buf.len, buf.buf);
            }

            vm_std_value_t value = vm_run_main(config, blocks->entry, blocks, std);
            if (value.tag == VM_TAG_ERROR) {
                exit(1);
            }
            if (echo) {
                vm_io_buffer_t buf = {0};
                vm_io_debug(&buf, 0, "", value, NULL);
                printf("%.*s", (int)buf.len, buf.buf);
            }

            if (config->dump_time) {
                clock_t end = clock();

                double diff = (double)(end - start) / CLOCKS_PER_SEC * 1000;
                printf("took: %.3fms\n", diff);
            }

            if (name != NULL) {
                break;
            }
        }
    }

    if (isrepl) {
        vm_std_set_arg(config, std, argv[0], "<repl>", argc - i, argv + i);

        vm_lang_lua_repl(config, std, blocks);
    }

    return 0;
}
