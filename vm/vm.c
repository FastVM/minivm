#include "vm.h"
#include "gc.h"
#include "libc.h"
#include "math.h"
#include "obj.h"

#if defined(VM_OS)
void os_putn(size_t n);
void os_puts(const char *str);
#endif

#if defined(VM_EMCC)
EM_JS(void, vm_do_eval, (size_t out, const char *src),
      { Module.vm_do_eval_func(out, UTF8ToString(src)); });
EM_JS(void, vm_do_file_put, (const char *src),
      { Module.vm_do_file_put_func(UTF8ToString(src)); });

#endif

#if defined(VM_DEBUG_OPCODE)
int printf(const char *, ...);
#define vm_debug_op(index_, op_)                                               \
  ({ printf("[%i: %i]\n", (int)index_, (int)op_); })
#else
#define vm_debug_op(index_, op_) ({})
#endif

#define vm_run_defer()                                                         \
  ({                                                                           \
    state->nlocals = locals - state->globals;                                  \
    state->index = index;                                                      \
    state->framenum = frame - state->frames;                                   \
    state->gc = *gc;                                                           \
    return state;                                                              \
  })

#define vm_run_next_op_forced()                                                \
  ({                                                                           \
    vm_debug_op(index, ops[index]);                                            \
    goto *jumps[index++];                                                      \
  })

#define vm_run_next_op() vm_run_next_op_forced()

#if defined(VM_BRANCH_DEFER)
#define vm_run_dec_defer()                                                     \
  ({                                                                           \
    if (remain-- <= 0)                                                         \
      vm_run_defer();                                                          \
    else                                                                       \
      vm_run_next_op();                                                        \
  })
#define vm_run_op(index_)                                                      \
  index = index_;                                                              \
  vm_run_dec_defer();

#else
#define vm_run_op(index_)                                                      \
  index = index_;                                                              \
  vm_run_next_op();
#endif

#define vm_read() (ops[index++])
#define vm_read_at(index_) (*(vm_opcode_t *)&ops[(index_)])

#define vm_run_write()                                                         \
  ({                                                                           \
    state->nlocals = locals - globals;                                         \
    state->index = index;                                                      \
    state->framenum = frame - state->frames;                                   \
  })

vm_save_t vm_state_to_save(vm_state_t *state) {
  vm_save_t save;
  vm_save_init(&save);
  vm_save_state(&save, state);
  vm_save_rewind(&save);
  return save;
}

vm_state_t *vm_run_save(vm_save_t save, size_t n, const vm_char_t *args[n]) {
  vm_state_t *state = vm_state_new(0, NULL);
  vm_save_get_state(&save, state);
  state->globals[0] = vm_state_global_from(&state->gc, n, args);
  return state;
}

