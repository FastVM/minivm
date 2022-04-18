
#include "jump.h"
#include "lib.h"
#include "opcode.h"
#include "reguse.h"
#include "gc.h"

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
  VM_INT_OP_BEQI,
  VM_INT_OP_BLTI,
  VM_INT_OP_BILT,
  VM_INT_OP_ADDI,
  VM_INT_OP_SUBI,
  VM_INT_OP_MULI,
  VM_INT_OP_DIVI,
  VM_INT_OP_MODI,
  VM_INT_OP_ISUB,
  VM_INT_OP_IDIV,
  VM_INT_OP_IMOD,
  VM_INT_OP_RETI,
  VM_INT_OP_PAIR,
  VM_INT_OP_PAIRI,
  VM_INT_OP_IPAIR,
  VM_INT_OP_FIRST,
  VM_INT_OP_SECOND,
};

#define VM_INT_DUMP() (vm_int_dump(nops, ops, index, nregs, named, regs, jumps, &buf, ptrs))
static void vm_int_dump(size_t nops, const vm_opcode_t *ops, size_t index, size_t nregs, uint8_t *named, size_t *regs, uint8_t *jumps, size_t **pbuf, void **ptrs)
{
  if ((jumps[index] & VM_JUMP_IN) || (jumps[index] & VM_JUMP_OUT))
  {
    size_t *buf = *pbuf;
    for (size_t i = 0; i < nregs; i++)
    {
      if (!named[i])
      {
        continue;
      }
      if (VM_REG_IS_USED(index, i))
      {
        *buf++ = (size_t) ptrs[VM_INT_OP_MOVI];
        *buf++ = i;
        *buf++ = regs[i];
      }
      named[i] = 0;
    }
    *pbuf = buf;
  }
}

