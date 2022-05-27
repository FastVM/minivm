
#include "../vm/asm.h"

int main(int argc, char **argv)
{
  // const char *dump = "out.bc";
  const char *dump = NULL;
  if (argc < 2)
  {
    fprintf(stderr, "too few args\n");
    return 1;
  }
  const char *src = vm_asm_io_read(argv[1]);
  if (src == NULL)
  {
    fprintf(stderr, "could not read file\n");
    return 1;
  }
  vm_asm_buf_t buf = vm_asm(src);
  vm_free((void *)src);
  if (buf.nops == 0) {
    fprintf(stderr, "could not assemble file");
    return 1;
  }
  if (dump) {
    FILE *out = fopen(dump, "wb");
    fwrite(buf.ops, sizeof(vm_opcode_t), buf.nops, out);
    fclose(out);
  } else {
    int res = vm_run_arch_int(buf.nops, buf.ops, NULL);
    if (res != 0)
    {
      fprintf(stderr, "could not run asm\n");
      return 1;
    }
  }
  free(buf.ops);
  return 0;
}
