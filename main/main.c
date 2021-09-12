#include <vm/vector.h>
#include <vm/vm.h>
#include <vm/libc.h>
#include <stdio.h>
#ifdef VM_COSMO
#include <cosmopolitan.h>
#endif

int main(int argc, char **argv)
{
    FILE *file = fopen(argv[1], "rb");
    vec_t ops = vec_new(char);
    while (!feof(file))
    {
        char op = (fgetc(file));
        vec_push(ops, op);
        printf("%i < %i\n", ops->length, ops->allocated);
    }
    fclose(file);
    vm_run((opcode_t *)vec_get(ops, 0));
    // vec_del(ops);
}