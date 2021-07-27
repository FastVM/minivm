
#include <vm/asm.h>

struct jmploc_t;
struct jmpfrom_t;
typedef struct jmploc_t jmploc_t;
typedef struct jmpfrom_t jmpfrom_t;

struct jmploc_t
{
    const char *name;
    int strlen;
    int where;
};

struct jmpfrom_t
{
    const char *match;
    int strlen;
    int *target;
};

void vm_asm_strip(const char **const src)
{
    while (**src == ' ' || **src == '\t')
    {
        *src += 1;
    }
}

void vm_asm_strip_endl(const char **const src)
{
    while (**src == ';' || **src == '\n' || **src == '\r' || **src == ' ' || **src == '\t')
    {
        *src += 1;
    }
}

integer_t vm_asm_read_num(const char **const src)
{
    const char *init = *src;
    bool negate = **src == '-';
    if (negate || **src == '+')
    {
        *src += 1;
    }
    integer_t ret = 0;
    int count = 0;
    while ('0' <= **src && **src <= '9')
    {
        ret = (ret * 10) + (**src - '0');
        *src += 1;
        count += 1;
    }
    if (count == 0)
    {
        *src = init;
        return 0;
    }
    return ret;
}

bool vm_asm_read_bool(const char **const src)
{
    const char *last = *src;
    if (**src == '#')
    {
        *src += 1;
        if (**src == 't')
        {
            *src += 1;
            return true;
        }
        else if (**src == 'f')
        {
            *src += 1;
            return false;
        }
    }
    *src = last;
    return false;
}

void vm_asm_skip_fun(const char **const src)
{
    const char *init = *src;
}

reg_t vm_asm_read_reg(const char **const src)
{
    const char *init = *src;
    if (**src != 'r')
    {
        return 0;
    }
    *src += 1;
    const char *last = *src;
    integer_t ret = vm_asm_read_num(src);
    if (last == *src)
    {
        *src = init;
        return 0;
    }
    if (ret < 0)
    {
        *src = init;
        return 0;
    }
    return (reg_t)ret;
}

opcode_t vm_asm_match_opcode(const char **const src)
{
    for (opcode_t op = 0; op < OPCODE_MAX1; op++)
    {
        const char *orig = *src;
        vm_asm_strip(src);
        const char *name = vm_opcode_name(op);
        int len = strlen(name);
        if (!strncmp(name, *src, len))
        {
            *src += len;
            const char *ret = *src;
            const char *fmt = vm_opcode_format(op);
            while (true)
            {
                vm_asm_strip(src);
                char cur = *fmt;
                fmt += 1;
                switch (cur)
                {
                case 'r':
                {
                    const char *init = *src;
                    vm_asm_read_reg(src);
                    if (init == *src)
                    {
                        goto fail;
                    }
                    break;
                }
                case 'l':
                {
                    const char *init = *src;
                    vm_asm_read_bool(src);
                    if (init == *src)
                    {
                        goto fail;
                    }
                    break;
                }
                case 'j':
                {
                    if (**src != '[')
                    {
                        goto fail;
                    }
                    while (**src != ']')
                    {
                        if (**src == '\0')
                        {
                            goto fail;
                        }
                        *src += 1;
                    }
                    *src += 1;
                    break;
                }
                case 'n':
                {
                    const char *init = *src;
                    vm_asm_read_num(src);
                    if (init == *src)
                    {
                        goto fail;
                    }
                    break;
                }
                case 'c':
                {
                    if (**src != '(')
                    {
                        goto fail;
                    }
                    *src += 1;
                    vm_asm_strip(src);
                    while (**src != ')')
                    {
                        if (**src == '\0')
                        {
                            goto fail;
                        }
                        const char *last = *src;
                        vm_asm_read_reg(src);
                        if (last == *src)
                        {
                            goto fail;
                        }
                        vm_asm_strip(src);
                    }
                    *src += 1;
                    break;
                }
                case 'f':
                {
                    goto fail;
                }
                case '\0':
                {
                    vm_asm_strip(src);
                    if (**src == '\n' || **src == '\r' || **src == ';' || **src == '\0')
                    {
                        goto success;
                    }
                    else
                    {
                        goto fail;
                    }
                }
                default:
                {
                    printf("error: invalid opcode format: %c\n", cur);
                    return -1;
                }
                }
            }
        success:
            vm_asm_strip(&ret);
            *src = ret;
            return op;
        }
    fail:
        *src = orig;
    }
    return (opcode_t)-1;
}

