
// IDIOMS:
// Leak Memory until I find a small GC.
// Undefined Behavior when opcodes are bad.
// Do not use C preprocessor `#.*` lines
// Types end in _t, and never use tagspace
// - only use `int` when it is defined by a LibC function to do so
//   * exit, printf, fclose, and main all use these
// Computed gotos are used along with a jump table.
// - `&&do_whatever` means get address of code at `do_whatever`
// - `goto *ptrs[ops[index++]];` means run the next opcode

// MiniVM needs these three at all times
typedef __SIZE_TYPE__ size_t;
typedef __UINT8_TYPE__ uint8_t;
typedef __INT32_TYPE__ int32_t;

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
  int32_t num;
  /// MiniVM array type
  vm_gc_entry_t *ptr;
};

/// This is the implementation of an array in minivm (DO NOT DEREF A POINTER TO THIS)
struct vm_gc_entry_t {
  // Only required to store 1GiB of items, whatever size they may be.
  /// The array's length in Objects
  size_t len;
  // because we only ever store pointers to the array it is okay for it to be without a set size
  /// The array's value store; zero indexed
  vm_obj_t arr[0];
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
vm_obj_t vm_run_from(const int32_t *ops, size_t index, vm_obj_t *locals,
                     vm_obj_t *next_locals) {
  // our dear jump table
  static void *ptrs[] = {
      /// [] -> Exit the program when something is wrong with nonzero status
      [0] = &&do_exit,

      /// [Reg rOut, Reg rIn] -> Store value of rIn to rOut
      [1] = &&do_store_reg,

      /// rCond must be a number, will not work with pointers correctly.
      /// [Reg rCond, Label lFalse, Label lTrue] when rCond == 0 -> jump to lFalse
      /// [Reg rCond, Label lFalse, Label lTrue] when rCond != 0 -> jump to lTrue
      [2] = &&do_branch_bool,

      /// [Reg rOut, Num nVar] -> Store number nVar into rOut
      [3] = &&do_store_int,

      /// [Label lDest] -> Jump to lDest
      [4] = &&do_jump,

      // jump is used to implent this as a size optimization
      /// [Label lEnd, Num nLen, Char sName[nLen], Num nRegs] -> Jump over function to lEnd. The function is named the value of sName. The function has nRegs registers allocated when called.
      [5] = &&do_func,

      // Binary Operators
      /// [Reg rOut, Ref rLeft, Reg rRight] -> Compute rLeft `op` rRight and store result to rOut
      [6] = &&do_add,
      [7] = &&do_sub,
      [8] = &&do_mul,
      [9] = &&do_div,
      [10] = &&do_mod,

      /// Undefined behavior if rLeft or rRight is not a Number
      /// [Reg rLeft, Reg rRight, Label lFalse, Label lTrue] when rLeft <= rRight -> jump to lTrue
      /// [Reg rLeft, Reg rRight, Label lFalse, Label lTrue] when rLeft > rRight -> jump to lFalse
      [11] = &&do_blte,

      /// vArgs is variadic, it has runtime length of nArgs.
      /// Registers are local to each function, essentially callee stored or a register stack.
      /// for argno in range[0 .. nArgs) : vArgs[argno] is stored intot the new function's Reg(argno+1)
      /// The label lFunc points to an instruction at the head of the function, this instruction must be directly after a func jump (Instruction with opcode 5)
      /// This is because static_call reads nRegs from ops[lFunc-1].
      /// [Reg rOut, Label lFunc, Num nArgs, Reg vArgs[nArgs]] -> call lFunc with all args given and return value to rOut. 
      [12] = &&do_call,

      /// [Reg rRet] -> jumps back to after the last static_call instruction. Puts the value held in rRet to the static_call's rOut
      [13] = &&do_return,

      /// [Reg rChar] -> prints the char represented by the number held by rChar, it should be utf8.
      [14] = &&do_putchar,

      /// [Reg rOut, Num nCount, Num sNums[nCount]] -> sNums is stored as an Array of nCount signed integers into rOut
      [15] = &&do_string,

      /// Undefined Behavior if rArray is not an Array
      /// [Reg rOut, Reg rArray] -> store the length of rArray into rOut
      [16] = &&do_length,

      /// Undefiend Behavior if rArray is not an Array
      /// Undefined Behvaior if rIndex < 0, not wrapping
      /// Undefined Behvaior if rIndex >= length(rArray)
      /// [Reg rOut, Reg rArray, Reg rIndex] -> get the (zero indexed) rIndex-th value in rArray; store into rOut
      [17] = &&do_get,

      /// Undefined Behavior if rArray is not an Array
      /// Undefined Behvaior if rIndex < 0; it is not wrapping
      /// Undefined Behvaior if rIndex > length(rArray)
      /// Undefiend Behvaior if rValue contians rArray; No recurisve datatypes (subject to change)
      /// [Reg rArray, Reg rIndex, Reg rValue] -> set the (zero indexed) rIndex-th value in rArray to the value held in rValue
      [18] = &&do_set,

      /// 32 bit integer version of opcode write
      /// [Reg rFilename, Reg rArray] -> dumps to file rFilename the contents of rArray as 32 bit signed integers
      [19] = &&do_dump,

      /// [Ref rOut, Reg rFilename] -> reads into rOut the contents file rFilename as an array of Char.
      [20] = &&do_read,

      /// Char version of opcode dump
      /// [Reg rFilename, Reg rArray] -> dumps to file rFilename the contents of rArray as an array of Char
      [21] = &&do_write,

      /// [Reg rOut, Num nRegs, Reg vRegs[nRegs]] -> vRegs is stored as an Array into rOut
      [22] = &&do_array,

      /// [Reg rOut, Reg rLeft, Reg rRight] -> concatenates rLeft to rRight so that rLeft's last element is direclty before rRight's first element into rOut
      [23] = &&do_cat,

      /// Undefined behavior if rLeft or rRight is not a Number
      /// [Reg rLeft, Reg rRight, Label lFalse, Label lTrue] when rLeft == rRight -> jump to lTrue
      /// [Reg rLeft, Reg rRight, Label lFalse, Label lTrue] when rLeft != rRight -> jump to lFalse
      [24] = &&do_beq,

      /// Undefined behavior if rLeft or rRight is not a Number
      /// [Reg rLeft, Reg rRight, Label lFalse, Label lTrue] when rLeft < rRight -> jump to lTrue
      /// [Reg rLeft, Reg rRight, Label lFalse, Label lTrue] when rLeft >= rRight -> jump to lFalse
      [25] = &&do_blt,
  };

  goto *ptrs[ops[index++]];
do_exit : { exit(1); }
do_return : { return locals[ops[index]]; }
do_branch_bool : {
  int32_t from = ops[index++];
  if (locals[from].num) {
    index = ops[index + 1];
    goto *ptrs[ops[index++]];
  } else {
    index = ops[index];
    goto *ptrs[ops[index++]];
  }
}
do_store_reg : {
  int32_t to = ops[index++];
  int32_t from = ops[index++];
  locals[to] = locals[from];
  goto *ptrs[ops[index++]];
}
do_store_int : {
  int32_t to = ops[index++];
  int32_t from = ops[index++];
  locals[to] = (vm_obj_t){.num = from};
  goto *ptrs[ops[index++]];
}
do_jump : {
  index = ops[index];
  goto *ptrs[ops[index++]];
}
do_func : {
  index = ops[index];
  goto *ptrs[ops[index++]];
}
do_add : {
  int32_t to = ops[index++];
  int32_t lhs = ops[index++];
  int32_t rhs = ops[index++];
  locals[to] = (vm_obj_t){.num = locals[lhs].num + locals[rhs].num};
  goto *ptrs[ops[index++]];
}
do_sub : {
  int32_t to = ops[index++];
  int32_t lhs = ops[index++];
  int32_t rhs = ops[index++];
  locals[to] = (vm_obj_t){.num = locals[lhs].num - locals[rhs].num};
  goto *ptrs[ops[index++]];
}
do_mul : {
  int32_t to = ops[index++];
  int32_t lhs = ops[index++];
  int32_t rhs = ops[index++];
  locals[to] = (vm_obj_t){.num = locals[lhs].num * locals[rhs].num};
  goto *ptrs[ops[index++]];
}
do_div : {
  int32_t to = ops[index++];
  int32_t lhs = ops[index++];
  int32_t rhs = ops[index++];
  locals[to] = (vm_obj_t){.num = locals[lhs].num / locals[rhs].num};
  goto *ptrs[ops[index++]];
}
do_mod : {
  int32_t to = ops[index++];
  int32_t lhs = ops[index++];
  int32_t rhs = ops[index++];
  locals[to] = (vm_obj_t){.num = locals[lhs].num % locals[rhs].num};
  goto *ptrs[ops[index++]];
}
do_blte : {
  vm_obj_t lhs = locals[ops[index++]];
  vm_obj_t rhs = locals[ops[index++]];
  index = ops[index + (lhs.num <= rhs.num)];
  goto *ptrs[ops[index++]];
}
do_call : {
  int32_t outreg = ops[index++];
  int32_t next_func = ops[index++];
  int32_t nargs = ops[index++];
  for (int32_t argno = 1; argno <= nargs; argno++) {
    int32_t regno = ops[index++];
    next_locals[argno] = locals[regno];
  }
  locals[outreg] = vm_run_from(ops, next_func, next_locals,
                               next_locals + ops[next_func - 1]);
  goto *ptrs[ops[index++]];
}
do_putchar : {
  int32_t from = ops[index++];
  int32_t val = locals[from].num;
  printf("%c", val);
  goto *ptrs[ops[index++]];
}
do_string : {
  int32_t outreg = ops[index++];
  int32_t nargs = ops[index++];
  vm_gc_entry_t *str = vm_array_new(nargs);
  for (size_t i = 0; i < nargs; i++) {
    int32_t num = ops[index++];
    str->arr[i] = (vm_obj_t){.num = num};
  }
  locals[outreg] = (vm_obj_t){.ptr = str};
  goto *ptrs[ops[index++]];
}
do_length : {
  int32_t outreg = ops[index++];
  vm_obj_t vec = locals[ops[index++]];
  locals[outreg] = (vm_obj_t){.num = vec.ptr->len};
  goto *ptrs[ops[index++]];
}
do_get : {
  int32_t outreg = ops[index++];
  vm_obj_t vec = locals[ops[index++]];
  vm_obj_t oindex = locals[ops[index++]];
  locals[outreg] = vec.ptr->arr[oindex.num];
  goto *ptrs[ops[index++]];
}
do_set : {
  vm_obj_t vec = locals[ops[index++]];
  vm_obj_t oindex = locals[ops[index++]];
  vm_obj_t value = locals[ops[index++]];
  vec.ptr->arr[oindex.num] = value;
  goto *ptrs[ops[index++]];
}
do_dump : {
  int32_t namreg = ops[index++];
  vm_gc_entry_t *sname = locals[namreg].ptr;
  int32_t slen = sname->len;
  char *name = malloc(sizeof(char) * (slen + 1));
  for (int32_t i = 0; i < slen; i++) {
    vm_obj_t obj = sname->arr[i];
    name[i] = obj.num;
  }
  name[slen] = '\0';
  vm_gc_entry_t *ent = locals[ops[index++]].ptr;
  int32_t xlen = ent->len;
  FILE *out = fopen(name, "wb");
  free(name);
  for (int32_t i = 0; i < xlen; i++) {
    vm_obj_t obj = ent->arr[i];
    int32_t op = obj.num;
    fwrite(&op, sizeof(int32_t), 1, out);
  }
  fclose(out);
  goto *ptrs[ops[index++]];
}
do_read : {
  int32_t outreg = ops[index++];
  vm_gc_entry_t *sname = locals[ops[index++]].ptr;
  int32_t slen = sname->len;
  char *name = malloc(sizeof(char) * (slen + 1));
  for (int32_t i = 0; i < slen; i++) {
    name[i] = sname->arr[i].num;
  }
  name[slen] = '\0';
  int32_t where = 0;
  int32_t nalloc = 64;
  FILE *in = fopen(name, "rb");
  free(name);
  if (in == (void *)0) {
    locals[outreg] = (vm_obj_t){.ptr = vm_array_new(0)};
    goto *ptrs[ops[index++]];
  }
  uint8_t *str = malloc(sizeof(uint8_t) * nalloc);
  for (;;) {
    uint8_t buf[2048];
    int32_t n = fread(buf, 1, 2048, in);
    for (int32_t i = 0; i < n; i++) {
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
  for (int32_t i = 0; i < where; i++) {
    ent->arr[i] = (vm_obj_t){.num = str[i]};
  }
  free(str);
  locals[outreg] = (vm_obj_t){.ptr = ent};
  goto *ptrs[ops[index++]];
}
do_write : {
  int32_t outreg = ops[index++];
  vm_gc_entry_t *sname = locals[outreg].ptr;
  int32_t slen = sname->len;
  char *name = malloc(sizeof(char) * (slen + 1));
  for (int32_t i = 0; i < slen; i++) {
    vm_obj_t obj = sname->arr[i];
    name[i] = obj.num;
  }
  name[slen] = '\0';
  vm_gc_entry_t *ent = locals[ops[index++]].ptr;
  int32_t xlen = ent->len;
  FILE *out = fopen(name, "wb");
  free(name);
  for (int32_t i = 0; i < xlen; i++) {
    vm_obj_t obj = ent->arr[i];
    uint8_t op = obj.num;
    fwrite(&op, 1, sizeof(uint8_t), out);
  }
  fclose(out);
  goto *ptrs[ops[index++]];
}
do_array : {
  int32_t outreg = ops[index++];
  int32_t nargs = ops[index++];
  vm_gc_entry_t *vec = vm_array_new(nargs);
  for (int32_t i = 0; i < nargs; i++) {
    int32_t vreg = ops[index++];
    vec->arr[i] = locals[vreg];
  }
  locals[outreg] = (vm_obj_t){.ptr = vec};
  goto *ptrs[ops[index++]];
}
do_cat : {
  int32_t to = ops[index++];
  vm_gc_entry_t *left = locals[ops[index++]].ptr;
  vm_gc_entry_t *right = locals[ops[index++]].ptr;
  vm_gc_entry_t *ent = vm_array_new(left->len + right->len);
  for (int32_t i = 0; i < left->len; i++) {
    ent->arr[i] = left->arr[i];
  }
  for (int32_t i = 0; i < right->len; i++) {
    ent->arr[left->len + i] = right->arr[i];
  }
  locals[to] = (vm_obj_t){.ptr = ent};
  goto *ptrs[ops[index++]];
}
do_beq : {
  vm_obj_t lhs = locals[ops[index++]];
  vm_obj_t rhs = locals[ops[index++]];
  index = ops[index + (lhs.num == rhs.num)];
  goto *ptrs[ops[index++]];
}
do_blt : {
  vm_obj_t lhs = locals[ops[index++]];
  vm_obj_t rhs = locals[ops[index++]];
  index = ops[index + (lhs.num < rhs.num)];
  goto *ptrs[ops[index++]];
}
}

/// allocates locals for the program and calls the vm hot loop
void vm_run(const int32_t *ops, int32_t nargs, const char **args) {
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
  int32_t *ops = malloc(sizeof(int32_t) * nalloc);
  size_t nops = 0;
  size_t size;
  for (;;) {
    int32_t op = 0;
    size = fread(&op, sizeof(int32_t), 1, file);
    if (size == 0) {
      break;
    }
    if (nops + 1 >= nalloc) {
      nalloc *= 4;
      ops = realloc(ops, sizeof(int32_t) * nalloc);
    }
    ops[nops++] = op;
  }
  fclose(file);
  vm_run(ops, argc - 2, argv + 2);
  free(ops);
}
