#include "int.h"
#include "../jump.h"
#include "../reguse.h"

#define vm_int_buf_grow() \
    if (buf - ret + 256 > alloc) \
    { \
      vm_int_arg_t length = buf - ret; \
      vm_int_arg_t old_alloc = alloc; \
      alloc = (length + 256) * 4; \
      ret = vm_realloc(ret, sizeof(uint8_t) * alloc); \
      buf = ret + length; \
      froms = vm_realloc(froms, sizeof(vm_int_arg_t) * alloc); \
      for (size_t i = old_alloc; i < alloc; i++) \
      { \
        froms[i] = 0; \
      } \
    }
#define vm_int_buf_put(type_, val_) ({vm_int_buf_grow();*(type_*)buf = (type_) val_; buf+=sizeof(type_); })
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
  vm_value_t regs[1 << 12] = {0};

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
    if ((jumps[index] & VM_JUMP_IN) || (jumps[index] & VM_JUMP_OUT))
    {
      for (size_t i = 0; i < nregs; i++)
      {
        if (!named[i])
        {
          continue;
        }
        if (vm_reg_is_used(nops, ops, jumps, index, i, 16))
        {
          vm_int_buf_put_op(VM_INT_OP_MOVC);
          vm_int_buf_put(vm_int_arg_t, i);
          vm_int_buf_put(vm_int_arg_t, regs[i].u);
        }
        named[i] = 0;
      }
    }
    locs[index] = buf - ret;
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
      vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
      break;
    }
    case VM_OPCODE_DJUMP:
    {
      vm_opcode_t locreg = ops[index++];
      vm_int_buf_put_op(VM_INT_OP_DJUMP);
      vm_int_buf_put(vm_int_arg_t, locreg);
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
      vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
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
        }
      }
      named[rreg] = 0;
      vm_int_buf_put_op(VM_INT_OP_CALL0 + nargs);
      froms[buf - ret] = func;
      vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
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
        }
      }
      named[rreg] = 0;
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
          vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jfalse;
          vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        }
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_UBB);
        vm_int_buf_put(vm_int_arg_t, val);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
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
          vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jfalse;
          vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        }
      }
      else if (named[rhs])
      {
        vm_int_buf_put_op(VM_INT_OP_UBEQC);
        vm_int_buf_put(vm_int_arg_t, lhs);
        vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
      }
      else if (named[lhs])
      {
        vm_int_buf_put_op(VM_INT_OP_UBEQC);
        vm_int_buf_put(vm_int_arg_t, rhs);
        vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_UBEQ);
        vm_int_buf_put(vm_int_arg_t, lhs);
        vm_int_buf_put(vm_int_arg_t, rhs);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
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
          vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jfalse;
          vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        }
      }
      else if (named[rhs])
      {
        vm_int_buf_put_op(VM_INT_OP_UBLTC);
        vm_int_buf_put(vm_int_arg_t, lhs);
        vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
      }
      else if (named[lhs])
      {
        vm_int_buf_put_op(VM_INT_OP_UCBLT);
        vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
        vm_int_buf_put(vm_int_arg_t, rhs);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_UBLT);
        vm_int_buf_put(vm_int_arg_t, lhs);
        vm_int_buf_put(vm_int_arg_t, rhs);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
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
      vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
      break;
    }
    case VM_OPCODE_CONS:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
          vm_int_buf_put_op(VM_INT_OP_CCONSC);
          vm_int_buf_put(vm_int_arg_t, out);
          vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
          vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
      }
      else if (named[lhs])
      {
        vm_int_buf_put_op(VM_INT_OP_CCONS);
        vm_int_buf_put(vm_int_arg_t, out);
        vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
        vm_int_buf_put(vm_int_arg_t, rhs);
      }
      else if (named[rhs])
      {
        vm_int_buf_put_op(VM_INT_OP_CONSC);
        vm_int_buf_put(vm_int_arg_t, out);
        vm_int_buf_put(vm_int_arg_t, lhs);
        vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_CONS);
        vm_int_buf_put(vm_int_arg_t, out);
        vm_int_buf_put(vm_int_arg_t, lhs);
        vm_int_buf_put(vm_int_arg_t, rhs);
      }
      named[out] = 0;
      break;
    }
    case VM_OPCODE_GETCAR:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      // if (named[lhs])
      // {
      //   vm_value_t *pair = regs[lhs].p;
      //   named[out] = 1;
      //   regs[out] = pair[0];
      // }
      // else
      // {
        vm_int_buf_put_op(VM_INT_OP_GETCAR);
        vm_int_buf_put(vm_int_arg_t, out);
        vm_int_buf_put(vm_int_arg_t, lhs);
        named[out] = 0;
      // }
      break;
    }
    case VM_OPCODE_GETCDR:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      // if (named[lhs])
      // {
      //   vm_value_t *pair = regs[lhs].p;
      //   named[out] = 1;
      //   regs[out] = pair[1];
      // }
      // else
      // {
        vm_int_buf_put_op(VM_INT_OP_GETCDR);
        vm_int_buf_put(vm_int_arg_t, out);
        vm_int_buf_put(vm_int_arg_t, lhs);
        named[out] = 0;
      // }
      break;
    }
    case VM_OPCODE_SETCAR:
    {
      vm_opcode_t pair = ops[index++];
      vm_opcode_t car = ops[index++];
      // if (named[pair])
      // {
      //   if (named[car]) {
      //     regs[pair].p[0] = regs[car];
      //   } else {
      //     vm_int_buf_put_op(VM_INT_OP_CONSC);
      //     vm_int_buf_put(vm_int_arg_t, pair);
      //     vm_int_buf_put(vm_int_arg_t, regs[car].u);
      //     vm_int_buf_put(vm_int_arg_t, regs[pair].p[1].u);
      //     named[pair] = 0;
      //   }
      // }
      // else
      // {
        if (named[car]) {
          vm_int_buf_put_op(VM_INT_OP_SETCARC);
          vm_int_buf_put(vm_int_arg_t, pair);
          vm_int_buf_put(vm_int_arg_t, regs[car].u);
        } else {
          vm_int_buf_put_op(VM_INT_OP_SETCAR);
          vm_int_buf_put(vm_int_arg_t, pair);
          vm_int_buf_put(vm_int_arg_t, car);
        }
      // }
      break;
    }
    case VM_OPCODE_SETCDR:
    {
      vm_opcode_t pair = ops[index++];
      vm_opcode_t cdr = ops[index++];
      // if (named[pair])
      // {
      //   if (named[cdr]) {
      //     regs[pair].p[1] = regs[cdr];
      //   } else {
      //     vm_int_buf_put_op(VM_INT_OP_CCONS);
      //     vm_int_buf_put(vm_int_arg_t, pair);
      //     vm_int_buf_put(vm_int_arg_t, regs[pair].p[0].u);
      //     vm_int_buf_put(vm_int_arg_t, regs[cdr].u);
      //     named[pair] = 0;
      //   }
      // }
      // else
      // {
        if (named[cdr]) {
          vm_int_buf_put_op(VM_INT_OP_SETCDRC);
          vm_int_buf_put(vm_int_arg_t, pair);
          vm_int_buf_put(vm_int_arg_t, regs[cdr].u);
        } else {
          vm_int_buf_put_op(VM_INT_OP_SETCDR);
          vm_int_buf_put(vm_int_arg_t, pair);
          vm_int_buf_put(vm_int_arg_t, cdr);
        }
      // }
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
          vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jfalse;
          vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        }
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_FBB);
        vm_int_buf_put(vm_int_arg_t, val);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
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
          vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jfalse;
          vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        }
      }
      else if (named[rhs])
      {
        vm_int_buf_put_op(VM_INT_OP_FBEQC);
        vm_int_buf_put(vm_int_arg_t, lhs);
        vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
      }
      else if (named[lhs])
      {
        vm_int_buf_put_op(VM_INT_OP_FBEQC);
        vm_int_buf_put(vm_int_arg_t, rhs);
        vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_FBEQ);
        vm_int_buf_put(vm_int_arg_t, lhs);
        vm_int_buf_put(vm_int_arg_t, rhs);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
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
          vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jfalse;
          vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        }
      }
      else if (named[rhs])
      {
        vm_int_buf_put_op(VM_INT_OP_FBLTC);
        vm_int_buf_put(vm_int_arg_t, lhs);
        vm_int_buf_put(vm_int_arg_t, regs[rhs].u);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
      }
      else if (named[lhs])
      {
        vm_int_buf_put_op(VM_INT_OP_FCBLT);
        vm_int_buf_put(vm_int_arg_t, regs[lhs].u);
        vm_int_buf_put(vm_int_arg_t, rhs);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_FBLT);
        vm_int_buf_put(vm_int_arg_t, lhs);
        vm_int_buf_put(vm_int_arg_t, rhs);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_int_arg_t, SIZE_MAX);
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
