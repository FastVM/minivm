
#include "../vm/asm.h"
#include "../vm/ir/toir.h"
#include "../vm/ir/opt.h"
#include "../vm/ir/be/js.h"
#include "../vm/ir/be/lua.h"
#include "../vm/ir/be/jit.h"

static const char *vm_asm_io_read(const char *filename)
{
    void *file = fopen(filename, "rb");
    if (file == NULL)
    {
        return NULL;
    }
    size_t nalloc = 16;
    char *ops = vm_malloc(sizeof(char) * nalloc);
    size_t nops = 0;
    size_t size;
    for (;;)
    {
        size = fread(&ops[nops], sizeof(char), 1, file);
        if (size == 0)
        {
            break;
        }
        nops += 1;
        if (nops + 4 >= nalloc)
        {
            nalloc *= 4;
            ops = vm_realloc(ops, sizeof(char) * nalloc);
        }
    }
    ops[nops] = '\0';
    fclose(file);
    return ops;
}

int main(int argc, char **argv)
{
    // const char *dump = "out.bc";
    const char *dump = NULL;
    const char *target = "vm";
    while (true)
    {
        if (argc < 2)
        {
            fprintf(stderr, "too few args\n");
            return 1;
        }
        if (!strcmp(argv[1], "-o") || !strcmp(argv[1], "--output"))
        {
            argv += 1;
            argc -= 1;
            dump = argv[1];
            argv += 1;
            argc -= 1;
        }
        if (!strcmp(argv[1], "-t") || !strcmp(argv[1], "--target"))
        {
            argv += 1;
            argc -= 1;
            if (argc < 2)
            {
                fprintf(stderr, "too few args following -t\n");
                return 1;
            }
            target = argv[1];
            argv += 1;
            argc -= 1;
            continue;
        }
        break;
    }
    const char *src = vm_asm_io_read(argv[1]);
    if (src == NULL)
    {
        fprintf(stderr, "could not read file\n");
        return 1;
    }
    vm_asm_buf_t buf = vm_asm(src);
    vm_free((void *)src);
    if (!strcmp(target, "js") || !strcmp(target, "lua") || !strcmp(target, "jit"))
    {
        vm_ir_block_t *blocks = vm_ir_parse(buf.nops, buf.ops);
        size_t nblocks = buf.nops;
        vm_ir_opt_all(&nblocks, &blocks);
        if (!strcmp(target, "js"))
        {
            if (dump == NULL)
            {
                dump = "out.js";
            }
            FILE *out = fopen(dump, "w");
            vm_ir_be_js(out, nblocks, blocks);
            fclose(out);
        }
        if (!strcmp(target, "lua"))
        {
            if (dump == NULL)
            {
                dump = "out.lua";
            }
            FILE *out = fopen(dump, "w");
            vm_ir_be_lua(out, nblocks, blocks);
            fclose(out);
        }
        if (!strcmp(target, "jit"))
        {
            vm_ir_be_jit(nblocks, blocks);
        }
    }
    else
    {
        if (buf.nops == 0)
        {
            fprintf(stderr, "could not assemble file\n");
            return 1;
        }
        if (dump)
        {
            void *out = fopen(dump, "wb");
            fwrite(buf.ops, sizeof(vm_opcode_t), buf.nops, out);
            fclose(out);
        }
        else
        {
            int res = vm_run_arch_int(buf.nops, buf.ops);
            if (res != 0)
            {
                fprintf(stderr, "could not run asm\n");
                return 1;
            }
        }
        vm_free(buf.ops);
        return 0;
    }
}
