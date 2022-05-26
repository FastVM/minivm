#include "int.h"
#include "../gc.h"
#include "../jump.h"
#include "../opcode.h"

#define vm_int_read_at(type_, index_) (*(type_*)&ops[index_])
#define vm_int_read_type(type_) ({type_ ret=*(type_*)&ops[index]; index += sizeof(type_); ret; })
#define vm_int_read() ({vm_int_read_type(vm_int_arg_t);})
#define vm_int_jump_next() ({goto *vm_int_read_type(void *);})

int vm_int_run(size_t nops, const vm_opcode_t *iops, vm_gc_t *gc, const vm_func_t *funcs)
{
  void *ptrs[] = {
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
      [VM_INT_OP_DJUMP] = &&exec_djump,
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
      [VM_INT_OP_CALL6] = &&exec_call6,
      [VM_INT_OP_CALL7] = &&exec_call7,
      [VM_INT_OP_CALL8] = &&exec_call8,
      [VM_INT_OP_DCALL0] = &&exec_dcall0,
      [VM_INT_OP_DCALL1] = &&exec_dcall1,
      [VM_INT_OP_DCALL2] = &&exec_dcall2,
      [VM_INT_OP_DCALL3] = &&exec_dcall3,
      [VM_INT_OP_DCALL4] = &&exec_dcall4,
      [VM_INT_OP_DCALL5] = &&exec_dcall5,
      [VM_INT_OP_DCALL6] = &&exec_dcall6,
      [VM_INT_OP_DCALL7] = &&exec_dcall7,
      [VM_INT_OP_DCALL8] = &&exec_dcall8,
      [VM_INT_OP_CONS] = &&exec_pair,
      [VM_INT_OP_CONSC] = &&exec_pairc,
      [VM_INT_OP_CCONS] = &&exec_ccons,
      [VM_INT_OP_CCONSC] = &&exec_cconsc,
      [VM_INT_OP_GETCAR] = &&exec_getcar,
      [VM_INT_OP_GETCDR] = &&exec_getcdr,
      [VM_INT_OP_SETCAR] = &&exec_setcar,
      [VM_INT_OP_SETCDR] = &&exec_setcdr,
      [VM_INT_OP_FTOU] = &&exec_ftou,
      [VM_INT_OP_UTOF] = &&exec_utof,
      [VM_INT_OP_FUNC0] = &&exec_func0,
      [VM_INT_OP_FUNC1] = &&exec_func1,
      [VM_INT_OP_FUNC2] = &&exec_func2,
      [VM_INT_OP_FUNC3] = &&exec_func3,
      [VM_INT_OP_FUNC4] = &&exec_func4,
      [VM_INT_OP_FUNC5] = &&exec_func5,
      [VM_INT_OP_FUNC6] = &&exec_func6,
      [VM_INT_OP_FUNC7] = &&exec_func7,
      [VM_INT_OP_FUNC8] = &&exec_func8,
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
  fprintf(stdout, "%c", (int)regs[reg].u);
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
exec_call6:
{
  vm_int_arg_t func = vm_int_read();
  vm_value_t r1 = regs[vm_int_read()];
  vm_value_t r2 = regs[vm_int_read()];
  vm_value_t r3 = regs[vm_int_read()];
  vm_value_t r4 = regs[vm_int_read()];
  vm_value_t r5 = regs[vm_int_read()];
  vm_value_t r6 = regs[vm_int_read()];
  vm_value_t r7 = regs[vm_int_read()];
  vm_value_t r8 = regs[vm_int_read()];
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  regs[2] = r2;
  regs[3] = r3;
  regs[4] = r4;
  regs[5] = r5;
  regs[6] = r6;
  regs[7] = r7;
  index = func;
  vm_int_jump_next();
}
exec_call7:
{
  vm_int_arg_t func = vm_int_read();
  vm_value_t r1 = regs[vm_int_read()];
  vm_value_t r2 = regs[vm_int_read()];
  vm_value_t r3 = regs[vm_int_read()];
  vm_value_t r4 = regs[vm_int_read()];
  vm_value_t r5 = regs[vm_int_read()];
  vm_value_t r6 = regs[vm_int_read()];
  vm_value_t r7 = regs[vm_int_read()];
  vm_value_t r8 = regs[vm_int_read()];
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  regs[2] = r2;
  regs[3] = r3;
  regs[4] = r4;
  regs[5] = r5;
  regs[6] = r6;
  regs[7] = r7;
  index = func;
  vm_int_jump_next();
}
exec_call8:
{
  vm_int_arg_t func = vm_int_read();
  vm_value_t r1 = regs[vm_int_read()];
  vm_value_t r2 = regs[vm_int_read()];
  vm_value_t r3 = regs[vm_int_read()];
  vm_value_t r4 = regs[vm_int_read()];
  vm_value_t r5 = regs[vm_int_read()];
  vm_value_t r6 = regs[vm_int_read()];
  vm_value_t r7 = regs[vm_int_read()];
  vm_value_t r8 = regs[vm_int_read()];
  vm_int_arg_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  regs[2] = r2;
  regs[3] = r3;
  regs[4] = r4;
  regs[5] = r5;
  regs[6] = r6;
  regs[7] = r7;
  regs[8] = r8;
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
exec_dcall6:
{
  vm_int_arg_t func = regs[vm_int_read()].u;
  vm_value_t r1 = regs[vm_int_read()];
  vm_value_t r2 = regs[vm_int_read()];
  vm_value_t r3 = regs[vm_int_read()];
  vm_value_t r4 = regs[vm_int_read()];
  vm_value_t r5 = regs[vm_int_read()];
  vm_value_t r6 = regs[vm_int_read()];
  vm_int_arg_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  regs[2] = r2;
  regs[3] = r3;
  regs[4] = r4;
  regs[5] = r5;
  regs[6] = r6;
  index = func;
  vm_int_jump_next();
}
exec_dcall7:
{
  vm_int_arg_t func = regs[vm_int_read()].u;
  vm_value_t r1 = regs[vm_int_read()];
  vm_value_t r2 = regs[vm_int_read()];
  vm_value_t r3 = regs[vm_int_read()];
  vm_value_t r4 = regs[vm_int_read()];
  vm_value_t r5 = regs[vm_int_read()];
  vm_value_t r6 = regs[vm_int_read()];
  vm_value_t r7 = regs[vm_int_read()];
  vm_int_arg_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  regs[2] = r2;
  regs[3] = r3;
  regs[4] = r4;
  regs[5] = r5;
  regs[6] = r6;
  regs[7] = r7;
  index = func;
  vm_int_jump_next();
}
exec_dcall8:
{
  vm_int_arg_t func = regs[vm_int_read()].u;
  vm_value_t r1 = regs[vm_int_read()];
  vm_value_t r2 = regs[vm_int_read()];
  vm_value_t r3 = regs[vm_int_read()];
  vm_value_t r4 = regs[vm_int_read()];
  vm_value_t r5 = regs[vm_int_read()];
  vm_value_t r6 = regs[vm_int_read()];
  vm_value_t r7 = regs[vm_int_read()];
  vm_value_t r8 = regs[vm_int_read()];
  vm_int_arg_t nregs = vm_int_read();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  regs[2] = r2;
  regs[3] = r3;
  regs[4] = r4;
  regs[5] = r5;
  regs[6] = r6;
  regs[7] = r7;
  regs[8] = r8;
  index = func;
  vm_int_jump_next();
}
exec_jump:
{
  vm_int_arg_t where = vm_int_read();
  index = where;
  vm_int_jump_next();
}
exec_djump:
{
  vm_int_arg_t reg = vm_int_read();
  index = regs[reg].u;
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
exec_ccons:
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
exec_cconsc:
{
  vm_gc_collect(gc, nregs, regs_base);
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t lhs = vm_int_read();
  vm_int_arg_t rhs = vm_int_read();
  vm_value_t *res = vm_gc_alloc(gc);
  res[0].u = lhs;
  res[1].u = rhs;
  regs[out].p = res;
  vm_int_jump_next();
}
exec_getcar:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t pair = vm_int_read();
  regs[out] = regs[pair].p[0];
  vm_int_jump_next();
}
exec_getcdr:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t pair = vm_int_read();
  regs[out] = regs[pair].p[1];
  vm_int_jump_next();
}
exec_setcar:
{
  vm_int_arg_t pair = vm_int_read();
  vm_int_arg_t val = vm_int_read();
  regs[pair].p[0] = regs[val];
  vm_int_jump_next();
}
exec_setcdr:
{
  vm_int_arg_t pair = vm_int_read();
  vm_int_arg_t val = vm_int_read();
  regs[pair].p[1] = regs[val];
  vm_int_jump_next();
}
exec_setcari:
{
  vm_int_arg_t pair = vm_int_read();
  vm_int_arg_t val = vm_int_read();
  regs[pair].p[0].u = val;
  vm_int_jump_next();
}
exec_setcdri:
{
  vm_int_arg_t pair = vm_int_read();
  vm_int_arg_t val = vm_int_read();
  regs[pair].p[1].u = val;
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
exec_func0:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t in = vm_int_read();
  regs[out] = funcs[in](gc, NULL);
  vm_int_jump_next();
}
exec_func1:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t in = vm_int_read();
  vm_value_t args[] = {
    regs[vm_int_read()],
  };
  regs[out] = funcs[in](gc, args);
  vm_int_jump_next();
}
exec_func2:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t in = vm_int_read();
  vm_value_t args[] = {
    regs[vm_int_read()],
    regs[vm_int_read()],
  };
  regs[out] = funcs[in](gc, args);
  vm_int_jump_next();
}
exec_func3:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t in = vm_int_read();
  vm_value_t args[] = {
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
  };
  regs[out] = funcs[in](gc, args);
  vm_int_jump_next();
}
exec_func4:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t in = vm_int_read();
  vm_value_t args[] = {
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
  };
  regs[out] = funcs[in](gc, args);
  vm_int_jump_next();
}
exec_func5:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t in = vm_int_read();
  vm_value_t args[] = {
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
  };
  regs[out] = funcs[in](gc, args);
  vm_int_jump_next();
}
exec_func6:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t in = vm_int_read();
  vm_value_t args[] = {
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
  };
  regs[out] = funcs[in](gc, args);
  vm_int_jump_next();
}
exec_func7:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t in = vm_int_read();
  vm_value_t args[] = {
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
  };
  regs[out] = funcs[in](gc, args);
  vm_int_jump_next();
}
exec_func8:
{
  vm_int_arg_t out = vm_int_read();
  vm_int_arg_t in = vm_int_read();
  vm_value_t args[] = {
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
    regs[vm_int_read()],
  };
  regs[out] = funcs[in](gc, args);
  vm_int_jump_next();
}
}

int vm_run_arch_int(size_t nops, const vm_opcode_t *ops, const vm_func_t *funcs)
{
  vm_gc_t gc;
  vm_gc_init(&gc);
  int res = vm_int_run(nops, ops, &gc, funcs);
  vm_gc_deinit(&gc);
  return res;
}
