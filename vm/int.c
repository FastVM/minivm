
#include "gc.h"
#include "jump.h"
#include "lib.h"
#include "opcode.h"
#include "reguse.h"

// typedef uint32_t vm_int_arg_t;
typedef size_t vm_int_arg_t;

enum vm_int_op_t
{
  VM_INT_OP_EXIT,
  VM_INT_OP_PUTC,
  VM_INT_OP_MOV,
  VM_INT_OP_MOVC,
  VM_INT_OP_UADD,
  VM_INT_OP_UADDC,
  VM_INT_OP_USUB,
  VM_INT_OP_USUBC,
  VM_INT_OP_UCSUB,
  VM_INT_OP_UMUL,
  VM_INT_OP_UMULC,
  VM_INT_OP_UDIV,
  VM_INT_OP_UDIVC,
  VM_INT_OP_UCDIV,
  VM_INT_OP_UMOD,
  VM_INT_OP_UMODC,
  VM_INT_OP_UCMOD,
  VM_INT_OP_FADD,
  VM_INT_OP_FADDC,
  VM_INT_OP_FSUB,
  VM_INT_OP_FSUBC,
  VM_INT_OP_FCSUB,
  VM_INT_OP_FMUL,
  VM_INT_OP_FMULC,
  VM_INT_OP_FDIV,
  VM_INT_OP_FDIVC,
  VM_INT_OP_FCDIV,
  VM_INT_OP_FMOD,
  VM_INT_OP_FMODC,
  VM_INT_OP_FCMOD,
  VM_INT_OP_JUMP,
  VM_INT_OP_UBB,
  VM_INT_OP_UBEQ,
  VM_INT_OP_UBEQC,
  VM_INT_OP_UBLT,
  VM_INT_OP_UBLTC,
  VM_INT_OP_UCBLT,
  VM_INT_OP_FBB,
  VM_INT_OP_FBEQ,
  VM_INT_OP_FBEQC,
  VM_INT_OP_FBLT,
  VM_INT_OP_FBLTC,
  VM_INT_OP_FCBLT,
  VM_INT_OP_RET,
  VM_INT_OP_RETC,
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
  VM_INT_OP_PAIR,
  VM_INT_OP_PAIRC,
  VM_INT_OP_CPAIR,
  VM_INT_OP_FIRST,
  VM_INT_OP_SECOND,
  VM_INT_OP_FTOU,
  VM_INT_OP_UTOF,
};

#define vm_int_buf_put(type_, val_) ({*(type_*)buf = (type_) val_; buf+=sizeof(type_); })
#define vm_int_buf_put_op(op_) (vm_int_buf_put(void *, ptrs[op_]))

