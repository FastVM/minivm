
#include "../jump.h"
#include "../lib.h"
#include "../opcode.h"

enum vm_int_op_t
{
  VM_INT_OP_EXIT,
  VM_INT_OP_PUTC,
  VM_INT_OP_MOV,
  VM_INT_OP_MOVI,
  VM_INT_OP_ADD,
  VM_INT_OP_SUB,
  VM_INT_OP_MUL,
  VM_INT_OP_DIV,
  VM_INT_OP_MOD,
  VM_INT_OP_RET,
  VM_INT_OP_BB,
  VM_INT_OP_BEQ,
  VM_INT_OP_BLT,
  VM_INT_OP_DEF,
  VM_INT_OP_CALL0,
  VM_INT_OP_CALL1,
  VM_INT_OP_CALL2,
  VM_INT_OP_CALL3,
  VM_INT_OP_CALL4,
  VM_INT_OP_CALL5,
  VM_INT_OP_DCALL0,
  VM_INT_OP_DCALL1,
  VM_INT_OP_DCALL2,
  VM_INT_OP_DCALL3,
  VM_INT_OP_DCALL4,
  VM_INT_OP_DCALL5,
  VM_INT_OP_TCALL0,
  VM_INT_OP_TCALL1,
  VM_INT_OP_TCALL2,
  VM_INT_OP_TCALL3,
  VM_INT_OP_TCALL4,
  VM_INT_OP_TCALL5,
  VM_INT_OP_JUMP,
};

uint32_t *vm_int_comp(size_t nops, const vm_opcode_t *ops, uint8_t *jumps)
{
  size_t index = 0;
  size_t cfunc = 0;

  size_t nregs = 0;
  uint32_t named[1 << 12] = {0};
  uint32_t regs[1 << 12] = {0};

  size_t alloc = 1 << 8;
  uint32_t *buf = vm_malloc(sizeof(uint32_t) * alloc);
  size_t *froms = vm_alloc0(sizeof(size_t) * alloc);
  size_t *locs = vm_alloc0(sizeof(size_t) * nops);
  uint32_t *ret = buf;

  while (index < nops)
  {
    locs[index] = buf - ret;
    if (buf - ret + 64 > alloc)
    {
      size_t length = buf - ret;
      alloc = (length + 64) * 4;
      ret = vm_realloc(ret, sizeof(uint32_t) * alloc);
      buf = ret + length;
      froms = vm_realloc(froms, sizeof(size_t) * alloc);
    }
    switch (ops[index++])
    {
    case VM_OPCODE_EXIT:
    {
      *buf++ = VM_INT_OP_EXIT;
      break;
    }
    case VM_OPCODE_REG:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t in = ops[index++];
      *buf++ = VM_INT_OP_MOV;
      *buf++ = out;
      *buf++ = in;
      break;
    }
    case VM_OPCODE_INT:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t in = ops[index++];
      *buf++ = VM_INT_OP_MOVI;
      *buf++ = out;
      *buf++ = in;
      break;
    }
    case VM_OPCODE_JUMP:
    {
      vm_opcode_t loc = ops[index++];
      *buf++ = VM_INT_OP_JUMP;
      froms[buf - ret] = loc;
      *buf++ = 255;
      break;
    }
    case VM_OPCODE_FUNC:
    {
      vm_opcode_t end = ops[index++];
      vm_opcode_t nargs = ops[index++];
      nregs = ops[index++];
      *buf++ = VM_INT_OP_JUMP;
      froms[buf - ret] = end;
      *buf++ = 255;
      *buf++ = nregs;
      cfunc = index;
      break;
    }
    case VM_OPCODE_ADD:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      *buf++ = VM_INT_OP_ADD;
      *buf++ = out;
      *buf++ = lhs;
      *buf++ = rhs;
      break;
    }
    case VM_OPCODE_SUB:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      *buf++ = VM_INT_OP_SUB;
      *buf++ = out;
      *buf++ = lhs;
      *buf++ = rhs;
      break;
    }
    case VM_OPCODE_MUL:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      *buf++ = VM_INT_OP_MUL;
      *buf++ = out;
      *buf++ = lhs;
      *buf++ = rhs;
      break;
    }
    case VM_OPCODE_DIV:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      *buf++ = VM_INT_OP_DIV;
      *buf++ = out;
      *buf++ = lhs;
      *buf++ = rhs;
      break;
    }
    case VM_OPCODE_MOD:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      *buf++ = VM_INT_OP_MOD;
      *buf++ = out;
      *buf++ = lhs;
      *buf++ = rhs;
      break;
    }
    case VM_OPCODE_TCALL:
    {
      vm_opcode_t nargs = ops[index++];
      *buf++ = VM_INT_OP_TCALL0 + nargs;
      froms[buf - ret] = cfunc;
      *buf++ = 255;
      for (int i = 0; i < nargs; i++)
      {
        *buf++ = ops[index++];
      }
      break;
    }
    case VM_OPCODE_CALL:
    {
      vm_opcode_t rreg = ops[index++];
      vm_opcode_t func = ops[index++];
      vm_opcode_t nargs = ops[index++];
      *buf++ = VM_INT_OP_CALL0 + nargs;
      froms[buf - ret] = func;
      *buf++ = 255;
      for (int i = 0; i < nargs; i++)
      {
        *buf++ = ops[index++];
      }
      *buf++ = nregs;
      *buf++ = rreg;
      break;
    }
    case VM_OPCODE_DCALL:
    {
      vm_opcode_t rreg = ops[index++];
      vm_opcode_t func = ops[index++];
      vm_opcode_t nargs = ops[index++];
      *buf++ = VM_INT_OP_DCALL0 + nargs;
      *buf++ = func;
      for (int i = 0; i < nargs; i++)
      {
        *buf++ = ops[index++];
      }
      *buf++ = nregs;
      *buf++ = rreg;
      break;
    }
    case VM_OPCODE_RET:
    {
      vm_opcode_t out = ops[index++];
      *buf++ = VM_INT_OP_RET;
      *buf++ = out;
      break;
    }
    case VM_OPCODE_PUTCHAR:
    {
      vm_opcode_t in = ops[index++];
      *buf++ = VM_INT_OP_PUTC;
      *buf++ = in;
      break;
    }
    case VM_OPCODE_BB:
    {
      vm_opcode_t val = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      *buf++ = VM_INT_OP_BB;
      *buf++ = val;
      froms[buf - ret] = jfalse;
      *buf++ = 255;
      froms[buf - ret] = jtrue;
      *buf++ = 255;
      break;
    }
    case VM_OPCODE_BEQ:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      *buf++ = VM_INT_OP_BEQ;
      *buf++ = lhs;
      *buf++ = rhs;
      froms[buf - ret] = jfalse;
      *buf++ = 255;
      froms[buf - ret] = jtrue;
      *buf++ = 255;
      break;
    }
    case VM_OPCODE_BLT:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      *buf++ = VM_INT_OP_BLT;
      *buf++ = lhs;
      *buf++ = rhs;
      froms[buf - ret] = jfalse;
      *buf++ = 255;
      froms[buf - ret] = jtrue;
      *buf++ = 255;
      break;
    }
    case VM_OPCODE_INTF:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t func = ops[index++];
      *buf++ = VM_INT_OP_MOVI;
      *buf++ = out;
      froms[buf - ret] = func;
      *buf++ = 255;
      break;
    }
    default:
    {
      printf("err %zu\n", (size_t)ops[index - 1]);
      return NULL;
    }
    }
  }
  size_t length = buf - ret;
  for (size_t i = 0; i < length; i++)
  {
    if (froms[i] != 0)
    {
      ret[i] = locs[froms[i]];
    }
  }
  return ret;
}

