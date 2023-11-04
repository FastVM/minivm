#include "../vm/ir.h"
#include "../vm/std/std.h"
#include "../vm/std/libs/io.h"
#include "../vm/lang/paka.h"
#include "../vm/be/tb.h"

void GC_disable();

void vm_main_help(char *arg0) {
    fprintf(stderr, "error: provide a command to minivm\n");
    fprintf(stderr, "example: %s run [FILE]\n", arg0);
}

int vm_main(char *argv0, int argc, char **argv) {
    if (argv[1] == NULL) {
        vm_main_help(argv0);
        return 1;
    } else if (!strcmp(argv[1], "time")) {
        clock_t start = clock();
        int res = vm_main(argv0, argc - 1, argv + 1);
        clock_t end = clock();
        double diff = (double) (end - start) / CLOCKS_PER_SEC;
        printf("\n--- took %fs ---\n", diff);
        return res;
    } else if (!strcmp(argv[1], "ir")) {
        argv += 1;
        argc -= 1;
        bool expr = false;
        while (argv[1] != NULL) {
            if (!strcmp(argv[1], "-e")) {
                expr = true;
            } else if (!strcmp(argv[1], "-f")) {
                expr = false;
            } else {
                char *filename = argv[1];
                char *src = expr ? filename : vm_io_read(filename);
                if (src == NULL) {
                    fprintf(stderr, "error: could not read file\n");
                    return 1;
                }
                vm_paka_blocks_t blocks = vm_paka_parse_blocks(src);
                for (size_t i = 0; i < blocks.len; i++) {
                    vm_print_block(stdout, blocks.blocks[i]);
                }
            }
            argv += 1;
            argc -= 1;
        }
        return 0;
    } else if (!strcmp(argv[1], "eval")) {
        for (int i = 2; i < argc; i++) {
            vm_block_t *block = vm_paka_parse(argv[i]);
            if (block == NULL) {
                fprintf(stderr, "error: could not parse file\n");
                return 1;
            }
            vm_std_value_t main_args[1];
            main_args[0].tag = VM_TAG_UNK;
            vm_tb_run(block, vm_std_new());
        }
        return 0;
    } else if (!strcmp(argv[1], "print")) {
        for (int i = 2; i < argc; i++) {
            vm_block_t *block = vm_paka_parse(argv[i]);
            if (block == NULL) {
                fprintf(stderr, "error: could not parse file\n");
                return 1;
            }
            vm_std_value_t main_args[1];
            main_args[0].tag = VM_TAG_UNK;
            vm_std_value_t res = vm_tb_run(block, vm_std_new());
            fprintf(stdout, "\n--- result #%i ---\n", i - 2);
            vm_io_debug(stdout, 0, "", res, NULL);
        }
        return 0;
    } else if (!strcmp(argv[1], "run")) {
        argv += 1;
        argc -= 1;
        size_t iters = 1;
        bool expr = false;
        char *filename = NULL;
        while (true) {
            if (argc < 2) {
                if (filename == NULL) {
                    fprintf(stderr, "error: minivm run: cannot run no file\n");
                    return 1;
                } else {
                    break;
                }
            }
            if (!strcmp(argv[1], "-e")) {
                expr = true;
            } else if (!strcmp(argv[1], "-f")) {
                expr = false;
            } else if (!strcmp(argv[1], "-n")) {
                argv += 1;
                argc -= 1;
                sscanf(argv[1], "%zu", &iters);
            } else if (filename != NULL) {
                fprintf(stderr, "error: cannot handle multiple files at cli\n");
                return 1;
            } else {
                filename = argv[1];
            }
            argv += 1;
            argc -= 1;
        }
        char *src = expr ? filename : vm_io_read(filename);
        if (src == NULL) {
            fprintf(stderr, "error: could not read file\n");
            return 1;
        }
        vm_block_t *block = vm_paka_parse(src);
        if (block == NULL) {
            fprintf(stderr, "error: could not parse file\n");
            return 1;
        }
        for (size_t i = 0; i < iters; i++) {
            vm_tb_run(block, vm_std_new());
        }
        return 0;
    } else {
        vm_main_help(argv0);
        return 1;
    }
}

int main(int argc, char **argv) {
    vm_init_mem();
    char *argv0 = argv[0];
    return vm_main(argv0, argc, argv);
}
