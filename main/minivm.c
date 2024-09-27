#include "../vm/ast/comp.h"
#include "../vm/backend/backend.h"
#include "../vm/vm.h"
#include "../vm/ir.h"
#include "../vm/io.h"
#include "../vm/gc.h"
#include "../vm/std.h"
#include "../vm/lua/repl.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#if VM_USE_SPALL
#define SPALL_AUTO_IMPLEMENTATION
#include "../vendor/spall/auto.h"
#endif

__attribute__((no_instrument_function)) int main(int argc, char **argv) {
#if VM_USE_SPALL
    spall_auto_init("out.spall");
    spall_auto_thread_init(0, SPALL_DEFAULT_BUFFER_SIZE);
#endif

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
                exit(1);
            }

            vm_ir_block_t *entry = vm_lang_lua_compile(vm, src, name ? name : "__expr__");

            vm_obj_t value = vm_run_main(vm, entry);
            if (vm_obj_is_error(value)) {
                exit(1);
            }
            if (echo) {
                vm_io_buffer_t *buf = vm_io_buffer_new();
                vm_io_buffer_obj_debug(buf, 0, "", value, NULL);
                printf("%.*s", (int)buf->len, buf->buf);
                vm_free(buf->buf);
                vm_free(buf);
            }

            vm_free(src);

            if (!f_flag) {
                break;
            }
        }
    }

    vm_state_delete(vm);

#if VM_USE_SPALL
    spall_auto_thread_quit();
    spall_auto_quit();
#endif
    return 0;
}
