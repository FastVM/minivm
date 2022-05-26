
#include "reguse.h"
#include "opcode.h"
#include "jump.h"

int vm_reg_is_used(size_t nops, const vm_opcode_t *ops, uint8_t *jumps, size_t index, size_t reg, size_t rem)
{
  if (rem == 0) {
    return 1;
  }
  while (index < nops)
  {
    switch (ops[index])
    {
    case VM_OPCODE_EXIT:
      return 0;
    case VM_OPCODE_DJUMP:
      return 1;
    case VM_OPCODE_RET:
      return ops[index + 1] == reg;
    case VM_OPCODE_FUNC:
    case VM_OPCODE_JUMP:
    {
      vm_opcode_t dest = ops[index + 1];
      return vm_reg_is_used(nops, ops, jumps, dest, reg, rem-1);
    }
    case VM_OPCODE_SETCAR:
    case VM_OPCODE_SETCDR:
    {
      if (ops[index + 1] == reg)
      {
        return 1;
      }
      if (ops[index + 2] == reg)
      {
        return 1;
      }
      break;
    }
    case VM_OPCODE_PUTCHAR:
    {
      if (ops[index + 1] == reg)
      {
        return 1;
      }
      break;
    }
    case VM_OPCODE_UBB:
    case VM_OPCODE_FBB:
    {
      if (ops[index + 1] == reg)
      {
        return 1;
      }
      vm_opcode_t jfalse = ops[index + 2];
      vm_opcode_t jtrue = ops[index + 3];
      return vm_reg_is_used(nops, ops, jumps, jfalse, reg, rem/2) || vm_reg_is_used(nops, ops, jumps, jtrue, reg, rem/2);
    }
    case VM_OPCODE_UBEQ:
    case VM_OPCODE_UBLT:
    case VM_OPCODE_FBEQ:
    case VM_OPCODE_FBLT:
    {
      if (ops[index + 1] == reg || ops[index + 2] == reg)
      {
        return 1;
      }
      vm_opcode_t jfalse = ops[index + 3];
      vm_opcode_t jtrue = ops[index + 4];
      return vm_reg_is_used(nops, ops, jumps, jfalse, reg, rem/2) || vm_reg_is_used(nops, ops, jumps, jtrue, reg, rem/2);
    }
    case VM_OPCODE_CONS:
    case VM_OPCODE_UADD:
    case VM_OPCODE_USUB:
    case VM_OPCODE_UMUL:
    case VM_OPCODE_UDIV:
    case VM_OPCODE_UMOD:
    case VM_OPCODE_FADD:
    case VM_OPCODE_FSUB:
    case VM_OPCODE_FMUL:
    case VM_OPCODE_FDIV:
    case VM_OPCODE_FMOD:
    {
      if (ops[index + 2] == reg || ops[index + 3] == reg)
      {
        return 1;
      }
      if (ops[index + 1] == reg)
      {
        return 0;
      }
      break;
    }
    case VM_OPCODE_GETCAR:
    case VM_OPCODE_GETCDR:
    case VM_OPCODE_REG:
      if (ops[index + 2] == reg)
      {
        return 1;
      }
      if (ops[index + 1] == reg)
      {
        return 0;
      }
      break;
    case VM_OPCODE_INT:
    case VM_OPCODE_FINT:
      if (ops[index + 1] == reg)
      {
        return 0;
      }
      break;
    case VM_OPCODE_XCALL:
    case VM_OPCODE_CALL:
    case VM_OPCODE_DCALL:
    {
      vm_opcode_t rreg = ops[index + 1];
      vm_opcode_t nargs = ops[index + 3];
      for (size_t i = 0; i < nargs; i++)
      {
        if (ops[index + 4 + i] == reg)
        {
          return 1;
        }
      }
      if (rreg == reg)
      {
        return 0;
      }
      break;
    }
    }
    index += 1;
    while (!(jumps[index] & VM_JUMP_INSTR))
    {
      index += 1;
    }
  }
  return 0;
}
