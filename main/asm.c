
#include "../vm/asm.h"
#include "../vm/ir/toir.h"
#include "../vm/ir/opt.h"
#include "../vm/ir/build.h"
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
    size_t runs = 1;
    const char *filename = NULL;
    while (true)
    {
        if (argc < 2)
        {
            if (filename == NULL)
            {
                fprintf(stderr, "too few args\n");
                return 1;
            }
            else
            {
                break;
            }
        }
        if (!strcmp(argv[1], "-o") || !strcmp(argv[1], "--output"))
        {
            argv += 1;
            argc -= 1;
            dump = argv[1];
            argv += 1;
            argc -= 1;
            continue;
        }
        if (!strcmp(argv[1], "-n"))
        {
            argv += 1;
            argc -= 1;
            size_t n = 0;
            char *ptr = argv[1];
            while (*ptr != '\0')
            {
                n *= 10;
                n += *ptr - '0';
                ptr += 1;
            }
            argv += 1;
            argc -= 1;
            runs = n;
            continue;
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
        if (filename != NULL)
        {
            fprintf(stderr, "cannot handle multiple files at cli\n");
            return 1;
        }
        else
        {
            filename = argv[1];
            argv += 1;
            argc -= 1;
        }
    }
    const char *src = vm_asm_io_read(filename);
    if (src == NULL)
    {
        fprintf(stderr, "could not read file\n");
        return 1;
    }
    for (size_t i = 0; i < runs; i++)
    {
        vm_bc_buf_t buf = vm_asm(src);
        if (!strcmp(target, "js") || !strcmp(target, "lua") || !strcmp(target, "jit"))
        {
            vm_ir_block_t *blocks = vm_ir_parse(buf.nops, buf.ops);
            size_t nblocks = buf.nops;
            if (!strcmp(target, "jit"))
            {
                vm_ir_be_jit(nblocks, blocks);
            }
            vm_ir_blocks_free(nblocks, blocks);
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
        }
        vm_free(buf.ops);
    }
    vm_free((void *)src);
    return 0;
}
