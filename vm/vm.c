
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
#include "gc.h"
#include "lib.h"

#define vm_read() (ops[index++].arg)
#define vm_jump_next() goto *ops[index++].op

typedef struct {
  size_t outreg;
  size_t nlocals;
  size_t index;
} vm_frame_t;

/// Creates the object in r0 at the beginning of the program.
vm_obj_t vm_global_from(vm_gc_t *gc, size_t len, const char **args) {
  // Just some minivm arrays.
  vm_obj_t global = vm_gc_new(gc, len);
  for (size_t i = 0; i < len; i++) {
    vm_obj_t ent = vm_gc_new(gc, vm_strlen(args[i]));
    for (const char *src = args[i]; *src != '\0'; src++) {
      vm_gc_set(gc, ent, src - args[i], vm_obj_num(*src));
    }
    vm_gc_set(gc, global, i, ent);
  }
  return global;
}

int vm_table_opt(size_t nops, vm_opcode_t *ops, void *const *const ptrs) {
  for (size_t i = 0; i < nops; ++i) {
    size_t op = ops[i].arg;
    ops[i].op = ptrs[op];
    switch (op) {
    case VM_OPCODE_EXIT:
      break;
    case VM_OPCODE_REG:
      i += 2;
      break;
    case VM_OPCODE_BB:
      i += 3;
      break;
    case VM_OPCODE_INT:
      i += 2;
      break;
    case VM_OPCODE_JUMP:
      i += 1;
      break;
    case VM_OPCODE_FUNC: {
      size_t namelen = ops[i + 3].arg;
      i += 1 + 2 + namelen + 1;
    } break;
    case VM_OPCODE_ADD:
      i += 3;
      break;
    case VM_OPCODE_SUB:
      i += 3;
      break;
    case VM_OPCODE_MUL:
      i += 3;
      break;
    case VM_OPCODE_DIV:
      i += 3;
      break;
    case VM_OPCODE_MOD:
      i += 3;
      break;
    case VM_OPCODE_POW:
      i += 3;
      break;
    case VM_OPCODE_CALL: {
      size_t nargs = ops[i + 3].arg;
      i += 3 + nargs;
    } break;
    case VM_OPCODE_RETURN:
      i += 1;
      break;
    case VM_OPCODE_PUTCHAR:
      i += 1;
      break;
    case VM_OPCODE_STRING: {
      size_t nargs = ops[i + 2].arg;
      i += 2 + nargs;
    } break;
    case VM_OPCODE_LENGTH:
      i += 2;
      break;
    case VM_OPCODE_GET:
      i += 3;
      break;
    case VM_OPCODE_SET:
      i += 3;
      break;
    case VM_OPCODE_DUMP:
      i += 2;
      break;
    case VM_OPCODE_READ:
      i += 2;
      break;
    case VM_OPCODE_WRITE:
      i += 2;
      break;
    case VM_OPCODE_ARRAY: {
      size_t nargs = ops[i + 2].arg;
      i += 2 + nargs;
    } break;
    case VM_OPCODE_CAT:
      i += 3;
      break;
    case VM_OPCODE_BEQ:
      i += 4;
      break;
    case VM_OPCODE_BLT:
      i += 4;
      break;
    case VM_OPCODE_ADDI:
      i += 3;
      break;
    case VM_OPCODE_SUBI:
      i += 3;
      break;
    case VM_OPCODE_MULI:
      i += 3;
      break;
    case VM_OPCODE_DIVI:
      i += 3;
      break;
    case VM_OPCODE_MODI:
      i += 3;
      break;
    case VM_OPCODE_CALL0:
      i += 2;
      break;
    case VM_OPCODE_CALL1:
      i += 3;
      break;
    case VM_OPCODE_CALL2:
      i += 4;
      break;
    case VM_OPCODE_CALL3:
      i += 5;
      break;
    case VM_OPCODE_GETI:
      i += 3;
      break;
    case VM_OPCODE_SETI:
      i += 3;
      break;
    case VM_OPCODE_BEQI:
      i += 4;
      break;
    case VM_OPCODE_BLTI:
      i += 4;
      break;
    case VM_OPCODE_BLTEI:
      i += 4;
      break;
    case VM_OPCODE_CALL_DYN: {
      size_t nargs = ops[i + 3].arg;
      i += 3 + nargs;
    } break;
    case VM_OPCODE_CALL_EXT:
      i += 3;
      break;
    default:
      printf("unknown opcode: %p\n", ops[i].op);
      return 1;
    }
  }
  return 0;
}

