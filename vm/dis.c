
#include "opcode.h"
#include "lib.h"

void vm_disassemble(const int *ops, size_t nops, int indent) {
  const int max_nested = 100;
  int nested[max_nested];
  int nested_size = 0;
  for (size_t i = 0; i < nops; ++i) {
    if (nested_size > 0 && i >= nested[nested_size - 1]) {
      --nested_size;
      printf("%*s", indent * nested_size + 2, "");
      printf("}\n");
    }
    printf("%*s", indent * nested_size, "");
    printf("l%zu:\n", i);
    printf("%*s", indent * nested_size + 2, "");
    const int op = ops[i];
    switch (ops[i]) {
    case VM_OPCODE_EXIT:
      printf("exit\n");
      break;
    case VM_OPCODE_REG:
      printf("reg r%i r%i\n", ops[i + 1], ops[i + 2]);
      i += 2;
      break;
    case VM_OPCODE_BB:
      printf("bb r%i l%i l%i\n", ops[i + 1], ops[i + 2], ops[i + 3]);
      i += 3;
      break;
    case VM_OPCODE_INT:
      printf("int r%i %d\n", ops[i + 1], ops[i + 2]);
      i += 2;
      break;
    case VM_OPCODE_JUMP:
      printf("jump l%i\n", ops[i + 1]);
      i += 1;
      break;
    case VM_OPCODE_FUNC: {
      const int index_after_func = ops[i + 1];
      const int fargs = ops[i + 2];
      const int namelen = ops[i + 3];
      printf("func ");
      for (int n = 0; n < namelen; n++) {
        printf("%c", (char)ops[i + 4 + n]);
      }
      printf(" (");
      for (int a = 1; a <= fargs; ++a) {
        if (a != 1) {
          printf(" ");
        }
        printf("r%i", a);
      }
      printf(") {\n");
      i += 1 + 2 + namelen + 1;
      if (nested_size < max_nested - 1) {
        nested[nested_size++] = index_after_func;
      }
    } break;
    case VM_OPCODE_ADD:
      printf("add r%i r%i r%i\n", ops[i + 1], ops[i + 2], ops[i + 3]);
      i += 3;
      break;
    case VM_OPCODE_SUB:
      printf("sub r%i r%i r%i\n", ops[i + 1], ops[i + 2], ops[i + 3]);
      i += 3;
      break;
    case VM_OPCODE_MUL:
      printf("mul r%i r%i r%i\n", ops[i + 1], ops[i + 2], ops[i + 3]);
      i += 3;
      break;
    case VM_OPCODE_DIV:
      printf("div r%i r%i r%i\n", ops[i + 1], ops[i + 2], ops[i + 3]);
      i += 3;
      break;
    case VM_OPCODE_MOD:
      printf("mod r%i r%i r%i\n", ops[i + 1], ops[i + 2], ops[i + 3]);
      i += 3;
      break;
    case VM_OPCODE_POW:
      printf("pow r%i r%i r%i\n", ops[i + 1], ops[i + 2], ops[i + 3]);
      i += 3;
      break;
    case VM_OPCODE_CALL: {
      printf("call r%i l%i (", ops[i + 1], ops[i + 2]);
      const int nargs = ops[i + 3];
      const int farg = i + 4;
      for (int a = 0; a < nargs; ++a) {
        if (a != 0) {
          printf(" ");
        }
        printf("r%i", ops[farg + a]);
      }
      printf(")");
      i += 3 + nargs;
    } break;
    case VM_OPCODE_RETURN:
      printf("return r%i\n", ops[i + 1]);
      i += 1;
      break;
    case VM_OPCODE_PUTCHAR:
      printf("putchar r%i\n", ops[i + 1]);
      i += 1;
      break;
    case VM_OPCODE_STRING: {
      printf("string r%i [", ops[i + 1]);
      const int nargs = ops[i + 2];
      const int farg = i + 3;
      for (int a = 0; a < nargs; ++a) {
        if (a != 0) {
          printf(" ");
        }
        const int ch = ops[farg + a];
        printf("%i", ch);
      }
      printf("]\n");
      i += 2 + nargs;
    } break;
    case VM_OPCODE_LENGTH:
      printf("length r%i r%i\n", ops[i + 1], ops[i + 2]);
      i += 2;
      break;
    case VM_OPCODE_GET:
      printf("get r%i r%i r%i\n", ops[i + 1], ops[i + 2], ops[i + 3]);
      i += 3;
      break;
    case VM_OPCODE_SET:
      printf("set r%i r%i r%i\n", ops[i + 1], ops[i + 2], ops[i + 3]);
      i += 3;
      break;
    case VM_OPCODE_DUMP:
      printf("dump r%i r%i\n", ops[i + 1], ops[i + 2]);
      i += 2;
      break;
    case VM_OPCODE_READ:
      printf("read r%i r%i\n", ops[i + 1], ops[i + 2]);
      i += 2;
      break;
    case VM_OPCODE_WRITE:
      printf("write r%i r%i\n", ops[i + 1], ops[i + 2]);
      i += 2;
      break;
    case VM_OPCODE_ARRAY: {
      printf("array r%i (", ops[i + 1]);
      const int nargs = ops[i + 2];
      const int farg = i + 3;
      for (int a = 0; a < nargs; ++a) {
        const int ch = ops[farg + a];
        if (a != 0) {
          printf(" ");
        }
        printf("r%i", ch);
      }
      printf(")\n");
      i += 2 + nargs;
    } break;
    case VM_OPCODE_CAT:
      printf("cat r%i r%i r%i\n", ops[i + 1], ops[i + 2], ops[i + 3]);
      i += 3;
      break;
    case VM_OPCODE_BEQ:
      printf("beq r%i r%i l%i l%i\n", ops[i + 1], ops[i + 2], ops[i + 3],
             ops[i + 4]);
      i += 4;
      break;
    case VM_OPCODE_BLT:
      printf("blt r%i r%i l%i l%i\n", ops[i + 1], ops[i + 2], ops[i + 3],
             ops[i + 4]);
      i += 4;
      break;
    case VM_OPCODE_ADDI:
      printf("add r%i r%i %i\n", ops[i + 1], ops[i + 2], ops[i + 3]);
      i += 3;
      break;
    case VM_OPCODE_SUBI:
      printf("sub r%i r%i %i\n", ops[i + 1], ops[i + 2], ops[i + 3]);
      i += 3;
      break;
    case VM_OPCODE_MULI:
      printf("mul r%i r%i %i\n", ops[i + 1], ops[i + 2], ops[i + 3]);
      i += 3;
      break;
    case VM_OPCODE_DIVI:
      printf("div r%i r%i %i\n", ops[i + 1], ops[i + 2], ops[i + 3]);
      i += 3;
      break;
    case VM_OPCODE_MODI:
      printf("mod r%i r%i %i\n", ops[i + 1], ops[i + 2], ops[i + 3]);
      i += 3;
      break;
    case VM_OPCODE_CALL0:
      printf("call r%i l%i ()\n", ops[i + 2], ops[i + 1]);
      i += 2;
      break;
    case VM_OPCODE_CALL1:
      printf("call r%i l%i (r%i)\n", ops[i + 1], ops[i + 2], ops[i + 3]);
      i += 3;
      break;
    case VM_OPCODE_CALL2:
      printf("call r%i l%i (r%i r%i)\n", ops[i + 1], ops[i + 2], ops[i + 3],
             ops[i + 4]);
      i += 4;
      break;
    case VM_OPCODE_CALL3:
      printf("call r%i l%i (r%i r%i r%i)\n", ops[i + 1], ops[i + 2], ops[i + 3],
             ops[i + 4], ops[i + 5]);
      i += 5;
      break;
    case VM_OPCODE_GETI:
      printf("get r%i r%i %i\n", ops[i + 1], ops[i + 2], ops[i + 3]);
      i += 3;
      break;
    case VM_OPCODE_SETI:
      printf("set r%i %i r%i\n", ops[i + 1], ops[i + 2], ops[i + 3]);
      i += 3;
      break;
    case VM_OPCODE_BEQI:
      printf("beq r%i %i l%i l%i\n", ops[i + 1], ops[i + 2], ops[i + 3],
             ops[i + 4]);
      i += 4;
      break;
    case VM_OPCODE_BLTI:
      printf("blt r%i %i l%i l%i\n", ops[i + 1], ops[i + 2], ops[i + 3],
             ops[i + 4]);
      i += 4;
      break;
    case VM_OPCODE_BLTEI:
      printf("blte r%i %i l%i l%i\n", ops[i + 1], ops[i + 2], ops[i + 3],
             ops[i + 4]);
      i += 4;
      break;
    case VM_OPCODE_CALL_DYN:
      printf("call r%i r%i (", ops[i + 1], ops[i + 2]);
      const int nargs = ops[i + 3];
      const int farg = i + 4;
      for (int a = 0; a < nargs; ++a) {
        if (a != 0) {
          printf(" ");
        }
        printf("r%i", ops[farg + a]);
      }
      printf(")");
    default:
      printf("unknown opcode: %d\n", op);
      // disassembly may not be reliable after an unknown opcode
    }
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("cannot run dis: not enough args\n");
    return 1;
  }
  FILE *file = fopen(argv[1], "rb");
  if (file == NULL) {
    printf("cannot run dis: file to run could not be read\n");
    return 2;
  }
  size_t nalloc = 1 << 8;
  int *ops = vm_malloc(sizeof(int) * nalloc);
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
      ops = vm_realloc(ops, sizeof(int) * nalloc);
    }
    ops[nops++] = op;
  }
  fclose(file);
  vm_disassemble(ops, nops, 4);
  free(ops);
}