#define vm_int_read() ops[index++]
#if 0
#define vm_int_jump_next()                         \
  printf("%zu: %zu\n", index, (size_t)ops[index]); \
  goto *ptrs[vm_int_read()]
#else
#define vm_int_jump_next() goto *ptrs[vm_int_read()]
#endif

void vm_int_run(uint32_t *ops)
{
  static void *ptrs[] = {
      [VM_INT_OP_EXIT] = &&exec_exit,
      [VM_INT_OP_PUTC] = &&exec_putc,
      [VM_INT_OP_MOV] = &&exec_mov,
      [VM_INT_OP_MOVI] = &&exec_movi,
      [VM_INT_OP_ADD] = &&exec_add,
      [VM_INT_OP_SUB] = &&exec_sub,
      [VM_INT_OP_MUL] = &&exec_mul,
      [VM_INT_OP_DIV] = &&exec_div,
      [VM_INT_OP_MOD] = &&exec_mod,
      [VM_INT_OP_RET] = &&exec_ret,
      [VM_INT_OP_BB] = &&exec_bb,
      [VM_INT_OP_BEQ] = &&exec_beq,
      [VM_INT_OP_BLT] = &&exec_blt,
      [VM_INT_OP_DEF] = &&exec_def,
      [VM_INT_OP_CALL0] = &&exec_call0,
      [VM_INT_OP_CALL1] = &&exec_call1,
      [VM_INT_OP_CALL2] = &&exec_call2,
      [VM_INT_OP_CALL3] = &&exec_call3,
      [VM_INT_OP_CALL4] = &&exec_call4,
      [VM_INT_OP_CALL5] = &&exec_call5,
      [VM_INT_OP_DCALL0] = &&exec_dcall0,
      [VM_INT_OP_DCALL1] = &&exec_dcall1,
      [VM_INT_OP_DCALL2] = &&exec_dcall2,
      [VM_INT_OP_DCALL3] = &&exec_dcall3,
      [VM_INT_OP_DCALL4] = &&exec_dcall4,
      [VM_INT_OP_DCALL5] = &&exec_dcall5,
      [VM_INT_OP_TCALL0] = &&exec_tcall0,
      [VM_INT_OP_TCALL1] = &&exec_tcall1,
      [VM_INT_OP_TCALL2] = &&exec_tcall2,
      [VM_INT_OP_TCALL3] = &&exec_tcall3,
      [VM_INT_OP_TCALL4] = &&exec_tcall4,
      [VM_INT_OP_TCALL5] = &&exec_tcall5,
      [VM_INT_OP_JUMP] = &&exec_jump,
  };
  size_t *stack = malloc(sizeof(size_t) * (1 << 12));
  size_t *regs = malloc(sizeof(size_t) * (1 << 20));
  size_t *stack_base = stack;
  size_t *regs_base = regs;
  size_t index = 0;
  vm_int_jump_next();
exec_exit:
{
  free(stack_base);
  free(regs_base);
  return;
}
exec_putc:
{
  size_t reg = vm_int_read();
  putchar((int)regs[reg]);
  vm_int_jump_next();
}
exec_mov:
{
  size_t out = vm_int_read();
  size_t in = vm_int_read();
  regs[out] = regs[in];
  vm_int_jump_next();
}
exec_movi:
{
  size_t out = vm_int_read();
  size_t in = vm_int_read();
  regs[out] = in;
  vm_int_jump_next();
}
exec_add:
{
  size_t out = vm_int_read();
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  regs[out] = regs[lhs] + regs[rhs];
  vm_int_jump_next();
}
exec_sub:
{
  size_t out = vm_int_read();
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  regs[out] = regs[lhs] - regs[rhs];
  vm_int_jump_next();
}
exec_mul:
{
  size_t out = vm_int_read();
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  regs[out] = regs[lhs] * regs[rhs];
  vm_int_jump_next();
}
exec_div:
{
  size_t out = vm_int_read();
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  regs[out] = regs[lhs] / regs[rhs];
  vm_int_jump_next();
}
exec_mod:
{
  size_t out = vm_int_read();
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  regs[out] = regs[lhs] % regs[rhs];
  vm_int_jump_next();
}
exec_ret:
{
  size_t inval = regs[vm_int_read()];
  index = *--stack;
  regs -= ops[index - 1];
  regs[vm_int_read()] = inval;
  vm_int_jump_next();
}
exec_bb:
{
  size_t in = vm_int_read();
  size_t jfalse = vm_int_read();
  size_t jtrue = vm_int_read();
  if (regs[in])
  {
    index = jtrue;
  }
  else
  {
    index = jfalse;
  }
  vm_int_jump_next();
}
exec_beq:
{
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  size_t jfalse = vm_int_read();
  size_t jtrue = vm_int_read();
  if (regs[lhs] == regs[rhs])
  {
    index = jtrue;
  }
  else
  {
    index = jfalse;
  }
  vm_int_jump_next();
}
exec_blt:
{
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  size_t jfalse = vm_int_read();
  size_t jtrue = vm_int_read();
  if (regs[lhs] < regs[rhs])
  {
    index = jtrue;
  }
  else
  {
    index = jfalse;
  }
  vm_int_jump_next();
}
exec_def:
{
  size_t next_index = vm_int_read();
  index = next_index;
  vm_int_jump_next();
}
exec_call0:
{
  size_t func = vm_int_read();
  size_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  index = func;
  vm_int_jump_next();
}
exec_call1:
{
  size_t func = vm_int_read();
  size_t r1 = regs[vm_int_read()];
  size_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  index = func;
  vm_int_jump_next();
}
exec_call2:
{
  size_t func = vm_int_read();
  size_t r1 = regs[vm_int_read()];
  size_t r2 = regs[vm_int_read()];
  size_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  regs[2] = r2;
  index = func;
  vm_int_jump_next();
}
exec_call3:
{
  size_t func = vm_int_read();
  size_t r1 = regs[vm_int_read()];
  size_t r2 = regs[vm_int_read()];
  size_t r3 = regs[vm_int_read()];
  size_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  regs[2] = r2;
  regs[3] = r2;
  index = func;
  vm_int_jump_next();
}
exec_call4:
{
  size_t func = vm_int_read();
  size_t r1 = regs[vm_int_read()];
  size_t r2 = regs[vm_int_read()];
  size_t r3 = regs[vm_int_read()];
  size_t r4 = regs[vm_int_read()];
  size_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  regs[2] = r2;
  regs[3] = r2;
  regs[4] = r2;
  index = func;
  vm_int_jump_next();
}
exec_call5:
{
  size_t func = vm_int_read();
  size_t r1 = regs[vm_int_read()];
  size_t r2 = regs[vm_int_read()];
  size_t r3 = regs[vm_int_read()];
  size_t r4 = regs[vm_int_read()];
  size_t r5 = regs[vm_int_read()];
  size_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  regs[2] = r2;
  regs[3] = r2;
  regs[4] = r2;
  regs[5] = r2;
  index = func;
  vm_int_jump_next();
}
exec_dcall0:
{
  size_t where = vm_int_read();
  size_t func = regs[where];
  size_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  index = func;
  vm_int_jump_next();
}
exec_dcall1:
{
  size_t func = regs[vm_int_read()];
  size_t r1 = regs[vm_int_read()];
  size_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  index = func;
  vm_int_jump_next();
}
exec_dcall2:
{
  size_t func = regs[vm_int_read()];
  size_t r1 = regs[vm_int_read()];
  size_t r2 = regs[vm_int_read()];
  size_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  regs[2] = r2;
  index = func;
  vm_int_jump_next();
}
exec_dcall3:
{
  size_t func = regs[vm_int_read()];
  size_t r1 = regs[vm_int_read()];
  size_t r2 = regs[vm_int_read()];
  size_t r3 = regs[vm_int_read()];
  size_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  regs[2] = r2;
  regs[3] = r2;
  index = func;
  vm_int_jump_next();
}
exec_dcall4:
{
  size_t func = regs[vm_int_read()];
  size_t r1 = regs[vm_int_read()];
  size_t r2 = regs[vm_int_read()];
  size_t r3 = regs[vm_int_read()];
  size_t r4 = regs[vm_int_read()];
  size_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  regs[2] = r2;
  regs[3] = r2;
  regs[4] = r2;
  index = func;
  vm_int_jump_next();
}
exec_dcall5:
{
  size_t func = regs[vm_int_read()];
  size_t r1 = regs[vm_int_read()];
  size_t r2 = regs[vm_int_read()];
  size_t r3 = regs[vm_int_read()];
  size_t r4 = regs[vm_int_read()];
  size_t r5 = regs[vm_int_read()];
  size_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  regs[2] = r2;
  regs[3] = r2;
  regs[4] = r2;
  regs[5] = r2;
  index = func;
  vm_int_jump_next();
}
exec_tcall0:
{
  size_t func = vm_int_read();
  index = func;
  vm_int_jump_next();
}
exec_tcall1:
{
  size_t func = vm_int_read();
  size_t r1 = regs[vm_int_read()];
  index = func;
  regs[1] = r1;
  vm_int_jump_next();
}
exec_tcall2:
{
  size_t func = vm_int_read();
  size_t r1 = regs[vm_int_read()];
  size_t r2 = regs[vm_int_read()];
  index = func;
  regs[1] = r1;
  regs[2] = r2;
  vm_int_jump_next();
}
exec_tcall3:
{
  size_t func = vm_int_read();
  size_t r1 = regs[vm_int_read()];
  size_t r2 = regs[vm_int_read()];
  size_t r3 = regs[vm_int_read()];
  index = func;
  regs[1] = r1;
  regs[2] = r2;
  regs[3] = r3;
  vm_int_jump_next();
}
exec_tcall4:
{
  size_t func = vm_int_read();
  size_t r1 = regs[vm_int_read()];
  size_t r2 = regs[vm_int_read()];
  size_t r3 = regs[vm_int_read()];
  size_t r4 = regs[vm_int_read()];
  index = func;
  regs[1] = r1;
  regs[2] = r2;
  regs[3] = r3;
  regs[4] = r4;
  vm_int_jump_next();
}
exec_tcall5:
{
  size_t func = vm_int_read();
  size_t r1 = regs[vm_int_read()];
  size_t r2 = regs[vm_int_read()];
  size_t r3 = regs[vm_int_read()];
  size_t r4 = regs[vm_int_read()];
  size_t r5 = regs[vm_int_read()];
  index = func;
  regs[1] = r1;
  regs[2] = r2;
  regs[3] = r3;
  regs[4] = r4;
  regs[5] = r5;
  vm_int_jump_next();
}
exec_jump:
{
  size_t where = vm_int_read();
  index = where;
  vm_int_jump_next();
}
fail:
  printf("not implemented: %zu@%zu\n", (size_t)ops[index - 1], index);
  return;
}

int vm_run(size_t nops, const vm_opcode_t *ops)
{
  uint8_t *jumps = vm_jump_all(nops, ops);
  if (jumps == NULL)
  {
    return 1;
  }
  uint32_t *iops = vm_int_comp(nops, ops, jumps);
  if (iops == NULL)
  {
    free(jumps);
    return 1;
  }
  vm_int_run(iops);
  free(jumps);
  return 0;
}
