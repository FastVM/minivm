#include "../vm/config.h"
#include "../vm/libc.h"
#include "../vm/save.h"
#include "../vm/state.h"
#include "../vm/vm.h"
#include "../vm/pass/print.h"

#define VM_CAN_NOT_RUN "cannot run vm: not enough args\n"
#define VM_CAN_NOT_OPEN "cannot open or read file\n"

#if defined(VM_TIME_MAIN)
#include <sys/time.h>
#endif

bool vm_main_str_eq(const char *a, const char *b) {
  while (true) {
    if (*a != *b) {
      return false;
    }
    if (*a == '\0') {
      return true;
    }
    a++;
    b++;
  }
}

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
    return vm_run_save(save, argc, argv);
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
    return state;
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
  if (vm_main_str_eq(argv[1], "--dis")) {
    vm_state_t *cur = vm_main_run(argv[2], argc - 3, &argv[3]);
    vm_pass_print(cur->nops, cur->ops);
    vm_state_del(cur);
  } else {
    vm_state_t *cur = vm_main_run(argv[1], argc - 2, &argv[2]);
    while (cur != NULL) {
      cur = vm_run(cur);
    }
  }
  return 0;
}

#endif