
// IDIOMS:
// Leak Memory until I find a small GC.
// Undefined Behavior when opcodes are bad.
// Do not use C preprocessor `#.*` lines
// Types end in _t, and never use tagspace
// - only use `int` when it is defined by a LibC function to do so
//   * exit, printf, fclose, and main all use these
// Computed gotos are used along with a jump table.
// - `&&do_whatever` means get address of code at `do_whatever`
// - `goto *ptrs[vm_read()];` means run the next opcode

#define vm_read() (ops[index++])
#define vm_jump_next() ({ goto *ptrs[vm_read()]; })

// MiniVM needs these three at all times
typedef __SIZE_TYPE__ size_t;

typedef __UINT8_TYPE__ uint8_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;

typedef __INT8_TYPE__ int8_t;
typedef __INT16_TYPE__ int16_t;
typedef __INT32_TYPE__ int32_t;
typedef __INT64_TYPE__ int64_t;

typedef int32_t vm_file_opcode_t;
typedef int32_t vm_opcode_t;
typedef int64_t vm_number_t;
typedef size_t vm_counter_t;

/// The value type of minivm
union vm_obj_t;
/// These represent constant sized mutable arrays
struct vm_gc_entry_t;

typedef union vm_obj_t vm_obj_t;
typedef struct vm_gc_entry_t vm_gc_entry_t;

// I define libc things myself, this massivly speeds up compilation
struct FILE;
typedef struct FILE FILE;

void exit(int code);
size_t strlen(const char *str);

void *malloc(size_t size);
void *realloc(void *ptr, size_t n);
void free(void *ptr);

int printf(const char *src, ...);
FILE *fopen(const char *src, const char *name);
int fclose(FILE *);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

/// This represents a value in MiniVM
union vm_obj_t {
  // Currently 32 bit signed integers are used
  // Must represnet what a 30 bit int can at least.
  /// MiniVM integer type
  vm_number_t num;
  /// MiniVM array type
  vm_gc_entry_t *ptr;
};

/// This is the implementation of an array in minivm (DO NOT DEREF A POINTER TO
/// THIS)
struct vm_gc_entry_t {
  // Only required to store 1GiB of items, whatever size they may be.
  /// The array's length in Objects
  size_t len;
  // because we only ever store pointers to the array it is okay for it to be
  // without a set size
  /// The array's value store; zero indexed
  vm_obj_t arr[0];
};

enum {
  VM_OPCODE_EXIT = 0,
  VM_OPCODE_REG = 1,
  VM_OPCODE_BB = 2,
  VM_OPCODE_INT = 3,
  VM_OPCODE_JUMP = 4,
  VM_OPCODE_FUNC = 5,
  VM_OPCODE_ADD = 6,
  VM_OPCODE_SUB = 7,
  VM_OPCODE_MUL = 8,
  VM_OPCODE_DIV = 9,
  VM_OPCODE_MOD = 10,
  VM_OPCODE_POW = 11,
  VM_OPCODE_CALL = 12,
  VM_OPCODE_RETURN = 13,
  VM_OPCODE_PUTCHAR = 14,
  VM_OPCODE_STRING = 15,
  VM_OPCODE_LENGTH = 16,
  VM_OPCODE_GET = 17,
  VM_OPCODE_SET = 18,
  VM_OPCODE_DUMP = 19,
  VM_OPCODE_READ = 20,
  VM_OPCODE_WRITE = 21,
  VM_OPCODE_ARRAY = 22,
  VM_OPCODE_CAT = 23,
  VM_OPCODE_BEQ = 24,
  VM_OPCODE_BLT = 25,
  VM_OPCODE_ADDI = 26,
  VM_OPCDOE_SUBI = 27,
  VM_OPCDOE_MULI = 28,
  VM_OPCODE_DIVI = 29,
  VM_OPCODE_MODI = 30,
  VM_OPCODE_CALL0 = 31,
  VM_OPCODE_CALL1 = 32,
  VM_OPCODE_CALL2 = 33,
  VM_OPCODE_CALL3 = 34,
  VM_OPCODE_GETI = 35,
  VM_OPCODE_SETI = 36,
  VM_OPCODE_BEQI = 37,
  VM_OPCODE_BLTI = 38,
  VM_OPCODE_BLTEI = 39,
};

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
      ent->arr[src - args[i]] = (vm_obj_t){.num = *src};
    }
    global->arr[i] = (vm_obj_t){.ptr = ent};
  }
  return (vm_obj_t){.ptr = global};
}

