
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

#if defined(VM_SAFE)
#define VM_RETURN(x_) \
  ({ \
    vm_free(locals_base); \
    locals_base = NULL; \
    vm_free(frames_base); \
    frames_base = NULL; \
    return x_; \
  })
#define vm_read_at(where_) \
  ({ \
    vm_opcode_t ii = (where_);\
    if (ii >= nops || ii < 0) { \
      VM_RETURN(VM_RUN_UNK_ARG); \
    } \
    ops[ii]; \
  })
#define vm_read() vm_read_at(index++)
#define vm_jump_next() \
  ({ \
    vm_opcode_t op = vm_read(); \
    if (op >= VM_OPCODE_MAX) { \
      VM_RETURN(VM_RUN_UNK_OPCODE); \
    } \
    if (op < 0) { \
      VM_RETURN(VM_RUN_UNK_OPCODE); \
    } \
    goto *ptrs[op]; \
  })
#define vm_local(n_) \
  ({ \
    vm_obj_t *ret = &locals[n_];\
    if (ret < locals || locals_max <= ret) { \
      VM_RETURN(VM_RUN_BAD_LOCAL); \
    } \
    ret; \
  })
#else
#define VM_RETURN(x_) ({return(x_);})
#define vm_read_at(where_) (ops[(where_)])
#define vm_read() (vm_read_at(index++))
#define vm_jump_next() goto *ptrs[vm_read()]
#define vm_local(n_) (&locals[n_])
#endif

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
    [VM_OPCODE_INTF] = &&do_int,
  };
  vm_opcode_t index = 0;
  
  size_t locals_alloc = 256;
  vm_obj_t *locals_base = vm_malloc(sizeof(vm_obj_t) * locals_alloc);
  vm_obj_t *locals_max = locals_base + locals_alloc;
  vm_obj_t *locals = &locals_base[0];
  for (size_t i = 0; i < locals_alloc; i++) {
    locals_base[i] = vm_obj_num(0);
  }

  size_t frames_alloc = 8;
  vm_frame_t *frames_base = vm_malloc(sizeof(vm_frame_t) * frames_alloc);
  vm_frame_t *frames_max = frames_base + frames_alloc; 
  vm_frame_t *frame = &frames_base[0];
  frame->nlocals = 0;
  frame += 1;
  frame->nlocals = 16;
  vm_jump_next();
