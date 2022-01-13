#include "../vm/config.h"
#include "../vm/libc.h"
#include "../vm/state.h"
#include "../vm/vm.h"

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

// Usage:
//
// minivm FILE
//
int main(int argc, const char *argv[argc]) {
  if (argc < 2) {
    for (const char *i = VM_CAN_NOT_RUN; *i != '\0'; i++) {
      vm_putchar(*i);
    }
    return 1;
  }
  vm_state_t *cur = vm_main_run(argv[1], argc - 2, &argv[2]);
  if (cur == NULL) {
    return 2;
  }
  while (cur != NULL) {
    cur = vm_run(cur);
  }
  return 0;
}
