#include "../vm/vm.h"
#include "../vm/libc.h"
#include "../vm/state.h"
#include "../vm/config.h"

#define VM_CAN_NOT_RUN "cannot run vm: not enough args\n"
#define VM_CAN_NOT_OPEN "cannot open or read file\n"

#if defined(VM_TIME_MAIN)
#include <sys/time.h>
#endif

int vm_main_run(vm_char_t *src, size_t argc, vm_char_t **argv)
{
    FILE *file = fopen(src, "rb");
    if (file == NULL)
    {
        for (const vm_char_t *i = VM_CAN_NOT_OPEN; *i != '\0'; i++)
        {
            vm_putchar(*i);
        }
        return 1;
    }
    vm_opcode_t *vm_ops = vm_malloc(sizeof(vm_opcode_t) * VM_OPS_UNITS);
    size_t nops = 0;
    uint8_t nver = 0;
    fread(&nver, 1, 1, file);
    if (nver <= 2)
    {
        vm_opcode_t *ops = &vm_ops[0];
        while (true)
        {
            uint16_t op = 0;
            size_t size = fread(&op, nver, 1, file);
            if (size == 0)
            {
                break;
            }
            nops += 1;
            *(ops++) = op;
        }
    }
    else if (nver <= 4)
    {
        vm_opcode_t *ops = &vm_ops[0];
        while (true)
        {
            uint32_t op = 0;
            size_t size = fread(&op, nver, 1, file);
            if (size == 0)
            {
                break;
            }
            nops += 1;
            *(ops++) = op;
        }
    }
    else if (nver <= 8)
    {
        vm_opcode_t *ops = &vm_ops[0];
        while (true)
        {
            uint64_t op = 0;
            size_t size = fread(&op, nver, 1, file);
            if (size == 0)
            {
                break;
            }
            nops += 1;
            *(ops++) = op;
        }
    }
    fclose(file);
    vm_state_t *state = vm_state_new(argc, (const char **) argv);
    vm_state_set_ops(state, nops, vm_ops);
    vm_run(state);
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

    vm_int_t ret = vm_main_run(file, argc - 2, &argv[2]);
  
    struct timeval end;
    gettimeofday(&end,NULL);

    printf("%.3lf\n", (double)((1000000 + end.tv_usec - start.tv_usec) % 1000000) / 1000.0);
#else
    vm_int_t ret = vm_main_run(file, argc - 2, &argv[2]);
#endif
    return ret;
}