uint8_t *vm_int_comp(size_t nops, const vm_opcode_t *ops, uint8_t *jumps, vm_gc_t *gc, void **ptrs)
{
  vm_int_arg_t index = 0;
  vm_int_arg_t cfunc = 0;
  vm_int_arg_t cend = 0;

  vm_int_arg_t nregs = 0;

  vm_int_arg_t alloc = 1 << 8;
  uint8_t *buf = vm_malloc(sizeof(uint8_t) * alloc);
  vm_int_arg_t *froms = vm_alloc0(sizeof(vm_int_arg_t) * alloc);
  vm_int_arg_t *locs = vm_alloc0(sizeof(vm_int_arg_t) * nops);
  uint8_t *ret = buf;

  uint8_t named[1 << 12] = {0};
  vm_value_t regs[1 << 12];

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
    if (index >= cend)
    {
      nregs = 16;
    }
    if ((jumps[index] & VM_JUMP_IN) || (jumps[index] & VM_JUMP_OUT))
    {
      for (size_t i = 0; i < nregs; i++)
      {
        if (!named[i])
        {
          continue;
        }
        if (VM_REG_IS_USED(index, i))
        {
          vm_int_buf_put_op(VM_INT_OP_MOVC);
          vm_int_buf_put(vm_int_arg_t, i);
          vm_int_buf_put(vm_int_arg_t, regs[i].u);
        }
        named[i] = 0;
      }
    }
    locs[index] = buf - ret;
    if (buf - ret + 256 > alloc)
    {
      vm_int_arg_t length = buf - ret;
      vm_int_arg_t old_alloc = alloc;
      alloc = (length + 256) * 4;
      ret = vm_realloc(ret, sizeof(uint8_t) * alloc);
      buf = ret + length;
      froms = vm_realloc(froms, sizeof(vm_int_arg_t) * alloc);
      for (size_t i = old_alloc; i < alloc; i++)
      {
        froms[i] = 0;
      }
    }
    switch (ops[index++])
    {
    case VM_OPCODE_EXIT:
    {
      vm_int_buf_put_op(VM_INT_OP_EXIT);
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
        vm_int_buf_put_op(VM_INT_OP_MOV);
        vm_int_buf_put(vm_int_arg_t, out);
        vm_int_buf_put(vm_int_arg_t, in);
      }
      break;
    }
    case VM_OPCODE_FTOU:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t in = ops[index++];
      if (named[in])
      {
        named[out] = 1;
        regs[out].u = regs[in].f;
      }
      else
      {
        named[out] = 0;
        vm_int_buf_put_op(VM_INT_OP_FTOU);
        vm_int_buf_put(vm_int_arg_t, out);
        vm_int_buf_put(vm_int_arg_t, in);
      }
      break;
    }
    case VM_OPCODE_UTOF:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t in = ops[index++];
      if (named[in])
      {
        named[out] = 1;
        regs[out].f = regs[in].u;
      }
      else
      {
        named[out] = 0;
        vm_int_buf_put_op(VM_INT_OP_UTOF);
        vm_int_buf_put(vm_int_arg_t, out);
        vm_int_buf_put(vm_int_arg_t, in);
      }
      break;
    }
    case VM_OPCODE_INT:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t in = ops[index++];
      named[out] = 1;
      regs[out].u = in;
      break;
    }
    case VM_OPCODE_FINT:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t in = ops[index++];
      named[out] = 1;
      regs[out].f = in;
      break;
    }
    case VM_OPCODE_FRAC:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      named[out] = 1;
      regs[out].f = lhs / rhs;
      break;
    }
    case VM_OPCODE_JUMP:
    {
      vm_opcode_t loc = ops[index++];
      vm_int_buf_put_op(VM_INT_OP_JUMP);
      froms[buf - ret] = loc;
      vm_int_buf_put(vm_int_arg_t, 255);
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
      vm_int_buf_put_op(VM_INT_OP_JUMP);
      froms[buf - ret] = cend;
      vm_int_buf_put(vm_int_arg_t, 255);
      vm_int_buf_put(vm_int_arg_t, nregs);
      cfunc = index;
      break;
    }
    case VM_OPCODE_UADD:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out].u = regs[lhs].u + regs[rhs].u;
      }
      else
      {
        if (named[lhs])
        {
          vm_int_buf_put_op(VM_INT_OP_UADDC);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, rhs);
          vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_UADDC);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_UADD);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, rhs);
        }
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_USUB:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out].u = regs[lhs].u - regs[rhs].u;
      }
      else
      {
        if (named[lhs])
        {
          vm_int_buf_put_op(VM_INT_OP_UCSUB);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
          vm_int_buf_put(vm_int_arg_t, rhs);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_USUBC);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_USUB);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, rhs);
        }
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_UMUL:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out].u = regs[lhs].u * regs[rhs].u;
      }
      else
      {
        if (named[lhs])
        {
          vm_int_buf_put_op(VM_INT_OP_UMULC);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, rhs);
          vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_UMULC);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_UMUL);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, rhs);
        }
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_UDIV:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out].u = regs[lhs].u / regs[rhs].u;
      }
      else
      {
        if (named[lhs])
        {
          vm_int_buf_put_op(VM_INT_OP_UCDIV);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
          vm_int_buf_put(vm_int_arg_t, rhs);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_UDIVC);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_UDIV);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, rhs);
        }
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_UMOD:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out].u = regs[lhs].u % regs[rhs].u;
      }
      else
      {
        if (named[lhs])
        {
          vm_int_buf_put_op(VM_INT_OP_UCMOD);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
          vm_int_buf_put(vm_int_arg_t, rhs);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_UMODC);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_UMOD);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, rhs);
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
          vm_int_buf_put_op(VM_INT_OP_MOVC);
          vm_int_buf_put(vm_int_arg_t, reg);
          vm_int_buf_put(vm_int_arg_t, regs[reg].u);
          named[reg] = 0;
        }
      }
      vm_int_buf_put_op(VM_INT_OP_TCALL0 + nargs);
      froms[buf - ret] = cfunc;
      vm_int_buf_put(vm_int_arg_t, 255);
      for (int i = 0; i < nargs; i++)
      {
        vm_int_buf_put(vm_int_arg_t, ops[index++]);
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
          vm_int_buf_put_op(VM_INT_OP_MOVC);
          vm_int_buf_put(vm_int_arg_t, reg);
          vm_int_buf_put(vm_int_arg_t, regs[reg].u);
          named[reg] = 0;
        }
      }
      vm_int_buf_put_op(VM_INT_OP_CALL0 + nargs);
      froms[buf - ret] = func;
      vm_int_buf_put(vm_int_arg_t, 255);
      for (int i = 0; i < nargs; i++)
      {
        vm_int_buf_put(vm_int_arg_t, ops[index++]);
      }
      vm_int_buf_put(vm_int_arg_t, nregs);
      vm_int_buf_put(vm_int_arg_t, rreg);
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
          vm_int_buf_put_op(VM_INT_OP_MOVC);
          vm_int_buf_put(vm_int_arg_t, reg);
          vm_int_buf_put(vm_int_arg_t, regs[reg].u);
          named[reg] = 0;
        }
      }
      vm_int_buf_put_op(VM_INT_OP_DCALL0 + nargs);
      vm_int_buf_put(vm_int_arg_t, func);
      for (int i = 0; i < nargs; i++)
      {
        vm_int_buf_put(vm_int_arg_t, ops[index++]);
      }
      vm_int_buf_put(vm_int_arg_t, nregs);
      vm_int_buf_put(vm_int_arg_t, rreg);
      break;
    }
    case VM_OPCODE_RET:
    {
      vm_opcode_t out = ops[index++];
      if (named[out])
      {
        vm_int_buf_put_op(VM_INT_OP_RETC);
        vm_int_buf_put(vm_int_arg_t, regs[out].u);
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_RET);
        vm_int_buf_put(vm_int_arg_t, out);
      }
      break;
    }
    case VM_OPCODE_PUTCHAR:
    {
      vm_opcode_t in = ops[index++];
      if (named[in])
      {
        vm_int_buf_put_op(VM_INT_OP_MOVC);
        vm_int_buf_put(vm_int_arg_t, in);
        vm_int_buf_put(vm_int_arg_t, regs[in].u);
      }
      vm_int_buf_put_op(VM_INT_OP_PUTC);
      vm_int_buf_put(vm_int_arg_t, in);
      break;
    }
    case VM_OPCODE_UBB:
    {
      vm_opcode_t val = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      if (named[val])
      {
        if (regs[val].u)
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jtrue;
          vm_int_buf_put(vm_int_arg_t, 255);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jfalse;
          vm_int_buf_put(vm_int_arg_t, 255);
        }
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_UBB);
        vm_int_buf_put(vm_int_arg_t, val);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, 255);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, 255);
      }
      break;
    }
    case VM_OPCODE_UBEQ:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      if (named[lhs] && named[rhs])
      {
        if (regs[lhs].u == regs[rhs].u)
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jtrue;
          vm_int_buf_put(vm_int_arg_t, 255);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jfalse;
          vm_int_buf_put(vm_int_arg_t, 255);
        }
      }
      else if (named[rhs])
      {
        vm_int_buf_put_op(VM_INT_OP_UBEQC);
        vm_int_buf_put(vm_int_arg_t, lhs);
        vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, 255);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, 255);
      }
      else if (named[lhs])
      {
        vm_int_buf_put_op(VM_INT_OP_UBEQC);
        vm_int_buf_put(vm_int_arg_t, rhs);
        vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, 255);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, 255);
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_UBEQ);
        vm_int_buf_put(vm_int_arg_t, lhs);
        vm_int_buf_put(vm_int_arg_t, rhs);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, 255);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, 255);
      }
      break;
    }
    case VM_OPCODE_UBLT:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      if (named[lhs] && named[rhs])
      {
        if (regs[lhs].u < regs[rhs].u)
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jtrue;
          vm_int_buf_put(vm_int_arg_t, 255);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jfalse;
          vm_int_buf_put(vm_int_arg_t, 255);
        }
      }
      else if (named[rhs])
      {
        vm_int_buf_put_op(VM_INT_OP_UBLTC);
        vm_int_buf_put(vm_int_arg_t, lhs);
        vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, 255);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, 255);
      }
      else if (named[lhs])
      {
        vm_int_buf_put_op(VM_INT_OP_UCBLT);
        vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
        vm_int_buf_put(vm_int_arg_t, rhs);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, 255);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, 255);
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_UBLT);
        vm_int_buf_put(vm_int_arg_t, lhs);
        vm_int_buf_put(vm_int_arg_t, rhs);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, 255);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, 255);
      }
      break;
    }
    case VM_OPCODE_INTF:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t func = ops[index++];
      vm_int_buf_put_op(VM_INT_OP_MOVC);
      vm_int_buf_put(vm_int_arg_t, out);
      froms[buf - ret] = func;
      vm_int_buf_put(vm_int_arg_t, 255);
      break;
    }
    case VM_OPCODE_PAIR:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        vm_int_arg_t *res = vm_gc_alloc_root(gc);
        res[0] = regs[lhs].u;
        res[1] = regs[rhs].u;
        named[out] = 1;
        regs[out].u = (size_t)res;
      }
      else
      {
        if (named[lhs])
        {
          vm_int_buf_put_op(VM_INT_OP_CPAIR);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
          vm_int_buf_put(vm_int_arg_t, rhs);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_PAIRC);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_PAIR);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, rhs);
        }
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_FIRST:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      if (named[lhs])
      {
        vm_value_t *pair = regs[lhs].p;
        named[out] = 1;
        regs[out] = pair[0];
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_FIRST);
        vm_int_buf_put(vm_int_arg_t, out);
        vm_int_buf_put(vm_int_arg_t, lhs);
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_SECOND:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      if (named[lhs])
      {
        vm_value_t *pair = regs[lhs].p;
        named[out] = 1;
        regs[out] = pair[1];
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_SECOND);
        vm_int_buf_put(vm_int_arg_t, out);
        vm_int_buf_put(vm_int_arg_t, lhs);
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_FADD:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out].f = regs[lhs].f + regs[rhs].f;
      }
      else
      {
        if (named[lhs])
        {
          vm_int_buf_put_op(VM_INT_OP_FADDC);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, rhs);
          vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_FADDC);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_FADD);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, rhs);
        }
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_FSUB:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out].f = regs[lhs].f - regs[rhs].f;
      }
      else
      {
        if (named[lhs])
        {
          vm_int_buf_put_op(VM_INT_OP_FCSUB);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
          vm_int_buf_put(vm_int_arg_t, rhs);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_FSUBC);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_FSUB);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, rhs);
        }
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_FMUL:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out].f = regs[lhs].f * regs[rhs].f;
      }
      else
      {
        if (named[lhs])
        {
          vm_int_buf_put_op(VM_INT_OP_FMULC);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, rhs);
          vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_FMULC);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_FMUL);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, rhs);
        }
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_FDIV:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out].f = regs[lhs].f / regs[rhs].f;
      }
      else
      {
        if (named[lhs])
        {
          vm_int_buf_put_op(VM_INT_OP_FCDIV);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
          vm_int_buf_put(vm_int_arg_t, rhs);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_FDIVC);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_FDIV);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, rhs);
        }
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_FMOD:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out].f = fmod(regs[lhs].f, regs[rhs].f);
      }
      else
      {
        if (named[lhs])
        {
          vm_int_buf_put_op(VM_INT_OP_FCMOD);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
          vm_int_buf_put(vm_int_arg_t, rhs);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_FMODC);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_FMOD);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, lhs);
          vm_int_buf_put(vm_int_arg_t, rhs);
        }
        named[out] = 0;
      }
      break;
    }

    case VM_OPCODE_FBB:
    {
      vm_opcode_t val = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      if (named[val])
      {
        if (regs[val].f)
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jtrue;
          vm_int_buf_put(vm_int_arg_t, 255);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jfalse;
          vm_int_buf_put(vm_int_arg_t, 255);
        }
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_FBB);
        vm_int_buf_put(vm_int_arg_t, val);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, 255);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, 255);
      }
      break;
    }
    case VM_OPCODE_FBEQ:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      if (named[lhs] && named[rhs])
      {
        if (regs[lhs].f == regs[rhs].f)
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jtrue;
          vm_int_buf_put(vm_int_arg_t, 255);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jfalse;
          vm_int_buf_put(vm_int_arg_t, 255);
        }
      }
      else if (named[rhs])
      {
        vm_int_buf_put_op(VM_INT_OP_FBEQC);
        vm_int_buf_put(vm_int_arg_t, lhs);
        vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, 255);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, 255);
      }
      else if (named[lhs])
      {
        vm_int_buf_put_op(VM_INT_OP_FBEQC);
        vm_int_buf_put(vm_int_arg_t, rhs);
        vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, 255);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, 255);
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_FBEQ);
        vm_int_buf_put(vm_int_arg_t, lhs);
        vm_int_buf_put(vm_int_arg_t, rhs);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, 255);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, 255);
      }
      break;
    }
    case VM_OPCODE_FBLT:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      if (named[lhs] && named[rhs])
      {
        if (regs[lhs].f < regs[rhs].f)
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jtrue;
          vm_int_buf_put(vm_int_arg_t, 255);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jfalse;
          vm_int_buf_put(vm_int_arg_t, 255);
        }
      }
      else if (named[rhs])
      {
        vm_int_buf_put_op(VM_INT_OP_FBLTC);
        vm_int_buf_put(vm_int_arg_t, lhs);
        vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, 255);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, 255);
      }
      else if (named[lhs])
      {
        vm_int_buf_put_op(VM_INT_OP_FCBLT);
        vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
        vm_int_buf_put(vm_int_arg_t, rhs);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, 255);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, 255);
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_FBLT);
        vm_int_buf_put(vm_int_arg_t, lhs);
        vm_int_buf_put(vm_int_arg_t, rhs);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, 255);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, 255);
      }
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
  vm_int_arg_t length = buf - ret;
  for (size_t i = 0; i < length; i++)
  {
    if (froms[i] != 0)
    {
      *(vm_int_arg_t *)&ret[i] = locs[froms[i]];
    }
  }
  vm_free(froms);
  vm_free(locs);
  return ret;
}

