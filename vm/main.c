#include <vm/vm.h>
#include <vm/asm.h>
#include <vm/dis.h>
#include <vm/vector.h>
#include <ctype.h>

int vm_main_file_length(FILE *input)
{
    fseek(input, 0L, SEEK_END);
    int size = ftell(input);
    fseek(input, 0, SEEK_SET);
    return size;
}

const char *vm_main_file_ext_ref(const char *str)
{
    const char *out = str;
    while (*str != '\0')
    {
        if (*str == '.')
        {
            out = str;
        }
        str += 1;
    }
    return out;
}

const char *vm_main_file_basename(const char *str)
{
    const char *out = str;
    while (*str != '\0')
    {
        if (*str == '/' || *str == '\\')
        {
            out = str;
        }
        str += 1;
    }
    return out;
}

void vm_main_run_file_bc(FILE *input)
{
    int file_len = vm_main_file_length(input);
    void *mem = calloc(1, file_len + 1);
    int res = fread(mem, 1, file_len, input);
    vm_run(mem);
    free(mem);
}

void vm_main_run_file_asm(FILE *input)
{
    int file_len = vm_main_file_length(input);
    char *src = calloc(1, file_len + 1);
    int nread = fread(src, 1, file_len, input);
    src[nread] = '\0';
    opcode_t *mem = vm_assemble(src).bytecode;
    free(src);
    vm_run(mem);
    free(mem);
}

void vm_main_conv_asm_to_bc(FILE *out, FILE *input)
{
    int file_len = vm_main_file_length(input);
    char *src = calloc(1, file_len + 1);
    int nread = fread(src, 1, file_len, input);
    src[nread] = '\0';
    vm_asm_result_t res = vm_assemble(src);
    free(src);
    fwrite(res.bytecode, 1, res.len, out);
    free(res.bytecode);
}

void vm_main_conv_bc_to_asm(FILE *out, FILE *input)
{
    int file_len = vm_main_file_length(input);
    opcode_t *src = calloc(1, file_len + 1);
    int nread = fread(src, 1, file_len, input);
    src[nread] = OPCODE_EXIT;
    char *res = vm_dis(src);
    free(src);
    fprintf(out, "%s", res);
    free(res);
}

void vm_main_file_copy_printable(FILE *out, FILE *input)
{
    while (!feof(input))
    {
        char c = fgetc(input);
        if (isprint(c))
        {
            fputc(c, out);
        }
    }
}

void vm_main_file_copy_bin(FILE *out, FILE *input)
{
    while (!feof(input))
    {
        char c = fgetc(input);
        fputc(c, out);
    }
}

int main(int argc, const char **argv)
{
    const char *mode = "run";
    const char *output = NULL;
    for (int argno = 1; argno < argc; argno++)
    {
        const char *name = argv[argno];
        if (*name != '-')
        {
            continue;
        }
        name += 1;
        if (*name == '-')
        {
            name += 1;
            if (*name == '\0')
            {
                printf("error: arguments with `--` are not supported yet\n");
                return 1;
            }
            else
            {
                printf("error: no long arguments yet: `--%s`\n", name);
                return 1;
            }
            continue;
        }
        if (*name == 'o')
        {
            name += 1;
            if (*name == '\0')
            {
                if (argno + 1 == argc)
                {
                    printf("error: no argument after -o\n");
                    return 1;
                }
                name = argv[++argno];
            }
            const char *ext = vm_main_file_ext_ref(name);
            if (!strcmp(ext, ".asm"))
            {
                mode = "asm";
            }
            else if (!strcmp(ext, ".bc"))
            {
                mode = "bc";
            }
            else
            {
                printf("error: could not figure output file type from: %s\n", name);
            }
            output = name;
            continue;
        }
        printf("error: command line flag cannot be parsed: -%s\n", name);
        return 1;
    }
    for (int argno = 1; argno < argc; argno++)
    {
        const char *name = argv[argno];
        if (*name == '-')
        {
            name += 1;
            if (*name == 'o')
            {
                name += 1;
                if (*name == '\0')
                {
                    argno += 1;
                }
            }
            continue;
        }
        const char *ext = vm_main_file_ext_ref(name);
        if (!strcmp(ext, ".bc"))
        {
            FILE *file = fopen(name, "rb");
            if (file == NULL)
            {
                printf("error: could not open file: %s\n", name);
                return 1;
            }
            if (!strcmp(mode, "run"))
            {
                vm_main_run_file_bc(file);
            }
            else if (!strcmp(mode, "asm"))
            {
                FILE *out = fopen(output, "w");
                vm_main_conv_bc_to_asm(out, file);
                fclose(out);
            }
            else if (!strcmp(mode, "bc"))
            {
                FILE *out = fopen(output, "w");
                vm_main_file_copy_bin(out, file);
                fclose(out);
            }
            else
            {
                printf("error: internal mode error, cannot deal with: %s\n", mode);
            }
            fclose(file);
        }
        else if (!strcmp(ext, ".asm"))
        {
            FILE *file = fopen(name, "r");
            if (file == NULL)
            {
                printf("error: could not open file: %s\n", name);
                return 1;
            }
            if (!strcmp(mode, "run"))
            {
                vm_main_run_file_asm(file);
            }
            else if (!strcmp(mode, "asm"))
            {
                FILE *out = fopen(output, "w");
                vm_main_file_copy_printable(out, file);
                fclose(out);
            }
            else if (!strcmp(mode, "bc"))
            {
                FILE *out = fopen(output, "wb");
                vm_main_conv_asm_to_bc(out, file);
                fclose(out);
            }
            else
            {
                printf("error: internal mode error, cannot deal with: %s\n", mode);
            }
            fclose(file);
        }
        else
        {
            printf("error: could not figure file type from: %s\n", name);
            FILE *file = fopen(name, "rb");
            if (file == NULL)
            {
                printf("note: file cannot be read by process, possible typo?\n");
            }
            else
            {
                printf("note: file found but needs to end in .asm or .bc\n");
            }
            fclose(file);
            return 1;
        }
    }
}