
#include "lib.h"
#include "obj.h"
#include "vm.h"

int main(int argc, const char **argv) {
  if (argc < 2) {
    printf("cannot run vm: not enough args\n");
    return 1;
  }
  FILE *file = fopen(argv[1], "rb");
  if (file == (void *)0) {
    printf("cannot run vm: file to run could not be read\n");
    return 2;
  }
  size_t nalloc = 1 << 8;
  vm_opcode_t *ops = vm_malloc(sizeof(vm_opcode_t) * nalloc);
  size_t nops = 0;
  size_t size;
  for (;;) {
    vm_file_opcode_t op = 0;
    size = fread(&op, sizeof(vm_file_opcode_t), 1, file);
    if (size == 0) {
      break;
    }
    if (nops + 1 >= nalloc) {
      nalloc *= 4;
      ops = vm_realloc(ops, sizeof(vm_opcode_t) * nalloc);
    }
    ops[nops++].arg = op;
  }
  fclose(file);
  int res = vm_run(nops, ops, argc - 2, argv + 2);
  vm_free(ops);
  return res;
}
