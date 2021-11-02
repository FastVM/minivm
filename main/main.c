#include "../vm/vm.h"
#include "../vm/libc.h"

struct FILE;
typedef struct FILE FILE;

FILE *fopen(const char *src, const char *name);
int fclose(FILE *);
int feof(FILE *);
int fgetc(FILE *);

#define VM_CAN_NOT_OPEN "cannot open or read file\n"

vm_opcode_t vm_ops[1 << 16];

int main(int argc, char *argv[argc])
{
    for (int i = 1; i < argc; i++)
    {
        FILE *file = fopen(argv[i], "rb");
        if (file == NULL)
        {
            for (const char *i = VM_CAN_NOT_OPEN; *i != '\0'; i++)
            {
                putchar(*i);
            }
            return 1;
        }
        vm_opcode_t *ops = &vm_ops[0];
        while (!feof(file))
        {
            vm_opcode_t op = (fgetc(file));
            *(ops++) = op;
        }
        fclose(file);
        vm_state_t *state = vm_state_new();
        vm_run(state, ops - vm_ops, vm_ops);
        vm_state_del(state);
    }
}