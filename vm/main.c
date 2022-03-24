
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

static inline vm_io_res_t vm_io_read(const char *filename) {
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
    ops[nops++].arg = (size_t) op;
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

vm_obj_t vm_run_ext(vm_gc_t *gc, size_t func, vm_obj_t obj) {
  return vm_obj_num(0);
}

int main(int rargc, const char **argv) {
  size_t argc = (size_t) rargc;
  
  const char *filename = NULL;

  size_t nargs = 0;
  const char **args = NULL;

  vm_config_t config = vm_config_init();

  for (size_t i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (vm_streq(argv[i], "--gc-enries")) {
        if (i + 1 == argc) {
          printf("cli: expected an argument to %s\n", argv[i]);
          return 1;
        }
        i += 1;
        config.gc_ents = vm_str_to_num(argv[i]);
        continue;
      }
      if (vm_streq(argv[i], "--gc-shrink")) {
        if (i + 1 == argc) {
          printf("cli: expected an argument to %s\n", argv[i]);
          return 1;
        }
        i += 1;
        if (vm_streq(argv[i], "true") || vm_streq(argv[i], "yes") || vm_streq(argv[i], "1")) {
          config.gc_shrink = 1;
          continue;
        }
        if (vm_streq(argv[i], "false") || vm_streq(argv[i], "no") || vm_streq(argv[i], "0")) {
          config.gc_shrink = 0;
          continue;
        }
        printf("unknown option to %s: %s\n", argv[i-1], argv[i]);
        return 1;
      }
      if (vm_streq(argv[i], "--gc-entries")) {
        if (i + 1 == argc) {
          printf("cli: expected an argument to %s\n", argv[i]);
          return 1;
        }
        i += 1;
        size_t len = vm_strlen(argv[i]);
        size_t mul = 1;
        if (vm_streq(argv[i], "0")) {
          mul = 0;
        } else if (argv[i][len - 1] == 'b' || argv[i][len - 1] == 'B') {
          mul = 1;
        } else if (argv[i][len - 1] == 'k' || argv[i][len - 1] == 'K') {
          mul = 1000;
        } else if (argv[i][len - 1] == 'm' || argv[i][len - 1] == 'M') {
          mul = 1000 * 1000;
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
