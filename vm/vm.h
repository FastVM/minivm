#pragma once

#include "libc.h"

typedef int16_t vm_reg_t;
typedef int16_t vm_opcode_t;

#include "obj.h"

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
    VM_OPCODE_BRANCH_TRUE= 12,
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
    VM_OPCODE_MAP_NEW = 34,
    VM_OPCODE_LOAD_GLOBAL = 35,
    VM_OPCODE_MAX1,
    VM_OPCODE_MAX2P = 128,
};

typedef struct
{
    int index;
    int nargs;
    vm_reg_t outreg;
    void *locals;
} vm_stack_frame_t;
