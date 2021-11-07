#include "../vm/vm.h"
#include "../vm/libc.h"
#include "../vm/state.h"

struct FILE;
typedef struct FILE FILE;

FILE *fopen(const char *src, const char *name);
int fclose(FILE *);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);

#define VM_CAN_NOT_RUN "cannot run vm: not enough args\n"
#define VM_CAN_NOT_OPEN "cannot open or read file\n"

vm_opcode_t vm_ops[1 << 24];
char vm_srcs[1 << 24];

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
    vm_opcode_t *ops = &vm_ops[0];
    while (true)
    {
        vm_opcode_t op;
        int size = fread(&op, 4, 1, file);
        if (size == 0)
        {
            break;
        }
        *(ops++) = op;
    }
    fclose(file);
    vm_state_t *state = vm_state_new(argc, (const char **) argv);
    vm_run(state, ops - vm_ops, vm_ops);
    vm_state_del(state);
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
    int nargs = argc - 2;
    char **args = vm_malloc(sizeof(char *) * nargs);
    char *csrc = &vm_srcs[0];
    for (size_t i = 0; i < nargs; i++)
    {
        FILE *file = fopen(argv[i + 2], "rb");
        args[i] = csrc;
        while (true)
        {
            int size = fread(csrc, 1, 2048, file);
            csrc += size;
            if (size < 2048) {
                *csrc = '\0';
                csrc += 1;
                break;
            }
        }
    }
    int ret = vm_main_run(file, argc - 2, args);
    vm_free(args);
    return ret;
}