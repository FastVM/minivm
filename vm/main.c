
#include "lib.h"
#include "vm.h"
#include "io.h"

typedef enum {
  VM_ARCH_INT,
#if defined(VM_USE_ARCH_X86)
  VM_ARCH_X86,
#endif
} vm_arch_t;

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
  bool jit = false;
  int index = 1;
  for (;;) {
    if (vm_streq(argv[index], "-jon")) {
      index += 1;
      jit = true;
    }
    if (vm_streq(argv[index], "-joff")) {
      index += 1;
      jit = false;
    }
    break;
  }
  vm_io_res_t ops = vm_io_read(argv[index]);
  if (ops.err != NULL) {
    printf("%s\n", ops.err);
    return 1;
  }
  int res;
  if (jit) {
#if defined(VM_USE_ARCH_X86)
    res = vm_run_arch_x86(ops.nops, ops.ops);
#else
    printf("error: not built with jit\n");
    return 1;
#endif
  } else {
    res = vm_run_arch_int(ops.nops, ops.ops);
  }
  vm_free(ops.ops);
  return res;
}