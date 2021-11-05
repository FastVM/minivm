#include "../vm/vm.h"
#include "../vm/libc.h"
#include "../vm/state.h"

struct FILE;
typedef struct FILE FILE;

FILE *fopen(const char *src, const char *name);
int fclose(FILE *);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);

#define VM_CAN_NOT_OPEN "cannot open or read file\n"

vm_opcode_t vm_ops[1 << 24];

int main(int argc, char *argv[argc])
{
    for (int i = 1; i < argc; i++)
    {
        FILE *file = fopen(argv[i], "rb");
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
            int size = fread(ops, 4, 1, file);
            if (size == 0)
            {
                break;
            }
            *(ops++) = op;
        }
        fclose(file);
        vm_state_t *state = vm_state_new();
        vm_run(state, ops - vm_ops, vm_ops);
        vm_state_del(state);
    }
}