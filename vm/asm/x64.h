#if !defined(VM_HEADER_JIT_X64)
#define VM_HEADER_JIT_X64

#include "../lib.h"

enum vm_x64_reg_t {
    VM_X64_REG_RAX,
    VM_X64_REG_RCX,
    VM_X64_REG_RDX,
    VM_X64_REG_RBX,
    VM_X64_REG_RSP,
    VM_X64_REG_RBP,
    VM_X64_REG_RSI,
    VM_X64_REG_RDI,
    VM_X64_REG_R8,
    VM_X64_REG_R9,
    VM_X64_REG_R10,
    VM_X64_REG_R11,
    VM_X64_REG_R12,
    VM_X64_REG_R13,
    VM_X64_REG_R14,
    VM_X64_REG_R15,
};

struct vm_x64_instr_t;
typedef struct vm_x64_instr_t vm_x64_instr_t;

struct vm_x64_arg_t;
typedef struct vm_x64_arg_t vm_x64_arg_t;

struct vm_x64_arg_t {
    union {
        uint8_t reg;
        int32_t imm32;
    };
    uint8_t tag;
};

struct vm_x64_instr_t {
    uint16_t opcode;
    vm_x64_arg_t out;
    vm_x64_arg_t args[3];
};

#endif