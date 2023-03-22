#include "../vm/asm.h"

#include "../vm/interp/int3.h"
#include "../vm/jit/jit.h"

vm_block_t *vm_paka_parse(const char *src);

static char *vm_asm_io_read(const char *filename) {
    void *file = fopen(filename, "rb");
    if (file == NULL) {
        return NULL;
    }
    size_t nalloc = 512;
    char *ops = vm_malloc(sizeof(char) * nalloc);
    size_t nops = 0;
    size_t size;
    for (;;) {
        if (nops + 256 >= nalloc) {
            nalloc = (nops + 256) * 2;
            ops = vm_realloc(ops, sizeof(char) * nalloc);
        }
        size = fread(&ops[nops], 1, 256, file);
        nops += size;
        if (size < 256) {
            break;
        }
    }
    ops[nops] = '\0';
    fclose(file);
    return ops;
}

void vm_x64_run(vm_block_t *block);

int main(int argc, char **argv) {
    const char *filename = NULL;
    size_t runs = 1;
    bool jon = false;
    const char *lang = "paka";
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
            continue;
        } else if (!strcmp(argv[1], "-jon")) {
            jon = true;
            argv += 1;
            argc -= 1;
            continue;
        } else if (!strcmp(argv[1], "-tasm")) {
            lang = "asm";
            argv += 1;
            argc -= 1;
            continue;
        } else if (!strcmp(argv[1], "-tpaka")) {
            lang = "paka";
            argv += 1;
            argc -= 1;
            continue;
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
    vm_block_t *block = NULL;
    if (!strcmp(lang, "paka")) {
        block = vm_paka_parse(src);
    } else {
        block = vm_parse_asm(src);
    }
    vm_free((void *)src);
    if (block == NULL) {
        return 1;
    }
    vm_jit_state_t *jstate = NULL;
    if (jon) {
        jstate = vm_jit_state_new();
    }
    for (size_t i = 0; i < runs; i++) {
        if (block == NULL) {
            fprintf(stderr, "could not parse file\n");
            return 1;
        } else if (jon) {
            vm_x64_run(block);
            vm_jit_run(jstate, block);
        } else {
            vm_state_t *state = (void*) vm_state_init(1 << 16);
            vm_run(state, block);
            vm_state_deinit( state);
        }
    }
    if (jon) {
        vm_jit_state_free(jstate);
    }
    return 0;
}
