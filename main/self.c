
#include "../vm/asm.h"
#include "../vm/ir/build.h"
#include "../vm/ir/toir.h"

void vm_ir_be_js(FILE *of, size_t nargs, vm_ir_block_t *blocks);

int main(int argc, char **argv) {
    char *src =
        "\n"
        "@__entry\n"
        "    r0 <- call main\n"
        "    exit\n"
        "\n"
        "func putn\n"
        "    r0 <- int 1\n"
        "    blt r1 r0 putn.digit putn.ret\n"
        "@putn.digit\n"
        "    r0 <- int 10\n"
        "    r0 <- div r1 r0\n"
        "    r0 <- call putn r0\n"
        "    r0 <- int 10\n"
        "    r1 <- mod r1 r0\n"
        "    r0 <- int 48\n"
        "    r1 <- add r1 r0\n"
        "    putchar r1\n"
        "@putn.ret\n"
        "    r0 <- int 0\n"
        "    ret r0\n"
        "end\n"
        "\n"
        "func main\n"
        "    r0 <- int 4984\n"
        "    r0 <- call putn r0\n"
        "    r0 <- int 10\n"
        "    putchar r0\n"
        "    exit\n"
        "end\n";
    vm_bc_buf_t buf = vm_asm(src);
    if (buf.nops == 0) {
        printf("zero ops, nothing to do!\n");
        __builtin_trap();
    }
    size_t nblocks = buf.nops;
    vm_ir_block_t *blocks = vm_ir_parse(nblocks, buf.ops);
    FILE *out = fopen("/dev/stdout", "w");
    vm_ir_be_js(out, nblocks, blocks);
    fclose(out);
    vm_ir_blocks_free(nblocks, blocks);
    vm_free(buf.ops);
    return 0;
}
