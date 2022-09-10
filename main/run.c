
#include "../vm/bc.h"
#include "../vm/ir/be/int3.h"
#include "../vm/ir/build.h"
#include "../vm/ir/toir.h"

static vm_bc_buf_t vm_io_bc_read(const char *filename) {
    void *file = fopen(filename, "rb");
    if (file == NULL) {
        return (vm_bc_buf_t){
            .nops = 0,
        };
    }
    size_t nalloc = 16;
    vm_opcode_t *ops = vm_malloc(sizeof(vm_opcode_t) * nalloc);
    size_t nops = 0;
    for (;;) {
        size_t size = fread(&ops[nops], sizeof(vm_opcode_t), 1, file);
        if (size == 0) {
            break;
        }
        nops += 1;
        if (nops + 4 >= nalloc) {
            nalloc *= 4;
            ops = vm_realloc(ops, sizeof(vm_opcode_t) * nalloc);
        }
    }
    ops[nops] = '\0';
    fclose(file);
    return (vm_bc_buf_t){
        .ops = ops,
        .nops = nops,
    };
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "need exactly 1 cli argument");
    }
    vm_bc_buf_t buf = vm_io_bc_read(argv[1]);
    if (buf.nops == 0) {
        fprintf(stderr, "could not read file\n");
        return 1;
    }
    size_t nblocks = buf.nops;
    vm_ir_block_t *blocks = vm_ir_parse(nblocks, buf.ops);
    vm_ir_be_int3(nblocks, blocks, NULL);
    vm_ir_blocks_free(nblocks, blocks);
    vm_free(buf.ops);
    return 0;
}
