#include "../vm/asm.h"

#include "../vm/interp/int3.h"

static char *vm_asm_io_read(const char *filename) {
    void *file = fopen(filename, "rb");
    if (file == NULL) {
        return NULL;
    }
    size_t nalloc = 16;
    char *ops = vm_malloc(sizeof(char) * nalloc);
    size_t nops = 0;
    size_t size;
    for (;;) {
        size = fread(&ops[nops], sizeof(char), 1, file);
        if (size == 0) {
            break;
        }
        nops += 1;
        if (nops + 4 >= nalloc) {
            nalloc *= 4;
            ops = vm_realloc(ops, sizeof(char) * nalloc);
        }
    }
    ops[nops] = '\0';
    fclose(file);
    return ops;
}

void vm_jit_run(vm_block_t *block);

int main(int argc, char **argv) {
    const char *filename = NULL;
    size_t runs = 1;
    bool jon = false;
    while (true) {
        if (argc < 2) {
            if (filename == NULL) {
                fprintf(stderr, "too few args\n");
                return 1;
            } else {
                break;
            }
        }
        if (!strcmp(argv[1], "-n")) {
            argv += 1;
            argc -= 1;
            size_t n = 0;
            char *ptr = argv[1];
            while (*ptr != '\0') {
                n *= 10;
                n += (size_t)(*ptr - '0');
                ptr += 1;
            }
            argv += 1;
            argc -= 1;
            runs = n;
            continue;
        } else if (!strcmp(argv[1], "-joff")) {
            jon = false;
            argv += 1;
            argc -= 1;
        } else if (!strcmp(argv[1], "-jon")) {
            jon = true;
            argv += 1;
            argc -= 1;
        } else if (filename != NULL) {
            fprintf(stderr, "cannot handle multiple files at cli\n");
            return 1;
        } else {
            filename = argv[1];
            argv += 1;
            argc -= 1;
        }
    }
    char *src = vm_asm_io_read(filename);
    if (src == NULL) {
        fprintf(stderr, "could not read file\n");
        return 1;
    }
    for (size_t i = 0; i < runs; i++) {
        vm_block_t *block = vm_parse_asm(src);
        if (block == NULL) {
            break;
        } else if (jon) {
            vm_jit_run(block);
        } else {
            vm_state_t *state = vm_state_init(1 << 16);
            vm_run(state, block);
            vm_state_deinit(state);
        }
    }
    vm_free((void *)src);
    return 0;
}
