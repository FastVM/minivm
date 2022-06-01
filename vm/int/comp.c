#include "int.h"
#include "../vm.h"
#include "../jump.h"
#include "../reguse.h"

#define vm_int_buf_grow() \
    if (buf - ret + (1<<12) > alloc) \
    { \
      size_t length = buf - ret; \
      size_t old_alloc = alloc; \
      alloc = (length + 256) * 4; \
      ret = vm_realloc(ret, sizeof(uint8_t) * alloc); \
      buf = ret + length; \
      froms = vm_realloc(froms, sizeof(vm_loc_t) * alloc); \
      for (size_t i = old_alloc; i < alloc; i++) \
      { \
        froms[i] = 0; \
      } \
    }
#define vm_int_buf_put(type_, val_) ({*(type_*)buf = val_; buf+=sizeof(type_); })
#define vm_int_buf_put_op(op_) ({vm_int_buf_put(void *, ptrs[op_]);})

uint8_t *vm_int_comp(size_t nops, const vm_opcode_t *ops, uint8_t *jumps, void **ptrs)
{
  vm_loc_t index = 0;
  vm_loc_t cfunc = 0;
  vm_loc_t cend = 0;

  vm_reg_t nregs = 256;

  size_t alloc = 1 << 8;
  uint8_t *buf = vm_malloc(sizeof(uint8_t) * alloc);
  vm_loc_t *froms = vm_alloc0(sizeof(vm_loc_t) * alloc);
  vm_loc_t *locs = vm_malloc(sizeof(vm_loc_t) * nops);
  uint8_t *ret = buf;

  uint8_t *named = vm_alloc0(sizeof(uint8_t) * (1 << 14));
  vm_value_t *regs = vm_malloc(sizeof(vm_value_t) * (1 << 12));

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
        if (named[i])
        {
          size_t tmpbuf[256] = {0};
          if (vm_reg_is_used(nops, ops, jumps, index, i, 256, tmpbuf, 0))
          {
            vm_int_buf_grow();
            vm_int_buf_put_op(VM_INT_OP_MOVC);
            vm_int_buf_put(vm_reg_t, i);
            vm_int_buf_put(vm_value_t, regs[i]);
          }
          named[i] = 0;
        }
      }
    }
    vm_int_buf_grow();
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
        vm_int_buf_put(vm_reg_t, out);
        vm_int_buf_put(vm_reg_t, in);
      }
      break;
    }
    case VM_OPCODE_FTOS:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t in = ops[index++];
      if (named[in])
      {
        named[out] = 1;
        regs[out].s = regs[in].f;
      }
      else
      {
        named[out] = 0;
        vm_int_buf_put_op(VM_INT_OP_FTOS);
        vm_int_buf_put(vm_reg_t, out);
        vm_int_buf_put(vm_reg_t, in);
      }
      break;
    }
    case VM_OPCODE_SINT:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t in = ops[index++];
      named[out] = 1;
      regs[out].s = in;
      break;
    }
    case VM_OPCODE_SNEG:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t in = ops[index++];
      named[out] = 1;
      regs[out].s = -(vm_int_t)in;
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
      vm_int_buf_put(vm_loc_t, 0);
      break;
    }
    case VM_OPCODE_DJUMP:
    {
      vm_opcode_t locreg = ops[index++];
      vm_int_buf_put_op(VM_INT_OP_DJUMP);
      vm_int_buf_put(vm_reg_t, locreg);
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
      vm_int_buf_put(vm_loc_t, 0);
      vm_int_buf_put(vm_reg_t, nregs);
      cfunc = index;
      break;
    }
    case VM_OPCODE_SADD:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out].s = regs[lhs].s + regs[rhs].s;
      }
      else
      {
        if (named[lhs])
        {
          vm_int_buf_put_op(VM_INT_OP_SADDC);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, rhs);
          vm_int_buf_put(vm_value_t, regs[lhs]);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_SADDC);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_value_t, regs[rhs]);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_SADD);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_reg_t, rhs);
        }
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_SSUB:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out].s = regs[lhs].s - regs[rhs].s;
      }
      else
      {
        if (named[lhs])
        {
          vm_int_buf_put_op(VM_INT_OP_SCSUB);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_value_t, regs[lhs]);
          vm_int_buf_put(vm_reg_t, rhs);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_SSUBC);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_value_t, regs[rhs]);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_SSUB);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_reg_t, rhs);
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
          vm_int_buf_put(vm_reg_t, reg);
          vm_int_buf_put(vm_value_t, regs[reg]);
        }
      }
      named[rreg] = 0;
      vm_int_buf_put_op(VM_INT_OP_CALL0 + nargs);
      froms[buf - ret] = func;
      vm_int_buf_put(vm_loc_t, 0);
      for (int i = 0; i < nargs; i++)
      {
        vm_int_buf_put(vm_reg_t, ops[index++]);
      }
      vm_int_buf_put(vm_reg_t, nregs);
      vm_int_buf_put(vm_reg_t, rreg);
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
          vm_int_buf_put(vm_reg_t, reg);
          vm_int_buf_put(vm_value_t, regs[reg]);
        }
      }
      named[rreg] = 0;
      vm_int_buf_put_op(VM_INT_OP_DCALL0 + nargs);
      vm_int_buf_put(vm_reg_t, func);
      for (int i = 0; i < nargs; i++)
      {
        vm_int_buf_put(vm_reg_t, ops[index++]);
      }
      vm_int_buf_put(vm_reg_t, nregs);
      vm_int_buf_put(vm_reg_t, rreg);
      break;
    }
    case VM_OPCODE_RET:
    {
      vm_opcode_t out = ops[index++];
      if (named[out])
      {
        vm_int_buf_put_op(VM_INT_OP_RETC);
        vm_int_buf_put(vm_value_t, regs[out]);
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_RET);
        vm_int_buf_put(vm_reg_t, out);
      }
      break;
    }
    case VM_OPCODE_PUTCHAR:
    {
      vm_opcode_t in = ops[index++];
      if (named[in])
      {
        vm_int_buf_put_op(VM_INT_OP_MOVC);
        vm_int_buf_put(vm_reg_t, in);
        vm_int_buf_put(vm_value_t, regs[in]);
      }
      vm_int_buf_put_op(VM_INT_OP_PUTC);
      vm_int_buf_put(vm_reg_t, in);
      break;
    }
    case VM_OPCODE_SBEQ:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      if (named[lhs] && named[rhs])
      {
        if (regs[lhs].s == regs[rhs].s)
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jtrue;
          vm_int_buf_put(vm_loc_t, 0);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jfalse;
          vm_int_buf_put(vm_loc_t, 0);
        }
      }
      else if (named[rhs])
      {
        vm_int_buf_put_op(VM_INT_OP_SBEQC);
        vm_int_buf_put(vm_reg_t, lhs);
        vm_int_buf_put(vm_value_t, regs[rhs]);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_loc_t, 0);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_loc_t, 0);
      }
      else if (named[lhs])
      {
        vm_int_buf_put_op(VM_INT_OP_SBEQC);
        vm_int_buf_put(vm_reg_t, rhs);
        vm_int_buf_put(vm_value_t, regs[lhs]);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_loc_t, 0);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_loc_t, 0);
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_SBEQ);
        vm_int_buf_put(vm_reg_t, lhs);
        vm_int_buf_put(vm_reg_t, rhs);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_loc_t, 0);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_loc_t, 0);
      }
      break;
    }
    case VM_OPCODE_ADDR:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t func = ops[index++];
      vm_int_buf_put_op(VM_INT_OP_MOVC);
      vm_int_buf_put(vm_reg_t, out);
      froms[buf - ret] = func;
      vm_int_buf_put(vm_loc_t, 0);
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
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, rhs);
          vm_int_buf_put(vm_value_t, regs[lhs]);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_FADDC);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_value_t, regs[rhs]);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_FADD);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_reg_t, rhs);
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
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_value_t, regs[lhs]);
          vm_int_buf_put(vm_reg_t, rhs);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_FSUBC);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_value_t, regs[rhs]);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_FSUB);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_reg_t, rhs);
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
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, rhs);
          vm_int_buf_put(vm_value_t, regs[lhs]);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_FMULC);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_value_t, regs[rhs]);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_FMUL);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_reg_t, rhs);
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
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_value_t, regs[lhs]);
          vm_int_buf_put(vm_reg_t, rhs);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_FDIVC);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_value_t, regs[rhs]);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_FDIV);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_reg_t, rhs);
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
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_value_t, regs[lhs]);
          vm_int_buf_put(vm_reg_t, rhs);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_FMODC);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_value_t, regs[rhs]);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_FMOD);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_reg_t, rhs);
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
          vm_int_buf_put(vm_loc_t, 0);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jfalse;
          vm_int_buf_put(vm_loc_t, 0);
        }
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_FBB);
        vm_int_buf_put(vm_reg_t, val);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_loc_t, 0);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_loc_t, 0);
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
          vm_int_buf_put(vm_loc_t, 0);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jfalse;
          vm_int_buf_put(vm_loc_t, 0);
        }
      }
      else if (named[rhs])
      {
        vm_int_buf_put_op(VM_INT_OP_FBEQC);
        vm_int_buf_put(vm_reg_t, lhs);
        vm_int_buf_put(vm_value_t, regs[rhs]);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_loc_t, 0);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_loc_t, 0);
      }
      else if (named[lhs])
      {
        vm_int_buf_put_op(VM_INT_OP_FBEQC);
        vm_int_buf_put(vm_reg_t, rhs);
        vm_int_buf_put(vm_value_t, regs[lhs]);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_loc_t, 0);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_loc_t, 0);
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_FBEQ);
        vm_int_buf_put(vm_reg_t, lhs);
        vm_int_buf_put(vm_reg_t, rhs);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_loc_t, 0);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_loc_t, 0);
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
          vm_int_buf_put(vm_loc_t, 0);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jfalse;
          vm_int_buf_put(vm_loc_t, 0);
        }
      }
      else if (named[rhs])
      {
        vm_int_buf_put_op(VM_INT_OP_FBLTC);
        vm_int_buf_put(vm_reg_t, lhs);
        vm_int_buf_put(vm_value_t, regs[rhs]);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_loc_t, 0);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_loc_t, 0);
      }
      else if (named[lhs])
      {
        vm_int_buf_put_op(VM_INT_OP_FCBLT);
        vm_int_buf_put(vm_value_t, regs[lhs]);
        vm_int_buf_put(vm_reg_t, rhs);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_loc_t, 0);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_loc_t, 0);
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_FBLT);
        vm_int_buf_put(vm_reg_t, lhs);
        vm_int_buf_put(vm_reg_t, rhs);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_loc_t, 0);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_loc_t, 0);
      }
      break;
    }
    case VM_OPCODE_SMUL:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out].s = regs[lhs].s * regs[rhs].s;
      }
      else
      {
        if (named[lhs])
        {
          vm_int_buf_put_op(VM_INT_OP_SMULC);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, rhs);
          vm_int_buf_put(vm_value_t, regs[lhs]);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_SMULC);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_value_t, regs[rhs]);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_SMUL);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_reg_t, rhs);
        }
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_SDIV:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out].s = regs[lhs].s / regs[rhs].s;
      }
      else
      {
        if (named[lhs])
        {
          vm_int_buf_put_op(VM_INT_OP_SCDIV);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_value_t, regs[lhs]);
          vm_int_buf_put(vm_reg_t, rhs);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_SDIVC);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_value_t, regs[rhs]);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_SDIV);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_reg_t, rhs);
        }
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_SMOD:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (named[lhs] && named[rhs])
      {
        named[out] = 1;
        regs[out].s = regs[lhs].s % regs[rhs].s;
      }
      else
      {
        if (named[lhs])
        {
          vm_int_buf_put_op(VM_INT_OP_SCMOD);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_value_t, regs[lhs]);
          vm_int_buf_put(vm_reg_t, rhs);
        }
        else if (named[rhs])
        {
          vm_int_buf_put_op(VM_INT_OP_SMODC);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_value_t, regs[rhs]);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_SMOD);
          vm_int_buf_put(vm_reg_t, out);
          vm_int_buf_put(vm_reg_t, lhs);
          vm_int_buf_put(vm_reg_t, rhs);
        }
        named[out] = 0;
      }
      break;
    }
    case VM_OPCODE_STOF:
    {
      vm_opcode_t out = ops[index++];
      vm_opcode_t in = ops[index++];
      if (named[in])
      {
        named[out] = 1;
        regs[out].f = regs[in].s;
      }
      else
      {
        named[out] = 0;
        vm_int_buf_put_op(VM_INT_OP_STOF);
        vm_int_buf_put(vm_reg_t, out);
        vm_int_buf_put(vm_reg_t, in);
      }
      break;
    }
    case VM_OPCODE_SBB:
    {
      vm_opcode_t val = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      if (named[val])
      {
        if (regs[val].s)
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jtrue;
          vm_int_buf_put(vm_loc_t, 0);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jfalse;
          vm_int_buf_put(vm_loc_t, 0);
        }
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_SBB);
        vm_int_buf_put(vm_reg_t, val);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_loc_t, 0);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_loc_t, 0);
      }
      break;
    }
    case VM_OPCODE_SBLT:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      if (named[lhs] && named[rhs])
      {
        if (regs[lhs].s < regs[rhs].s)
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jtrue;
          vm_int_buf_put(vm_loc_t, 0);
        }
        else
        {
          vm_int_buf_put_op(VM_INT_OP_JUMP);
          froms[buf - ret] = jfalse;
          vm_int_buf_put(vm_loc_t, 0);
        }
      }
      else if (named[rhs])
      {
        vm_int_buf_put_op(VM_INT_OP_SBLTC);
        vm_int_buf_put(vm_reg_t, lhs);
        vm_int_buf_put(vm_value_t, regs[rhs]);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_loc_t, 0);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_loc_t, 0);
      }
      else if (named[lhs])
      {
        vm_int_buf_put_op(VM_INT_OP_SCBLT);
        vm_int_buf_put(vm_value_t, regs[lhs]);
        vm_int_buf_put(vm_reg_t, rhs);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_loc_t, 0);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_loc_t, 0);
      }
      else
      {
        vm_int_buf_put_op(VM_INT_OP_SBLT);
        vm_int_buf_put(vm_reg_t, lhs);
        vm_int_buf_put(vm_reg_t, rhs);
        froms[buf - ret] = jfalse;
        vm_int_buf_put(vm_loc_t, 0);
        froms[buf - ret] = jtrue;
        vm_int_buf_put(vm_loc_t, 0);
      }
      break;
    }
    default:
    {
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
      *(vm_loc_t *)&ret[i] = locs[froms[i]];
    }
  }
  vm_free(froms);
  vm_free(locs);
  return ret;
}