VM_API vm_state_t *vm_run(vm_state_t *state) {
#if defined(VM_BRANCH_DEFER)
  int remain = 1000;
#endif
  const vm_opcode_t *ops = state->ops;
  vm_obj_t *const globals = state->globals;
  vm_obj_t *locals = globals + state->nlocals;
  size_t index = state->index;
  vm_stack_frame_t *frame = state->frames + state->framenum;
  vm_gc_t *const gc = &state->gc;
  static void *ptrs[] = {
      [VM_OPCODE_EXIT] = &&do_exit,
      [VM_OPCODE_STORE_REG] = &&do_store_reg,
      [VM_OPCODE_STORE_NONE] = &&do_store_none,
      [VM_OPCODE_STORE_BOOL] = &&do_store_bool,
      [VM_OPCODE_STORE_INT] = &&do_store_int,
      [VM_OPCODE_LOAD_GLOBAL] = &&do_load_global,
      [VM_OPCODE_SWAP_REG] = &&do_swap_reg,
      [VM_OPCODE_INDEX_GET_INT] = &&do_index_get_int,
      [VM_OPCODE_INDEX_SET_INT] = &&do_index_set_int,
      [VM_OPCODE_JUMP] = &&do_jump,
      [VM_OPCODE_FUNC] = &&do_func,
      [VM_OPCODE_ADD] = &&do_add,
      [VM_OPCODE_SUB] = &&do_sub,
      [VM_OPCODE_MUL] = &&do_mul,
      [VM_OPCODE_DIV] = &&do_div,
      [VM_OPCODE_MOD] = &&do_mod,
      [VM_OPCODE_POW] = &&do_pow,
      [VM_OPCODE_STATIC_CALL] = &&do_static_call,
      [VM_OPCODE_RETURN] = &&do_return,
      [VM_OPCODE_PUTCHAR] = &&do_putchar,
      [VM_OPCODE_STRING_NEW] = &&do_string_new,
      [VM_OPCODE_LENGTH] = &&do_length,
      [VM_OPCODE_INDEX_GET] = &&do_index_get,
      [VM_OPCODE_INDEX_SET] = &&do_index_set,
      [VM_OPCODE_EXEC] = &&do_exec,
      [VM_OPCODE_SAVE] = &&do_save,
      [VM_OPCODE_TYPE] = &&do_type,
      [VM_OPCODE_DUMP] = &&do_dump,
      [VM_OPCODE_WRITE] = &&do_write,
      [VM_OPCODE_READ] = &&do_read,
      [VM_OPCODE_STATIC_ARRAY_NEW] = &&do_static_array_new,
      [VM_OPCODE_STATIC_CONCAT] = &&do_static_concat,
      [VM_OPCODE_STATIC_CALL0] = &&do_static_call0,
      [VM_OPCODE_STATIC_CALL1] = &&do_static_call1,
      [VM_OPCODE_STATIC_CALL2] = &&do_static_call2,
      [VM_OPCODE_STATIC_CALL3] = &&do_static_call3,
      [VM_OPCODE_BRANCH_EQUAL] = &&do_branch_equal,
      [VM_OPCODE_BRANCH_NOT_EQUAL] = &&do_branch_not_equal,
      [VM_OPCODE_BRANCH_LESS] = &&do_branch_less,
      [VM_OPCODE_BRANCH_GREATER] = &&do_branch_greater,
      [VM_OPCODE_BRANCH_LESS_THAN_EQUAL] = &&do_branch_less_than_equal,
      [VM_OPCODE_BRANCH_GREATER_THAN_EQUAL] = &&do_branch_greater_than_equal,
      [VM_OPCODE_BRANCH_BOOL] = &&do_branch_bool,
      [VM_OPCODE_ADD_INT] = &&do_inc,
      [VM_OPCODE_SUB_INT] = &&do_dec,
      [VM_OPCODE_BRANCH_EQUAL_INT] = &&do_branch_equal_int,
      [VM_OPCODE_BRANCH_NOT_EQUAL_INT] = &&do_branch_not_equal_int,
      [VM_OPCODE_BRANCH_LESS_INT] = &&do_branch_less_int,
      [VM_OPCODE_BRANCH_GREATER_INT] = &&do_branch_greater_int,
      [VM_OPCODE_BRANCH_LESS_THAN_EQUAL_INT] = &&do_branch_less_than_equal_int,
      [VM_OPCODE_BRANCH_GREATER_THAN_EQUAL_INT] =
          &&do_branch_greater_than_equal_int,
      [VM_OPCODE_MUL_INT] = &&do_mul_int,
      [VM_OPCODE_DIV_INT] = &&do_div_int,
      [VM_OPCODE_MOD_INT] = &&do_mod_int,
      [VM_OPCODE_POW_INT] = &&do_pow_int,
  };
  if (state->jumps == NULL) {
    vm_state_ptrs(state, ptrs);
  }
  void *const *const jumps = (void *const *const)state->jumps;
  vm_run_next_op_forced();
do_exit : {
  vm_state_del(state);
  return NULL;
}
do_return : {
  vm_reg_t from = vm_read();
  vm_obj_t val = locals[from];
  frame--;
  locals = locals - frame->nlocals;
  vm_reg_t outreg = frame->outreg;
  locals[outreg] = val;
  vm_run_op(frame->index);
}
do_store_none : {
  vm_reg_t to = vm_read();
  locals[to] = vm_obj_of_none();
  vm_run_next_op();
}
do_store_bool : {
  vm_reg_t to = vm_read();
  vm_int_t from = (int)vm_read();
  locals[to] = vm_obj_of_bool((bool)from);
  vm_run_next_op();
}
do_store_reg : {
  vm_reg_t to = vm_read();
  vm_reg_t from = vm_read();
  locals[to] = locals[from];
  vm_run_next_op();
}
do_store_int : {
  vm_reg_t to = vm_read();
  vm_int_t from = vm_read();
  locals[to] = vm_obj_of_num(from);
  vm_run_next_op();
}
do_load_global : {
  vm_reg_t to = vm_read();
  vm_reg_t from = vm_read();
  locals[to] = globals[from];
  vm_run_next_op();
}
do_swap_reg : {
  vm_reg_t r1 = vm_read();
  vm_reg_t r2 = vm_read();
  vm_obj_t v1 = locals[r1];
  vm_obj_t v2 = locals[r2];
  locals[r1] = v2;
  locals[r2] = v1;
  vm_run_next_op();
}
do_index_get_int : {
  vm_reg_t outreg = vm_read();
  vm_reg_t reg = vm_read();
  vm_reg_t ind = vm_read();
  vm_obj_t vec = locals[reg];
  locals[outreg] = vm_gc_get_index(gc, vm_obj_to_ptr(gc, vec), ind);
  vm_run_next_op();
}
do_index_set_int : {
  vm_reg_t reg = vm_read();
  vm_reg_t ind = vm_read();
  vm_reg_t val = vm_read();
  vm_obj_t vec = locals[reg];
  vm_obj_t value = locals[val];
  vm_gc_set_index(gc, vm_obj_to_ptr(gc, vec), ind, value);
  vm_run_next_op();
}
do_jump : {
  vm_loc_t to = vm_read();
  vm_run_op(to);
}
do_func : {
  vm_loc_t to = vm_read();
  vm_run_op(to);
}
do_add : {
  vm_reg_t to = vm_read();
  vm_reg_t lhs = vm_read();
  vm_reg_t rhs = vm_read();
  locals[to] = vm_obj_num_add(locals[lhs], locals[rhs]);
  vm_run_next_op();
}
do_sub : {
  vm_reg_t to = vm_read();
  vm_reg_t lhs = vm_read();
  vm_reg_t rhs = vm_read();
  locals[to] = vm_obj_num_sub(locals[lhs], locals[rhs]);
  vm_run_next_op();
}
do_mul : {
  vm_reg_t to = vm_read();
  vm_reg_t lhs = vm_read();
  vm_reg_t rhs = vm_read();
  locals[to] = vm_obj_num_mul(locals[lhs], locals[rhs]);
  vm_run_next_op();
}
do_div : {
  vm_reg_t to = vm_read();
  vm_reg_t lhs = vm_read();
  vm_reg_t rhs = vm_read();
  locals[to] = vm_obj_num_div(locals[lhs], locals[rhs]);
  vm_run_next_op();
}
do_mod : {
  vm_reg_t to = vm_read();
  vm_reg_t lhs = vm_read();
  vm_reg_t rhs = vm_read();
  locals[to] = vm_obj_num_mod(locals[lhs], locals[rhs]);
  vm_run_next_op();
}
do_pow : {
  vm_reg_t to = vm_read();
  vm_reg_t lhs = vm_read();
  vm_reg_t rhs = vm_read();
  locals[to] = vm_obj_num_pow(locals[lhs], locals[rhs]);
  vm_run_next_op();
}
do_static_call : {
  vm_reg_t outreg = vm_read();
  vm_loc_t next_func = vm_read();
  vm_int_t nargs = vm_read();
  vm_obj_t *next_locals = locals + frame->nlocals;
  for (vm_int_t argno = 1; argno <= nargs; argno++) {
    vm_reg_t regno = vm_read();
    next_locals[argno] = locals[regno];
  }
  locals = next_locals;
  frame->index = index;
  frame->outreg = outreg;
  frame++;
  frame->nlocals = vm_read_at(next_func - 1);
  vm_run_op(next_func);
}
do_putchar : {
  vm_reg_t from = vm_read();
  vm_int_t val = vm_obj_to_num(locals[from]);
  vm_putchar(val);
  vm_run_next_op();
}
do_string_new : {
  vm_gc_run1(gc, globals);
  vm_reg_t outreg = vm_read();
  vm_int_t nargs = vm_read();
  vm_gc_entry_t *str = vm_gc_static_array_new(gc, nargs);
  for (size_t i = 0; i < nargs; i++) {
    vm_number_t num = vm_read();
    vm_gc_set_index(gc, str, i, vm_obj_of_num(num));
  }
  locals[outreg] = vm_obj_of_ptr(gc, str);
  vm_run_next_op();
}
do_length : {
  vm_reg_t outreg = vm_read();
  vm_reg_t reg = vm_read();
  vm_obj_t vec = locals[reg];
  locals[outreg] = vm_obj_of_num(vm_gc_sizeof(gc, vm_obj_to_ptr(gc, vec)));
  vm_run_next_op();
}
do_index_get : {
  vm_reg_t outreg = vm_read();
  vm_reg_t reg = vm_read();
  vm_reg_t ind = vm_read();
  vm_obj_t vec = locals[reg];
  vm_obj_t oindex = locals[ind];
  locals[outreg] =
      vm_gc_get_index(gc, vm_obj_to_ptr(gc, vec), vm_obj_to_num(oindex));
  vm_run_next_op();
}
do_index_set : {
  vm_reg_t reg = vm_read();
  vm_reg_t ind = vm_read();
  vm_reg_t val = vm_read();
  vm_obj_t vec = locals[reg];
  vm_obj_t oindex = locals[ind];
  vm_obj_t value = locals[val];
  vm_gc_set_index(gc, vm_obj_to_ptr(gc, vec), vm_obj_to_num(oindex), value);
  vm_run_next_op();
}
do_type : {
  vm_reg_t outreg = vm_read();
  vm_reg_t valreg = vm_read();
  vm_obj_t obj = locals[valreg];
  vm_number_t num = -1;
  if (vm_obj_is_none(obj)) {
    num = 0;
  }
  if (vm_obj_is_bool(obj)) {
    num = 1;
  }
  if (vm_obj_is_num(obj)) {
    num = 2;
  }
  if (vm_obj_is_ptr(obj)) {
    num = 3;
  }
  locals[outreg] = vm_obj_of_num(num);
  vm_run_next_op();
}
do_exec : {
  vm_reg_t in = vm_read();
  vm_reg_t argreg = vm_read();
  vm_gc_entry_t *ent = vm_obj_to_ptr(gc, locals[in]);
  vm_int_t xlen = vm_gc_sizeof(gc, ent);
  vm_opcode_t *xops = vm_malloc(sizeof(vm_opcode_t) * xlen);
  for (vm_int_t i = 0; i < xlen; i++) {
    vm_obj_t obj = vm_gc_get_index(gc, ent, i);
    vm_number_t n = vm_obj_to_num(obj);
    xops[i] = (vm_opcode_t)n;
  }
  vm_state_t *xstate = vm_state_new(0, NULL);
  xstate->globals[0] = vm_gc_dup(&xstate->gc, gc, locals[argreg]);
  vm_state_set_ops(xstate, xlen, xops);
  vm_state_del(state);
  return xstate;
}
do_save : {
  vm_reg_t namreg = vm_read();
  vm_run_write();

  vm_gc_entry_t *sname = vm_obj_to_ptr(gc, locals[namreg]);
  locals[namreg] = vm_obj_of_none();
  gc->max = 0;
  vm_gc_run1(gc, globals);
  vm_int_t slen = vm_gc_sizeof(gc, sname);
  vm_char_t *name = vm_malloc(sizeof(vm_char_t) * (slen + 1));
  for (vm_int_t i = 0; i < slen; i++) {
    vm_obj_t obj = vm_gc_get_index(gc, sname, i);
    name[i] = vm_obj_to_num(obj);
  }
  name[slen] = '\0';

  vm_save_t save;
  vm_save_init(&save);
  vm_save_state(&save, state);
  FILE *out = fopen(name, "wb");
  vm_free(name);
  uint8_t z = 0;
  fwrite(&z, 1, 1, out);
  fwrite(save.str, 1, save.len, out);
  fclose(out);
  vm_save_deinit(&save);
  vm_state_del(state);
  return NULL;
}
do_dump : {
  vm_reg_t namreg = vm_read();
  vm_reg_t inreg = vm_read();

  vm_gc_entry_t *sname = vm_obj_to_ptr(gc, locals[namreg]);
  vm_int_t slen = vm_gc_sizeof(gc, sname);
  vm_char_t *name = vm_malloc(sizeof(vm_char_t) * (slen + 1));
  for (vm_int_t i = 0; i < slen; i++) {
    vm_obj_t obj = vm_gc_get_index(gc, sname, i);
    name[i] = vm_obj_to_num(obj);
  }
  name[slen] = '\0';

  vm_gc_entry_t *ent = vm_obj_to_ptr(gc, locals[inreg]);
  uint8_t size = sizeof(vm_opcode_t);
  vm_int_t xlen = vm_gc_sizeof(gc, ent);
  FILE *out = fopen(name, "wb");
  vm_free(name);
  fwrite(&size, 1, 1, out);
  for (vm_int_t i = 0; i < xlen; i++) {
    vm_obj_t obj = vm_gc_get_index(gc, ent, i);
    vm_opcode_t op = vm_obj_to_num(obj);
    fwrite(&op, sizeof(vm_opcode_t), 1, out);
  }
  fclose(out);
  vm_run_next_op();
}
do_read : {
  vm_reg_t outreg = vm_read();
  vm_reg_t namereg = vm_read();
  vm_gc_entry_t *sname = vm_obj_to_ptr(gc, locals[namereg]);
  vm_int_t slen = vm_gc_sizeof(gc, sname);
  vm_char_t *name = vm_malloc(sizeof(vm_char_t) * (slen + 1));
  for (vm_int_t i = 0; i < slen; i++) {
    vm_obj_t obj = vm_gc_get_index(gc, sname, i);
    name[i] = vm_obj_to_num(obj);
  }
  name[slen] = '\0';
  if (name[0] == '#') {
    locals[outreg] = vm_obj_of_none();
#if defined(VM_EMCC)
    vm_do_eval(locals - globals + outreg, name + 1);
    vm_free(name);
    vm_run_defer();
#else
    vm_run_next_op();
#endif
  } else {
#if defined(VM_EMCC)
    vm_do_file_put(name);
#endif
    vm_int_t where = 0;
    vm_int_t nalloc = 64;
    FILE *in = fopen(name, "rb");
    vm_free(name);
    if (in == NULL) {
      locals[outreg] = vm_obj_of_none();
      vm_run_next_op();
    }
    vm_char_t *str = vm_malloc(sizeof(vm_char_t) * nalloc);
    while (true) {
      uint8_t buf[2048];
      vm_int_t n = fread(buf, 1, 2048, in);
      for (vm_int_t i = 0; i < n; i++) {
        if (where + 4 >= nalloc) {
          nalloc = 4 + nalloc * 2;
          str = vm_realloc(str, sizeof(vm_char_t) * nalloc);
        }
        str[where] = buf[i];
        where += 1;
      }
      if (n < 2048) {
        break;
      }
    }
    fclose(in);
    vm_gc_entry_t *ent = vm_gc_static_array_new(gc, where);
    for (vm_int_t i = 0; i < where; i++) {
      vm_gc_set_index(gc, ent, i, vm_obj_of_num(str[i]));
    }
    vm_free(str);
    locals[outreg] = vm_obj_of_ptr(gc, ent);
    vm_run_next_op();
  }
}
do_write : {
  vm_reg_t outreg = vm_read();
  vm_reg_t inreg = vm_read();
  vm_gc_entry_t *sname = vm_obj_to_ptr(gc, locals[outreg]);
  vm_int_t slen = vm_gc_sizeof(gc, sname);
  vm_char_t *name = vm_malloc(sizeof(vm_char_t) * (slen + 1));
  for (vm_int_t i = 0; i < slen; i++) {
    vm_obj_t obj = vm_gc_get_index(gc, sname, i);
    name[i] = vm_obj_to_num(obj);
  }
  name[slen] = '\0';
  vm_gc_entry_t *ent = vm_obj_to_ptr(gc, locals[inreg]);
  vm_int_t xlen = vm_gc_sizeof(gc, ent);
  FILE *out = fopen(name, "wb");
  vm_free(name);
  for (vm_int_t i = 0; i < xlen; i++) {
    vm_obj_t obj = vm_gc_get_index(gc, ent, i);
    uint8_t op = vm_obj_to_num(obj);
    fwrite(&op, 1, sizeof(uint8_t), out);
  }
  fclose(out);
  vm_run_next_op();
}
do_static_array_new : {
  vm_gc_run1(gc, globals);
  vm_reg_t outreg = vm_read();
  vm_int_t nargs = vm_read();
  vm_gc_entry_t *vec = vm_gc_static_array_new(gc, nargs);
  for (vm_int_t i = 0; i < nargs; i++) {
    vm_reg_t vreg = vm_read();
    vm_gc_set_index(gc, vec, i, locals[vreg]);
  }
  locals[outreg] = vm_obj_of_ptr(gc, vec);
  vm_run_next_op();
}
do_static_concat : {
  vm_gc_run1(gc, globals);
  vm_reg_t to = vm_read();
  vm_reg_t lhs = vm_read();
  vm_reg_t rhs = vm_read();
  vm_obj_t o1 = locals[lhs];
  vm_obj_t o2 = locals[rhs];
  locals[to] = vm_gc_static_concat(gc, o1, o2);
  vm_run_next_op();
}
do_static_call0 : {
  vm_reg_t outreg = vm_read();
  vm_loc_t next_func = vm_read();
  vm_obj_t *next_locals = locals + frame->nlocals;
  locals = next_locals;
  frame->index = index;
  frame->outreg = outreg;
  frame++;
  frame->nlocals = vm_read_at(next_func - 1);
  vm_run_op(next_func);
}
do_static_call1 : {
  vm_reg_t outreg = vm_read();
  vm_loc_t next_func = vm_read();
  vm_obj_t *next_locals = locals + frame->nlocals;
  next_locals[1] = locals[vm_read()];
  locals = next_locals;
  frame->index = index;
  frame->outreg = outreg;
  frame++;
  frame->nlocals = vm_read_at(next_func - 1);
  vm_run_op(next_func);
}
do_static_call2 : {
  vm_reg_t outreg = vm_read();
  vm_loc_t next_func = vm_read();
  vm_obj_t *next_locals = locals + frame->nlocals;
  next_locals[1] = locals[vm_read()];
  next_locals[2] = locals[vm_read()];
  locals = next_locals;
  frame->index = index;
  frame->outreg = outreg;
  frame++;
  frame->nlocals = vm_read_at(next_func - 1);
  vm_run_op(next_func);
}
do_static_call3 : {
  vm_reg_t outreg = vm_read();
  vm_loc_t next_func = vm_read();
  vm_obj_t *next_locals = locals + frame->nlocals;
  next_locals[1] = locals[vm_read()];
  next_locals[2] = locals[vm_read()];
  next_locals[3] = locals[vm_read()];
  locals = next_locals;
  frame->index = index;
  frame->outreg = outreg;
  frame++;
  frame->nlocals = vm_read_at(next_func - 1);
  vm_run_op(next_func);
}
do_branch_equal : {
  vm_obj_t lhs = locals[vm_read()];
  vm_obj_t rhs = locals[vm_read()];
  if (vm_obj_eq(gc, lhs, rhs)) {
    vm_loc_t jt = vm_read_at(index + 1);
    vm_run_op(jt);
  } else {
    vm_loc_t jf = vm_read_at(index);
    vm_run_op(jf);
  }
}
do_branch_not_equal : {
  vm_obj_t lhs = locals[vm_read()];
  vm_obj_t rhs = locals[vm_read()];
  if (vm_obj_neq(gc, lhs, rhs)) {
    vm_loc_t jt = vm_read_at(index + 1);
    vm_run_op(jt);
  } else {
    vm_loc_t jf = vm_read_at(index);
    vm_run_op(jf);
  }
}
do_branch_less : {
  vm_obj_t lhs = locals[vm_read()];
  vm_obj_t rhs = locals[vm_read()];
  if (vm_obj_lt(lhs, rhs)) {
    vm_loc_t jt = vm_read_at(index + 1);
    vm_run_op(jt);
  } else {
    vm_loc_t jf = vm_read_at(index);
    vm_run_op(jf);
  }
}
do_branch_greater : {
  vm_obj_t lhs = locals[vm_read()];
  vm_obj_t rhs = locals[vm_read()];
  if (vm_obj_gt(lhs, rhs)) {
    vm_loc_t jt = vm_read_at(index + 1);
    vm_run_op(jt);
  } else {
    vm_loc_t jf = vm_read_at(index);
    vm_run_op(jf);
  }
}
do_branch_less_than_equal : {
  vm_obj_t lhs = locals[vm_read()];
  vm_obj_t rhs = locals[vm_read()];
  if (vm_obj_lte(lhs, rhs)) {
    vm_loc_t jt = vm_read_at(index + 1);
    vm_run_op(jt);
  } else {
    vm_loc_t jf = vm_read_at(index);
    vm_run_op(jf);
  }
}
do_branch_greater_than_equal : {
  vm_obj_t lhs = locals[vm_read()];
  vm_obj_t rhs = locals[vm_read()];
  if (vm_obj_gte(lhs, rhs)) {
    vm_loc_t jt = vm_read_at(index + 1);
    vm_run_op(jt);
  } else {
    vm_loc_t jf = vm_read_at(index);
    vm_run_op(jf);
  }
}
do_branch_bool : {
  vm_reg_t from = vm_read();
  if (vm_obj_to_bool(locals[from])) {
    vm_loc_t jt = vm_read_at(index + 1);
    vm_run_op(jt);
  } else {
    vm_loc_t jf = vm_read_at(index);
    vm_run_op(jf);
  }
}
do_inc : {
  vm_reg_t to = vm_read();
  vm_reg_t lhs = vm_read();
  vm_int_t rhs = vm_read();
  locals[to] = vm_obj_num_addc(locals[lhs], rhs);
  vm_run_next_op();
}
do_dec : {
  vm_reg_t to = vm_read();
  vm_reg_t lhs = vm_read();
  vm_int_t rhs = vm_read();
  locals[to] = vm_obj_num_subc(locals[lhs], rhs);
  vm_run_next_op();
}
do_branch_equal_int : {
  vm_obj_t lhs = locals[vm_read()];
  vm_int_t rhs = vm_read();
  if (vm_obj_ieq(lhs, rhs)) {
    vm_loc_t jt = vm_read_at(index + 1);
    vm_run_op(jt);
  } else {
    vm_loc_t jf = vm_read_at(index);
    vm_run_op(jf);
  }
}
do_branch_not_equal_int : {
  vm_obj_t lhs = locals[vm_read()];
  vm_int_t rhs = vm_read();
  if (vm_obj_ineq(lhs, rhs)) {
    vm_loc_t jt = vm_read_at(index + 1);
    vm_run_op(jt);
  } else {
    vm_loc_t jf = vm_read_at(index);
    vm_run_op(jf);
  }
}
do_branch_less_int : {
  vm_obj_t lhs = locals[vm_read()];
  vm_int_t rhs = vm_read();
  if (vm_obj_ilt(lhs, rhs)) {
    vm_loc_t jt = vm_read_at(index + 1);
    vm_run_op(jt);
  } else {
    vm_loc_t jf = vm_read_at(index);
    vm_run_op(jf);
  }
}
do_branch_greater_int : {
  vm_obj_t lhs = locals[vm_read()];
  vm_int_t rhs = vm_read();
  if (vm_obj_igt(lhs, rhs)) {
    vm_loc_t jt = vm_read_at(index + 1);
    vm_run_op(jt);
  } else {
    vm_loc_t jf = vm_read_at(index);
    vm_run_op(jf);
  }
}
do_branch_less_than_equal_int : {
  vm_obj_t lhs = locals[vm_read()];
  vm_int_t rhs = vm_read();
  if (vm_obj_ilte(lhs, rhs)) {
    vm_loc_t jt = vm_read_at(index + 1);
    vm_run_op(jt);
  } else {
    vm_loc_t jf = vm_read_at(index);
    vm_run_op(jf);
  }
}
do_branch_greater_than_equal_int : {
  vm_obj_t lhs = locals[vm_read()];
  vm_int_t rhs = vm_read();
  if (vm_obj_igte(lhs, rhs)) {
    vm_loc_t jt = vm_read_at(index + 1);
    vm_run_op(jt);
  } else {
    vm_loc_t jf = vm_read_at(index);
    vm_run_op(jf);
  }
}
do_mul_int : {
  vm_reg_t to = vm_read();
  vm_reg_t lhs = vm_read();
  vm_reg_t rhs = vm_read();
  locals[to] = vm_obj_num_mulc(locals[lhs], rhs);
  vm_run_next_op();
}
do_div_int : {
  vm_reg_t to = vm_read();
  vm_reg_t lhs = vm_read();
  vm_reg_t rhs = vm_read();
  locals[to] = vm_obj_num_divc(locals[lhs], rhs);
  vm_run_next_op();
}
do_mod_int : {
  vm_reg_t to = vm_read();
  vm_reg_t lhs = vm_read();
  vm_reg_t rhs = vm_read();
  locals[to] = vm_obj_num_modc(locals[lhs], rhs);
  vm_run_next_op();
}
do_pow_int : {
  vm_reg_t to = vm_read();
  vm_reg_t lhs = vm_read();
  vm_reg_t rhs = vm_read();
  locals[to] = vm_obj_num_powc(locals[lhs], rhs);
  vm_run_next_op();
}
}