
#include "io.h"
#include "lib.h"
#include "vm.h"

static inline size_t vm_str_to_num(const char *str)
{
  size_t ret = 0;
  while ('0' <= *str && *str <= '9')
  {
    ret = ret * 10;
    ret += (size_t)(*str - '0');
    str += 1;
  }
  return ret;
}

int main(int argc, const char **argv)
{
  int index = 1;
  for (;;)
  {
    // args parsing
    break;
  }
  vm_io_res_t ops = vm_io_read(argv[index]);
  if (ops.err != NULL)
  {
    printf("%s\n", ops.err);
    return 1;
  }
  int res = vm_run_arch_int(ops.nops, ops.ops);
  vm_free(ops.ops);
  return res;
}