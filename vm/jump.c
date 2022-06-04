
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
    case VM_OPCODE_INT:
    case VM_OPCODE_NEG:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t num = ops[index++];
      break;
    }
    case VM_OPCODE_STR:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t nints = ops[index++];
      index += nints;
      break;
    }
    case VM_OPCODE_ARR:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t size = ops[index++];
      break;
    }
    case VM_OPCODE_MAP:
    {
      vm_opcode_t outreg = ops[index++];
      break;
    }
    case VM_OPCODE_GET:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t obj = ops[index++];
      vm_opcode_t ind = ops[index++];
      break;
    }
    case VM_OPCODE_SET:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t obj = ops[index++];
      vm_opcode_t ind = ops[index++];
      break;
    }
    case VM_OPCODE_RET:
    {
      vm_opcode_t outreg = ops[index++];
      return;
    }
    case VM_OPCODE_ADD:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_SUB:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_MUL:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_DIV:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_MOD:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_BB:
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
    case VM_OPCODE_ADDR:
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
    case VM_OPCODE_DJUMP:
    {
      vm_opcode_t jreg = ops[index++];
      return;
    }
    case VM_OPCODE_JUMP:
    {
      vm_opcode_t over = ops[index++];
      vm_jump_reachable_from(over, nops, ops, jumps);
      return;
    }
    case VM_OPCODE_BEQ:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      vm_jump_reachable_from(jfalse, nops, ops, jumps);
      vm_jump_reachable_from(jtrue, nops, ops, jumps);
      return;
    }
    case VM_OPCODE_BLT:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      vm_jump_reachable_from(jfalse, nops, ops, jumps);
      vm_jump_reachable_from(jtrue, nops, ops, jumps);
      return;
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
    case VM_OPCODE_INT:
    case VM_OPCODE_NEG:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t value = ops[index++];
      break;
    }
    case VM_OPCODE_STR:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t nints = ops[index++];
      index += nints;
      break;
    }
    case VM_OPCODE_ARR:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t size = ops[index++];
      break;
    }
    case VM_OPCODE_MAP:
    {
      vm_opcode_t outreg = ops[index++];
      break;
    }
    case VM_OPCODE_GET:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t obj = ops[index++];
      vm_opcode_t ind = ops[index++];
      break;
    }
    case VM_OPCODE_SET:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t obj = ops[index++];
      vm_opcode_t ind = ops[index++];
      break;
    }
    case VM_OPCODE_RET:
    {
      ret[index-1] |= VM_JUMP_OUT;
      vm_opcode_t outreg = ops[index++];
      break;
    }
    case VM_OPCODE_ADD:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_SUB:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_MUL:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_DIV:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_MOD:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_BB:
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
    case VM_OPCODE_DJUMP:
    {
      ret[index-1] |= VM_JUMP_OUT;
      vm_opcode_t reg = ops[index++];
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
    case VM_OPCODE_BEQ:
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
    case VM_OPCODE_BLT:
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
    case VM_OPCODE_ADDR:
    {
      vm_opcode_t reg = ops[index++];
      vm_opcode_t func = ops[index++];
      break;
    }
    default:
      fprintf(stderr, "unknown opcode: %zu\n", (size_t)ops[index - 1]);
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
