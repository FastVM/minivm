#include "int.h"
#include "../gc.h"
#include "../jump.h"
#include "../opcode.h"

#define vm_int_read_at(type_, index_) (*(type_*)&ops[index_])
#define vm_int_read_type(type_) ({type_ ret=*(type_*)&ops[index]; index += sizeof(type_); ret; })
#define vm_int_read_reg() ({vm_int_read_type(vm_reg_t);})
#if defined(VM_DEBUG_READS)
#define vm_int_read_load() ({vm_value_t *r = &regs[vm_int_read_reg()]; fprintf(stderr, "  r%zu = %zu|%lf @%i\n", r - regs, (size_t) r->u, r->f, __LINE__); *r;})
#define vm_int_read_store() ({vm_value_t *r = &regs[vm_int_read_reg()]; fprintf(stderr, "-> r%zu @%i\n", r-regs, __LINE__); r;})
#define vm_int_read_value() ({vm_value_t v = vm_int_read_type(vm_value_t); fprintf(stderr, "  value %zu|%lf\n", (size_t) v.u, v.f); v; })
#define vm_int_read_loc() ({vm_int_read_type(vm_loc_t);})
#define vm_int_jump_next() ({fprintf(stderr, "end %zu @%zu\n", (size_t) index, (size_t) __LINE__); goto *vm_int_read_type(void *);})
#else
#define vm_int_read_load() ({regs[vm_int_read_reg()];})
#define vm_int_read_store() ({&regs[vm_int_read_reg()];})
#define vm_int_read_value() ({vm_int_read_type(vm_value_t);})
#define vm_int_read_loc() ({vm_int_read_type(vm_loc_t);})
#define vm_int_jump_next() ({goto *vm_int_read_type(void *);})
#endif

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
      [VM_INT_OP_FTOS] = &&exec_ftos,
      [VM_INT_OP_UTOS] = &&exec_utos,
      [VM_INT_OP_STOF] = &&exec_stof,
      [VM_INT_OP_STOU] = &&exec_stou,
      [VM_INT_OP_SMUL] = &&exec_smul,
      [VM_INT_OP_SMULC] = &&exec_smulc,
      [VM_INT_OP_SDIV] = &&exec_sdiv,
      [VM_INT_OP_SDIVC] = &&exec_sdivc,
      [VM_INT_OP_SCDIV] = &&exec_scdiv,
      [VM_INT_OP_SMOD] = &&exec_smod,
      [VM_INT_OP_SMODC] = &&exec_smodc,
      [VM_INT_OP_SCMOD] = &&exec_scmod,
      [VM_INT_OP_SBB] = &&exec_sbb,
      [VM_INT_OP_SBLT] = &&exec_sblt,
      [VM_INT_OP_SBLTC] = &&exec_sbltc,
      [VM_INT_OP_SCBLT] = &&exec_scblt,
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
  size_t nframes = 5000;
  size_t nregs0 = nframes * 16;
  vm_value_t *regs = vm_malloc(sizeof(vm_value_t) * nregs0);
  vm_value_t *regs_base = regs;
  vm_reg_t *stack = vm_malloc(sizeof(vm_value_t) * nframes);
  vm_reg_t *stack_base = stack;
  vm_loc_t index = 0;
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
  vm_value_t reg = vm_int_read_load();
  putchar(reg.u);
  vm_int_jump_next();
}
exec_mov:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t in = vm_int_read_load();
  *out = in;
  vm_int_jump_next();
}
exec_movc:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t in = vm_int_read_value();
  *out = in;
  vm_int_jump_next();
}
exec_uadd:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_load();
  out->u = lhs.u + rhs.u;
  vm_int_jump_next();
}
exec_usub:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_load();
  out->u = lhs.u - rhs.u;
  vm_int_jump_next();
}
exec_umul:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_load();
  out->u = lhs.u * rhs.u;
  vm_int_jump_next();
}
exec_udiv:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_load();
  out->u = lhs.u / rhs.u;
  vm_int_jump_next();
}
exec_umod:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_load();
  out->u = lhs.u % rhs.u;
  vm_int_jump_next();
}
exec_ret:
{
  vm_value_t inval = vm_int_read_load();
  index = *--stack;
  regs -= vm_int_read_at(vm_reg_t, index - sizeof(vm_reg_t));
  *vm_int_read_store() = inval;
  vm_int_jump_next();
}
exec_ubb:
{
  vm_value_t in = vm_int_read_load();
  index = vm_int_read_at(vm_loc_t, index + (in.u != 0) * sizeof(vm_loc_t));
  vm_int_jump_next();
}
exec_ubeq:
{
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_load();
  index = vm_int_read_at(vm_loc_t, index + (lhs.u == rhs.u) * sizeof(vm_loc_t));
  vm_int_jump_next();
}
exec_ublt:
{
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_load();
  index = vm_int_read_at(vm_loc_t, index + (lhs.u < rhs.u) * sizeof(vm_loc_t));
  vm_int_jump_next();
}
exec_call0:
{
  vm_loc_t func = vm_int_read_loc();
  vm_reg_t nregs = vm_int_read_reg();
  *stack++ = index;
  regs += nregs;
  index = func;
  vm_int_jump_next();
}
exec_call1:
{
  vm_loc_t func = vm_int_read_loc();
  vm_value_t r1 = vm_int_read_load();
  vm_reg_t nregs = vm_int_read_reg();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  index = func;
  vm_int_jump_next();
}
exec_call2:
{
  vm_loc_t func = vm_int_read_loc();
  vm_value_t r1 = vm_int_read_load();
  vm_value_t r2 = vm_int_read_load();
  vm_reg_t nregs = vm_int_read_reg();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  regs[2] = r2;
  index = func;
  vm_int_jump_next();
}
exec_call3:
{
  vm_loc_t func = vm_int_read_loc();
  vm_value_t r1 = vm_int_read_load();
  vm_value_t r2 = vm_int_read_load();
  vm_value_t r3 = vm_int_read_load();
  vm_reg_t nregs = vm_int_read_reg();
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
  vm_loc_t func = vm_int_read_loc();
  vm_value_t r1 = vm_int_read_load();
  vm_value_t r2 = vm_int_read_load();
  vm_value_t r3 = vm_int_read_load();
  vm_value_t r4 = vm_int_read_load();
  vm_reg_t nregs = vm_int_read_reg();
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
  vm_loc_t func = vm_int_read_loc();
  vm_value_t r1 = vm_int_read_load();
  vm_value_t r2 = vm_int_read_load();
  vm_value_t r3 = vm_int_read_load();
  vm_value_t r4 = vm_int_read_load();
  vm_value_t r5 = vm_int_read_load();
  vm_reg_t nregs = vm_int_read_reg();
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
  vm_loc_t func = vm_int_read_loc();
  vm_value_t r1 = vm_int_read_load();
  vm_value_t r2 = vm_int_read_load();
  vm_value_t r3 = vm_int_read_load();
  vm_value_t r4 = vm_int_read_load();
  vm_value_t r5 = vm_int_read_load();
  vm_value_t r6 = vm_int_read_load();
  vm_reg_t nregs = vm_int_read_reg();
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
exec_call7:
{
  vm_loc_t func = vm_int_read_loc();
  vm_value_t r1 = vm_int_read_load();
  vm_value_t r2 = vm_int_read_load();
  vm_value_t r3 = vm_int_read_load();
  vm_value_t r4 = vm_int_read_load();
  vm_value_t r5 = vm_int_read_load();
  vm_value_t r6 = vm_int_read_load();
  vm_value_t r7 = vm_int_read_load();
  vm_reg_t nregs = vm_int_read_reg();
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
  vm_loc_t func = vm_int_read_loc();
  vm_value_t r1 = vm_int_read_load();
  vm_value_t r2 = vm_int_read_load();
  vm_value_t r3 = vm_int_read_load();
  vm_value_t r4 = vm_int_read_load();
  vm_value_t r5 = vm_int_read_load();
  vm_value_t r6 = vm_int_read_load();
  vm_value_t r7 = vm_int_read_load();
  vm_value_t r8 = vm_int_read_load();
  vm_reg_t nregs = vm_int_read_reg();
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
  vm_loc_t func = vm_int_read_load().u;
  vm_reg_t nregs = vm_int_read_reg();
  *stack++ = index;
  regs += nregs;
  index = func;
  vm_int_jump_next();
}
exec_dcall1:
{
  vm_loc_t func = vm_int_read_load().u;
  vm_value_t r1 = vm_int_read_load();
  vm_reg_t nregs = vm_int_read_reg();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  index = func;
  vm_int_jump_next();
}
exec_dcall2:
{
  vm_loc_t func = vm_int_read_load().u;
  vm_value_t r1 = vm_int_read_load();
  vm_value_t r2 = vm_int_read_load();
  vm_reg_t nregs = vm_int_read_reg();
  *stack++ = index;
  regs += nregs;
  regs[1] = r1;
  regs[2] = r2;
  index = func;
  vm_int_jump_next();
}
exec_dcall3:
{
  vm_loc_t func = vm_int_read_load().u;
  vm_value_t r1 = vm_int_read_load();
  vm_value_t r2 = vm_int_read_load();
  vm_value_t r3 = vm_int_read_load();
  vm_reg_t nregs = vm_int_read_reg();
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
  vm_loc_t func = vm_int_read_load().u;
  vm_value_t r1 = vm_int_read_load();
  vm_value_t r2 = vm_int_read_load();
  vm_value_t r3 = vm_int_read_load();
  vm_value_t r4 = vm_int_read_load();
  vm_reg_t nregs = vm_int_read_reg();
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
  vm_loc_t func = vm_int_read_load().u;
  vm_value_t r1 = vm_int_read_load();
  vm_value_t r2 = vm_int_read_load();
  vm_value_t r3 = vm_int_read_load();
  vm_value_t r4 = vm_int_read_load();
  vm_value_t r5 = vm_int_read_load();
  vm_reg_t nregs = vm_int_read_reg();
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
  vm_loc_t func = vm_int_read_load().u;
  vm_value_t r1 = vm_int_read_load();
  vm_value_t r2 = vm_int_read_load();
  vm_value_t r3 = vm_int_read_load();
  vm_value_t r4 = vm_int_read_load();
  vm_value_t r5 = vm_int_read_load();
  vm_value_t r6 = vm_int_read_load();
  vm_reg_t nregs = vm_int_read_reg();
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
  vm_loc_t func = vm_int_read_load().u;
  vm_value_t r1 = vm_int_read_load();
  vm_value_t r2 = vm_int_read_load();
  vm_value_t r3 = vm_int_read_load();
  vm_value_t r4 = vm_int_read_load();
  vm_value_t r5 = vm_int_read_load();
  vm_value_t r6 = vm_int_read_load();
  vm_value_t r7 = vm_int_read_load();
  vm_reg_t nregs = vm_int_read_reg();
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
  vm_loc_t func = vm_int_read_load().u;
  vm_value_t r1 = vm_int_read_load();
  vm_value_t r2 = vm_int_read_load();
  vm_value_t r3 = vm_int_read_load();
  vm_value_t r4 = vm_int_read_load();
  vm_value_t r5 = vm_int_read_load();
  vm_value_t r6 = vm_int_read_load();
  vm_value_t r7 = vm_int_read_load();
  vm_value_t r8 = vm_int_read_load();
  vm_reg_t nregs = vm_int_read_reg();
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
  vm_loc_t loc = vm_int_read_loc();
  index = loc;
  vm_int_jump_next();
}
exec_djump:
{
  vm_value_t reg = vm_int_read_load();
  index = reg.u;
  vm_int_jump_next();
}
exec_ubeqc:
{
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_value();
  index = vm_int_read_at(vm_loc_t, index + (lhs.u == rhs.u) * sizeof(vm_loc_t));
  vm_int_jump_next();
}
exec_ubltc:
{
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_value();
  index = vm_int_read_at(vm_loc_t, index + (lhs.u < rhs.u) * sizeof(vm_loc_t));
  vm_int_jump_next();
}
exec_ucblt:
{
  vm_value_t lhs = vm_int_read_value();
  vm_value_t rhs = vm_int_read_load();
  index = vm_int_read_at(vm_loc_t, index + (lhs.u < rhs.u) * sizeof(vm_loc_t));
  vm_int_jump_next();
}
exec_uaddc:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_value();
  out->u = lhs.u + rhs.u;
  vm_int_jump_next();
}
exec_usubc:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_value();
  out->u = lhs.u - rhs.u;
  vm_int_jump_next();
}
exec_umulc:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_value();
  out->u = lhs.u * rhs.u;
  vm_int_jump_next();
}
exec_udivc:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_value();
  out->u = lhs.u / rhs.u;
  vm_int_jump_next();
}
exec_umodc:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_value();
  out->u = lhs.u % rhs.u;
  vm_int_jump_next();
}
exec_ucsub:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_value();
  vm_value_t rhs = vm_int_read_load();
  out->u = lhs.u - rhs.u;
  vm_int_jump_next();
}
exec_ucdiv:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_value();
  vm_value_t rhs = vm_int_read_load();
  out->u = lhs.u / rhs.u;
  vm_int_jump_next();
}
exec_ucmod:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_value();
  vm_value_t rhs = vm_int_read_load();
  out->u = lhs.u % rhs.u;
  vm_int_jump_next();
}
exec_retc:
{
  vm_value_t inval = vm_int_read_value();
  index = *--stack;
  regs -= vm_int_read_at(vm_reg_t, index - sizeof(vm_reg_t));
  *vm_int_read_store()= inval;
  vm_int_jump_next();
}
exec_pair:
{
  vm_gc_collect(gc, nregs0, regs_base);
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_load();
  vm_value_t *res = vm_gc_alloc(gc);
  res[0] = lhs;
  res[1] = rhs;
  out->p = res;
  vm_int_jump_next();
}
exec_pairc:
{
  vm_gc_collect(gc, nregs0, regs_base);
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_value();
  vm_value_t *res = vm_gc_alloc(gc);
  res[0] = lhs;
  res[1] = rhs;
  out->p = res;
  vm_int_jump_next();
}
exec_ccons:
{
  vm_gc_collect(gc, nregs0, regs_base);
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_value();
  vm_value_t rhs = vm_int_read_load();
  vm_value_t *res = vm_gc_alloc(gc);
  res[0] = lhs;
  res[1] = rhs;
  out->p = res;
  vm_int_jump_next();
}
exec_cconsc:
{
  vm_gc_collect(gc, nregs0, regs_base);
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_value();
  vm_value_t rhs = vm_int_read_value();
  vm_value_t *res = vm_gc_alloc(gc);
  res[0] = lhs;
  res[1] = rhs;
  out->p = res;
  vm_int_jump_next();
}
exec_getcar:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t pair = vm_int_read_load();
  *out = pair.p[0];
  vm_int_jump_next();
}
exec_getcdr:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t pair = vm_int_read_load();
  *out = pair.p[1];
  vm_int_jump_next();
}
exec_setcar:
{
  vm_value_t pair = vm_int_read_load();
  vm_value_t val = vm_int_read_load();
  pair.p[0] = val;
  vm_int_jump_next();
}
exec_setcdr:
{
  vm_value_t pair = vm_int_read_load();
  vm_value_t val = vm_int_read_load();
  pair.p[1] = val;
  vm_int_jump_next();
}
exec_setcari:
{
  vm_value_t pair = vm_int_read_load();
  vm_value_t val = vm_int_read_load();
  pair.p[0] = val;
  vm_int_jump_next();
}
exec_setcdri:
{
  vm_value_t pair = vm_int_read_load();
  vm_value_t val = vm_int_read_load();
  pair.p[1] = val;
  vm_int_jump_next();
}
exec_fadd:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_load();
  out->f = lhs.f + rhs.f;
  vm_int_jump_next();
}
exec_faddc:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_value();
  out->f = lhs.f + rhs.f;
  vm_int_jump_next();
}
exec_fsub:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_load();
  out->f = lhs.f - rhs.f;
  vm_int_jump_next();
}
exec_fsubc:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_value();
  out->f = lhs.f - rhs.f;
  vm_int_jump_next();
}
exec_fcsub:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_value();
  vm_value_t rhs = vm_int_read_load();
  out->f = lhs.f - rhs.f;
  vm_int_jump_next();
}
exec_fmul:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_load();
  out->f = lhs.f * rhs.f;
  vm_int_jump_next();
}
exec_fmulc:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_value();
  vm_value_t rhs = vm_int_read_value();
  out->f = lhs.f * rhs.f;
  vm_int_jump_next();
}
exec_fdiv:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_load();
  out->f = lhs.f / rhs.f;
  vm_int_jump_next();
}
exec_fdivc:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_value();
  out->f = lhs.f / rhs.f;
  vm_int_jump_next();
}
exec_fcdiv:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_value();
  vm_value_t rhs = vm_int_read_load();
  out->f = lhs.f / rhs.f;
  vm_int_jump_next();
}
exec_fmod:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_load();
  out->f = fmod(lhs.f, rhs.f);
  vm_int_jump_next();
}
exec_fmodc:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_value();
  out->f = fmod(lhs.f, rhs.f);
  vm_int_jump_next();
}
exec_fcmod:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_value();
  vm_value_t rhs = vm_int_read_load();
  out->f = fmod(lhs.f, rhs.f);
  vm_int_jump_next();
}
exec_fbb:
{
  vm_value_t in = vm_int_read_load();
  index = vm_int_read_at(vm_loc_t, index + (in.f != 0) * sizeof(vm_loc_t));
  vm_int_jump_next();
}
exec_fbeq:
{
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_load();
  index = vm_int_read_at(vm_loc_t, index + (lhs.f == rhs.f) * sizeof(vm_loc_t));
  vm_int_jump_next();
}
exec_fblt:
{
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_load();
  index = vm_int_read_at(vm_loc_t, index + (lhs.f < rhs.f) * sizeof(vm_loc_t));
  vm_int_jump_next();
}
exec_fbeqc:
{
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_value();
  index = vm_int_read_at(vm_loc_t, index + (lhs.f == rhs.f) * sizeof(vm_loc_t));
  vm_int_jump_next();
}
exec_fbltc:
{
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_value();
  index = vm_int_read_at(vm_loc_t, index + (lhs.f < rhs.f) * sizeof(vm_loc_t));
  vm_int_jump_next();
}
exec_fcblt:
{
  vm_value_t lhs = vm_int_read_value();
  vm_value_t rhs = vm_int_read_load();
  index = vm_int_read_at(vm_loc_t, index + (lhs.f < rhs.f) * sizeof(vm_loc_t));
  vm_int_jump_next();
}
exec_ftou:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t in = vm_int_read_load();
  out->u = in.f;
  vm_int_jump_next();
}
exec_utof:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t in = vm_int_read_load();
  out->f = in.u;
  vm_int_jump_next();
}
exec_ftos:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t in = vm_int_read_load();
  out->s = in.f;
  vm_int_jump_next();
}
exec_utos:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t in = vm_int_read_load();
  out->s = in.u;
  vm_int_jump_next();
}
exec_stof:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t in = vm_int_read_load();
  out->f = in.s;
  vm_int_jump_next();
}
exec_stou:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t in = vm_int_read_load();
  out->u = in.s;
  vm_int_jump_next();
}
exec_smul:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_load();
  out->s = lhs.s * rhs.s;
  vm_int_jump_next();
}
exec_smulc:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_value();
  out->s = lhs.s * rhs.s;
  vm_int_jump_next();
}
exec_sdiv:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_load();
  out->s = lhs.s / rhs.s;
  vm_int_jump_next();
}
exec_sdivc:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_value();
  out->s = lhs.s / rhs.s;
  vm_int_jump_next();
}
exec_scdiv:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_value();
  vm_value_t rhs = vm_int_read_load();
  out->s = lhs.s / rhs.s;
  vm_int_jump_next();
}
exec_smod:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_load();
  out->s = lhs.s % rhs.s;
  vm_int_jump_next();
}
exec_smodc:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_value();
  out->s = lhs.s % rhs.s;
  vm_int_jump_next();
}
exec_scmod:
{
  vm_value_t *out = vm_int_read_store();
  vm_value_t lhs = vm_int_read_value();
  vm_value_t rhs = vm_int_read_load();
  out->s = lhs.s % rhs.s;
  vm_int_jump_next();
}
exec_sbb:
{
  vm_value_t in = vm_int_read_load();
  index = vm_int_read_at(vm_loc_t, index + (in.s != 0) * sizeof(vm_loc_t));
  vm_int_jump_next();
}
exec_sblt:
{
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_load();
  index = vm_int_read_at(vm_loc_t, index + (lhs.s < rhs.s) * sizeof(vm_loc_t));
  vm_int_jump_next();
}
exec_sbltc:
{
  vm_value_t lhs = vm_int_read_load();
  vm_value_t rhs = vm_int_read_value();
  index = vm_int_read_at(vm_loc_t, index + (lhs.s < rhs.s) * sizeof(vm_loc_t));
  vm_int_jump_next();
}
exec_scblt:
{
  vm_value_t lhs = vm_int_read_value();
  vm_value_t rhs = vm_int_read_load();
  index = vm_int_read_at(vm_loc_t, index + (lhs.s < rhs.s) * sizeof(vm_loc_t));
  vm_int_jump_next();
}
}

int vm_run_arch_int(size_t nops, const vm_opcode_t *ops, const vm_func_t *funcs)
{
  vm_gc_t *gc = vm_malloc(sizeof(vm_gc_t));
  vm_gc_init(gc);
  int res = vm_int_run(nops, ops, gc, funcs);
  vm_gc_deinit(gc);
  return res;
}
