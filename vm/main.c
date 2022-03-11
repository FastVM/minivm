
#include "lib.h"
#include "obj.h"
#include "vm.h"

struct vm_io_res_t;
typedef struct vm_io_res_t vm_io_res_t;

struct vm_io_res_t {
  size_t nops;
  vm_opcode_t *ops;
  const char *err;
};

vm_io_res_t vm_io_read(const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (file == NULL) {
    return (vm_io_res_t){
        .err = "cannot run vm: file to run could not be read",
    };
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
  return (vm_io_res_t){
      .nops = nops,
      .ops = ops,
      .err = NULL,
  };
}

size_t vm_str_to_num(const char *str) {
  size_t ret = 0;
  while ('0' <= *str && *str <= '9') {
    ret = ret * 10;
    ret += *str - '0';
    str += 1;
  }
  return ret;
}

int main(int argc, const char **argv) {
  const char *filename = NULL;

  size_t nargs = 0;
  const char **args = NULL;

  vm_config_t config = vm_config_init();

  for (size_t i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (!strcmp(argv[i], "--growth")) {
        if (i + 1 == argc) {
          printf("cli: expected an argument to --growth\n");
          return 1;
        }
        i += 1;
        config.gc_grow = vm_str_to_num(argv[i]);
        continue;
      }
      if (!strcmp(argv[i], "--shrink")) {
        if (i + 1 == argc) {
          printf("cli: expected an argument to --shrink\n");
          return 1;
        }
        i += 1;
        if (!strcmp(argv[i], "true") || strcmp(argv[i], "yes") || !strcmp(argv[i], "1")) {
          config.gc_shrink = 1;
          continue;
        }
        if (!strcmp(argv[i], "false") || strcmp(argv[i], "no") || !strcmp(argv[i], "0")) {
          config.gc_shrink = 0;
          continue;
        }
      }
      if (!strcmp(argv[i], "--init")) {
        if (i + 1 == argc) {
          printf("cli: expected an argument to --init\n");
          return 1;
        }
        i += 1;
        size_t len = strlen(argv[i]);
        size_t mul = 1;
        if (!strcmp(argv[i], "0")) {
          mul = 0;
        } else if (argv[i][len - 1] == 'b' || argv[i][len - 1] == 'B') {
          mul = 1;
        } else if (argv[i][len - 1] == 'k' || argv[i][len - 1] == 'K') {
          mul = 1 << 10;
        } else if (argv[i][len - 1] == 'm' || argv[i][len - 1] == 'M') {
          mul = 1 << 20;
        } else if (argv[i][len - 1] == 'g' || argv[i][len - 1] == 'G') {
          mul = 1 << 30;
        } else {
          printf("cli: init's argument needs prefix k m or g (example --init 100k) (got %c)\n", argv[i][len - 1]);
          return 1;
        }
        size_t n = vm_str_to_num(argv[i]);
        config.gc_init = n * mul;
        continue;
      }
      printf("cli: unknown option: %s\n", argv[i]);
      return 1;
    } else {
      filename = argv[i];
      nargs = argc - (i + 1);
      args = &argv[i + 1];
      break;
    }
  }

  if (filename == NULL) {
    printf("no filename specified\n");
  }

  vm_io_res_t ops = vm_io_read(filename);
  if (ops.err != NULL) {
    printf("%s\n", ops.err);
    return 1;
  }
  int res = vm_run(config, ops.nops, ops.ops, nargs, args);
  vm_free(ops.ops);
  return res;
}
