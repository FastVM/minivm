#include "../vm/ir.h"
#include "../vm/std/std.h"
#include "../vm/std/libs/io.h"
#include "../vm/std/libs/dot.h"
#include "../vm/lang/paka.h"
#include "../vm/jit/x64.h"

#include <readline/readline.h>
#include <readline/history.h>
#include <time.h>

void vm_asm_repl(void) {
    int count = 1;
    vm_table_t *std = vm_std_new();
    read_history(".minivm-history");
    while (true) {
        char in_n[32];
        int in_len = snprintf(in_n, 31, "(%d)> ", count);
        char *buf = readline(in_n);
        if (buf == NULL) {
            return;
        }
        add_history(buf);
        remove(".minivm-history");
        write_history(".minivm-history");
        struct timespec ts1;
        clock_gettime(CLOCK_REALTIME, &ts1);
        vm_block_t *block = vm_paka_parse(buf);
        if (block == NULL) {
            printf("error: could not parse file\n");
            continue;
        }
        vm_std_value_t main_args[1];
        main_args[0].tag = VM_TAG_UNK;
        vm_std_value_t got;
        got = vm_x64_run(block, std, main_args);
        struct timespec ts2;
        clock_gettime(CLOCK_REALTIME, &ts2);
        size_t time = ts2.tv_nsec - ts1.tv_nsec;
        char out_n[32];
        int out_len = snprintf(out_n, 31, "#%i = ", count);
        vm_io_debug(stdout, 0, out_n, got, NULL);
        vm_pair_t pair = (vm_pair_t) {
            .key_val.str = "time",
            .key_tag = VM_TAG_STR,
        };
        vm_table_get_pair(std, &pair);
        for (size_t i = 0; i < out_len - 2; i++) {
            printf(" ");
        }
        printf("^ ");
        if (time < 1000) {
            printf("took %zuns\n", time % 1000);
        } else if (time < 1000 * 1000) {
            printf("took %zuus %zuns\n", time / 1000 % 1000, time % 1000);
        } else if (time < 1000 * 1000 * 1000) {
            printf("took %zums %zuus %zuns\n", time / 1000 / 1000 % 1000, time / 1000 % 1000, time % 1000);
        } else {
            printf("took %zus %zums %zuus %zuns\n", time / 1000 / 1000 / 1000, time / 1000 / 1000 % 1000, time / 1000 % 1000, time % 1000);
        }
        count += 1;
    }
}

enum {
    VM_DUMP_IR,
    VM_DUMP_DOT,
};

int main(int argc, char **argv) {
    vm_init_mem();
    if (!strcmp(argv[1], "ir")) {
        argv += 1;
        argc -= 1;
        while (argv[1] != NULL) {
            const char *filename = argv[1];
            char *src = vm_io_read(filename);
            if (src == NULL) {
                fprintf(stderr, "error: could not read file\n");
                return 1;
            }
            vm_paka_blocks_t blocks = vm_paka_parse_blocks(src);
            for (size_t i = 0; i < blocks.len; i++) {
                vm_print_block(stdout, blocks.blocks[i]);
            }
            argv += 1;
            argc -= 1;
        }
    } else if (!strcmp(argv[1], "dot")) {
        argv += 1;
        argc -= 1;
        while (argv[1] != NULL) {
            const char *filename = argv[1];
            char *src = vm_io_read(filename);
            if (src == NULL) {
                fprintf(stderr, "error: could not read file\n");
                return 1;
            }
            vm_block_t *block = vm_paka_parse(src);
            vm_dot_block(stdout, block);
            argv += 1;
            argc -= 1;
        }
    } else if (!strcmp(argv[1], "repl")) {
        vm_asm_repl();
    } else if (!strcmp(argv[1], "run")) {
        argv += 1;
        argc -= 1;
        const char *filename = NULL;
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
            } else {
                filename = argv[1];
                argv += 1;
                argc -= 1;
            }
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
