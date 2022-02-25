
// IDIOMS:
// Leak Memory until I find a small GC.
// Undefined Behavior when opcodes are bad.
// Do not use C preprocessor `#.*` lines
// Types end in _t, and never use tagspace
// - only use `int` when it is defined by a LibC function to do so
//   * exit, printf, fclose, and main all use these
// Computed gotos are used along with a jump table.
// - `&&do_whatever` means get address of code at `do_whatever`
// - `vm_jump_next();` means run the next opcode

#include "lib.h"
#include "opcode.h"

#define vm_read() (ops[index++].arg)
#define vm_jump_next() ({ goto *ops[index++].op; })
#define vm_obj_num(v_) ((vm_obj_t){.num = v_})
#define vm_obj_ptr(v_) ((vm_obj_t){.ptr = v_})
#define vm_obj_to_num(o_) ((o_).num)
#define vm_obj_to_ptr(o_) ((o_).ptr)

/// This represents a value in MiniVM
union vm_obj_t {
  /// MiniVM integer type
  vm_number_t num;
  /// MiniVM array type
  vm_gc_entry_t *ptr;
};

/// This is the implementation of an array in minivm
// (DO NOT DEREF A POINTER TO THIS)
struct vm_gc_entry_t {
  // Only required to store 1GiB of items, whatever size they may be.
  /// The array's length in Objects
  size_t len;
  // because we only ever store pointers to the array it is okay for it to be
  // without a set size
  /// The array's value store; zero indexed
  vm_obj_t arr[0];
};

typedef struct {
  vm_number_t outreg;
  vm_number_t index;
  vm_number_t nlocals;
} vm_frame_t;

/// Creates an uninitialized array of length size
vm_gc_entry_t *vm_array_new(size_t size) {
  // Trick for variadic array sizes.
  vm_gc_entry_t *ent = malloc(sizeof(vm_gc_entry_t) + sizeof(vm_obj_t) * size);
  ent->len = size;
  return ent;
}

/// Creates the object in r0 at the beginning of the program.
vm_obj_t vm_global_from(size_t len, const char **args) {
  // Just some minivm arrays.
  vm_gc_entry_t *global = vm_array_new(len);
  for (size_t i = 0; i < len; i++) {
    vm_gc_entry_t *ent = vm_array_new(strlen(args[i]));
    for (const char *src = args[i]; *src != '\0'; src++) {
      ent->arr[src - args[i]] = vm_obj_num(*src);
    }
    global->arr[i] = vm_obj_ptr(ent);
  }
  return vm_obj_ptr(global);
}

int vm_table_opt(size_t nops, vm_opcode_t *ops, void *const *const ptrs) {
  for (size_t i = 0; i < nops; ++i) {
    vm_number_t op = ops[i].arg;
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
      vm_number_t namelen = ops[i + 3].arg;
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
      vm_number_t nargs = ops[i + 3].arg;
      i += 3 + nargs;
    } break;
    case VM_OPCODE_RETURN:
      i += 1;
      break;
    case VM_OPCODE_PUTCHAR:
      i += 1;
      break;
    case VM_OPCODE_STRING: {
      vm_number_t nargs = ops[i + 2].arg;
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
      vm_number_t nargs = ops[i + 2].arg;
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
    case VM_OPCDOE_SUBI:
      i += 3;
      break;
    case VM_OPCDOE_MULI:
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
    default:
      printf("unknown opcode: %p\n", ops[i].op);
      // disassembly may not be reliable after an unknown opcode
    }
  }
  return 0;
}