#define vm_int_read_at(type_, index_) (*(type_*)&ops[index_])
#define vm_int_read_type(type_) ({type_ ret=*(type_*)&ops[index]; index += sizeof(type_); ret; })
#define vm_int_read() ({vm_int_read_type(vm_int_arg_t);})
#define vm_int_jump_next() ({goto *vm_int_read_type(void *);})

int vm_int_run(size_t nops, const vm_opcode_t *iops, vm_gc_t *gc)
{
  static void *ptrs[] = {
      [VM_INT_OP_EXIT] = &&exec_exit,
      [VM_INT_OP_PUTC] = &&exec_putc,
      [VM_INT_OP_MOV] = &&exec_mov,
      [VM_INT_OP_MOVC] = &&exec_movc,
      [VM_INT_OP_UADD] = &&exec_uadd,
      [VM_INT_OP_UADDC] = &&exec_uaddc,
      [VM_INT_OP_USUB] = &&exec_usub,
      [VM_INT_OP_USUBC] = &&exec_usubc,
      [VM_INT_OP_UCSUB] = &&exec_ucsub,
      [VM_INT_OP_UMUL] = &&exec_umul,
      [VM_INT_OP_UMULC] = &&exec_umulc,
      [VM_INT_OP_UDIV] = &&exec_udiv,
      [VM_INT_OP_UDIVC] = &&exec_udivc,
      [VM_INT_OP_UCDIV] = &&exec_ucdiv,
      [VM_INT_OP_UMOD] = &&exec_umod,
      [VM_INT_OP_UMODC] = &&exec_umodc,
      [VM_INT_OP_UCMOD] = &&exec_ucmod,
      [VM_INT_OP_FADD] = &&exec_fadd,
      [VM_INT_OP_FADDC] = &&exec_faddc,
      [VM_INT_OP_FSUB] = &&exec_fsub,
      [VM_INT_OP_FSUBC] = &&exec_fsubc,
      [VM_INT_OP_FCSUB] = &&exec_fcsub,
      [VM_INT_OP_FMUL] = &&exec_fmul,
      [VM_INT_OP_FMULC] = &&exec_fmulc,
      [VM_INT_OP_FDIV] = &&exec_fdiv,
      [VM_INT_OP_FDIVC] = &&exec_fdivc,
      [VM_INT_OP_FCDIV] = &&exec_fcdiv,
      [VM_INT_OP_FMOD] = &&exec_fmod,
      [VM_INT_OP_FMODC] = &&exec_fmodc,
      [VM_INT_OP_FCMOD] = &&exec_fcmod,
      [VM_INT_OP_JUMP] = &&exec_jump,
      [VM_INT_OP_UBB] = &&exec_ubb,
      [VM_INT_OP_UBEQ] = &&exec_ubeq,
      [VM_INT_OP_UBEQC] = &&exec_ubeqc,
      [VM_INT_OP_UBLT] = &&exec_ublt,
      [VM_INT_OP_UBLTC] = &&exec_ubltc,
      [VM_INT_OP_UCBLT] = &&exec_ucblt,
      [VM_INT_OP_FBB] = &&exec_fbb,
      [VM_INT_OP_FBEQ] = &&exec_fbeq,
      [VM_INT_OP_FBEQC] = &&exec_fbeqc,
      [VM_INT_OP_FBLT] = &&exec_fblt,
      [VM_INT_OP_FBLTC] = &&exec_fbltc,
      [VM_INT_OP_FCBLT] = &&exec_fcblt,
      [VM_INT_OP_RET] = &&exec_ret,
      [VM_INT_OP_RETC] = &&exec_retc,
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
      [VM_INT_OP_PAIR] = &&exec_pair,
      [VM_INT_OP_PAIRC] = &&exec_pairc,
      [VM_INT_OP_CPAIR] = &&exec_cpair,
      [VM_INT_OP_FIRST] = &&exec_first,
      [VM_INT_OP_SECOND] = &&exec_second,
      [VM_INT_OP_FTOU] = &&exec_ftou,
      [VM_INT_OP_UTOF] = &&exec_utof,
  };
  uint8_t *jumps = vm_jump_all(nops, iops);
  if (jumps == NULL)
  {
    return 1;
  }
  uint8_t *ops = vm_int_comp(nops, iops, jumps, gc, &ptrs[0]);
  vm_free(jumps);
  if (ops == NULL)
  {
    return 1;
  }
  vm_int_arg_t nframes = 1000;
  vm_int_arg_t nregs = nframes * 8;
  vm_value_t *regs = vm_malloc(sizeof(vm_value_t) * nregs);
  vm_value_t *regs_base = regs;
  vm_int_arg_t *stack = vm_malloc(sizeof(vm_int_arg_t) * nframes);
  vm_int_arg_t *stack_base = stack;
  vm_int_arg_t index = 0;
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
  vm_int_arg_t reg = vm_int_read();
  printf("%c", (int)regs[reg].u);
  vm_int_jump_next();
}
exec_mov:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t in = vm_int_read();
  regs[out] = regs[in];
  vm_int_jump_next();
}
exec_movc:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t in = vm_int_read();
  regs[out].u = in;
  vm_int_jump_next();
}
exec_uadd:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].u = regs[lhs].u + regs[rhs].u;
  vm_int_jump_next();
}
exec_usub:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].u = regs[lhs].u - regs[rhs].u;
  vm_int_jump_next();
}
exec_umul:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].u = regs[lhs].u * regs[rhs].u;
  vm_int_jump_next();
}
exec_udiv:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].u = regs[lhs].u / regs[rhs].u;
  vm_int_jump_next();
}
exec_umod:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].u = regs[lhs].u % regs[rhs].u;
  vm_int_jump_next();
}
exec_ret:
{
  vm_int_arg_t inval = regs[vm_int_read()].u;
  index = *--stack;
  regs -= vm_int_read_at(vm_int_arg_t, index - sizeof(vm_int_arg_t));
  regs[vm_int_read()].u = inval;
  vm_int_jump_next();
}
exec_ubb:
{
  vm_int_arg_t in = vm_int_read();
  index = vm_int_read_at(vm_int_arg_t, index + (regs[in].u != 0) * sizeof(vm_int_arg_t));
  vm_int_jump_next();
}
exec_ubeq:
{
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  index = vm_int_read_at(vm_int_arg_t, index + (regs[lhs].u == regs[rhs].u) * sizeof(vm_int_arg_t));
  vm_int_jump_next();
}
exec_ublt:
{
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  index = vm_int_read_at(vm_int_arg_t, index + (regs[lhs].u < regs[rhs].u) * sizeof(vm_int_arg_t));
  vm_int_jump_next();
}
exec_call0:
{
  vm_int_arg_t func = vm_int_read();
  vm_int_arg_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  index = func;
  vm_int_jump_next();
}
exec_call1:
{
  vm_int_arg_t func = vm_int_read();
  vm_value_t r1 = regs[vm_int_read()];
  vm_int_arg_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  index = func;
  vm_int_jump_next();
}
exec_call2:
{
  vm_int_arg_t func = vm_int_read();
  vm_value_t r1 = regs[vm_int_read()];
  vm_value_t r2 = regs[vm_int_read()];
  vm_int_arg_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  regs[2] = r2;
  index = func;
  vm_int_jump_next();
}
exec_call3:
{
  vm_int_arg_t func = vm_int_read();
  vm_value_t r1 = regs[vm_int_read()];
  vm_value_t r2 = regs[vm_int_read()];
  vm_value_t r3 = regs[vm_int_read()];
  vm_int_arg_t nregs = vm_int_read();
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
  vm_int_arg_t func = vm_int_read();
  vm_value_t r1 = regs[vm_int_read()];
  vm_value_t r2 = regs[vm_int_read()];
  vm_value_t r3 = regs[vm_int_read()];
  vm_value_t r4 = regs[vm_int_read()];
  vm_int_arg_t nregs = vm_int_read();
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
  vm_int_arg_t func = vm_int_read();
  vm_value_t r1 = regs[vm_int_read()];
  vm_value_t r2 = regs[vm_int_read()];
  vm_value_t r3 = regs[vm_int_read()];
  vm_value_t r4 = regs[vm_int_read()];
  vm_value_t r5 = regs[vm_int_read()];
  vm_int_arg_t nregs = vm_int_read();
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
  vm_int_arg_t where = vm_int_read();
  vm_int_arg_t func = regs[where].u;
  vm_int_arg_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  index = func;
  vm_int_jump_next();
}
exec_dcall1:
{
  vm_int_arg_t func = regs[vm_int_read()].u;
  vm_value_t r1 = regs[vm_int_read()];
  vm_int_arg_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  index = func;
  vm_int_jump_next();
}
exec_dcall2:
{
  vm_int_arg_t func = regs[vm_int_read()].u;
  vm_value_t r1 = regs[vm_int_read()];
  vm_value_t r2 = regs[vm_int_read()];
  vm_int_arg_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  regs[2] = r2;
  index = func;
  vm_int_jump_next();
}
exec_dcall3:
{
  vm_int_arg_t func = regs[vm_int_read()].u;
  vm_value_t r1 = regs[vm_int_read()];
  vm_value_t r2 = regs[vm_int_read()];
  vm_value_t r3 = regs[vm_int_read()];
  vm_int_arg_t nregs = vm_int_read();
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
  vm_int_arg_t func = regs[vm_int_read()].u;
  vm_value_t r1 = regs[vm_int_read()];
  vm_value_t r2 = regs[vm_int_read()];
  vm_value_t r3 = regs[vm_int_read()];
  vm_value_t r4 = regs[vm_int_read()];
  vm_int_arg_t nregs = vm_int_read();
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
  vm_int_arg_t func = regs[vm_int_read()].u;
  vm_value_t r1 = regs[vm_int_read()];
  vm_value_t r2 = regs[vm_int_read()];
  vm_value_t r3 = regs[vm_int_read()];
  vm_value_t r4 = regs[vm_int_read()];
  vm_value_t r5 = regs[vm_int_read()];
  vm_int_arg_t nregs = vm_int_read();
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
  vm_int_arg_t func = vm_int_read();
  index = func;
  vm_int_jump_next();
}
exec_tcall1:
{
  vm_int_arg_t func = vm_int_read();
  vm_value_t r1 = regs[vm_int_read()];
  index = func;
  regs[1] = r1;
  vm_int_jump_next();
}
exec_tcall2:
{
  vm_int_arg_t func = vm_int_read();
  vm_value_t r1 = regs[vm_int_read()];
  vm_value_t r2 = regs[vm_int_read()];
  index = func;
  regs[1] = r1;
  regs[2] = r2;
  vm_int_jump_next();
}
exec_tcall3:
{
  vm_int_arg_t func = vm_int_read();
  vm_value_t r1 = regs[vm_int_read()];
  vm_value_t r2 = regs[vm_int_read()];
  vm_value_t r3 = regs[vm_int_read()];
  index = func;
  regs[1] = r1;
  regs[2] = r2;
  regs[3] = r3;
  vm_int_jump_next();
}
exec_tcall4:
{
  vm_int_arg_t func = vm_int_read();
  vm_value_t r1 = regs[vm_int_read()];
  vm_value_t r2 = regs[vm_int_read()];
  vm_value_t r3 = regs[vm_int_read()];
  vm_value_t r4 = regs[vm_int_read()];
  index = func;
  regs[1] = r1;
  regs[2] = r2;
  regs[3] = r3;
  regs[4] = r4;
  vm_int_jump_next();
}
exec_tcall5:
{
  vm_int_arg_t func = vm_int_read();
  vm_value_t r1 = regs[vm_int_read()];
  vm_value_t r2 = regs[vm_int_read()];
  vm_value_t r3 = regs[vm_int_read()];
  vm_value_t r4 = regs[vm_int_read()];
  vm_value_t r5 = regs[vm_int_read()];
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
  vm_int_arg_t where = vm_int_read();
  index = where;
  vm_int_jump_next();
}
exec_ubeqc:
{
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  index = vm_int_read_at(vm_int_arg_t, index + (regs[lhs].u == rhs) * sizeof(vm_int_arg_t));
  vm_int_jump_next();
}
exec_ubltc:
{
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  index = vm_int_read_at(vm_int_arg_t, index + (regs[lhs].u < rhs) * sizeof(vm_int_arg_t));
  vm_int_jump_next();
}
exec_ucblt:
{
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  index = vm_int_read_at(vm_int_arg_t, index + (lhs < regs[rhs].u) * sizeof(vm_int_arg_t));
  vm_int_jump_next();
}
exec_uaddc:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].u = regs[lhs].u + rhs;
  vm_int_jump_next();
}
exec_usubc:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].u = regs[lhs].u - rhs;
  vm_int_jump_next();
}
exec_umulc:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].u = regs[lhs].u * rhs;
  vm_int_jump_next();
}
exec_udivc:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].u = regs[lhs].u / rhs;
  vm_int_jump_next();
}
exec_umodc:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].u = regs[lhs].u % rhs;
  vm_int_jump_next();
}
exec_ucsub:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].u = lhs - regs[rhs].u;
  vm_int_jump_next();
}
exec_ucdiv:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].u = lhs / regs[rhs].u;
  vm_int_jump_next();
}
exec_ucmod:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].u = lhs % regs[rhs].u;
  vm_int_jump_next();
}
exec_retc:
{
  vm_int_arg_t inval = vm_int_read();
  index = *--stack;
  regs -= vm_int_read_at(vm_int_arg_t, index - sizeof(vm_int_arg_t));
  regs[vm_int_read()].u = inval;
  vm_int_jump_next();
}
exec_pair:
{
  vm_gc_collect(gc, nregs, regs_base);
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  vm_value_t *res = vm_gc_alloc(gc);
  res[0] = regs[lhs];
  res[1] = regs[rhs];
  regs[out].p = res;
  vm_int_jump_next();
}
exec_pairc:
{
  vm_gc_collect(gc, nregs, regs_base);
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  vm_value_t *res = vm_gc_alloc(gc);
  res[0] = regs[lhs];
  res[1].u = rhs;
  regs[out].p = res;
  vm_int_jump_next();
}
exec_cpair:
{
  vm_gc_collect(gc, nregs, regs_base);
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  vm_value_t *res = vm_gc_alloc(gc);
  res[0].u = lhs;
  res[1] = regs[rhs];
  regs[out].p = res;
  vm_int_jump_next();
}
exec_first:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t pair = vm_int_read();
  regs[out] = regs[pair].p[0];
  vm_int_jump_next();
}
exec_second:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t pair = vm_int_read();
  regs[out] = regs[pair].p[1];
  vm_int_jump_next();
}
exec_fadd:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].f = regs[lhs].f + regs[rhs].f;
  vm_int_jump_next();
}
exec_faddc:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].f = regs[lhs].f + *(double *)&rhs;
  vm_int_jump_next();
}
exec_fsub:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].f = regs[lhs].f - regs[rhs].f;
  vm_int_jump_next();
}
exec_fsubc:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].f = regs[lhs].f - *(double *)&rhs;
  vm_int_jump_next();
}
exec_fcsub:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].f = *(double *)&lhs - regs[rhs].f;
  vm_int_jump_next();
}
exec_fmul:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].f = regs[lhs].f * regs[rhs].f;
  vm_int_jump_next();
}
exec_fmulc:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].f = regs[lhs].f * *(double *)&rhs;
  vm_int_jump_next();
}
exec_fdiv:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].f = regs[lhs].f / regs[rhs].f;
  vm_int_jump_next();
}
exec_fdivc:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].f = regs[lhs].f / *(double *)&rhs;
  vm_int_jump_next();
}
exec_fcdiv:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].f = *(double *)&lhs / regs[rhs].f;
  vm_int_jump_next();
}
exec_fmod:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].f = fmod(regs[lhs].f, regs[rhs].f);
  vm_int_jump_next();
}
exec_fmodc:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].f = fmod(regs[lhs].f, *(double *)&rhs);
  vm_int_jump_next();
}
exec_fcmod:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  regs[out].f = fmod(*(double *)&lhs, regs[rhs].f);
  vm_int_jump_next();
}
exec_fbb:
{
  vm_int_arg_t in = vm_int_read();
  index = vm_int_read_at(vm_int_arg_t, index + (regs[in].f != 0) * sizeof(vm_int_arg_t));
  vm_int_jump_next();
}
exec_fbeq:
{
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  index = vm_int_read_at(vm_int_arg_t, index + (regs[lhs].f == regs[rhs].f) * sizeof(vm_int_arg_t));
  vm_int_jump_next();
}
exec_fblt:
{
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  index = vm_int_read_at(vm_int_arg_t, index + (regs[lhs].f < regs[rhs].f) * sizeof(vm_int_arg_t));
  vm_int_jump_next();
}
exec_fbeqc:
{
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  index = vm_int_read_at(vm_int_arg_t, index + (regs[lhs].f == *(double *)&rhs) * sizeof(vm_int_arg_t));
  vm_int_jump_next();
}
exec_fbltc:
{
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  index = vm_int_read_at(vm_int_arg_t, index + (regs[lhs].f < *(double *)&rhs) * sizeof(vm_int_arg_t));
  vm_int_jump_next();
}
exec_fcblt:
{
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  index = vm_int_read_at(vm_int_arg_t, index + (*(double *)&lhs < regs[rhs].f) * sizeof(vm_int_arg_t));
  vm_int_jump_next();
}
exec_ftou:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t in = vm_int_read();
  regs[out].u = regs[in].f;
  vm_int_jump_next();
}
exec_utof:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t in = vm_int_read();
  regs[out].f = regs[in].u;
  vm_int_jump_next();
}
}

int vm_run_arch_int(size_t nops, const vm_opcode_t *ops)
{
  vm_gc_t gc;
  vm_gc_init(&gc);
  int res = vm_int_run(nops, ops, &gc);
  vm_gc_deinit(&gc);
  return res;
}
