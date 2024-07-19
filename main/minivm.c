#include "../vm/ast/comp.h"
#include "../vm/backend/backend.h"
#include "../vm/vm.h"
#include "../vm/ir.h"
#include "../vm/io.h"
#include "../vm/std.h"
#include "../vm/lua/repl.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv) {
    vm_t *vm = vm_state_new();

    bool echo = false;
    bool isrepl = true;
    int i = 1;
    while (i < argc) {
        char *arg = argv[i++];
        if (!strcmp(arg, "--")) {
            break;
        }
        if (!strcmp(arg, "-v")) {
            printf("MiniVM " VM_VERSION "\n");
        } else if (!strcmp(arg, "--echo")) {
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
        } else {
            isrepl = false;

            clock_t start = clock();

            const char *name = NULL;
            const char *src;
            bool f_flag = true;
            if (!strcmp(arg, "-e")) {
                src = vm_strdup(argv[i++]);
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
                vm_state_delete(vm);
                return 1;
            }

            vm_block_t *entry = vm_compile(vm, src, name ? name : "__expr__");

            vm_obj_t value = vm_run_main(vm, entry);
            if (value.tag == VM_TAG_ERROR) {
                vm_state_delete(vm);
                return 1;
            }
            if (echo) {
                vm_io_buffer_t buf = {0};
                vm_io_debug(&buf, 0, "", value, NULL);
                printf("%.*s", (int)buf.len, buf.buf);
            }

            vm_free(src);

            if (!f_flag) {
                break;
            }
        }
    }

    if (isrepl) {
        vm_repl(vm);
    }

    vm_state_delete(vm);

    return 0;
}
