
#if !defined(VM_HEADER_OPCODE)
#define VM_HEADER_OPCODE

#include "lib.h"

enum {
  VM_OPCODE_EXIT,
  VM_OPCODE_REG,
  VM_OPCODE_JUMP,
  VM_OPCODE_FUNC,

  VM_OPCODE_CALL,
  VM_OPCODE_DCALL,
  VM_OPCODE_ADDR,
  VM_OPCODE_RET,

  VM_OPCODE_PUTCHAR,

  VM_OPCODE_INT,
  VM_OPCODE_NEG,
  VM_OPCODE_ADD,
  VM_OPCODE_SUB,
  VM_OPCODE_MUL,
  VM_OPCODE_DIV,
  VM_OPCODE_MOD,
  VM_OPCODE_BB,
  VM_OPCODE_BEQ,
  VM_OPCODE_BLT,

  VM_OPCODE_STR,

  VM_OPCODE_ARR,
  VM_OPCODE_SET,
  VM_OPCODE_GET,
  VM_OPCODE_LEN,

  VM_OPCODE_XCALL,

  VM_OPCODE_TYPE,
};

typedef uint32_t vm_opcode_t;

int vm_reg_is_used(size_t nops, const vm_opcode_t *ops, uint8_t *jumps,
                   size_t index, size_t reg, size_t nbuf, size_t *buf,
                   size_t head);

#endif
