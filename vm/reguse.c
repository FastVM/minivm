
#include "jump.h"
#include "opcode.h"

int vm_reg_is_used(size_t nops, const vm_opcode_t *ops, uint8_t *jumps,
                   size_t index, size_t reg, size_t nbuf, size_t *buf,
                   size_t head) {
  if (head == nbuf) {
    return 1;
  }
  for (size_t i = 0; i < head; i++) {
    if (buf[i] == index) {
      return 0;
    }
  }
  buf[head] = index;
  while (index < nops) {
    switch (ops[index]) {
    case VM_OPCODE_EXIT:
      return 0;
    case VM_OPCODE_RET:
      return ops[index + 1] == reg;
    case VM_OPCODE_FUNC:
    case VM_OPCODE_JUMP: {
      vm_opcode_t dest = ops[index + 1];
      return vm_reg_is_used(nops, ops, jumps, dest, reg, nbuf, buf, head + 1);
    }
    case VM_OPCODE_PUTCHAR: {
      if (ops[index + 1] == reg) {
        return 1;
      }
      break;
    }
    case VM_OPCODE_BB: {
      if (ops[index + 1] == reg) {
        return 1;
      }
      vm_opcode_t jfalse = ops[index + 2];
      vm_opcode_t jtrue = ops[index + 3];
      return vm_reg_is_used(nops, ops, jumps, jfalse, reg, nbuf, buf,
                            head + 1) ||
             vm_reg_is_used(nops, ops, jumps, jtrue, reg, nbuf, buf, head + 1);
    }
    case VM_OPCODE_BEQ:
    case VM_OPCODE_BLT: {
      if (ops[index + 1] == reg || ops[index + 2] == reg) {
        return 1;
      }
      vm_opcode_t jfalse = ops[index + 3];
      vm_opcode_t jtrue = ops[index + 4];
      return vm_reg_is_used(nops, ops, jumps, jfalse, reg, nbuf, buf,
                            head + 1) ||
             vm_reg_is_used(nops, ops, jumps, jtrue, reg, nbuf, buf, head + 1);
    }
    case VM_OPCODE_ADD:
    case VM_OPCODE_SUB:
    case VM_OPCODE_MUL:
    case VM_OPCODE_DIV:
    case VM_OPCODE_MOD:
    case VM_OPCODE_GET:
    case VM_OPCODE_SET: {
      if (ops[index + 2] == reg || ops[index + 3] == reg) {
        return 1;
      }
      if (ops[index + 1] == reg) {
        return 0;
      }
      break;
    }
    case VM_OPCODE_ARR:
    case VM_OPCODE_LEN:
    case VM_OPCODE_TYPE:
    case VM_OPCODE_REG:
      if (ops[index + 2] == reg) {
        return 1;
      }
      if (ops[index + 1] == reg) {
        return 0;
      }
      break;
    case VM_OPCODE_INT:
    case VM_OPCODE_NEG:
      if (ops[index + 1] == reg) {
        return 0;
      }
      break;
    case VM_OPCODE_CALL:
    case VM_OPCODE_XCALL:
    case VM_OPCODE_DCALL: {
      vm_opcode_t rreg = ops[index + 1];
      vm_opcode_t nargs = ops[index + 3];
      for (size_t i = 0; i < nargs; i++) {
        if (ops[index + 4 + i] == reg) {
          return 1;
        }
      }
      if (rreg == reg) {
        return 0;
      }
      break;
    }
    }
    index += 1;
    while ((jumps[index] & VM_JUMP_INSTR) == 0) {
      index += 1;
    }
  }
  return 0;
}