size_t *vm_int_comp(size_t nops, const vm_opcode_t *ops, uint8_t *jumps, vm_gc_t *gc, void **ptrs)
{
  size_t index = 0;
  size_t cfunc = 0;
  size_t cend = 0;

  size_t nregs = 0;

  size_t alloc = 1 << 8;
  size_t *buf = vm_malloc(sizeof(size_t) * alloc);
  size_t *froms = vm_alloc0(sizeof(size_t) * alloc);
  size_t *locs = vm_alloc0(sizeof(size_t) * nops);
  size_t *ret = buf;

  uint8_t named[1 << 12] = {0};
  size_t regs[1 << 12];

  while (index < nops)
  {
    while (index < cend && (jumps[index] & VM_JUMP_REACH) == 0 && ops[index] != VM_OPCODE_FUNC)
    {
      index++;
      while ((jumps[index] & VM_JUMP_INSTR) == 0)
      {
        index++;
      }
    }
    VM_INT_DUMP();
    locs[index] = buf - ret;
    if (buf - ret + 64 > alloc)
    {
      size_t length = buf - ret;
      size_t old_alloc = alloc;
      alloc = (length + 64) * 4;
      ret = vm_realloc(ret, sizeof(size_t) * alloc);
      buf = ret + length;
      froms = vm_realloc(froms, sizeof(size_t) * alloc);
      for (size_t i = old_alloc; i < alloc; i++) {
        froms[i] = 0;
      }
    }
    switch (ops[index++])
    {
    case VM_OPCODE_EXIT:
    {
      *buf++ = (size_t) ptrs[VM_INT_OP_EXIT];
      break;
    }
    case VM_OPCODE_REG:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t in = ops[index++];
      if (named[in])
      {
        named[out] = 1;
        regs[out] = regs[in];
      }
      else
      {
        named[out] = 0;
        *buf++ = (size_t) ptrs[VM_INT_OP_MOV];
        *buf++ = out;
        *buf++ = in;
      }
      break;
    }
    case VM_OPCODE_INT:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t in = ops[index++];
      named[out] = 1;
      regs[out] = in;
      break;
    }
    case VM_OPCODE_JUMP:
    {
      vm_opcode_t loc = ops[index++];
      *buf++ = (size_t) ptrs[VM_INT_OP_JUMP];
      froms[buf - ret] = loc;
      *buf++ = 255;
      break;
    }
    case VM_OPCODE_FUNC:
    {
      for (size_t i = 0; i < nregs; i++)
      {
        named[i] = 0;
      }
      cend = ops[index++];
      vm_opcode_t nargs = ops[index++];
      nregs = ops[index++];
      *buf++ = (size_t) ptrs[VM_INT_OP_JUMP];
      froms[buf - ret] = cend;
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
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out] = regs[lhs] + regs[rhs];
      }
      else
      {
        if (named[lhs])
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_ADDI];
          *buf++ = out;
          *buf++ = rhs;
          *buf++ = regs[lhs];
        }
        else if (named[rhs])
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_ADDI];
          *buf++ = out;
          *buf++ = lhs;
          *buf++ = regs[rhs];
        }
        else
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_ADD];
          *buf++ = out;
          *buf++ = lhs;
          *buf++ = rhs;
        }
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_SUB:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out] = regs[lhs] - regs[rhs];
      }
      else
      {
        if (named[lhs])
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_ISUB];
          *buf++ = out;
          *buf++ = regs[lhs];
          *buf++ = rhs;
        }
        else if (named[rhs])
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_SUBI];
          *buf++ = out;
          *buf++ = lhs;
          *buf++ = regs[rhs];
        }
        else
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_SUB];
          *buf++ = out;
          *buf++ = lhs;
          *buf++ = rhs;
        }
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_PAIR:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        size_t *res = vm_gc_alloc_root(gc);
        res[0] = regs[lhs];
        res[1] = regs[rhs];
        named[out] = 1;
        regs[out] = (size_t) res;
      } else {
        if (named[lhs])
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_IPAIR];
          *buf++ = out;
          *buf++ = regs[lhs];
          *buf++ = rhs;
        }
        else if (named[rhs])
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_PAIRI];
          *buf++ = out;
          *buf++ = lhs;
          *buf++ = regs[rhs];
        }
        else
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_PAIR];
          *buf++ = out;
          *buf++ = lhs;
          *buf++ = rhs;
        }
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_FIRST:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      if (named[lhs]) {
        size_t *pair = (size_t*) regs[lhs];
        named[out] = 1;
        regs[out] = pair[0];
      } else {
        *buf++ = (size_t) ptrs[VM_INT_OP_FIRST];
        *buf++ = out;
        *buf++ = lhs;
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_SECOND:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      if (named[lhs]) {
        size_t *pair = (size_t*) regs[lhs];
        named[out] = 1;
        regs[out] = pair[1];
      } else {
        *buf++ = (size_t) ptrs[VM_INT_OP_SECOND];
        *buf++ = out;
        *buf++ = lhs;
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_MUL:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out] = regs[lhs] * regs[rhs];
      }
      else
      {
        if (named[lhs])
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_MULI];
          *buf++ = out;
          *buf++ = rhs;
          *buf++ = regs[lhs];
        }
        else if (named[rhs])
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_MULI];
          *buf++ = out;
          *buf++ = lhs;
          *buf++ = regs[rhs];
        }
        else
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_MUL];
          *buf++ = out;
          *buf++ = lhs;
          *buf++ = rhs;
        }
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_DIV:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out] = regs[lhs] - regs[rhs];
      }
      else
      {
        if (named[lhs])
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_IDIV];
          *buf++ = out;
          *buf++ = regs[lhs];
          *buf++ = rhs;
        }
        else if (named[rhs])
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_DIVI];
          *buf++ = out;
          *buf++ = lhs;
          *buf++ = regs[rhs];
        }
        else
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_DIV];
          *buf++ = out;
          *buf++ = lhs;
          *buf++ = rhs;
        }
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_MOD:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out] = regs[lhs] - regs[rhs];
      }
      else
      {
        if (named[lhs])
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_IMOD];
          *buf++ = out;
          *buf++ = regs[lhs];
          *buf++ = rhs;
        }
        else if (named[rhs])
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_MODI];
          *buf++ = out;
          *buf++ = lhs;
          *buf++ = regs[rhs];
        }
        else
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_MOD];
          *buf++ = out;
          *buf++ = lhs;
          *buf++ = rhs;
        }
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_TCALL:
    {
      vm_opcode_t nargs = ops[index++];
      for (int i = 0; i < nargs; i++)
      {
        vm_opcode_t reg = ops[index + i];
        if (named[reg])
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_MOVI];
          *buf++ = reg;
          *buf++ = regs[reg];
          named[reg] = 0;
        }
      }
      *buf++ = (size_t) ptrs[VM_INT_OP_TCALL0 + nargs];
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
      for (int i = 0; i < nargs; i++)
      {
        vm_opcode_t reg = ops[index + i];
        if (named[reg])
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_MOVI];
          *buf++ = reg;
          *buf++ = regs[reg];
          named[reg] = 0;
        }
      }
      *buf++ = (size_t) ptrs[VM_INT_OP_CALL0 + nargs];
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
      for (int i = 0; i < nargs; i++)
      {
        vm_opcode_t reg = ops[index + i];
        if (named[reg])
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_MOVI];
          *buf++ = reg;
          *buf++ = regs[reg];
          named[reg] = 0;
        }
      }
      *buf++ = (size_t) ptrs[VM_INT_OP_DCALL0 + nargs];
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
      if (named[out])
      {
        *buf++ = (size_t) ptrs[VM_INT_OP_RETI];
        *buf++ = regs[out];
      }
      else
      {
        *buf++ = (size_t) ptrs[VM_INT_OP_RET];
        *buf++ = out;
      }
      break;
    }
    case VM_OPCODE_PUTCHAR:
    {
      vm_opcode_t in = ops[index++];
      if (named[in])
      {
        *buf++ = (size_t) ptrs[VM_INT_OP_MOVI];
        *buf++ = in;
        *buf++ = regs[in];
      }
      *buf++ = (size_t) ptrs[VM_INT_OP_PUTC];
      *buf++ = in;
      break;
    }
    case VM_OPCODE_BB:
    {
      vm_opcode_t val = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      if (named[val])
      {
        if (regs[val])
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_JUMP];
          froms[buf - ret] = jtrue;
          *buf++ = 255;
        }
        else
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_JUMP];
          froms[buf - ret] = jfalse;
          *buf++ = 255;
        }
      }
      else
      {
        *buf++ = (size_t) ptrs[VM_INT_OP_BB];
        *buf++ = val;
        froms[buf - ret] = jfalse;
        *buf++ = 255;
        froms[buf - ret] = jtrue;
        *buf++ = 255;
      }
      break;
    }
    case VM_OPCODE_BEQ:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      if (named[lhs] && named[rhs])
      {
        if (regs[lhs] == regs[rhs])
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_JUMP];
          froms[buf - ret] = jtrue;
          *buf++ = 255;
        }
        else
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_JUMP];
          froms[buf - ret] = jfalse;
          *buf++ = 255;
        }
      }
      else if (named[rhs])
      {
        *buf++ = (size_t) ptrs[VM_INT_OP_BEQI];
        *buf++ = lhs;
        *buf++ = regs[rhs];
        froms[buf - ret] = jfalse;
        *buf++ = 255;
        froms[buf - ret] = jtrue;
        *buf++ = 255;
      }
      else if (named[lhs])
      {
        *buf++ = (size_t) ptrs[VM_INT_OP_BEQI];
        *buf++ = rhs;
        *buf++ = regs[lhs];
        froms[buf - ret] = jfalse;
        *buf++ = 255;
        froms[buf - ret] = jtrue;
        *buf++ = 255;
      }
      else
      {
        *buf++ = (size_t) ptrs[VM_INT_OP_BEQ];
        *buf++ = lhs;
        *buf++ = rhs;
        froms[buf - ret] = jfalse;
        *buf++ = 255;
        froms[buf - ret] = jtrue;
        *buf++ = 255;
      }
      break;
    }
    case VM_OPCODE_BLT:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      if (named[lhs] && named[rhs])
      {
        if (regs[lhs] < regs[rhs])
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_JUMP];
          froms[buf - ret] = jtrue;
          *buf++ = 255;
        }
        else
        {
          *buf++ = (size_t) ptrs[VM_INT_OP_JUMP];
          froms[buf - ret] = jfalse;
          *buf++ = 255;
        }
      }
      else if (named[rhs])
      {
        *buf++ = (size_t) ptrs[VM_INT_OP_BLTI];
        *buf++ = lhs;
        *buf++ = regs[rhs];
        froms[buf - ret] = jfalse;
        *buf++ = 255;
        froms[buf - ret] = jtrue;
        *buf++ = 255;
      }
      else if (named[lhs])
      {
        *buf++ = (size_t) ptrs[VM_INT_OP_BILT];
        *buf++ = regs[lhs];
        *buf++ = rhs;
        froms[buf - ret] = jfalse;
        *buf++ = 255;
        froms[buf - ret] = jtrue;
        *buf++ = 255;
      }
      else
      {
        *buf++ = (size_t) ptrs[VM_INT_OP_BLT];
        *buf++ = lhs;
        *buf++ = rhs;
        froms[buf - ret] = jfalse;
        *buf++ = 255;
        froms[buf - ret] = jtrue;
        *buf++ = 255;
      }
      break;
    }
    case VM_OPCODE_INTF:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t func = ops[index++];
      *buf++ = (size_t) ptrs[VM_INT_OP_MOVI];
      *buf++ = out;
      froms[buf - ret] = func;
      *buf++ = 255;
      break;
    }
    default:
    {
      printf("err %zu\n", (size_t)ops[index - 1]);
      vm_free(froms);
      vm_free(locs);
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
  vm_free(froms);
  vm_free(locs);
  return ret;
}

