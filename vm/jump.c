
#include "jump.h"

void vm_jump_reachable_from(size_t index, size_t nops, const vm_opcode_t *ops, uint8_t *jumps)
{
  if (jumps[index] & VM_JUMP_REACH)
  {
    return;
  }
  while (index < nops)
  {
    jumps[index] |= VM_JUMP_REACH;
    switch (ops[index++])
    {
    case VM_OPCODE_EXIT:
    {
      return;
    }
    case VM_OPCODE_REG:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t inreg = ops[index++];
      break;
    }
    case VM_OPCODE_FRAC:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_INT:
    case VM_OPCODE_FINT:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t num = ops[index++];
      break;
    }
    case VM_OPCODE_RET:
    {
      vm_opcode_t outreg = ops[index++];
      return;
    }
    case VM_OPCODE_UADD:
    case VM_OPCODE_FADD:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_USUB:
    case VM_OPCODE_FSUB:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_UMUL:
    case VM_OPCODE_FMUL:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_UDIV:
    case VM_OPCODE_FDIV:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_UMOD:
    case VM_OPCODE_FMOD:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_UBB:
    case VM_OPCODE_FBB:
    {
      vm_opcode_t inreg = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      vm_jump_reachable_from(jfalse, nops, ops, jumps);
      vm_jump_reachable_from(jtrue, nops, ops, jumps);
      return;
    }
    case VM_OPCODE_CALL:
    {
      vm_opcode_t rreg = ops[index++];
      vm_opcode_t func = ops[index++];
      vm_opcode_t nargs = ops[index++];
      index += nargs;
      vm_jump_reachable_from(func, nops, ops, jumps);
      break;
    }
    case VM_OPCODE_DCALL:
    {
      vm_opcode_t rreg = ops[index++];
      vm_opcode_t func = ops[index++];
      vm_opcode_t nargs = ops[index++];
      index += nargs;
      break;
    }
    case VM_OPCODE_INTF:
    {
      vm_opcode_t reg = ops[index++];
      vm_opcode_t func = ops[index++];
      vm_jump_reachable_from(func, nops, ops, jumps);
      break;
    }
    case VM_OPCODE_PUTCHAR:
    {
      vm_opcode_t inreg = ops[index++];
      break;
    }
    case VM_OPCODE_JUMP:
    {
      vm_opcode_t over = ops[index++];
      vm_jump_reachable_from(over, nops, ops, jumps);
      return;
    }
    case VM_OPCODE_FUNC:
    {
      vm_opcode_t over = ops[index++];
      vm_opcode_t nargs = ops[index++];
      vm_opcode_t nregs = ops[index++];
      // vm_jump_reachable_from(index, nops, ops, jumps);
      vm_jump_reachable_from(over, nops, ops, jumps);
      return;
    }
    case VM_OPCODE_UBEQ:
    case VM_OPCODE_FBEQ:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      vm_jump_reachable_from(jfalse, nops, ops, jumps);
      vm_jump_reachable_from(jtrue, nops, ops, jumps);
      return;
    }
    case VM_OPCODE_UBLT:
    case VM_OPCODE_FBLT:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      vm_jump_reachable_from(jfalse, nops, ops, jumps);
      vm_jump_reachable_from(jtrue, nops, ops, jumps);
      return;
    }
    case VM_OPCODE_CONS:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t car = ops[index++];
      vm_opcode_t cdr = ops[index++];
      break;
    }
    case VM_OPCODE_GETCAR:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t pair = ops[index++];
      break;
    }
    case VM_OPCODE_GETCDR:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t pair = ops[index++];
      break;
    }
    case VM_OPCODE_SETCAR:
    {
      vm_opcode_t pair = ops[index++];
      vm_opcode_t val = ops[index++];
      break;
    }
    case VM_OPCODE_SETCDR:
    {
      vm_opcode_t pair = ops[index++];
      vm_opcode_t val = ops[index++];
      break;
    }
    default:
      return;
    }
  }
}

void vm_jump_reachable(size_t nops, const vm_opcode_t *ops, uint8_t *jumps)
{
  vm_jump_reachable_from(0, nops, ops, jumps);
}

