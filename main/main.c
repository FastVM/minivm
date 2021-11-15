#include "../vm/vm.h"
#include "../vm/libc.h"
#include "../vm/state.h"

#define VM_CAN_NOT_RUN "cannot run vm: not enough args\n"
#define VM_CAN_NOT_OPEN "cannot open or read file\n"

#include <sys/time.h>

int vm_main_run(char *src, size_t argc, char **argv)
{
    FILE *file = fopen(src, "rb");
    if (file == NULL)
    {
        for (const char *i = VM_CAN_NOT_OPEN; *i != '\0'; i++)
        {
            vm_putchar(*i);
        }
        return 1;
    }
    uint8_t nver = 2;
    fread(&nver, 1, 1, file);
    vm_opcode_t *vm_ops = vm_malloc(1 << 24);
    vm_opcode_t *ops = &vm_ops[0];
    while (true)
    {
        vm_opcode_t op;
        int size = fread(&op, sizeof(vm_opcode_t), 1, file);
        if (nver != sizeof(vm_opcode_t))
        {
            uint8_t xbuf[nver - sizeof(vm_opcode_t)];
            fread(xbuf, nver - sizeof(vm_opcode_t), 1, file);
        }
        if (size == 0)
        {
            break;
        }
        *(ops++) = op;
    }
    fclose(file);
    vm_state_t *state = vm_state_new(argc, (const char **) argv);
    vm_run(state, vm_ops);
    vm_state_del(state);
    vm_free(vm_ops);
    return 0;
}

int main(int argc, char *argv[argc])
{
    if (argc < 2)
    {
        for (const char *i = VM_CAN_NOT_RUN; *i != '\0'; i++)
        {
            vm_putchar(*i);
        }
    }
    char *file = argv[1];
#if defined(VM_TIME_MAIN)
    struct timeval start;
    gettimeofday(&start,NULL);

    int ret = vm_main_run(file, argc - 2, &argv[2]);
  
    struct timeval end;
    gettimeofday(&end,NULL);

    printf("%.3lf\n", (double)((1000000 + end.tv_usec - start.tv_usec) % 1000000) / 1000.0);
#else
    int ret = vm_main_run(file, argc - 2, &argv[2]);
#endif
    return ret;
}