#define vm_int_read() ops[index++]
#if 0
#define vm_int_jump_next()                         \
  printf("%zu: %zu\n", index, (size_t)ops[index]); \
  goto *(void*)vm_int_read()
#else
#define vm_int_jump_next() goto *(void*)vm_int_read()
#endif

int vm_int_run(size_t nops, const vm_opcode_t *iops, vm_gc_t *gc)
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
      [VM_INT_OP_BEQI] = &&exec_beqi,
      [VM_INT_OP_BLTI] = &&exec_blti,
      [VM_INT_OP_BILT] = &&exec_bilt,
      [VM_INT_OP_ADDI] = &&exec_addi,
      [VM_INT_OP_SUBI] = &&exec_subi,
      [VM_INT_OP_MULI] = &&exec_muli,
      [VM_INT_OP_DIVI] = &&exec_divi,
      [VM_INT_OP_MODI] = &&exec_modi,
      [VM_INT_OP_ISUB] = &&exec_isub,
      [VM_INT_OP_IDIV] = &&exec_idiv,
      [VM_INT_OP_IMOD] = &&exec_imod,
      [VM_INT_OP_RETI] = &&exec_reti,
      [VM_INT_OP_PAIR] = &&exec_pair,
      [VM_INT_OP_PAIRI] = &&exec_pairi,
      [VM_INT_OP_IPAIR] = &&exec_ipair,
      [VM_INT_OP_FIRST] = &&exec_first,
      [VM_INT_OP_SECOND] = &&exec_second,
  };
  uint8_t *jumps = vm_jump_all(nops, iops);
  if (jumps == NULL)
  {
    return 1;
  }
  size_t *ops = vm_int_comp(nops, iops, jumps, gc, &ptrs[0]);
  vm_free(jumps);
  if (ops == NULL)
  {
    return 1;
  }
  size_t nframes = 1000;
  size_t nregs = nframes * 8;
  size_t *stack = vm_malloc(sizeof(size_t) * nframes);
  size_t *regs = vm_malloc(sizeof(size_t) * nregs);
  size_t *stack_base = stack;
  size_t *regs_base = regs;
  size_t index = 0;
  vm_int_jump_next();