/// VM hot loop
int vm_run_from(vm_gc_t *gc, size_t nops, vm_opcode_t *ops, vm_obj_t globals) {
  // our dear jump table
  static void *const ptrs[] = {
      [VM_OPCODE_EXIT] = &&do_exit,         [VM_OPCODE_REG] = &&do_store_reg,
      [VM_OPCODE_BB] = &&do_branch_bool,    [VM_OPCODE_INT] = &&do_store_int,
      [VM_OPCODE_JUMP] = &&do_jump,         [VM_OPCODE_FUNC] = &&do_func,
      [VM_OPCODE_ADD] = &&do_add,           [VM_OPCODE_SUB] = &&do_sub,
      [VM_OPCODE_MUL] = &&do_mul,           [VM_OPCODE_DIV] = &&do_div,
      [VM_OPCODE_MOD] = &&do_mod,           [VM_OPCODE_POW] = &&do_pow,
      [VM_OPCODE_CALL] = &&do_call,         [VM_OPCODE_RETURN] = &&do_return,
      [VM_OPCODE_PUTCHAR] = &&do_putchar,   [VM_OPCODE_STRING] = &&do_string,
      [VM_OPCODE_LENGTH] = &&do_length,     [VM_OPCODE_GET] = &&do_get,
      [VM_OPCODE_SET] = &&do_set,           [VM_OPCODE_DUMP] = &&do_dump,
      [VM_OPCODE_READ] = &&do_read,         [VM_OPCODE_WRITE] = &&do_write,
      [VM_OPCODE_ARRAY] = &&do_array,       [VM_OPCODE_CAT] = &&do_cat,
      [VM_OPCODE_BEQ] = &&do_beq,           [VM_OPCODE_BLT] = &&do_blt,
      [VM_OPCODE_ADDI] = &&do_addi,         [VM_OPCODE_SUBI] = &&do_subi,
      [VM_OPCODE_MULI] = &&do_muli,         [VM_OPCODE_DIVI] = &&do_divi,
      [VM_OPCODE_MODI] = &&do_modi,         [VM_OPCODE_CALL0] = &&do_call0,
      [VM_OPCODE_CALL1] = &&do_call1,       [VM_OPCODE_CALL2] = &&do_call2,
      [VM_OPCODE_CALL3] = &&do_call3,       [VM_OPCODE_GETI] = &&do_geti,
      [VM_OPCODE_SETI] = &&do_seti,         [VM_OPCODE_BEQI] = &&do_beqi,
      [VM_OPCODE_BLTI] = &&do_blti,         [VM_OPCODE_BLTEI] = &&do_bltei,
      [VM_OPCODE_CALL_DYN] = &&do_call_dyn, [VM_OPCODE_CALL_EXT] = &&do_call_ext,
  };
  if (vm_table_opt(nops, ops, ptrs)) {
    return 1;
  }
  size_t index = 0;
  vm_obj_t *locals_base = vm_calloc(sizeof(vm_obj_t) * (1 << 16));
  // for (size_t i = 0; i < (1 << 16); i++) {
  //   locals_base[i] = 0;
  // }
  vm_gc_set_locals(gc, (1 << 16), locals_base);
  vm_obj_t *locals = locals_base;
  locals[0] = globals;
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
do_return : {
  size_t from = vm_read();
  vm_obj_t val = locals[from];
  frame--;
  locals = locals - frame->nlocals;
  size_t outreg = frame->outreg;
  locals[outreg] = val;
  index = frame->index;
  vm_jump_next();
}
do_branch_bool : {
  size_t from = vm_read();
  if (vm_obj_to_num(locals[from])) {
    index = ops[index + 1].arg;
    vm_jump_next();
  } else {
    index = ops[index].arg;
    vm_jump_next();
  }
}
do_store_reg : {
  size_t to = vm_read();
  size_t from = vm_read();
  locals[to] = locals[from];
  vm_jump_next();
}
do_store_int : {
  size_t to = vm_read();
  size_t from = vm_read();
  locals[to] = vm_obj_num(from);
  vm_jump_next();
}
do_jump : {
  index = ops[index].arg;
  vm_jump_next();
}
do_func : {
  index = ops[index].arg;
  vm_jump_next();
}
do_add : {
  size_t to = vm_read();
  size_t lhs = vm_read();
  size_t rhs = vm_read();
  locals[to] =
      vm_obj_num(vm_obj_to_num(locals[lhs]) + vm_obj_to_num(locals[rhs]));
  vm_jump_next();
}
do_sub : {
  size_t to = vm_read();
  size_t lhs = vm_read();
  size_t rhs = vm_read();
  locals[to] =
      vm_obj_num(vm_obj_to_num(locals[lhs]) - vm_obj_to_num(locals[rhs]));
  vm_jump_next();
}
do_mul : {
  size_t to = vm_read();
  size_t lhs = vm_read();
  size_t rhs = vm_read();
  locals[to] =
      vm_obj_num(vm_obj_to_num(locals[lhs]) * vm_obj_to_num(locals[rhs]));
  vm_jump_next();
}
do_div : {
  size_t to = vm_read();
  size_t lhs = vm_read();
  size_t rhs = vm_read();
  locals[to] =
      vm_obj_num(vm_obj_to_num(locals[lhs]) / vm_obj_to_num(locals[rhs]));
  vm_jump_next();
}
do_mod : {
  size_t to = vm_read();
  size_t lhs = vm_read();
  size_t rhs = vm_read();
  locals[to] =
      vm_obj_num(vm_obj_to_num(locals[lhs]) % vm_obj_to_num(locals[rhs]));
  vm_jump_next();
}
do_pow : {
  size_t to = vm_read();
  size_t lhs = vm_read();
  size_t rhs = vm_read();
  vm_number_t base = vm_obj_to_num(locals[lhs]);
  vm_number_t exp = vm_obj_to_num(locals[rhs]);
  vm_number_t result = 1;
  for (;;) {
    if (exp & 1) {
      result *= base;
    }
    exp >>= 1;
    if (!exp) {
      break;
    }
    base *= base;
  }
  locals[to] = vm_obj_num(result);
  vm_jump_next();
}
do_call : {
  size_t outreg = vm_read();
  size_t next_func = vm_read();
  size_t nargs = vm_read();
  vm_obj_t *next_locals = locals + frame->nlocals;
  for (size_t argno = 1; argno <= nargs; argno++) {
    size_t regno = vm_read();
    next_locals[argno] = locals[regno];
  }
  locals = next_locals;
  frame->index = index;
  frame->outreg = outreg;
  frame++;
  frame->nlocals = ops[next_func - 1].arg;
  index = next_func;
  vm_jump_next();
}
do_putchar : {
  size_t from = vm_read();
  vm_number_t val = vm_obj_to_num(locals[from]);
  printf("%c", (int)val);
  vm_jump_next();
}
do_string : {
  size_t outreg = vm_read();
  size_t nargs = vm_read();
  vm_obj_t str = vm_gc_new(gc, nargs);
  for (size_t i = 0; i < nargs; i++) {
    size_t num = vm_read();
    vm_gc_set(gc, str, i, vm_obj_num(num));
  }
  locals[outreg] = str;
  vm_gc_collect(gc);
  vm_jump_next();
}
do_length : {
  size_t outreg = vm_read();
  vm_obj_t vec = locals[vm_read()];
  locals[outreg] = vm_obj_num(vm_gc_len(gc, vec));
  vm_jump_next();
}
do_get : {
  size_t outreg = vm_read();
  vm_obj_t vec = locals[vm_read()];
  vm_obj_t oindex = locals[vm_read()];
  locals[outreg] = vm_gc_get(gc, vec, vm_obj_to_num(oindex));
  vm_jump_next();
}
do_set : {
  vm_obj_t vec = locals[vm_read()];
  vm_obj_t oindex = locals[vm_read()];
  vm_obj_t value = locals[vm_read()];
  vm_gc_set(gc, vec, vm_obj_to_num(oindex), value);
  vm_jump_next();
}
do_dump : {
  size_t namreg = vm_read();
  vm_obj_t sname = locals[namreg];
  size_t slen = vm_gc_len(gc, sname);
  char *name = vm_malloc(sizeof(char) * (slen + 1));
  for (vm_counter_t i = 0; i < slen; i++) {
    vm_obj_t obj = vm_gc_get(gc, sname, i);
    name[i] = (char)vm_obj_to_num(obj);
  }
  name[slen] = '\0';
  vm_obj_t ent = locals[vm_read()];
  size_t xlen = vm_gc_len(gc, ent);
  FILE *out = fopen(name, "wb");
  vm_free(name);
  for (vm_counter_t i = 0; i < xlen; i++) {
    vm_obj_t obj = vm_gc_get(gc, ent, i);
    vm_file_opcode_t op = (vm_file_opcode_t)vm_obj_to_num(obj);
    fwrite(&op, sizeof(vm_file_opcode_t), 1, out);
  }
  fclose(out);
  vm_jump_next();
}
do_read : {
  size_t outreg = vm_read();
  vm_obj_t sname = locals[vm_read()];
  size_t slen = vm_gc_len(gc, sname);
  char *name = vm_malloc(sizeof(char) * (slen + 1));
  for (vm_counter_t i = 0; i < slen; i++) {
    name[i] = (char)vm_obj_to_num(vm_gc_get(gc, sname, i));
  }
  name[slen] = '\0';
  size_t where = 0;
  size_t nalloc = 64;
  FILE *in = fopen(name, "rb");
  vm_free(name);
  if (in == NULL) {
    locals[outreg] = vm_gc_new(gc, 0);
    vm_gc_collect(gc);
    vm_jump_next();
  }
  uint8_t *str = vm_malloc(sizeof(uint8_t) * nalloc);
  for (;;) {
    uint8_t buf[2048];
    size_t n = fread(buf, 1, 2048, in);
    for (vm_counter_t i = 0; i < n; i++) {
      if (where + 4 >= nalloc) {
        nalloc = 4 + nalloc * 2;
        str = vm_realloc(str, sizeof(uint8_t) * nalloc);
      }
      str[where] = buf[i];
      where += 1;
    }
    if (n < 2048) {
      break;
    }
  }
  fclose(in);
  vm_obj_t ent = vm_gc_new(gc, where);
  for (vm_counter_t i = 0; i < where; i++) {
    vm_gc_set(gc, ent, i, vm_obj_num(str[i]));
  }
  vm_free(str);
  locals[outreg] = ent;
  vm_gc_collect(gc);
  vm_jump_next();
}
do_write : {
  size_t outreg = vm_read();
  vm_obj_t sname = locals[outreg];
  size_t slen = vm_gc_len(gc, sname);
  char *name = vm_malloc(sizeof(char) * (slen + 1));
  for (vm_counter_t i = 0; i < slen; i++) {
    vm_obj_t obj = vm_gc_get(gc, sname, i);
    name[i] = (char)vm_obj_to_num(obj);
  }
  name[slen] = '\0';
  vm_obj_t ent = locals[vm_read()];
  size_t xlen = vm_gc_len(gc, ent);
  FILE *out = fopen(name, "wb");
  vm_free(name);
  for (vm_counter_t i = 0; i < xlen; i++) {
    vm_obj_t obj = vm_gc_get(gc, ent, i);
    uint8_t op = (uint8_t)vm_obj_to_num(obj);
    fwrite(&op, 1, sizeof(uint8_t), out);
  }
  fclose(out);
  vm_jump_next();
}
do_array : {
  size_t outreg = vm_read();
  size_t nargs = vm_read();
  vm_obj_t vec = vm_gc_new(gc, nargs);
  for (vm_counter_t i = 0; i < nargs; i++) {
    size_t vreg = vm_read();
    vm_gc_set(gc, vec, i, locals[vreg]);
  }
  locals[outreg] = vec;
  vm_gc_collect(gc);
  vm_jump_next();
}
do_cat : {
  size_t to = vm_read();
  vm_obj_t left = locals[vm_read()];
  vm_obj_t right = locals[vm_read()];
  vm_obj_t ent = vm_gc_new(gc, vm_gc_len(gc, left) + vm_gc_len(gc, right));
  for (vm_counter_t i = 0; i < vm_gc_len(gc, left); i++) {
    vm_gc_set(gc, ent, i, vm_gc_get(gc, left, i));
  }
  for (vm_counter_t i = 0; i < vm_gc_len(gc, right); i++) {
    vm_gc_set(gc, ent, vm_gc_len(gc, left) + i, vm_gc_get(gc, right, i));
  }
  locals[to] = ent;
  vm_gc_collect(gc);
  vm_jump_next();
}
do_beq : {
  vm_obj_t lhs = locals[vm_read()];
  vm_obj_t rhs = locals[vm_read()];
  index = ops[index + (vm_obj_to_num(lhs) == vm_obj_to_num(rhs))].arg;
  vm_jump_next();
}
do_blt : {
  vm_obj_t lhs = locals[vm_read()];
  vm_obj_t rhs = locals[vm_read()];
  index = ops[index + (vm_obj_to_num(lhs) < vm_obj_to_num(rhs))].arg;
  vm_jump_next();
}
do_addi : {
  size_t to = vm_read();
  vm_obj_t lhs = locals[vm_read()];
  size_t rhs = vm_read();
  locals[to] = vm_obj_num(vm_obj_to_num(lhs) + (vm_number_t)rhs);
  vm_jump_next();
}
do_subi : {
  size_t to = vm_read();
  vm_obj_t lhs = locals[vm_read()];
  size_t rhs = vm_read();
  locals[to] = vm_obj_num(vm_obj_to_num(lhs) - (vm_number_t)rhs);
  vm_jump_next();
}
do_muli : {
  size_t to = vm_read();
  vm_obj_t lhs = locals[vm_read()];
  size_t rhs = vm_read();
  locals[to] = vm_obj_num(vm_obj_to_num(lhs) * (vm_number_t)rhs);
  vm_jump_next();
}
do_divi : {
  size_t to = vm_read();
  vm_obj_t lhs = locals[vm_read()];
  size_t rhs = vm_read();
  locals[to] = vm_obj_num(vm_obj_to_num(lhs) / (vm_number_t)rhs);
  vm_jump_next();
}
do_modi : {
  size_t to = vm_read();
  vm_obj_t lhs = locals[vm_read()];
  size_t rhs = vm_read();
  locals[to] = vm_obj_num(vm_obj_to_num(lhs) % (vm_number_t)rhs);
  vm_jump_next();
}
do_call0 : {
  size_t outreg = vm_read();
  size_t next_func = vm_read();
  vm_obj_t *next_locals = locals + frame->nlocals;
  locals = next_locals;
  frame->index = index;
  frame->outreg = outreg;
  frame++;
  frame->nlocals = ops[next_func - 1].arg;
  index = next_func;
  vm_jump_next();
}
do_call1 : {
  size_t outreg = vm_read();
  size_t next_func = vm_read();
  vm_obj_t *next_locals = locals + frame->nlocals;
  next_locals[1] = locals[vm_read()];
  locals = next_locals;
  frame->index = index;
  frame->outreg = outreg;
  frame++;
  frame->nlocals = ops[next_func - 1].arg;
  index = next_func;
  vm_jump_next();
}
do_call2 : {
  size_t outreg = vm_read();
  size_t next_func = vm_read();
  vm_obj_t *next_locals = locals + frame->nlocals;
  next_locals[1] = locals[vm_read()];
  next_locals[2] = locals[vm_read()];
  locals = next_locals;
  frame->index = index;
  frame->outreg = outreg;
  frame++;
  frame->nlocals = ops[next_func - 1].arg;
  index = next_func;
  vm_jump_next();
}
do_call3 : {
  size_t outreg = vm_read();
  size_t next_func = vm_read();
  vm_obj_t *next_locals = locals + frame->nlocals;
  next_locals[1] = locals[vm_read()];
  next_locals[2] = locals[vm_read()];
  next_locals[3] = locals[vm_read()];
  locals = next_locals;
  frame->index = index;
  frame->outreg = outreg;
  frame++;
  frame->nlocals = ops[next_func - 1].arg;
  index = next_func;
  vm_jump_next();
}
do_geti : {
  size_t outreg = vm_read();
  vm_obj_t vec = locals[vm_read()];
  size_t oindex = vm_read();
  locals[outreg] = vm_gc_get(gc, vec, oindex);
  vm_jump_next();
}
do_seti : {
  vm_obj_t vec = locals[vm_read()];
  size_t oindex = vm_read();
  vm_obj_t value = locals[vm_read()];
  vm_gc_set(gc, vec, oindex, value);
  vm_jump_next();
}
do_beqi : {
  vm_obj_t lhs = locals[vm_read()];
  size_t rhs = vm_read();
  index = ops[index + (vm_obj_to_num(lhs) == (vm_number_t)rhs)].arg;
  vm_jump_next();
}
do_blti : {
  vm_obj_t lhs = locals[vm_read()];
  size_t rhs = vm_read();
  index = ops[index + (vm_obj_to_num(lhs) < (vm_number_t)rhs)].arg;
  vm_jump_next();
}
do_bltei : {
  vm_obj_t lhs = locals[vm_read()];
  size_t rhs = vm_read();
  index = ops[index + (vm_obj_to_num(lhs) <= (vm_number_t)rhs)].arg;
  vm_jump_next();
}
do_call_dyn : {
  size_t outreg = vm_read();
  size_t next_func_reg = vm_read();
  size_t next_func = vm_obj_to_num(locals[next_func_reg]);
  size_t nargs = vm_read();
  vm_obj_t *next_locals = locals + frame->nlocals;
  for (size_t argno = 1; argno <= nargs; argno++) {
    size_t regno = vm_read();
    next_locals[argno] = locals[regno];
  }
  locals = next_locals;
  frame->index = index;
  frame->outreg = outreg;
  frame++;
  frame->nlocals = ops[next_func - 1].arg;
  index = next_func;
  vm_jump_next();
}
do_call_ext : {
  size_t outreg = vm_read();
  size_t next_func = vm_read();
  size_t inreg = vm_read();
  locals[outreg] = vm_run_ext(gc, next_func, locals[inreg]);
  vm_jump_next();
}
}

/// allocates locals for the program and calls the vm hot loop
int vm_run(vm_config_t config, size_t nops, vm_opcode_t *ops, size_t nargs,
           const char **args) {
  vm_gc_t gc = vm_gc_init(config);
  int res = vm_run_from(&gc, nops, ops, vm_global_from(&gc, nargs, args));
  vm_gc_deinit(gc);
  return res;
}
