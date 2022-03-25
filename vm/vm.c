
// IDIOMS:
// Undefined Behavior when opcodes are bad.
// Do not use C preprocessor `#.*` lines
// Types end in _t, and never use tagspace
// - only use `int` when it is defined by a LibC function to do so
//   * exit, printf, fclose, and main all use these
// Computed gotos are used along with a jump table.
// - `&&do_whatever` means get address of code at `do_whatever`
// - `vm_jump_next();` means run the next opcode

#include "vm.h"
#include "obj.h"
#include "lib.h"

#define vm_read() (ops[index++])
#define vm_jump_next() goto *ptrs[ops[index++]]

typedef struct {
  vm_opcode_t outreg;
  vm_opcode_t nlocals;
  vm_opcode_t index;
} vm_frame_t;

/// VM hot loop
int vm_run(size_t nops, const vm_opcode_t *ops) {
  // our dear jump table
  static void *const ptrs[] = {
    [VM_OPCODE_EXIT] = &&do_exit,
    [VM_OPCODE_REG] = &&do_reg,
    [VM_OPCODE_INT] = &&do_int,
    [VM_OPCODE_JUMP] = &&do_jump,
    [VM_OPCODE_FUNC] = &&do_func,
    [VM_OPCODE_ADD] = &&do_add,
    [VM_OPCODE_SUB] = &&do_sub,
    [VM_OPCODE_MUL] = &&do_mul,
    [VM_OPCODE_DIV] = &&do_div,
    [VM_OPCODE_MOD] = &&do_mod,
    [VM_OPCODE_CALL] = &&do_call,
    [VM_OPCODE_RETURN] = &&do_return,
    [VM_OPCODE_PUTCHAR] = &&do_putchar,
    [VM_OPCODE_BB] = &&do_bb,
    [VM_OPCODE_EQ] = &&do_eq,
    [VM_OPCODE_LT] = &&do_lt,
    [VM_OPCODE_DCALL] = &&do_dcall,
    [VM_OPCODE_XCALL] = &&do_xcall,
    [VM_OPCODE_CONS] = &&do_cons,
    [VM_OPCODE_CAR] = &&do_car,
    [VM_OPCODE_CDR] = &&do_cdr,
    [VM_OPCODE_FREE] = &&do_free,
  };
  vm_opcode_t index = 0;
  vm_obj_t *locals_base = vm_malloc(sizeof(vm_obj_t) * (1 << 16));
  vm_obj_t *locals = locals_base;
  vm_frame_t *frames = vm_malloc(sizeof(vm_frame_t) * (1 << 10));
  vm_frame_t *frame = &frames[0];
  frame->nlocals = 0;
  frame += 1;
  frame->nlocals = 256;
  vm_jump_next();
do_exit : {
  vm_free(locals_base);
  vm_free(frames);
  return 0;
}
do_reg : {
  vm_opcode_t to = vm_read();
  vm_opcode_t from = vm_read();
  locals[to] = locals[from];
  vm_jump_next();
}
do_int : {
  vm_opcode_t to = vm_read();
  vm_opcode_t from = vm_read();
  locals[to] = vm_obj_num(from);
  vm_jump_next();
}
do_jump : {
  index = ops[index];
  vm_jump_next();
}
do_func : {
  index = ops[index];
  vm_jump_next();
}
do_add : {
  vm_opcode_t to = vm_read();
  vm_opcode_t lhs = vm_read();
  vm_opcode_t rhs = vm_read();
  locals[to] =
      vm_obj_num(vm_obj_to_num(locals[lhs]) + vm_obj_to_num(locals[rhs]));
  vm_jump_next();
}
do_sub : {
  vm_opcode_t to = vm_read();
  vm_opcode_t lhs = vm_read();
  vm_opcode_t rhs = vm_read();
  locals[to] =
      vm_obj_num(vm_obj_to_num(locals[lhs]) - vm_obj_to_num(locals[rhs]));
  vm_jump_next();
}
do_mul : {
  vm_opcode_t to = vm_read();
  vm_opcode_t lhs = vm_read();
  vm_opcode_t rhs = vm_read();
  locals[to] =
      vm_obj_num(vm_obj_to_num(locals[lhs]) * vm_obj_to_num(locals[rhs]));
  vm_jump_next();
}
do_div : {
  vm_opcode_t to = vm_read();
  vm_opcode_t lhs = vm_read();
  vm_opcode_t rhs = vm_read();
  locals[to] =
      vm_obj_num(vm_obj_to_num(locals[lhs]) / vm_obj_to_num(locals[rhs]));
  vm_jump_next();
}
do_mod : {
  vm_opcode_t to = vm_read();
  vm_opcode_t lhs = vm_read();
  vm_opcode_t rhs = vm_read();
  locals[to] =
      vm_obj_num(vm_obj_to_num(locals[lhs]) % vm_obj_to_num(locals[rhs]));
  vm_jump_next();
}
do_call : {
  vm_opcode_t outreg = vm_read();
  vm_opcode_t next_func = vm_read();
  vm_opcode_t nargs = vm_read();
  vm_obj_t *next_locals = locals + frame->nlocals;
  for (vm_opcode_t argno = 1; argno <= nargs; argno++) {
    vm_opcode_t regno = vm_read();
    next_locals[argno] = locals[regno];
  }
  locals = next_locals;
  frame->index = index;
  frame->outreg = outreg;
  frame++;
  frame->nlocals = ops[next_func - 1];
  index = next_func;
  vm_jump_next();
}
do_return : {
  vm_opcode_t from = vm_read();
  vm_obj_t val = locals[from];
  frame--;
  locals = locals - frame->nlocals;
  vm_opcode_t outreg = frame->outreg;
  locals[outreg] = val;
  index = frame->index;
  vm_jump_next();
}
do_putchar : {
  vm_opcode_t from = vm_read();
  int val = (int) vm_obj_to_num(locals[from]);
  printf("%c", (int)val);
  vm_jump_next();
}
do_bb : {
  vm_opcode_t from = vm_read();
  if (vm_obj_to_num(locals[from])) {
    index = ops[index + 1];
    vm_jump_next();
  } else {
    index = ops[index];
    vm_jump_next();
  }
}
do_eq : {
  vm_opcode_t outreg = vm_read();
  vm_obj_t lhs = locals[vm_read()];
  vm_obj_t rhs = locals[vm_read()];
  locals[outreg] = vm_obj_num(vm_obj_to_num(lhs) == vm_obj_to_num(rhs));
  vm_jump_next();
}
do_lt : {
  vm_opcode_t outreg = vm_read();
  vm_obj_t lhs = locals[vm_read()];
  vm_obj_t rhs = locals[vm_read()];
  locals[outreg] = vm_obj_num(vm_obj_to_num(lhs) < vm_obj_to_num(rhs));
  vm_jump_next();
}
do_dcall : {
  vm_opcode_t outreg = vm_read();
  vm_opcode_t next_func_reg = vm_read();
  vm_opcode_t next_func = vm_obj_to_num(locals[next_func_reg]);
  vm_opcode_t nargs = vm_read();
  vm_obj_t *next_locals = locals + frame->nlocals;
  for (vm_opcode_t argno = 1; argno <= nargs; argno++) {
    vm_opcode_t regno = vm_read();
    next_locals[argno] = locals[regno];
  }
  locals = next_locals;
  frame->index = index;
  frame->outreg = outreg;
  frame++;
  frame->nlocals = ops[next_func - 1];
  index = next_func;
  vm_jump_next();
}
do_xcall : {
  vm_opcode_t outreg = vm_read();
  vm_opcode_t next_func = vm_read();
  vm_opcode_t inreg = vm_read();
  locals[outreg] = vm_run_ext(next_func, locals[inreg]);
  vm_jump_next();
}
do_cons : {
  vm_opcode_t outreg = vm_read();
  vm_obj_t car = locals[vm_read()];
  vm_obj_t cdr = locals[vm_read()];
  vm_obj_t *pair = vm_malloc(sizeof(vm_obj_t) * 2);
  pair[0] = car;
  pair[1] = cdr;
  locals[outreg] = vm_obj_pair(pair);
  vm_jump_next();
}
do_car : {
  vm_opcode_t outreg = vm_read();
  vm_obj_t pair = locals[vm_read()];
  locals[outreg] = vm_obj_car(pair);
  vm_jump_next();
}
do_cdr : {
  vm_opcode_t outreg = vm_read();
  vm_obj_t pair = locals[vm_read()];
  locals[outreg] = vm_obj_cdr(pair);
  vm_jump_next();
}
do_free : {
  vm_free(vm_obj_to_pair(locals[vm_read()]));
  vm_jump_next();
}
}