
#include "../vm/asm.h"

static const char *vm_asm_io_read(const char *filename)
{
  void *file = fopen(filename, "rb");
  if (file == NULL)
  {
    return NULL;
  }
  size_t nalloc = 16;
  char *ops = vm_malloc(sizeof(char) * nalloc);
  size_t nops = 0;
  size_t size;
  for (;;)
  {
    size = fread(&ops[nops], sizeof(char), 1, file);
    if (size == 0)
    {
      break;
    }
    nops += 1;
    if (nops + 4 >= nalloc)
    {
      nalloc *= 4;
      ops = vm_realloc(ops, sizeof(char) * nalloc);
    }
  }
  ops[nops] = '\0';
  fclose(file);
  return ops;
}

void vm_test_toir(size_t nops, const vm_opcode_t *ops);

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
  vm_test_toir(buf.nops, buf.ops);
  vm_free((void *)src);
  if (buf.nops == 0) {
    fprintf(stderr, "could not assemble file\n");
    return 1;
  }
  if (dump) {
    void *out = fopen(dump, "wb");
    fwrite(buf.ops, sizeof(vm_opcode_t), buf.nops, out);
    fclose(out);
  } else {
    int res = vm_run_arch_int(buf.nops, buf.ops);
    if (res != 0)
    {
      fprintf(stderr, "could not run asm\n");
      return 1;
    }
  }
  vm_free(buf.ops);
  return 0;
}