exec_exit:
{
  vm_free(stack_base);
  vm_free(regs_base);
  vm_free(ops);
  return 0;
}
exec_putc:
{
  size_t reg = vm_int_read();
  printf("%c", (int)regs[reg]);
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
  index = ops[index + (regs[in]!=0)];
  vm_int_jump_next();
}
exec_beq:
{
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  index = ops[index+(regs[lhs] == regs[rhs])];
  vm_int_jump_next();
}
exec_blt:
{
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  index = ops[index+(regs[lhs] < regs[rhs])];
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
  regs[3] = r3;
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
  regs[3] = r3;
  regs[4] = r4;
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
  regs[3] = r3;
  regs[4] = r4;
  regs[5] = r5;
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
  regs[3] = r3;
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
  regs[3] = r3;
  regs[4] = r4;
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
  regs[3] = r3;
  regs[4] = r4;
  regs[5] = r5;
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
exec_beqi:
{
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  index = ops[index+(regs[lhs] == rhs)];
  vm_int_jump_next();
}
exec_blti:
{
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  index = ops[index+(regs[lhs] < rhs)];
  vm_int_jump_next();
}
exec_bilt:
{
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  index = ops[index+(lhs < regs[rhs])];
  vm_int_jump_next();
}
exec_addi:
{
  size_t out = vm_int_read();
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  regs[out] = regs[lhs] + rhs;
  vm_int_jump_next();
}
exec_subi:
{
  size_t out = vm_int_read();
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  regs[out] = regs[lhs] - rhs;
  vm_int_jump_next();
}
exec_muli:
{
  size_t out = vm_int_read();
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  regs[out] = regs[lhs] * rhs;
  vm_int_jump_next();
}
exec_divi:
{
  size_t out = vm_int_read();
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  regs[out] = regs[lhs] / rhs;
  vm_int_jump_next();
}
exec_modi:
{
  size_t out = vm_int_read();
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  regs[out] = regs[lhs] % rhs;
  vm_int_jump_next();
}
exec_isub:
{
  size_t out = vm_int_read();
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  regs[out] = lhs - regs[rhs];
  vm_int_jump_next();
}
exec_idiv:
{
  size_t out = vm_int_read();
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  regs[out] = lhs / regs[rhs];
  vm_int_jump_next();
}
exec_imod:
{
  size_t out = vm_int_read();
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  regs[out] = lhs % regs[rhs];
  vm_int_jump_next();
}
exec_reti:
{
  size_t inval = vm_int_read();
  index = *--stack;
  regs -= ops[index - 1];
  regs[vm_int_read()] = inval;
  vm_int_jump_next();
}
exec_pair:
{
  vm_gc_collect(gc, nregs, regs_base);
  size_t out = vm_int_read();
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  size_t *res = vm_gc_alloc(gc);
  res[0] = regs[lhs];
  res[1] = regs[rhs];
  regs[out] = (size_t) res;
  vm_int_jump_next();
}
exec_pairi:
{
  vm_gc_collect(gc, nregs, regs_base);
  size_t out = vm_int_read();
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  size_t *res = vm_gc_alloc(gc);
  res[0] = regs[lhs];
  res[1] = rhs;
  regs[out] = (size_t) res;
  vm_int_jump_next();
}
exec_ipair:
{
  vm_gc_collect(gc, nregs, regs_base);
  size_t out = vm_int_read();
  size_t lhs = vm_int_read();
  size_t rhs = vm_int_read();
  size_t *res = vm_gc_alloc(gc);
  res[0] = lhs;
  res[1] = regs[rhs];
  regs[out] = (size_t) res;
  vm_int_jump_next();
}
exec_first:
{
  size_t out = vm_int_read();
  size_t pair = vm_int_read();
  regs[out] = ((size_t*)regs[pair])[0];
  vm_int_jump_next();
}
exec_second:
{
  size_t out = vm_int_read();
  size_t pair = vm_int_read();
  regs[out] = ((size_t*)regs[pair])[1];
  vm_int_jump_next();
}
fail:
  printf("not implemented: %zu@%zu\n", (size_t)ops[index - 1], index);
  return 1;
}

int vm_run_arch_int(size_t nops, const vm_opcode_t *ops)
{
  vm_gc_t gc;
  vm_gc_init(&gc);
  int res = vm_int_run(nops, ops, &gc);
  vm_gc_deinit(&gc);
  return res;
}
