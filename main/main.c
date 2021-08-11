#include <vm/vector.h>
#include <vm/vm.h>
#include <vm/libc.h>
#ifdef VM_COSMO
#include <cosmopolitan.h>
#endif

int main()
{
    vec_t ops = vec_new(opcode_t);
    while (!feof(stdin))
    {
        vec_push(ops, (opcode_t)(fgetc(stdin)));
    }
    vm_run(&ops->values[0]);
    vec_del(ops);
}