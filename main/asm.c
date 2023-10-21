#include "../vm/ir.h"
#include "../vm/std/std.h"
#include "../vm/std/libs/io.h"
#include "../vm/lang/paka.h"
#include "../vm/jit/tb.h"
#include "../vm/opt/opt.h"

#define VM_DEFAULT_OPT "0"

int main(int argc, char **argv) {
    vm_init_mem();
    if (!strcmp(argv[1], "ir")) {
        argv += 1;
        argc -= 1;
        const char *opt = VM_DEFAULT_OPT;
        while (argv[1] != NULL) {
            const char *filename = argv[1];
            if (filename[0] == '-' && filename[1] == 'O') {
                opt = &filename[2];
            } else {
                char *src = vm_io_read(filename);
                if (src == NULL) {
                    fprintf(stderr, "error: could not read file\n");
                    return 1;
                }
                vm_paka_blocks_t blocks = vm_paka_parse_blocks(src);
                vm_opt_do_passes(opt, blocks.blocks[0]);
                for (size_t i = 0; i < blocks.len; i++) {
                    vm_print_block(stdout, blocks.blocks[i]);
                }
            }
            argv += 1;
            argc -= 1;
        }
    } else if (!strcmp(argv[1], "print")) {
        for (int i = 2; i < argc; i++) {
            vm_block_t *block = vm_paka_parse(argv[i]);
            if (block == NULL) {
                fprintf(stderr, "error: could not parse file\n");
                return 1;
            }
            vm_std_value_t main_args[1];
            main_args[0].tag = VM_TAG_UNK;
            vm_std_value_t res = vm_x64_run(block, vm_std_new(), main_args);
            fprintf(stdout, "\n--- result #%i ---\n", i - 2);
            vm_io_debug(stdout, 0, "", res, NULL);
        }
    } else if (!strcmp(argv[1], "run")) {
        argv += 1;
        argc -= 1;
        const char *filename = NULL;
        const char *opt = VM_DEFAULT_OPT;
        while (true) {
            if (argc < 2) {
                if (filename == NULL) {
                    fprintf(stderr, "error: minivm run: cannot run no file\n");
                    return 1;
                } else {
                    break;
                }
            }
            if (filename != NULL) {
                fprintf(stderr, "error: cannot handle multiple files at cli\n");
                return 1;
            } else if (argv[1][0] == '-' && argv[1][1] == 'O') {
                opt = &argv[1][2];
            } else {
                filename = argv[1];
            }
            argv += 1;
            argc -= 1;
        }
        char *src = vm_io_read(filename);
        if (src == NULL) {
            fprintf(stderr, "error: could not read file\n");
            return 1;
        }
        vm_block_t *block = vm_paka_parse(src);
        if (block == NULL) {
            fprintf(stderr, "error: could not parse file\n");
            return 1;
        }
        vm_free((void *)src);
        vm_opt_do_passes(opt, block);
        vm_std_value_t main_args[1];
        main_args[0].tag = VM_TAG_UNK;
        vm_x64_run(block, vm_std_new(), main_args);
        return 0;
    } else {
        fprintf(stderr, "error: provide a command to minivm\n");
        fprintf(stderr, "example: %s run [FILE]", argv[0]);
        return 0;
    }
}
