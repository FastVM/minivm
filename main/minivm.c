#include "../vm/ast/comp.h"
#include "../vm/backend/backend.h"
#include "../vm/vm.h"
#include "../vm/ir.h"
#include "../vm/save/value.h"
#include "../vm/io.h"
#include "../vm/std.h"
#include "../vm/lua/repl.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv) {
    vm_mem_init();
    
    vm_t *vm = vm_malloc(sizeof(vm_t));
    *vm = (vm_t) {
        .use_num = VM_USE_NUM_F64,
        .regs = vm_malloc(sizeof(vm_obj_t) * 65536),
    };

    vm_std_new(vm);
    
    vm->blocks = vm_malloc(sizeof(vm_blocks_t));
    *vm->blocks = (vm_blocks_t) {0};

    bool echo = false;
    bool isrepl = true;
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
        } else if (!strcmp(arg, "--dump-ir")) {
            vm->dump_ir = true;
        } else if (!strcmp(arg, "--no-dump-ir")) {
            vm->dump_ir = false;
        } else if (!strcmp(arg, "--repl")) {
            vm_repl(vm);
            isrepl = false;
        } else if (!strncmp(arg, "--file=", 7)) {
            arg += 7;
            vm->save_file = arg;
            FILE *f = fopen(vm->save_file, "rb");
            if (f != NULL) {
                vm_save_t save = vm_save_load(f);
                fclose(f);
                vm_load_value(vm, save);
            }
        } else if (!strncmp(arg, "--load=", 7)) {
            arg += 7;
            FILE *f = fopen(arg, "rb");
            if (f != NULL) {
                vm_save_t save = vm_save_load(f);
                fclose(f);
                vm_load_value(vm, save);
            }
        } else if (!strncmp(arg, "--save=", 7)) {
            arg += 7;
            vm_save_t save = vm_save_value(vm);
            FILE *f = fopen(arg, "wb");
            if (f != NULL) {
                fwrite(save.buf, 1, save.len, f);
                fclose(f);
            }
        } else if (!strncmp(arg, "--number=", 9)) {
            arg += 9;
            if (!strcmp(arg, "i8")) {
                vm->use_num = VM_USE_NUM_I8;
            } else if (!strcmp(arg, "i16")) {
                vm->use_num = VM_USE_NUM_I16;
            } else if (!strcmp(arg, "i32")) {
                vm->use_num = VM_USE_NUM_I32;
            } else if (!strcmp(arg, "i64")) {
                vm->use_num = VM_USE_NUM_I64;
            } else if (!strcmp(arg, "f32")) {
                vm->use_num = VM_USE_NUM_F32;
            } else if (!strcmp(arg, "f64")) {
                vm->use_num = VM_USE_NUM_F64;
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
                vm_std_set_arg(vm, argv[0], name, argc - i, &argv[i]);
            }

            if (src == NULL) {
                fprintf(stderr, "error: no such file: %s\n", arg);
                return 1;
            }

            vm_block_t *entry = vm_compile(vm, src);

            vm_obj_t value = vm_run_main(vm, entry);
            if (value.tag == VM_TAG_ERROR) {
                return 1;
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
        vm_repl(vm);
    }

    return 0;
}