/// VM hot loop
vm_obj_t vm_run_from(const vm_opcode_t *ops, size_t index, vm_obj_t *locals,
                     vm_obj_t *next_locals) {
  // our dear jump table
  static void *ptrs[] = {
      /// [] -> Exit the program when something is wrong with nonzero status
      [VM_OPCODE_EXIT] = &&do_exit,

      /// [Reg rOut, Reg rIn] -> Store value of rIn to rOut
      [VM_OPCODE_REG] = &&do_store_reg,

      /// rCond must be a number, will not work with pointers correctly.
      /// [Reg rCond, Label lFalse, Label lTrue] when rCond == 0 -> jump to
      /// lFalse
      /// [Reg rCond, Label lFalse, Label lTrue] when rCond != 0 -> jump to
      /// lTrue
      [VM_OPCODE_BB] = &&do_branch_bool,

      /// [Reg rOut, Num nVar] -> Store number nVar into rOut
      [VM_OPCODE_INT] = &&do_store_int,

      /// [Label lDest] -> Jump to lDest
      [VM_OPCODE_JUMP] = &&do_jump,

      // jump is used to implent this as a size optimization
      /// [Label lEnd, Num nLen, Char sName[nLen], Num nRegs] -> Jump over
      /// function to lEnd. The function is named the value of sName. The
      /// function has nRegs registers allocated when called.
      [VM_OPCODE_FUNC] = &&do_func,

      // Binary Operators
      /// [Reg rOut, Ref rLeft, Reg rRight] -> Compute rLeft `op` rRight and
      /// store result to rOut
      [VM_OPCODE_ADD] = &&do_add,
      [VM_OPCODE_SUB] = &&do_sub,
      [VM_OPCODE_MUL] = &&do_mul,
      [VM_OPCODE_DIV] = &&do_div,
      [VM_OPCODE_MOD] = &&do_mod,
      [VM_OPCODE_POW] = &&do_pow,

      /// vArgs is variadic, it has runtime length of nArgs.
      /// Registers are local to each function, essentially callee stored or a
      /// register stack.
      /// for argno in range[0 .. nArgs) : vArgs[argno] is stored intot the new
      /// function's Reg(argno+1)
      /// The label lFunc points to an instruction at the head of the function,
      /// this instruction must be directly after a func jump (Instruction with
      /// opcode 5)
      /// This is because static_call reads nRegs from ops[lFunc-1].
      /// [Reg rOut, Label lFunc, Num nArgs, Reg vArgs[nArgs]] -> call lFunc
      /// with all args given and return value to rOut.
      [VM_OPCODE_CALL] = &&do_call,

      /// [Reg rRet] -> jumps back to after the last static_call instruction.
      /// Puts the value held in rRet to the static_call's rOut
      [VM_OPCODE_RETURN] = &&do_return,

      /// [Reg rChar] -> prints the char represented by the number held by
      /// rChar, it should be utf8.
      [VM_OPCODE_PUTCHAR] = &&do_putchar,

      /// [Reg rOut, Num nCount, Num sNums[nCount]] -> sNums is stored as an
      /// Array of nCount signed integers into rOut
      [VM_OPCODE_STRING] = &&do_string,

      /// Undefined Behavior if rArray is not an Array
      /// [Reg rOut, Reg rArray] -> store the length of rArray into rOut
      [VM_OPCODE_LENGTH] = &&do_length,

      /// Undefiend Behavior if rArray is not an Array
      /// Undefined Behvaior if rIndex < 0, not wrapping
      /// Undefined Behvaior if rIndex >= length(rArray)
      /// [Reg rOut, Reg rArray, Reg rIndex] -> get the (zero indexed) rIndex-th
      /// value in rArray; store into rOut
      [VM_OPCODE_GET] = &&do_get,

      /// Undefined Behavior if rArray is not an Array
      /// Undefined Behvaior if rIndex < 0; it is not wrapping
      /// Undefined Behvaior if rIndex > length(rArray)
      /// Undefiend Behvaior if rValue contians rArray; No recurisve datatypes
      /// (subject to change)
      /// [Reg rArray, Reg rIndex, Reg rValue] -> set the (zero indexed)
      /// rIndex-th value in rArray to the value held in rValue
      [VM_OPCODE_SET] = &&do_set,

      /// 32 bit integer version of opcode write
      /// [Reg rFilename, Reg rArray] -> dumps to file rFilename the contents of
      /// rArray as 32 bit signed integers
      [VM_OPCODE_DUMP] = &&do_dump,

      /// [Ref rOut, Reg rFilename] -> reads into rOut the contents file
      /// rFilename as an array of Char.
      [VM_OPCODE_READ] = &&do_read,

      /// Char version of opcode dump
      /// [Reg rFilename, Reg rArray] -> dumps to file rFilename the contents of
      /// rArray as an array of Char
      [VM_OPCODE_WRITE] = &&do_write,

      /// [Reg rOut, Num nRegs, Reg vRegs[nRegs]] -> vRegs is stored as an Array
      /// into rOut
      [VM_OPCODE_ARRAY] = &&do_array,

      /// [Reg rOut, Reg rLeft, Reg rRight] -> concatenates rLeft to rRight so
      /// that rLeft's last element is direclty before rRight's first element
      /// into rOut
      [VM_OPCODE_CAT] = &&do_cat,

      /// Undefined behavior if rLeft or rRight is not a Number
      /// [Reg rLeft, Reg rRight, Label lFalse, Label lTrue] when rLeft ==
      /// rRight -> jump to lTrue
      /// [Reg rLeft, Reg rRight, Label lFalse, Label lTrue] when rLeft !=
      /// rRight -> jump to lFalse
      [VM_OPCODE_BEQ] = &&do_beq,

      /// Undefined behavior if rLeft or rRight is not a Number
      /// [Reg rLeft, Reg rRight, Label lFalse, Label lTrue] when rLeft < rRight
      /// -> jump to lTrue
      /// [Reg rLeft, Reg rRight, Label lFalse, Label lTrue] when rLeft >=
      /// rRight -> jump to lFalse
      [VM_OPCODE_BLT] = &&do_blt,

      /// undocumented
      [VM_OPCODE_ADDI] = &&do_addi,
      [VM_OPCDOE_SUBI] = &&do_subi,
      [VM_OPCDOE_MULI] = &&do_muli,
      [VM_OPCODE_DIVI] = &&do_divi,
      [VM_OPCODE_MODI] = &&do_modi,
      [VM_OPCODE_CALL0] = &&do_call0,
      [VM_OPCODE_CALL1] = &&do_call1,
      [VM_OPCODE_CALL2] = &&do_call2,
      [VM_OPCODE_CALL3] = &&do_call3,
      [VM_OPCODE_GETI] = &&do_geti,
      [VM_OPCODE_SETI] = &&do_seti,
      [VM_OPCODE_BEQI] = &&do_beqi,
      [VM_OPCODE_BLTI] = &&do_blti,
      [VM_OPCODE_BLTEI] = &&do_bltei,
  };

  goto *ptrs[vm_read()];
do_exit : { exit(1); }
do_return : { return locals[ops[index]]; }
do_branch_bool : {
  vm_opcode_t from = vm_read();
  if (locals[from].num) {
    index = ops[index + 1];
    goto *ptrs[vm_read()];
  } else {
    index = ops[index];
    goto *ptrs[vm_read()];
  }
}
do_store_reg : {
  vm_opcode_t to = vm_read();
  vm_opcode_t from = vm_read();
  locals[to] = locals[from];
  goto *ptrs[vm_read()];
}
do_store_int : {
  vm_opcode_t to = vm_read();
  vm_opcode_t from = vm_read();
  locals[to] = (vm_obj_t){.num = from};
  goto *ptrs[vm_read()];
}
do_jump : {
  index = ops[index];
  goto *ptrs[vm_read()];
}
do_func : {
  index = ops[index];
  goto *ptrs[vm_read()];
}
do_add : {
  vm_opcode_t to = vm_read();
  vm_opcode_t lhs = vm_read();
  vm_opcode_t rhs = vm_read();
  locals[to] = (vm_obj_t){.num = locals[lhs].num + locals[rhs].num};
  goto *ptrs[vm_read()];
}
do_sub : {
  vm_opcode_t to = vm_read();
  vm_opcode_t lhs = vm_read();
  vm_opcode_t rhs = vm_read();
  locals[to] = (vm_obj_t){.num = locals[lhs].num - locals[rhs].num};
  goto *ptrs[vm_read()];
}
do_mul : {
  vm_opcode_t to = vm_read();
  vm_opcode_t lhs = vm_read();
  vm_opcode_t rhs = vm_read();
  locals[to] = (vm_obj_t){.num = locals[lhs].num * locals[rhs].num};
  goto *ptrs[vm_read()];
}
do_div : {
  vm_opcode_t to = vm_read();
  vm_opcode_t lhs = vm_read();
  vm_opcode_t rhs = vm_read();
  locals[to] = (vm_obj_t){.num = locals[lhs].num / locals[rhs].num};
  goto *ptrs[vm_read()];
}
do_mod : {
  vm_opcode_t to = vm_read();
  vm_opcode_t lhs = vm_read();
  vm_opcode_t rhs = vm_read();
  locals[to] = (vm_obj_t){.num = locals[lhs].num % locals[rhs].num};
  goto *ptrs[vm_read()];
}
do_pow : {
  vm_opcode_t to = vm_read();
  vm_opcode_t lhs = vm_read();
  vm_opcode_t rhs = vm_read();
  vm_number_t base = locals[lhs].num;
  vm_number_t exp = locals[rhs].num;
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
  locals[to] = (vm_obj_t){.num = result};
  goto *ptrs[vm_read()];
}
do_call : {
  vm_opcode_t outreg = vm_read();
  vm_opcode_t next_func = vm_read();
  vm_opcode_t nargs = vm_read();
  for (vm_counter_t argno = 1; argno <= nargs; argno++) {
    vm_opcode_t regno = vm_read();
    next_locals[argno] = locals[regno];
  }
  locals[outreg] = vm_run_from(ops, next_func, next_locals,
                               next_locals + ops[next_func - 1]);
  goto *ptrs[vm_read()];
}
do_putchar : {
  vm_opcode_t from = vm_read();
  vm_number_t val = locals[from].num;
  printf("%c", (int) val);
  goto *ptrs[vm_read()];
}
do_string : {
  vm_opcode_t outreg = vm_read();
  vm_opcode_t nargs = vm_read();
  vm_gc_entry_t *str = vm_array_new(nargs);
  for (size_t i = 0; i < nargs; i++) {
    vm_opcode_t num = vm_read();
    str->arr[i] = (vm_obj_t){.num = num};
  }
  locals[outreg] = (vm_obj_t){.ptr = str};
  goto *ptrs[vm_read()];
}
do_length : {
  vm_opcode_t outreg = vm_read();
  vm_obj_t vec = locals[vm_read()];
  locals[outreg] = (vm_obj_t){.num = vec.ptr->len};
  goto *ptrs[vm_read()];
}
do_get : {
  vm_opcode_t outreg = vm_read();
  vm_obj_t vec = locals[vm_read()];
  vm_obj_t oindex = locals[vm_read()];
  locals[outreg] = vec.ptr->arr[oindex.num];
  goto *ptrs[vm_read()];
}
do_set : {
  vm_obj_t vec = locals[vm_read()];
  vm_obj_t oindex = locals[vm_read()];
  vm_obj_t value = locals[vm_read()];
  vec.ptr->arr[oindex.num] = value;
  goto *ptrs[vm_read()];
}
do_dump : {
  vm_opcode_t namreg = vm_read();
  vm_gc_entry_t *sname = locals[namreg].ptr;
  size_t slen = sname->len;
  char *name = malloc(sizeof(char) * (slen + 1));
  for (vm_counter_t i = 0; i < slen; i++) {
    vm_obj_t obj = sname->arr[i];
    name[i] = obj.num;
  }
  name[slen] = '\0';
  vm_gc_entry_t *ent = locals[vm_read()].ptr;
  size_t xlen = ent->len;
  FILE *out = fopen(name, "wb");
  free(name);
  for (vm_counter_t i = 0; i < xlen; i++) {
    vm_obj_t obj = ent->arr[i];
    vm_file_opcode_t op = obj.num;
    fwrite(&op, sizeof(vm_file_opcode_t), 1, out);
  }
  fclose(out);
  goto *ptrs[vm_read()];
}
do_read : {
  vm_opcode_t outreg = vm_read();
  vm_gc_entry_t *sname = locals[vm_read()].ptr;
  size_t slen = sname->len;
  char *name = malloc(sizeof(char) * (slen + 1));
  for (vm_counter_t i = 0; i < slen; i++) {
    name[i] = sname->arr[i].num;
  }
  name[slen] = '\0';
  size_t where = 0;
  size_t nalloc = 64;
  FILE *in = fopen(name, "rb");
  free(name);
  if (in == (void *)0) {
    locals[outreg] = (vm_obj_t){.ptr = vm_array_new(0)};
    goto *ptrs[vm_read()];
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
    ent->arr[i] = (vm_obj_t){.num = str[i]};
  }
  free(str);
  locals[outreg] = (vm_obj_t){.ptr = ent};
  goto *ptrs[vm_read()];
}
do_write : {
  vm_opcode_t outreg = vm_read();
  vm_gc_entry_t *sname = locals[outreg].ptr;
  size_t slen = sname->len;
  char *name = malloc(sizeof(char) * (slen + 1));
  for (vm_counter_t i = 0; i < slen; i++) {
    vm_obj_t obj = sname->arr[i];
    name[i] = obj.num;
  }
  name[slen] = '\0';
  vm_gc_entry_t *ent = locals[vm_read()].ptr;
  size_t xlen = ent->len;
  FILE *out = fopen(name, "wb");
  free(name);
  for (vm_counter_t i = 0; i < xlen; i++) {
    vm_obj_t obj = ent->arr[i];
    uint8_t op = obj.num;
    fwrite(&op, 1, sizeof(uint8_t), out);
  }
  fclose(out);
  goto *ptrs[vm_read()];
}
do_array : {
  vm_opcode_t outreg = vm_read();
  vm_opcode_t nargs = vm_read();
  vm_gc_entry_t *vec = vm_array_new(nargs);
  for (vm_counter_t i = 0; i < nargs; i++) {
    vm_opcode_t vreg = vm_read();
    vec->arr[i] = locals[vreg];
  }
  locals[outreg] = (vm_obj_t){.ptr = vec};
  goto *ptrs[vm_read()];
}
do_cat : {
  vm_opcode_t to = vm_read();
  vm_gc_entry_t *left = locals[vm_read()].ptr;
  vm_gc_entry_t *right = locals[vm_read()].ptr;
  vm_gc_entry_t *ent = vm_array_new(left->len + right->len);
  for (vm_counter_t i = 0; i < left->len; i++) {
    ent->arr[i] = left->arr[i];
  }
  for (vm_counter_t i = 0; i < right->len; i++) {
    ent->arr[left->len + i] = right->arr[i];
  }
  locals[to] = (vm_obj_t){.ptr = ent};
  goto *ptrs[vm_read()];
}
do_beq : {
  vm_obj_t lhs = locals[vm_read()];
  vm_obj_t rhs = locals[vm_read()];
  index = ops[index + (lhs.num == rhs.num)];
  goto *ptrs[vm_read()];
}
do_blt : {
  vm_obj_t lhs = locals[vm_read()];
  vm_obj_t rhs = locals[vm_read()];
  index = ops[index + (lhs.num < rhs.num)];
  goto *ptrs[vm_read()];
}
do_addi : {
  vm_opcode_t to = vm_read();
  vm_obj_t lhs = locals[vm_read()];
  vm_opcode_t rhs = vm_read();
  locals[to] = (vm_obj_t) {.num = lhs.num + rhs};
  goto *ptrs[vm_read()];
}
do_subi : {
  vm_opcode_t to = vm_read();
  vm_obj_t lhs = locals[vm_read()];
  vm_opcode_t rhs = vm_read();
  locals[to] = (vm_obj_t) {.num = lhs.num - rhs};
  goto *ptrs[vm_read()];
}
do_muli : {
  vm_opcode_t to = vm_read();
  vm_obj_t lhs = locals[vm_read()];
  vm_opcode_t rhs = vm_read();
  locals[to] = (vm_obj_t) {.num = lhs.num * rhs};
  goto *ptrs[vm_read()];
}
do_divi : {
  vm_opcode_t to = vm_read();
  vm_obj_t lhs = locals[vm_read()];
  vm_opcode_t rhs = vm_read();
  locals[to] = (vm_obj_t) {.num = lhs.num / rhs};
  goto *ptrs[vm_read()];
}
do_modi : {
  vm_opcode_t to = vm_read();
  vm_obj_t lhs = locals[vm_read()];
  vm_opcode_t rhs = vm_read();
  locals[to] = (vm_obj_t) {.num = lhs.num % rhs};
  goto *ptrs[vm_read()];
}
do_call0 : {
  vm_opcode_t outreg = vm_read();
  vm_opcode_t next_func = vm_read();
  locals[outreg] = vm_run_from(ops, next_func, next_locals,
                               next_locals + ops[next_func - 1]);
  goto *ptrs[vm_read()];
}
do_call1 : {
  vm_opcode_t outreg = vm_read();
  vm_opcode_t next_func = vm_read();
  next_locals[1] = locals[vm_read()];
  locals[outreg] = vm_run_from(ops, next_func, next_locals,
                               next_locals + ops[next_func - 1]);
  goto *ptrs[vm_read()];
}
do_call2 : {
  vm_opcode_t outreg = vm_read();
  vm_opcode_t next_func = vm_read();
  next_locals[1] = locals[vm_read()];
  next_locals[2] = locals[vm_read()];
  locals[outreg] = vm_run_from(ops, next_func, next_locals,
                               next_locals + ops[next_func - 1]);
  goto *ptrs[vm_read()];
}
do_call3 : {
  vm_opcode_t outreg = vm_read();
  vm_opcode_t next_func = vm_read();
  next_locals[1] = locals[vm_read()];
  next_locals[2] = locals[vm_read()];
  next_locals[3] = locals[vm_read()];
  locals[outreg] = vm_run_from(ops, next_func, next_locals,
                               next_locals + ops[next_func - 1]);
  goto *ptrs[vm_read()];
}
do_geti : {
  vm_opcode_t outreg = vm_read();
  vm_obj_t vec = locals[vm_read()];
  vm_opcode_t oindex = vm_read();
  locals[outreg] = vec.ptr->arr[oindex];
  goto *ptrs[vm_read()];
}
do_seti : {
  vm_obj_t vec = locals[vm_read()];
  vm_opcode_t oindex = vm_read();
  vm_obj_t value = locals[vm_read()];
  vec.ptr->arr[oindex] = value;
  goto *ptrs[vm_read()];
}
do_beqi : {
  vm_obj_t lhs = locals[vm_read()];
  vm_opcode_t rhs = vm_read();
  index = ops[index + (lhs.num == rhs)];
  goto *ptrs[vm_read()];
}
do_blti : {
  vm_obj_t lhs = locals[vm_read()];
  vm_opcode_t rhs = vm_read();
  index = ops[index + (lhs.num < rhs)];
  goto *ptrs[vm_read()];
}
do_bltei : {
  vm_obj_t lhs = locals[vm_read()];
  vm_opcode_t rhs = vm_read();
  index = ops[index + (lhs.num <= rhs)];
  goto *ptrs[vm_read()];
}
}

/// allocates locals for the program and calls the vm hot loop
void vm_run(const vm_opcode_t *ops, size_t nargs, const char **args) {
  vm_obj_t *locals = malloc(sizeof(vm_obj_t) * (1 << 16));
  locals[0] = vm_global_from(nargs, args);
  vm_run_from(ops, 0, locals, locals + 256);
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
    ops[nops++] = op;
  }
  fclose(file);
  vm_run(ops, argc - 2, argv + 2);
  free(ops);
}
