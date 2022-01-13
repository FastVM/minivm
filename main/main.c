#include "../vm/config.h"
#include "../vm/libc.h"
#include "../vm/state.h"
#include "../vm/vm.h"

vm_state_t *vm_main_run(const vm_char_t *src, size_t argc,
                        const vm_char_t **argv) {
  FILE *file = fopen(src, "rb");
  if (file == NULL) {
    printf("cannot open or read file\n");
    return NULL;
  }
  uint8_t nver = 0;
  fread(&nver, 1, 1, file);
  size_t nops = 0;
  vm_opcode_t *vm_ops = vm_malloc(sizeof(vm_opcode_t) * (1 << 20));
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
    printf("cannot run vm: not enough args\n");
    return 1;
  }
  vm_gc_start();
  vm_state_t *cur = vm_main_run(argv[1], argc - 2, &argv[2]);
  if (cur == NULL) {
    return 2;
  }
  vm_run(cur);
}
