
#include <vm/vm.h>
#include <vm/debug.h>

char *vm_dis(opcode_t *head)
{
    int alloc = 256;
    char *base = malloc(alloc);
    char *ret = base;
    int index = 0;
    while (true)
    {
        opcode_t opcode = *ret;
        ret++;
        if (opcode == OPCODE_EXIT)
        {
            break;
        }
        const char *fmt = vm_opcode_format(opcode);
        for (int findex = 0; fmt[findex] != '\0'; findex++)
        {
            int len = ret - base;
            if (len + 256 < alloc)
            {
                alloc = alloc * 16 + 256;
                ret = realloc(ret, alloc);
            }
            int max = alloc - len;
            if (findex == 0)
            {
                max -= snprintf(ret, max, " ");
            }
            else
            {
                max -= snprintf(ret, max, ", ");
            }
            switch (fmt[findex])
            {
            case 'r':
            {
                max -= snprintf(ret, max, "r%i", *(reg_t *)head);
                head += sizeof(reg_t);
                break;
            }
            case 'j':
            {
                max -= snprintf(ret, max, "[%i]", *(int *)head);
                head += sizeof(int);
                break;
            }
            case 'l':
            {
                max -= snprintf(ret, max, "%s", *(bool *)head ? "1" : "0");
                head += sizeof(bool);
                break;
            }
            case 'c':
            {
                return NULL;
                // int nargs = *(reg_t *)head;
                // head += sizeof(reg_t);
                // max -= snprintf(ret, max, "(");
                // for (int argno = 0; argno < nargs; argno++)
                // {
                //     if (argno != 0)
                //     {
                //         max -= snprintf(ret, max, ", ");
                //     }
                //     max -= snprintf(ret, max, "r%i", *(reg_t *)head);
                //     head += sizeof(reg_t);
                // }
                // max -= snprintf(ret, max, ")");
                // break;
            }
            case 'n':
            {
                if (fmod(*(integer_t *)head, 1) == 0)
                {
                    max -= snprintf(ret, max, "%i", *(integer_t *)head);
                }
                else
                {
                    max -= snprintf(ret, max, "%i", *(integer_t *)head);
                }
                head += sizeof(integer_t);
                break;
            }
            case 'f':
            {
                max -= snprintf(ret, max, "{%i}", *(reg_t *)head);
                head += *(reg_t *)head + sizeof(reg_t);
                break;
            }
            default:
            {
                max -= snprintf(ret, max, "error: opcode %c\n", fmt[findex]);
            }
            }
        }
    }
}