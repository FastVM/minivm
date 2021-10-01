#include <vm/vm.h>
#include <vm/libc.h>

#define VM_CAN_NO_OPEN "cannot open or read file\n"

opcode_t vm_ops[1 << 16];

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
            for (const char *i = VM_CAN_NO_OPEN; *i != '\0'; i++)
            {
                putchar(*i);
            }
            return 1;
        }
        opcode_t *ops = &vm_ops[0];
        while (!feof(file))
        {
            opcode_t op = (fgetc(file));
            *(ops++) = op;
        }
        fclose(file);
        vm_run(vm_ops);
    }
}