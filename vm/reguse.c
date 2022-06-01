
#include "reguse.h"
#include "opcode.h"
#include "jump.h"

int vm_reg_is_used(size_t nops, const vm_opcode_t *ops, uint8_t *jumps, size_t index, size_t reg, size_t nbuf, size_t* buf, size_t head)
{
  if (head == nbuf) {
    return 1;
  }
  for (size_t i = 0; i < head; i++) {
    if (buf[head] == index) {
      return 0;
    }
  }
  buf[head] = index;
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
      return vm_reg_is_used(nops, ops, jumps, dest, reg, nbuf, buf, head+1);
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
    case VM_OPCODE_SBB:
    case VM_OPCODE_FBB:
    {
      if (ops[index + 1] == reg)
      {
        return 1;
      }
      vm_opcode_t jfalse = ops[index + 2];
      vm_opcode_t jtrue = ops[index + 3];
      return vm_reg_is_used(nops, ops, jumps, jfalse, reg, nbuf, buf, head+1) || vm_reg_is_used(nops, ops, jumps, jtrue, reg, nbuf, buf, head+1);
    }
    case VM_OPCODE_UBEQ:
    case VM_OPCODE_UBLT:
    case VM_OPCODE_SBEQ:
    case VM_OPCODE_SBLT:
    case VM_OPCODE_FBEQ:
    case VM_OPCODE_FBLT:
    {
      if (ops[index + 1] == reg || ops[index + 2] == reg)
      {
        return 1;
      }
      vm_opcode_t jfalse = ops[index + 3];
      vm_opcode_t jtrue = ops[index + 4];
      return vm_reg_is_used(nops, ops, jumps, jfalse, reg, nbuf, buf, head+1) || vm_reg_is_used(nops, ops, jumps, jtrue, reg, nbuf, buf, head+1);
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
    case VM_OPCODE_SADD:
    case VM_OPCODE_SSUB:
    case VM_OPCODE_SMUL:
    case VM_OPCODE_SDIV:
    case VM_OPCODE_SMOD:
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
    case VM_OPCODE_UINT:
    case VM_OPCODE_FINT:
    case VM_OPCODE_SINT:
    case VM_OPCODE_SNEG:
      if (ops[index + 1] == reg)
      {
        return 0;
      }
      break;
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
    while ((jumps[index] & VM_JUMP_INSTR) == 0)
    {
      index += 1;
    }
  }
  return 0;
}
