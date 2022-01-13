
#include "print.h"

int printf(const char *fmt, ...);

#define vm_read()                                                              \
  ({                                                                           \
    size_t last = index;                                                           \
    index += 1;                                                                    \
    ops[last];                                                         \
  })

void vm_pass_print(size_t nops, const vm_opcode_t *ops) {
  size_t index = 0;
  while (index < nops) {
    printf("l%zu: ", index);
    vm_opcode_t op = vm_read();
    switch (op) {
    case VM_OPCODE_EXIT: {
      printf("exit\n");
      break;
    }
    case VM_OPCODE_STORE_REG: {
      printf("store_reg r%i r%i\n", vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_STORE_NONE: {
      printf("store_none r%i\n", vm_read());
      break;
    }
    case VM_OPCODE_STORE_BOOL: {
      printf("store_bool r%i %s\n", vm_read(), vm_read() ? "true" : "false");
      break;
    }
    case VM_OPCODE_STORE_INT: {
      printf("store_int r%i %i\n",vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_SWAP_REG: {
      printf("swap_reg r%i r%i\n",vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_LOAD_GLOBAL: {
      printf("load_global r%i %i\n",vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_INDEX_GET_INT: {
      printf("index_get_int r%i r%i %i\n",vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_INDEX_SET_INT: {
      printf("index_set_int r%i %i r%i\n",vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_JUMP: {
      printf("jump l%i\n",vm_read());
      break;
    }
    case VM_OPCODE_FUNC: {
      printf("func l%i %i\n", vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_ADD: {
      printf("add r%i r%i r%i\n",vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_SUB: {
      printf("sub r%i r%i r%i\n",vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_MUL: {
      printf("mul r%i r%i r%i\n",vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_DIV: {
      printf("div r%i r%i r%i\n",vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_MOD: {
      printf("mod r%i r%i r%i\n",vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_POW: {
      printf("pow r%i r%i r%i\n",vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_STATIC_CALL: {
      printf("static_call r%i l%i ",vm_read(), vm_read());
      printf("(");
      size_t n = vm_read();
      for (size_t i = 0; i < n; i++) {
        if (i != 0) {
          printf(" ");
        }
        printf("r%i",vm_read());
      }
      printf(")\n");
      break;
    }
    case VM_OPCODE_RETURN: {
      printf("return r%i\n",vm_read());
      break;
    }
    case VM_OPCODE_PUTCHAR: {
      printf("putchar r%i\n",vm_read());
      break;
    }
    case VM_OPCODE_STRING_NEW: {
      printf("string_new r%i ",vm_read());
      printf("[");
      size_t n = vm_read();
      for (size_t i = 0; i < n; i++) {
        if (i != 0) {
          printf(" ");
        }
        printf("%i",vm_read());
      }
      printf("]\n");
      break;
    }
    case VM_OPCODE_LENGTH: {
      printf("length r%i r%i\n",vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_INDEX_GET: {
      printf("index_get r%i r%i r%i\n",vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_INDEX_SET: {
      printf("index_set r%i r%i r%i\n",vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_TYPE: {
      printf("type r%i r%i\n",vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_EXEC: {
      printf("exec r%i r%i\n",vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_SAVE: {
      printf("save r%i\n",vm_read());
      break;
    }
    case VM_OPCODE_DUMP: {
      printf("dump r%i r%i\n",vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_READ: {
      printf("read r%i r%i\n",vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_WRITE: {
      printf("write r%i r%i\n",vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_STATIC_ARRAY_NEW: {
      printf("static_array_new r%i ",vm_read());
      printf("(");
      size_t n = vm_read();
      for (size_t i = 0; i < n; i++) {
        if (i != 0) {
          printf(" ");
        }
        printf("r%i",vm_read());
      }
      printf(")\n");
      break;
    }
    case VM_OPCODE_STATIC_CONCAT: {
      printf("static_concat r%i r%i r%i\n",vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_STATIC_CALL0: {
      printf("static_call0 r%i l%i\n",vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_STATIC_CALL1: {
      printf("static_call1 r%i l%i r%i\n",vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_STATIC_CALL2: {
      printf("static_call2 r%i l%i r%i r%i\n",vm_read(), vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_STATIC_CALL3: {
      printf("static_call3 r%i l%i r%i r%i r%i\n",vm_read(), vm_read(), vm_read(), vm_read(),
             vm_read());
      break;
    }
    case VM_OPCODE_BRANCH_EQUAL: {
      printf("branch_equal r%i r%i l%i l%i\n",vm_read(), vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_BRANCH_NOT_EQUAL: {
      printf("branch_not_equal r%i r%i l%i l%i\n",vm_read(), vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_BRANCH_LESS: {
      printf("branch_less r%i r%i l%i l%i\n",vm_read(), vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_BRANCH_GREATER: {
      printf("branch_greater r%i r%i l%i l%i\n",vm_read(), vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_BRANCH_LESS_THAN_EQUAL: {
      printf("branch_less_than_equal r%i r%i l%i l%i\n",vm_read(), vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_BRANCH_GREATER_THAN_EQUAL: {
      printf("branch_greater_than_equal r%i r%i l%i l%i\n",vm_read(), vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_BRANCH_BOOL: {
      printf("branch_bool r%i l%i l%i\n",vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_SUB_INT: {
      printf("sub_int r%i r%i %i\n",vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_ADD_INT: {
      printf("add_int r%i r%i %i\n",vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_BRANCH_EQUAL_INT: {
      printf("branch_equal_int r%i %i l%i l%i\n",vm_read(), vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_BRANCH_NOT_EQUAL_INT: {
      printf("branch_not_equal_int r%i %i l%i l%i\n",vm_read(), vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_BRANCH_LESS_INT: {
      printf("branch_less_int r%i %i l%i l%i\n",vm_read(), vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_BRANCH_GREATER_INT: {
      printf("branch_greater_int r%i %i l%i l%i\n",vm_read(), vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_BRANCH_LESS_THAN_EQUAL_INT: {
      printf("branch_less_than_equal_int r%i %i l%i l%i\n",vm_read(), vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_BRANCH_GREATER_THAN_EQUAL_INT: {
      printf("branch_greater_than_equal_int r%i %i l%i l%i\n",vm_read(), vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_MUL_INT: {
      printf("mul_int r%i r%i %i\n",vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_DIV_INT: {
      printf("div_int r%i r%i %i\n",vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_MOD_INT: {
      printf("mod_int r%i r%i %i\n",vm_read(), vm_read(), vm_read());
      break;
    }
    case VM_OPCODE_POW_INT: {
      printf("pow_int r%i r%i %i\n",vm_read(), vm_read(), vm_read());
      break;
    }
    default: {
      printf("<<<%i>>>\n", op);
      break;
    }
    }
    }
  }