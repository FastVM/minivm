
#include <vm/vm.h>
#include <vm/debug.h>

char *vm_dis(opcode_t *head)
{
    opcode_t *bcbase = head;
    int alloc = 256;
    char *base = malloc(alloc);
    char *ret = base;
    int index = 0;
    while (true)
    {
        opcode_t opcode = *head;
        if (opcode == OPCODE_EXIT)
        {
            break;
        }
        const char *name = vm_opcode_name(opcode);
        ret += snprintf(ret, alloc - (ret - base) - 1, "[0x%04lX] %s", head - bcbase, name);
        head++;
        const char *fmt = vm_opcode_format(opcode);
        for (int findex = 0; fmt[findex] != '\0'; findex++)
        {
            int len = ret - base;
            if (len + 128 < alloc)
            {
                alloc = alloc * 16 + 256;
                base = realloc(base, alloc);
                ret = base + len;
            }
            ret += snprintf(ret, alloc - len - 1, " ");
            switch (fmt[findex])
            {
            case 'r':
            {
                ret += snprintf(ret, alloc - len - 1, "r%i", *(reg_t *)head);
                head += sizeof(reg_t);
                break;
            }
            case 'j':
            {
                ret += snprintf(ret, alloc - len - 1, "[0x%04X]", *(int *)head);
                head += sizeof(int);
                break;
            }
            case 'l':
            {
                ret += snprintf(ret, alloc - len - 1, "%s", *(bool *)head ? "1" : "0");
                head += sizeof(bool);
                break;
            }
            case 'c':
            {
                return NULL;
                // int nargs = *(reg_t *)head;
                // head += sizeof(reg_t);
                // ret += snprintf(ret, alloc - len - 1, "(");
                // for (int argno = 0; argno < nargs; argno++)
                // {
                //     if (argno != 0)
                //     {
                //         ret += snprintf(ret, alloc - len - 1, ", ");
                //     }
                //     ret += snprintf(ret, alloc - len - 1, "r%i", *(reg_t *)head);
                //     head += sizeof(reg_t);
                // }
                // ret += snprintf(ret, alloc - len - 1, ")");
                // break;
            }
            case 'n':
            {
                if (fmod(*(integer_t *)head, 1) == 0)
                {
                    ret += snprintf(ret, alloc - len - 1, "%i", *(integer_t *)head);
                }
                else
                {
                    ret += snprintf(ret, alloc - len - 1, "%i", *(integer_t *)head);
                }
                head += sizeof(integer_t);
                break;
            }
            case 'f':
            {
                ret += snprintf(ret, alloc - len - 1, "{%i}", *(reg_t *)head);
                head += *(reg_t *)head + sizeof(reg_t);
                break;
            }
            default:
            {
                ret += snprintf(ret, alloc - len - 1, "error: opcode %c\n", fmt[findex]);
            }
            }
        }
        ret += snprintf(ret, alloc - (ret - base) - 1, "\n");
    }
    *ret = '\0';
    return base;
}