do_exit : {
  VM_RETURN(VM_RUN_OKAY);
}
do_reg : {
  vm_opcode_t to = vm_read();
  vm_opcode_t from = vm_read();
  *vm_local(to) = *vm_local(from);
  vm_jump_next();
}
do_int : {
  vm_opcode_t to = vm_read();
  vm_opcode_t from = vm_read();
  *vm_local(to) = vm_obj_num(from);
  vm_jump_next();
}
do_jump : {
  index = vm_read_at(index);
  vm_jump_next();
}
do_func : {
  index = vm_read_at(index);
  vm_jump_next();
}
do_add : {
  vm_opcode_t to = vm_read();
  vm_opcode_t lhs = vm_read();
  vm_opcode_t rhs = vm_read();
  *vm_local(to) =
      vm_obj_num(vm_obj_to_num(*vm_local(lhs)) + vm_obj_to_num(*vm_local(rhs)));
  vm_jump_next();
}
do_sub : {
  vm_opcode_t to = vm_read();
  vm_opcode_t lhs = vm_read();
  vm_opcode_t rhs = vm_read();
  *vm_local(to) =
      vm_obj_num(vm_obj_to_num(*vm_local(lhs)) - vm_obj_to_num(*vm_local(rhs)));
  vm_jump_next();
}
do_mul : {
  vm_opcode_t to = vm_read();
  vm_opcode_t lhs = vm_read();
  vm_opcode_t rhs = vm_read();
  *vm_local(to) =
      vm_obj_num(vm_obj_to_num(*vm_local(lhs)) * vm_obj_to_num(*vm_local(rhs)));
  vm_jump_next();
}
do_div : {
  vm_opcode_t to = vm_read();
  vm_opcode_t lhs = vm_read();
  vm_opcode_t rhs = vm_read();
  size_t lhsv = vm_obj_to_num(*vm_local(lhs));
  size_t rhsv = vm_obj_to_num(*vm_local(rhs));
#if defined(VM_SAFE)
  if (rhsv == 0) {
    VM_RETURN(VM_RUN_DIV_ZERO);
  }
#endif
  *vm_local(to) = vm_obj_num(lhsv / rhsv);
  vm_jump_next();
}
do_mod : {
  vm_opcode_t to = vm_read();
  vm_opcode_t lhs = vm_read();
  vm_opcode_t rhs = vm_read();
  size_t lhsv = vm_obj_to_num(*vm_local(lhs));
  size_t rhsv = vm_obj_to_num(*vm_local(rhs));
#if defined(VM_SAFE)
  if (rhsv == 0) {
    VM_RETURN(VM_RUN_MOD_ZERO);
  }
#endif
  *vm_local(to) = vm_obj_num(lhsv % rhsv);
  vm_jump_next();
}
do_call : {
  vm_opcode_t outreg = vm_read();
  vm_opcode_t next_func = vm_read();
  vm_opcode_t nargs = vm_read();
  size_t nregs = vm_read_at(next_func - 1);
  vm_obj_t *max_write = locals + frame->nlocals + nregs + nargs;
  if (max_write >= locals_max) {
    size_t nth = locals - locals_base;
    size_t needed = max_write - locals_base;
    size_t start = locals_alloc;
    while (locals_alloc <= needed) {
      locals_alloc *= 4;
    }
    locals_base = vm_realloc(locals_base, sizeof(vm_obj_t) * locals_alloc);
    for (size_t i = start; i < locals_alloc; i++) {
      locals_base[i] = vm_obj_num(0);
    }
    locals = &locals_base[nth];
    locals_max = locals_base + locals_alloc;
  }
  vm_obj_t *next_locals = locals + frame->nlocals;
  for (vm_opcode_t argno = 1; argno <= nargs; argno++) {
    vm_opcode_t regno = vm_read();
    next_locals[argno] = *vm_local(regno);
  }
  locals = next_locals;
  frame->index = index;
  frame->outreg = outreg;
  frame++;
  if (frame == frames_max) {
    size_t nth = frame - frames_base;
    frames_alloc *= 4;
    frames_base = vm_realloc(frames_base, sizeof(vm_frame_t) * frames_alloc);
    frame = &frames_base[nth];
    frames_max = frames_base + frames_alloc;
  }
  frame->nlocals = nregs;
  index = next_func;
  vm_jump_next();
}
do_return : {
  vm_opcode_t from = vm_read();
  vm_obj_t val = *vm_local(from);
  frame--;
  locals = locals - frame->nlocals;
  vm_opcode_t outreg = frame->outreg;
  *vm_local(outreg) = val;
  index = frame->index;
  vm_jump_next();
}
do_putchar : {
  vm_opcode_t from = vm_read();
  int val = (int) vm_obj_to_num(*vm_local(from));
  printf("%c", (int)val);
  vm_jump_next();
}
do_bb : {
  vm_opcode_t from = vm_read();
  if (vm_obj_to_num(*vm_local(from))) {
    index = vm_read_at(index + 1);
    vm_jump_next();
  } else {
    index = vm_read_at(index);
    vm_jump_next();
  }
}
do_eq : {
  vm_opcode_t outreg = vm_read();
  vm_obj_t lhs = *vm_local(vm_read());
  vm_obj_t rhs = *vm_local(vm_read());
  *vm_local(outreg) = vm_obj_num(vm_obj_to_num(lhs) == vm_obj_to_num(rhs));
  vm_jump_next();
}
do_lt : {
  vm_opcode_t outreg = vm_read();
  vm_obj_t lhs = *vm_local(vm_read());
  vm_obj_t rhs = *vm_local(vm_read());
  *vm_local(outreg) = vm_obj_num(vm_obj_to_num(lhs) < vm_obj_to_num(rhs));
  vm_jump_next();
}
do_dcall : {
  vm_opcode_t outreg = vm_read();
  vm_opcode_t next_func = vm_obj_to_num(*vm_local(vm_read()));
  vm_opcode_t nargs = vm_read();
  size_t nregs = vm_read_at(next_func - 1);
  vm_obj_t *max_write = locals + frame->nlocals + nregs + nargs;
  if (max_write >= locals_max) {
    size_t nth = locals - locals_base;
    size_t needed = max_write - locals_base;
    size_t start = locals_alloc;
    while (locals_alloc <= needed) {
      locals_alloc *= 4;
    }
    locals_base = vm_realloc(locals_base, sizeof(vm_obj_t) * locals_alloc);
    for (size_t i = start; i < locals_alloc; i++) {
      locals_base[i] = vm_obj_num(0);
    }
    locals = &locals_base[nth];
    locals_max = locals_base + locals_alloc;
  }
  vm_obj_t *next_locals = locals + frame->nlocals;
  for (vm_opcode_t argno = 1; argno <= nargs; argno++) {
    vm_opcode_t regno = vm_read();
    next_locals[argno] = *vm_local(regno);
  }
  locals = next_locals;
  frame->index = index;
  frame->outreg = outreg;
  frame++;
  if (frame == frames_max) {
    size_t nth = frame - frames_base;
    frames_alloc *= 4;
    frames_base = vm_realloc(frames_base, sizeof(vm_frame_t) * frames_alloc);
    frame = &frames_base[nth];
    frames_max = frames_base + frames_alloc;
  }
  frame->nlocals = nregs;
  index = next_func;
  vm_jump_next();
}
}