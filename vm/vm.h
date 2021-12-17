#pragma once

#include "libc.h"

typedef int32_t vm_reg_t;
typedef int32_t vm_opcode_t;

#include "obj.h"

enum vm_opcode_t {
  VM_OPCODE_EXIT = 0,
  VM_OPCODE_STORE_REG = 1,
  VM_OPCODE_STORE_NONE = 2,
  VM_OPCODE_STORE_BOOL = 3,
  VM_OPCODE_STORE_INT = 4,
  // empty
  // empty
  // empty
  // empty
  // empty
  // empty
  VM_OPCODE_JUMP = 11,
  VM_OPCODE_FUNC = 12,
  VM_OPCODE_ADD = 13,
  VM_OPCODE_SUB = 14,
  VM_OPCODE_MUL = 15,
  VM_OPCODE_DIV = 16,
  VM_OPCODE_MOD = 17,
  // empty
  VM_OPCODE_STATIC_CALL = 19,
  VM_OPCODE_RETURN = 20,
  VM_OPCODE_PUTCHAR = 21,
  VM_OPCODE_STRING_NEW = 22,
  // empty
  VM_OPCODE_LENGTH = 24,
  VM_OPCODE_INDEX_GET = 25,
  VM_OPCODE_INDEX_SET = 26,
  VM_OPCODE_TYPE = 27,
  VM_OPCODE_EXEC = 28,
  // empty
  // empty
  VM_OPCODE_DUMP = 31,
  VM_OPCODE_READ = 32,
  VM_OPCODE_WRITE = 33,
  VM_OPCODE_LOAD_GLOBAL = 34,
  VM_OPCODE_DYNAMIC_CALL = 35,
  VM_OPCODE_STATIC_ARRAY_NEW = 36,
  VM_OPCODE_STATIC_CONCAT = 37,
  VM_OPCODE_STATIC_CALL0 = 38,
  VM_OPCODE_STATIC_CALL1 = 39,
  // emty
  VM_OPCODE_STATIC_CALL2 = 41,
  VM_OPCODE_STATIC_CALL3 = 42,
  VM_OPCODE_BRANCH_EQUAL = 43,
  VM_OPCODE_BRANCH_NOT_EQUAL = 44,
  VM_OPCODE_BRANCH_LESS = 45,
  VM_OPCODE_BRANCH_GREATER = 46,
  VM_OPCODE_BRANCH_LESS_THAN_EQUAL = 47,
  VM_OPCODE_BRANCH_GREATER_THAN_EQUAL = 48,
  VM_OPCODE_BRANCH_BOOL = 49,
  VM_OPCODE_INC = 50,
  VM_OPCODE_DEC = 51,
  VM_OPCODE_BRANCH_EQUAL_INT = 52,
  VM_OPCODE_BRANCH_NOT_EQUAL_INT = 53,
  VM_OPCODE_BRANCH_LESS_INT = 54,
  VM_OPCODE_BRANCH_GREATER_INT = 55,
  VM_OPCODE_BRANCH_LESS_THAN_EQUAL_INT = 56,
  VM_OPCODE_BRANCH_GREATER_THAN_EQUAL_INT = 57,

  VM_OPCODE_MAX1,
  VM_OPCODE_MAX2P = 128,
};

typedef struct {
  vm_reg_t outreg;
  vm_opcode_t index;
  vm_opcode_t nlocals;
} vm_stack_frame_t;

#include "state.h"

void vm_run(vm_state_t *state);
bool vm_run_some(vm_state_t *state);
void vm_run_some_rec(vm_state_t **cur);