/// VM hot loop
void vm_run_from(size_t nops, vm_opcode_t *ops, vm_obj_t globals) {
  // our dear jump table
  static void *const ptrs[] = {
      [VM_OPCODE_EXIT] = &&do_exit,       [VM_OPCODE_REG] = &&do_store_reg,
      [VM_OPCODE_BB] = &&do_branch_bool,  [VM_OPCODE_INT] = &&do_store_int,
      [VM_OPCODE_JUMP] = &&do_jump,       [VM_OPCODE_FUNC] = &&do_func,
      [VM_OPCODE_ADD] = &&do_add,         [VM_OPCODE_SUB] = &&do_sub,
      [VM_OPCODE_MUL] = &&do_mul,         [VM_OPCODE_DIV] = &&do_div,
      [VM_OPCODE_MOD] = &&do_mod,         [VM_OPCODE_POW] = &&do_pow,
      [VM_OPCODE_CALL] = &&do_call,       [VM_OPCODE_RETURN] = &&do_return,
      [VM_OPCODE_PUTCHAR] = &&do_putchar, [VM_OPCODE_STRING] = &&do_string,
      [VM_OPCODE_LENGTH] = &&do_length,   [VM_OPCODE_GET] = &&do_get,
      [VM_OPCODE_SET] = &&do_set,         [VM_OPCODE_DUMP] = &&do_dump,
      [VM_OPCODE_READ] = &&do_read,       [VM_OPCODE_WRITE] = &&do_write,
      [VM_OPCODE_ARRAY] = &&do_array,     [VM_OPCODE_CAT] = &&do_cat,
      [VM_OPCODE_BEQ] = &&do_beq,         [VM_OPCODE_BLT] = &&do_blt,
      [VM_OPCODE_ADDI] = &&do_addi,       [VM_OPCDOE_SUBI] = &&do_subi,
      [VM_OPCDOE_MULI] = &&do_muli,       [VM_OPCODE_DIVI] = &&do_divi,
      [VM_OPCODE_MODI] = &&do_modi,       [VM_OPCODE_CALL0] = &&do_call0,
      [VM_OPCODE_CALL1] = &&do_call1,     [VM_OPCODE_CALL2] = &&do_call2,
      [VM_OPCODE_CALL3] = &&do_call3,     [VM_OPCODE_GETI] = &&do_geti,
      [VM_OPCODE_SETI] = &&do_seti,       [VM_OPCODE_BEQI] = &&do_beqi,
      [VM_OPCODE_BLTI] = &&do_blti,       [VM_OPCODE_BLTEI] = &&do_bltei,
  };
  vm_table_opt(nops, ops, ptrs);
  size_t index = 0;
  vm_obj_t *locals = malloc(sizeof(vm_obj_t) * (1 << 16));
  locals[0] = globals;
  vm_frame_t *frames = malloc(sizeof(vm_frame_t) * (1 << 12));
  vm_frame_t *frame = &frames[0];
  frame->nlocals = 0;
  frame += 1;
  frame->nlocals = 256;
  vm_jump_next();
do_exit : { return; }
do_return : {
  vm_number_t from = vm_read();
  vm_obj_t val = locals[from];
  frame--;
  locals = locals - frame->nlocals;
  vm_number_t outreg = frame->outreg;
  locals[outreg] = val;
  index = frame->index;
  vm_jump_next();
}
do_branch_bool : {
  vm_number_t from = vm_read();
  if (vm_obj_to_num(locals[from])) {
    index = ops[index + 1].arg;
    vm_jump_next();
  } else {
    index = ops[index].arg;
    vm_jump_next();
  }
}
do_store_reg : {
  vm_number_t to = vm_read();
  vm_number_t from = vm_read();
  locals[to] = locals[from];
  vm_jump_next();
}
do_store_int : {
  vm_number_t to = vm_read();
  vm_number_t from = vm_read();
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
  vm_number_t to = vm_read();
  vm_number_t lhs = vm_read();
  vm_number_t rhs = vm_read();
  locals[to] =
      vm_obj_num(vm_obj_to_num(locals[lhs]) + vm_obj_to_num(locals[rhs]));
  vm_jump_next();
}
do_sub : {
  vm_number_t to = vm_read();
  vm_number_t lhs = vm_read();
  vm_number_t rhs = vm_read();
  locals[to] =
      vm_obj_num(vm_obj_to_num(locals[lhs]) - vm_obj_to_num(locals[rhs]));
  vm_jump_next();
}
do_mul : {
  vm_number_t to = vm_read();
  vm_number_t lhs = vm_read();
  vm_number_t rhs = vm_read();
  locals[to] =
      vm_obj_num(vm_obj_to_num(locals[lhs]) * vm_obj_to_num(locals[rhs]));
  vm_jump_next();
}
do_div : {
  vm_number_t to = vm_read();
  vm_number_t lhs = vm_read();
  vm_number_t rhs = vm_read();
  locals[to] =
      vm_obj_num(vm_obj_to_num(locals[lhs]) / vm_obj_to_num(locals[rhs]));
  vm_jump_next();
}
do_mod : {
  vm_number_t to = vm_read();
  vm_number_t lhs = vm_read();
  vm_number_t rhs = vm_read();
  locals[to] =
      vm_obj_num(vm_obj_to_num(locals[lhs]) % vm_obj_to_num(locals[rhs]));
  vm_jump_next();
}
do_pow : {
  vm_number_t to = vm_read();
  vm_number_t lhs = vm_read();
  vm_number_t rhs = vm_read();
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
  vm_number_t outreg = vm_read();
  vm_number_t next_func = vm_read();
  vm_number_t nargs = vm_read();
  vm_obj_t *next_locals = locals + frame->nlocals;
  for (vm_number_t argno = 1; argno <= nargs; argno++) {
    vm_number_t regno = vm_read();
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
  vm_number_t from = vm_read();
  vm_number_t val = vm_obj_to_num(locals[from]);
  printf("%c", (int)val);
  vm_jump_next();
}
do_string : {
  vm_number_t outreg = vm_read();
  vm_number_t nargs = vm_read();
  vm_gc_entry_t *str = vm_array_new(nargs);
  for (size_t i = 0; i < nargs; i++) {
    vm_number_t num = vm_read();
    str->arr[i] = vm_obj_num(num);
  }
  locals[outreg] = vm_obj_ptr(str);
  vm_jump_next();
}
do_length : {
  vm_number_t outreg = vm_read();
  vm_obj_t vec = locals[vm_read()];
  locals[outreg] = vm_obj_num(vm_obj_to_ptr(vec)->len);
  vm_jump_next();
}
do_get : {
  vm_number_t outreg = vm_read();
  vm_obj_t vec = locals[vm_read()];
  vm_obj_t oindex = locals[vm_read()];
  locals[outreg] = vm_obj_to_ptr(vec)->arr[vm_obj_to_num(oindex)];
  vm_jump_next();
}
do_set : {
  vm_obj_t vec = locals[vm_read()];
  vm_obj_t oindex = locals[vm_read()];
  vm_obj_t value = locals[vm_read()];
  vm_obj_to_ptr(vec)->arr[vm_obj_to_num(oindex)] = value;
  vm_jump_next();
}
do_dump : {
  vm_number_t namreg = vm_read();
  vm_gc_entry_t *sname = vm_obj_to_ptr(locals[namreg]);
  size_t slen = sname->len;
  char *name = malloc(sizeof(char) * (slen + 1));
  for (vm_counter_t i = 0; i < slen; i++) {
    vm_obj_t obj = sname->arr[i];
    name[i] = vm_obj_to_num(obj);
  }
  name[slen] = '\0';
  vm_gc_entry_t *ent = vm_obj_to_ptr(locals[vm_read()]);
  size_t xlen = ent->len;
  FILE *out = fopen(name, "wb");
  free(name);
  for (vm_counter_t i = 0; i < xlen; i++) {
    vm_obj_t obj = ent->arr[i];
    vm_file_opcode_t op = vm_obj_to_num(obj);
    fwrite(&op, sizeof(vm_file_opcode_t), 1, out);
  }
  fclose(out);
  vm_jump_next();
}
do_read : {
  vm_number_t outreg = vm_read();
  vm_gc_entry_t *sname = vm_obj_to_ptr(locals[vm_read()]);
  size_t slen = sname->len;
  char *name = malloc(sizeof(char) * (slen + 1));
  for (vm_counter_t i = 0; i < slen; i++) {
    name[i] = vm_obj_to_num(sname->arr[i]);
  }
  name[slen] = '\0';
  size_t where = 0;
  size_t nalloc = 64;
  FILE *in = fopen(name, "rb");
  free(name);
  if (in == (void *)0) {
    locals[outreg] = vm_obj_ptr(vm_array_new(0));
    vm_jump_next();
  }
  uint8_t *str = malloc(sizeof(uint8_t) * nalloc);
  for (;;) {
    uint8_t buf[2048];
    size_t n = fread(buf, 1, 2048, in);
    for (vm_counter_t i = 0; i < n; i++) {
      if (where + 4 >= nalloc) {
        nalloc = 4 + nalloc * 2;
        str = realloc(str, sizeof(uint8_t) * nalloc);
      }
      str[where] = buf[i];
      where += 1;
    }
    if (n < 2048) {
      break;
    }
  }
  fclose(in);
  vm_gc_entry_t *ent = vm_array_new(where);
  for (vm_counter_t i = 0; i < where; i++) {
    ent->arr[i] = vm_obj_num(str[i]);
  }
  free(str);
  locals[outreg] = vm_obj_ptr(ent);
  vm_jump_next();
}
do_write : {
  vm_number_t outreg = vm_read();
  vm_gc_entry_t *sname = vm_obj_to_ptr(locals[outreg]);
  size_t slen = sname->len;
  char *name = malloc(sizeof(char) * (slen + 1));
  for (vm_counter_t i = 0; i < slen; i++) {
    vm_obj_t obj = sname->arr[i];
    name[i] = vm_obj_to_num(obj);
  }
  name[slen] = '\0';
  vm_gc_entry_t *ent = vm_obj_to_ptr(locals[vm_read()]);
  size_t xlen = ent->len;
  FILE *out = fopen(name, "wb");
  free(name);
  for (vm_counter_t i = 0; i < xlen; i++) {
    vm_obj_t obj = ent->arr[i];
    uint8_t op = vm_obj_to_num(obj);
    fwrite(&op, 1, sizeof(uint8_t), out);
  }
  fclose(out);
  vm_jump_next();
}
do_array : {
  vm_number_t outreg = vm_read();
  vm_number_t nargs = vm_read();
  vm_gc_entry_t *vec = vm_array_new(nargs);
  for (vm_counter_t i = 0; i < nargs; i++) {
    vm_number_t vreg = vm_read();
    vec->arr[i] = locals[vreg];
  }
  locals[outreg] = vm_obj_ptr(vec);
  vm_jump_next();
}
do_cat : {
  vm_number_t to = vm_read();
  vm_gc_entry_t *left = vm_obj_to_ptr(locals[vm_read()]);
  vm_gc_entry_t *right = vm_obj_to_ptr(locals[vm_read()]);
  vm_gc_entry_t *ent = vm_array_new(left->len + right->len);
  for (vm_counter_t i = 0; i < left->len; i++) {
    ent->arr[i] = left->arr[i];
  }
  for (vm_counter_t i = 0; i < right->len; i++) {
    ent->arr[left->len + i] = right->arr[i];
  }
  locals[to] = vm_obj_ptr(ent);
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
  vm_number_t to = vm_read();
  vm_obj_t lhs = locals[vm_read()];
  vm_number_t rhs = vm_read();
  locals[to] = vm_obj_num(vm_obj_to_num(lhs) + rhs);
  vm_jump_next();
}
do_subi : {
  vm_number_t to = vm_read();
  vm_obj_t lhs = locals[vm_read()];
  vm_number_t rhs = vm_read();
  locals[to] = vm_obj_num(vm_obj_to_num(lhs) - rhs);
  vm_jump_next();
}
do_muli : {
  vm_number_t to = vm_read();
  vm_obj_t lhs = locals[vm_read()];
  vm_number_t rhs = vm_read();
  locals[to] = vm_obj_num(vm_obj_to_num(lhs) * rhs);
  vm_jump_next();
}
do_divi : {
  vm_number_t to = vm_read();
  vm_obj_t lhs = locals[vm_read()];
  vm_number_t rhs = vm_read();
  locals[to] = vm_obj_num(vm_obj_to_num(lhs) / rhs);
  vm_jump_next();
}
do_modi : {
  vm_number_t to = vm_read();
  vm_obj_t lhs = locals[vm_read()];
  vm_number_t rhs = vm_read();
  locals[to] = vm_obj_num(vm_obj_to_num(lhs) % rhs);
  vm_jump_next();
}
do_call0 : {
  vm_number_t outreg = vm_read();
  vm_number_t next_func = vm_read();
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
  vm_number_t outreg = vm_read();
  vm_number_t next_func = vm_read();
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
  vm_number_t outreg = vm_read();
  vm_number_t next_func = vm_read();
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
  vm_number_t outreg = vm_read();
  vm_number_t next_func = vm_read();
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
  vm_number_t outreg = vm_read();
  vm_obj_t vec = locals[vm_read()];
  vm_number_t oindex = vm_read();
  locals[outreg] = vm_obj_to_ptr(vec)->arr[oindex];
  vm_jump_next();
}
do_seti : {
  vm_obj_t vec = locals[vm_read()];
  vm_number_t oindex = vm_read();
  vm_obj_t value = locals[vm_read()];
  vm_obj_to_ptr(vec)->arr[oindex] = value;
  vm_jump_next();
}
do_beqi : {
  vm_obj_t lhs = locals[vm_read()];
  vm_number_t rhs = vm_read();
  index = ops[index + (vm_obj_to_num(lhs) == rhs)].arg;
  vm_jump_next();
}
do_blti : {
  vm_obj_t lhs = locals[vm_read()];
  vm_number_t rhs = vm_read();
  index = ops[index + (vm_obj_to_num(lhs) < rhs)].arg;
  vm_jump_next();
}
do_bltei : {
  vm_obj_t lhs = locals[vm_read()];
  vm_number_t rhs = vm_read();
  index = ops[index + (vm_obj_to_num(lhs) <= rhs)].arg;
  vm_jump_next();
}
}

/// allocates locals for the program and calls the vm hot loop
void vm_run(size_t nops, vm_opcode_t *ops, size_t nargs, const char **args) {
  vm_run_from(nops, ops, vm_global_from(nargs, args));
}

int main(int argc, const char **argv) {
  if (argc < 2) {
    printf("cannot run vm: not enough args\n");
    return 1;
  }
  FILE *file = fopen(argv[1], "rb");
  if (file == (void *)0) {
    printf("cannot run vm: file to run could not be read\n");
    return 2;
  }
  size_t nalloc = 1 << 8;
  vm_opcode_t *ops = malloc(sizeof(vm_opcode_t) * nalloc);
  size_t nops = 0;
  size_t size;
  for (;;) {
    vm_file_opcode_t op = 0;
    size = fread(&op, sizeof(vm_file_opcode_t), 1, file);
    if (size == 0) {
      break;
    }
    if (nops + 1 >= nalloc) {
      nalloc *= 4;
      ops = realloc(ops, sizeof(vm_opcode_t) * nalloc);
    }
    ops[nops++].arg = op;
  }
  fclose(file);
  vm_run(nops, ops, argc - 2, argv + 2);
  free(ops);
}