int vm_asm_read_opcode(opcode_t *buffer, opcode_t op, const char **const src, vec_t *replaces)
{
    opcode_t *init = buffer;
    const char *fmt = vm_opcode_format(op);
    *(opcode_t *)buffer = op;
    buffer += 1;
    while (true)
    {
        vm_asm_strip(src);
        char cur = *fmt;
        fmt += 1;
        switch (cur)
        {
        case 'r':
        {
            reg_t reg = vm_asm_read_reg(src);
            *(reg_t *)buffer = reg;
            buffer += sizeof(reg_t);
            break;
        }
        case 'l':
        {
            bool log = vm_asm_read_bool(src);
            *(bool *)buffer = log;
            buffer += sizeof(bool);
            break;
        }
        case 'j':
        {
            *src += 1;
            const char *first = *src;
            while (**src != ']')
            {
                *src += 1;
            }
            jmpfrom_t loc = (jmpfrom_t){
                .match = first,
                .strlen = *src - first,
                .target = (integer_t *)buffer,
            };
            *src += 1;
            buffer += sizeof(integer_t);
            vec_push(*replaces, loc);
            break;
        }
        case 'n':
        {
            integer_t reg = vm_asm_read_num(src);
            *(integer_t *)buffer = reg;
            buffer += sizeof(integer_t);
            break;
        }
        case 'c':
        {
            *src += 1;
            vm_asm_strip(src);
            opcode_t *count_loc = buffer;
            buffer += sizeof(reg_t);
            reg_t count = 0;
            while (**src != ')')
            {
                reg_t reg = vm_asm_read_reg(src);
                vm_asm_strip(src);
                *(reg_t *)buffer = reg;
                buffer += sizeof(reg_t);
                count += 1;
            }
            *(reg_t *)count_loc = count;
            *src += 1;
            break;
        }
        case 'f':
        {
            printf("no funcs yet\n");
            return 0;
        }
        case '\0':
        {
            return buffer - init;
        }
        default:
        {
            printf("error: invalid opcode format: %c\n", cur);
            return -1;
        }
        }
    }
}

vm_asm_result_t vm_assemble(const char *src)
{
    int nalloc = 1 << 16;
    opcode_t *ret = malloc(nalloc);
    opcode_t *mem = ret;
    vm_asm_strip_endl(&src);
    vec_t jmplocs = vec_new(jmploc_t);
    vec_t replaces = vec_new(jmpfrom_t);
    while (*src != '\0')
    {
        if (*src == '[')
        {
            src += 1;
            const char *first = src;
            while (*src != ']')
            {
                src += 1;
                if (*src == '\0')
                {
                    printf("error: expected ] before end of string\n");
                    free(ret);
                    vec_del(jmplocs);
                    return vm_asm_result_fail;
                }
            }
            jmploc_t loc = (jmploc_t){
                .name = first,
                .strlen = src - first,
                .where = mem - ret,
            };
            src += 1;
            vec_push(jmplocs, loc);
        }
        else
        {
            int index = mem - ret;
            if (index + (1 << 12) > nalloc)
            {
                nalloc = nalloc * 2 + (1 << 16);
                ret = realloc(ret, nalloc);
            }
            const char *last = src;
            opcode_t res = vm_asm_match_opcode(&src);
            if (last == src)
            {
                printf("error: could not figure out opcode\n");
                printf("line: ");
                while (*src != '\n' && *src != '\r' && *src != '\0')
                {
                    printf("%c", *src);
                    src += 1;
                }
                printf("\n");
                free(ret);
                vec_del(jmplocs);
                vec_del(replaces);
                return vm_asm_result_fail;
            }
            if (res == -1)
            {
                printf("error: could not figure out opcode args\n");
                free(ret);
                vec_del(jmplocs);
                vec_del(replaces);
                return vm_asm_result_fail;
            }
            vm_asm_strip(&src);
            int nbytes = vm_asm_read_opcode(mem, res, &src, &replaces);
            if (nbytes == 0)
            {
                printf("error: internal reader of opcodes is broken somhow\n");
                free(ret);
                vec_del(jmplocs);
                vec_del(replaces);
                return vm_asm_result_fail;
            }
            mem += nbytes;
        }
        vm_asm_strip_endl(&src);
    }
    *mem = OPCODE_EXIT;
    mem += 1;
    vec_foreach(ref1, replaces)
    {
        jmpfrom_t *jfrom = ref1;
        bool found = false;
        vec_foreach(ref2, jmplocs)
        {
            jmploc_t *jloc = ref2;
            if (jloc->strlen != jfrom->strlen)
            {
                continue;
            }
            int len = jfrom->strlen;
            if (strncmp(jfrom->match, jloc->name, len) != 0)
            {
                continue;
            }
            *jfrom->target = jloc->where;
            found = true;
            break;
        };
        if (!found)
        {
            printf("%s\n", jfrom->match);
            printf("error: could not find label: [%.*s]\n", jfrom->strlen, jfrom->match);
            free(ret);
            vec_del(jmplocs);
            vec_del(replaces);
            return vm_asm_result_fail;
        }
    };
    return (vm_asm_result_t){
        .bytecode = ret,
        .len = mem - ret,
    };
}