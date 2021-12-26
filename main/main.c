#include "../vm/config.h"
#include "../vm/libc.h"
#include "../vm/save.h"
#include "../vm/state.h"
#include "../vm/vm.h"

#define VM_CAN_NOT_RUN "cannot run vm: not enough args\n"
#define VM_CAN_NOT_OPEN "cannot open or read file\n"

#if defined(VM_TIME_MAIN)
#include <sys/time.h>
#endif

vm_state_t *vm_main_run(const vm_char_t *src, size_t argc,
                        const vm_char_t **argv) {
  FILE *file = fopen(src, "rb");
  if (file == NULL) {
    for (const vm_char_t *i = VM_CAN_NOT_OPEN; *i != '\0'; i++) {
      vm_putchar(*i);
    }
    return NULL;
  }
  uint8_t nver = 0;
  fread(&nver, 1, 1, file);
  if (nver == 0) {
    vm_save_t save;
    vm_save_init(&save);
    while (true) {
      uint8_t byte = 0;
      size_t size = fread(&byte, 1, 1, file);
      if (size == 0) {
        break;
      }
      vm_save_byte(&save, byte);
    }
    vm_save_rewind(&save);
    vm_state_t *state = vm_run_save(save, argc, argv);
    vm_save_deinit(&save);
    return state;
  } else {
    size_t nops = 0;
    vm_opcode_t *vm_ops = vm_malloc(sizeof(vm_opcode_t) * VM_OPS_UNITS);
    if (nver <= 2) {
      vm_opcode_t *ops = &vm_ops[0];
      while (true) {
        uint16_t op = 0;
        size_t size = fread(&op, nver, 1, file);
        if (size == 0) {
          break;
        }
        nops += 1;
        *(ops++) = op;
      }
    } else if (nver <= 4) {
      vm_opcode_t *ops = &vm_ops[0];
      while (true) {
        uint32_t op = 0;
        size_t size = fread(&op, nver, 1, file);
        if (size == 0) {
          break;
        }
        nops += 1;
        *(ops++) = op;
      }
    } else if (nver <= 8) {
      vm_opcode_t *ops = &vm_ops[0];
      while (true) {
        uint64_t op = 0;
        size_t size = fread(&op, nver, 1, file);
        if (size == 0) {
          break;
        }
        nops += 1;
        *(ops++) = op;
      }
    }
    fclose(file);
    vm_state_t *state = vm_state_new(argc, (const char **)argv);
    vm_state_set_ops(state, nops, vm_ops);
    return vm_run(state);
  }
}

#if defined(VM_EMCC)

static size_t vm_main_nargs = 1;
static const char *vm_main_args_space[1 << 6] = {"./bin/minivm"};

static size_t vm_main_strlen(const char *src) {
  size_t n = 0;
  while (*src != '\0') {
    n += 1;
    src += 1;
  }
  return n;
}

int printf(const char *fmt, ...);

VM_API void vm_main_add_arg(const char *src) {
  size_t len = vm_main_strlen(src) + 1;
  char *xsrc = vm_malloc(len);
  for (size_t i = 0; i < len; i++) {
    xsrc[i] = src[i];
  }
  vm_main_args_space[vm_main_nargs++] = xsrc;
}

VM_API vm_state_t *vm_main_default(void) {
  if (vm_main_nargs < 2) {
    for (const char *i = VM_CAN_NOT_RUN; *i != '\0'; i++) {
      vm_putchar(*i);
    }
    return NULL;
  }
  return vm_main_run(vm_main_args_space[1], vm_main_nargs - 2,
                     &vm_main_args_space[2]);
}

#else

int main(int argc, const char *argv[argc]) {
  if (argc < 2) {
    for (const char *i = VM_CAN_NOT_RUN; *i != '\0'; i++) {
      vm_putchar(*i);
    }
  }
  // #if defined(VM_TIME_MAIN)
  //   struct timeval start;
  //   gettimeofday(&start, NULL);

  //   vm_int_t ret = vm_main_run(argv[1], argc - 2, &argv[2]);

  //   struct timeval end;
  //   gettimeofday(&end, NULL);

  //   printf("%.3lf\n",
  //          (double)((1000000 + end.tv_usec - start.tv_usec) % 1000000) /
  //          1000.0);
  // #else
  vm_state_t *ret = vm_main_run(argv[1], argc - 2, &argv[2]);
  while (ret != NULL) {
    ret = vm_run(ret);
  }
  // #endif
  return 0;
}

#endif