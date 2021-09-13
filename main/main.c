#include <vm/vector.h>
#include <vm/vm.h>
#include <vm/libc.h>
#ifdef VM_COSMO
#include <cosmopolitan.h>
#else
#include <stdio.h>
#endif

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
    {
        FILE *file = fopen(argv[i], "rb");
        if (file == NULL)
        {
            printf("cannot open file: %s\n", argv[i]);
            return 1;
        }
        vec_t ops = vec_new(char);
        while (!feof(file))
        {
            char op = (fgetc(file));
            vec_push(ops, op);
        }
        fclose(file);
        vm_run((opcode_t *)vec_get(ops, 0));
        vec_del(ops);
    }
}