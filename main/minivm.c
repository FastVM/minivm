#include "../vm/ast/comp.h"
#include "../vm/ast/print.h"
#include "../vm/backend/backend.h"
#include "../vm/vm.h"
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
        if (!strcmp(arg, "--echo")) {
            echo = true;
        } else if (!strcmp(arg, "--no-echo")) {
            echo = false;
        } else if (!strcmp(arg, "--repl")) {
            vm_lang_lua_repl(config, std, blocks);
            isrepl = false;
        } else if (!strncmp(arg, "--file=", 7)) {
            arg += 7;
            config->save_file = arg;
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
        } else {
            isrepl = false;

            clock_t start = clock();

            const char *name = NULL;
            const char *src;
            bool f_flag = true;
            if (!strcmp(arg, "-e")) {
                src = argv[i++];
            } else {
                f_flag = !strcmp(arg, "-f");
                if (f_flag) {
                    arg = argv[i++];
                }
                src = vm_io_read(arg);
                name = arg;
                vm_std_set_arg(config, std, argv[0], name, argc - i, &argv[i]);
            }

            if (src == NULL) {
                fprintf(stderr, "error: no such file: %s\n", arg);
            }

            vm_ast_node_t node = vm_lang_lua_parse(config, src);
            vm_blocks_add_src(blocks, src);
            vm_ast_comp_more(node, blocks);

            vm_std_value_t value = vm_run_main(config, blocks->entry, blocks, std);
            if (value.tag == VM_TAG_ERROR) {
                exit(1);
            }
            if (echo) {
                vm_io_buffer_t buf = {0};
                vm_io_debug(&buf, 0, "", value, NULL);
                printf("%.*s", (int)buf.len, buf.buf);
            }

            if (!f_flag) {
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
