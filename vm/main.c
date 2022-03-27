
#include "lib.h"
#include "obj.h"
#include "vm.h"

vm_obj_t vm_run_ext(size_t func, vm_obj_t obj) {
  return vm_obj_num(0);
}

#if defined(VM_FUZZ)
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  // for (size_t i = 0; i < size / 2; i++) {
  //   vm_opcode_t op = ((vm_opcode_t*)data)[i];
  //   printf("%hu ", op);
  // }
  // printf("\n");
  if (size < 2) {
    return 0;
  }
  vm_run(size / sizeof(vm_opcode_t), (vm_opcode_t*) data);
  return 0;
}
#else
struct vm_io_res_t;
typedef struct vm_io_res_t vm_io_res_t;

struct vm_io_res_t {
  size_t nops;
  vm_opcode_t *ops;
  const char *err;
};

static inline vm_io_res_t vm_io_read(const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (file == NULL) {
    return (vm_io_res_t){
        .err = "cannot run vm: file to run could not be read",
    };
  }
  size_t nalloc = 1 << 16;
  vm_opcode_t *ops = vm_malloc(sizeof(vm_opcode_t) * nalloc);
  size_t nops = 0;
  size_t size;
  for (;;) {
    vm_opcode_t op = 0;
    size = fread(&op, sizeof(vm_opcode_t), 1, file);
    if (size == 0) {
      break;
    }
    if (nops + 1 >= nalloc) {
      return (vm_io_res_t){
          .err = "cannot run vm: file to run could not be read",
      };
    }
    ops[nops++] = (size_t) op;
  }
  fclose(file);
  return (vm_io_res_t){
      .nops = nops,
      .ops = ops,
      .err = NULL,
  };
}

static inline size_t vm_str_to_num(const char *str) {
  size_t ret = 0;
  while ('0' <= *str && *str <= '9') {
    ret = ret * 10;
    ret += (size_t) (*str - '0');
    str += 1;
  }
  return ret;
}

int main(int argc, const char **argv) {
  if (argc < 2) {
    printf("need a file argument");
    return 1;
  }
  vm_io_res_t ops = vm_io_read(argv[1]);
  if (ops.err != NULL) {
    printf("%s\n", ops.err);
    return 1;
  }
  int res = vm_run(ops.nops, ops.ops);
  vm_free(ops.ops);
  return res;
}
#endif