#pragma once

#include "libc.h"

typedef int32_t vm_reg_t;
typedef int32_t vm_opcode_t;

#include "obj.h"

typedef union
{
    enum {
        VM_RUN_SOME_DEAD,
        VM_RUN_SOME_OUT_OF_GAS,
        VM_RUN_SOME_FORK,
    } tag;
} vm_run_some_t;

enum vm_opcode_t
{
    VM_OPCODE_EXIT = 0,
    VM_OPCODE_STORE_REG = 1,
    VM_OPCODE_STORE_NONE = 2,
    VM_OPCODE_STORE_BOOL = 3,
    VM_OPCODE_STORE_INT = 4,
    VM_OPCODE_EQUAL = 5,
    VM_OPCODE_NOT_EQUAL = 6,
    VM_OPCODE_LESS = 7,
    VM_OPCODE_GREATER = 8,
    VM_OPCODE_LESS_THAN_EQUAL = 9,
    VM_OPCODE_GREATER_THAN_EQUAL = 10,
    VM_OPCODE_JUMP = 11,
    VM_OPCODE_BRANCH_TRUE = 12,
    VM_OPCODE_ADD = 13,
    VM_OPCODE_SUB = 14,
    VM_OPCODE_MUL = 15,
    VM_OPCODE_DIV = 16,
    VM_OPCODE_MOD = 17,
    VM_OPCODE_CONCAT = 18,
    VM_OPCODE_STATIC_CALL = 19,
    VM_OPCODE_RETURN = 20,
    VM_OPCODE_PUTCHAR = 21,
    VM_OPCODE_STRING_NEW = 22,
    VM_OPCODE_ARRAY_NEW = 23,
    VM_OPCODE_LENGTH = 24,
    VM_OPCODE_INDEX_GET = 25,
    VM_OPCODE_INDEX_SET = 26,
    VM_OPCODE_TYPE = 27,
    VM_OPCODE_EXEC = 28,
    VM_OPCODE_EXTEND = 29,
    VM_OPCODE_PUSH = 30,
    VM_OPCODE_DUMP = 31,
    VM_OPCODE_READ = 32,
    VM_OPCODE_WRITE = 33,
    VM_OPCODE_LOAD_GLOBAL = 34,
    VM_OPCODE_DYNAMIC_CALL = 35,
    VM_OPCODE_STATIC_ARRAY_NEW = 36,
    VM_OPCODE_STATIC_CONCAT = 37,
    VM_OPCODE_STATIC_CALL0 = 38,
    VM_OPCODE_STATIC_CALL1 = 39,
    VM_OPCODE_STATIC_CALL2 = 41,
    VM_OPCODE_STATIC_CALL3 = 42,
    VM_OPCODE_BRANCH_EQUAL = 43,
    VM_OPCODE_BRANCH_NOT_EQUAL = 44,
    VM_OPCODE_BRANCH_LESS = 45,
    VM_OPCODE_BRANCH_GREATER = 46,
    VM_OPCODE_BRANCH_LESS_THAN_EQUAL = 47,
    VM_OPCODE_BRANCH_GREATER_THAN_EQUAL = 48,
    VM_OPCODE_BRANCH_BOOL = 49,

    VM_OPCODE_MAX1,
    VM_OPCODE_MAX2P = 128,
};

typedef struct
{
    vm_obj_t *locals;
    vm_reg_t outreg;
    size_t index;
} vm_stack_frame_t;

#include "state.h"
#include "thread.h"

void vm_run(vm_state_t *state);
void vm_run_some(vm_state_t *state);
void vm_run_some_rec(vm_state_t **cur);