uint8_t *vm_jump_base(size_t nops, const vm_opcode_t *ops)
{
  uint8_t *ret = vm_alloc0(sizeof(uint8_t) * nops);
  size_t index = 0;
  while (index < nops)
  {
    ret[index] |= VM_JUMP_INSTR;
    switch (ops[index++])
    {
    case VM_OPCODE_EXIT:
    {
      ret[index-1] |= VM_JUMP_OUT;
      break;
    }
    case VM_OPCODE_REG:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t inreg = ops[index++];
      break;
    }
    case VM_OPCODE_FTOU:
    case VM_OPCODE_UTOF:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t inreg = ops[index++];
      break;
    }
    case VM_OPCODE_INT:
    case VM_OPCODE_FINT:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t num = ops[index++];
      break;
    }
    case VM_OPCODE_RET:
    {
      ret[index-1] |= VM_JUMP_OUT;
      vm_opcode_t outreg = ops[index++];
      break;
    }
    case VM_OPCODE_UADD:
    case VM_OPCODE_FADD:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_USUB:
    case VM_OPCODE_FSUB:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_UMUL:
    case VM_OPCODE_FMUL:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_UDIV:
    case VM_OPCODE_FDIV:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_UMOD:
    case VM_OPCODE_FMOD:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_UBB:
    {
      ret[index-1] |= VM_JUMP_OUT;
      vm_opcode_t inreg = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      ret[jfalse] |= VM_JUMP_IN;
      ret[jtrue] |= VM_JUMP_IN;
      break;
    }
    case VM_OPCODE_CALL:
    {
      vm_opcode_t rreg = ops[index++];
      vm_opcode_t func = ops[index++];
      vm_opcode_t nargs = ops[index++];
      index += nargs;
      break;
    }
    case VM_OPCODE_DCALL:
    {
      vm_opcode_t rreg = ops[index++];
      vm_opcode_t func = ops[index++];
      vm_opcode_t nargs = ops[index++];
      index += nargs;
      break;
    }
    case VM_OPCODE_PUTCHAR:
    {
      vm_opcode_t inreg = ops[index++];
      break;
    }
    case VM_OPCODE_JUMP:
    {
      ret[index-1] |= VM_JUMP_OUT;
      vm_opcode_t over = ops[index++];
      ret[over] |= VM_JUMP_IN;
      break;
    }
    case VM_OPCODE_FUNC:
    {
      vm_opcode_t over = ops[index++];
      vm_opcode_t nargs = ops[index++];
      vm_opcode_t nregs = ops[index++];
      ret[index] = VM_JUMP_INIT;
      break;
    }
    case VM_OPCODE_UBEQ:
    case VM_OPCODE_FBEQ:
    {
      ret[index-1] |= VM_JUMP_OUT;
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      ret[jfalse] |= VM_JUMP_IN;
      ret[jtrue] |= VM_JUMP_IN;
      break;
    }
    case VM_OPCODE_UBLT:
    case VM_OPCODE_FBLT:
    {
      ret[index-1] |= VM_JUMP_OUT;
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      ret[jfalse] |= VM_JUMP_IN;
      ret[jtrue] |= VM_JUMP_IN;
      break;
    }
    case VM_OPCODE_INTF:
    {
      vm_opcode_t reg = ops[index++];
      vm_opcode_t func = ops[index++];
      break;
    }
    case VM_OPCODE_CONS:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t car = ops[index++];
      vm_opcode_t cdr = ops[index++];
      break;
    }
    case VM_OPCODE_GETCAR:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t pair = ops[index++];
      break;
    }
    case VM_OPCODE_GETCDR:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t pair = ops[index++];
      break;
    }
    case VM_OPCODE_SETCAR:
    {
      vm_opcode_t pair = ops[index++];
      vm_opcode_t reg = ops[index++];
      break;
    }
    case VM_OPCODE_SETCDR:
    {
      vm_opcode_t pair = ops[index++];
      vm_opcode_t reg = ops[index++];
      break;
    }
    default:
      printf("unknown opcode: %zu\n", (size_t)ops[index - 1]);
      return NULL;
    }
  }
  return ret;
}

uint8_t *vm_jump_all(size_t nops, const vm_opcode_t *ops)
{
  uint8_t *jumps = vm_jump_base(nops, ops);
  if (jumps == NULL)
  {
    return NULL;
  }
  vm_jump_reachable(nops, ops, jumps);
  return jumps;
}
