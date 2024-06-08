
#include "io.h"

vm_save_t vm_save_load(FILE *in) {
    size_t nalloc = 512;
    uint8_t *ops = vm_malloc(sizeof(char) * nalloc);
    size_t nops = 0;
    size_t size;
    while (true) {
        if (nops + 256 >= nalloc) {
            nalloc = (nops + 256) * 2;
            ops = vm_realloc(ops, sizeof(char) * nalloc);
        }
        size = fread(&ops[nops], 1, 256, in);
        nops += size;
        if (size < 256) {
            break;
        }
    }
    return (vm_save_t){
        .len = nops,
        .buf = ops,
    };
}
