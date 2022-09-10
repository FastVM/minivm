
#include "../vm/asm.h"

#include "../vm/ir/be/int3.h"
#include "../vm/ir/build.h"
#include "../vm/ir/toir.h"

static const char *vm_asm_io_read(const char *filename) {
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

int main(int argc, char **argv) {
    // const char *dump = "out.bc";
    const char *dump = NULL;
    const char *filename = NULL;
    size_t jit = 1;
    size_t runs = 1;
    size_t jitdumpir = 0;
    size_t jitdumpopt = 0;
    while (true) {
        if (argc < 2) {
            if (filename == NULL) {
                fprintf(stderr, "too few args\n");
                return 1;
            } else {
                break;
            }
        }
        if (!strcmp(argv[1], "-o") || !strcmp(argv[1], "--output")) {
            argv += 1;
            argc -= 1;
            dump = argv[1];
            argv += 1;
            argc -= 1;
            continue;
        }
        if (!strcmp(argv[1], "-n")) {
            argv += 1;
            argc -= 1;
            size_t n = 0;
            char *ptr = argv[1];
            while (*ptr != '\0') {
                n *= 10;
                n += *ptr - '0';
                ptr += 1;
            }
            argv += 1;
            argc -= 1;
            runs = n;
            continue;
        }
        if (argv[1][0] == '-' && argv[1][1] == 'i') {
            char *tmp = argv[1] + 2;
            argv += 1;
            argc -= 1;
            if (!strcmp(tmp, "dump=pre")) {
                jitdumpir = 1;
            } else if (!strcmp(tmp, "dump=opt")) {
                jitdumpopt = 1;
            } else {
                fprintf(stderr, "unknown -j option: -j%s\n", tmp);
                return 1;
            }
            continue;
        }
        if (filename != NULL) {
            fprintf(stderr, "cannot handle multiple files at cli\n");
            return 1;
        } else {
            filename = argv[1];
            argv += 1;
            argc -= 1;
        }
    }
    if (!jit && (jitdumpopt || jitdumpir)) {
        fprintf(stderr, "cannot use -jdump with out jit turned on (-jon vs -joff)");
        return 1;
    }
    const char *src = vm_asm_io_read(filename);
    if (src == NULL) {
        fprintf(stderr, "could not read file\n");
        return 1;
    }
    for (size_t i = 0; i < runs; i++) {
        vm_bc_buf_t buf = vm_asm(src);
        size_t nblocks = buf.nops;
        vm_ir_block_t *blocks = vm_ir_parse(nblocks, buf.ops);
        if (jitdumpir) {
            vm_ir_print_blocks(stderr, nblocks, blocks);
        }
        if (jitdumpopt) {
            vm_ir_print_blocks(stderr, nblocks, blocks);
        }
        vm_ir_be_int3(nblocks, blocks, NULL);
        vm_ir_blocks_free(nblocks, blocks);
        vm_free(buf.ops);
    }
    vm_free((void *)src);
    return 0;
}
