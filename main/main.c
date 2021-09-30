#include <vm/vector.h>
#include <vm/vm.h>
#include <vm/libc.h>
#if defined(VM_COSMO)
#include <cosmopolitan.h>
#else
#include <stdio.h>
#endif

int main(int argc, char **argv)
{
    int times = 0;
    int head = 1;
    if ('0' <= argv[1][0] && argv[1][0] <= '9')
    {
        for (char *c = argv[1]; *c != '\0'; c++)
        {
            times *= 10;
            times += *c - '0';
        }
        head++;
    }
    if (times == 0) {
        times = 1;
    }
    for (int i = head; i < argc; i++)
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
        for (int j = times; j > 0; j--) 
        {
            vm_run((opcode_t *)vec_get(ops, 0));
        }
        vec_del(ops);
    }
}