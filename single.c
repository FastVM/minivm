
union vm_obj_t;
struct vm_gc_entry_t;
struct FILE;

typedef __SIZE_TYPE__ size_t;
typedef __INT32_TYPE__ int32_t;
typedef struct FILE FILE;
typedef union vm_obj_t vm_obj_t;
typedef struct vm_gc_entry_t vm_gc_entry_t;

size_t strlen(const char *str);

void *GC_malloc(size_t size);
void *GC_realloc(void *ptr, size_t n);
void GC_free(void *ptr);

int printf(const char *src, ...);
FILE *fopen(const char *src, const char *name);
int fclose(FILE *);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

typedef struct {
  int32_t outreg;
  int32_t index;
  int32_t nlocals;
} vm_stack_frame_t;

union vm_obj_t {
  _Bool log;
  int32_t num;
  vm_gc_entry_t *ptr;
};

struct vm_gc_entry_t {
  size_t len;
  vm_obj_t arr[0];
};

void GC_init(void);
void *GC_malloc(size_t size);

vm_gc_entry_t *vm_gc_static_array_new(size_t size) {
  vm_gc_entry_t *ent =
      GC_malloc(sizeof(vm_gc_entry_t) + sizeof(vm_obj_t) * size);
  ent->len = size;
  return ent;
}

vm_obj_t vm_global_from(size_t len, const char **args) {
  vm_gc_entry_t *global = vm_gc_static_array_new(len);
  for (size_t i = 0; i < len; i++) {
    vm_gc_entry_t *ent = vm_gc_static_array_new(strlen(args[i]));
    for (const char *src = args[i]; *src != '\0'; src++) {
      ent->arr[src - args[i]] = (vm_obj_t){.num = *src};
    }
    global->arr[i] = (vm_obj_t){.ptr = ent};
  }
  return (vm_obj_t){.ptr = global};
}

void vm_run(const int32_t *ops, size_t nargs, const char **args) {
  vm_obj_t *locals = GC_malloc(sizeof(vm_obj_t) * (1 << 16));
  locals[0] = vm_global_from(nargs, args);
  size_t index = 0;
  vm_stack_frame_t *frame = GC_malloc(sizeof(vm_stack_frame_t) * (1 << 12));
  frame->nlocals = 256;
  static void *ptrs[] = {
      [0] = &&do_exit,
      [1] = &&do_store_reg,
      [2] = &&do_store_bool,
      [3] = &&do_store_int,
      [4] = &&do_jump,
      [5] = &&do_jump,
      [6] = &&do_add,
      [7] = &&do_sub,
      [8] = &&do_mul,
      [9] = &&do_div,
      [10] = &&do_mod,
      [11] = &&do_pow,
      [12] = &&do_static_call,
      [13] = &&do_return,
      [14] = &&do_putchar,
      [15] = &&do_string_new,
      [16] = &&do_length,
      [17] = &&do_index_get,
      [18] = &&do_index_set,
      [19] = &&do_dump,
      [20] = &&do_read,
      [21] = &&do_write,
      [22] = &&do_static_array_new,
      [23] = &&do_static_concat,
      [24] = &&do_branch_equal,
      [25] = &&do_branch_less,
      [26] = &&do_branch_less_than_equal,
      [27] = &&do_branch_bool,
      [28] = &&do_inc,
  };
  goto *ptrs[ops[index++]];
do_exit : { return; }
do_return : {
  vm_obj_t val = locals[ops[index++]];
  frame--;
  locals = locals - frame->nlocals;
  int32_t outreg = frame->outreg;
  locals[outreg] = val;
  index = frame->index;
  goto *ptrs[ops[index++]];
}
do_store_bool : {
  int32_t to = ops[index++];
  int32_t from = (int)ops[index++];
  locals[to] = (vm_obj_t){.log = (_Bool)from};
  goto *ptrs[ops[index++]];
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
do_pow : {
  int32_t to = ops[index++];
  int32_t lhs = locals[ops[index++]].num;
  int32_t rhs = locals[ops[index++]].num;
  int32_t res = 1;
  while (rhs > 0) {
    if (rhs % 2 == 1) {
      res *= lhs;
    }
    rhs /= 2;
    lhs *= rhs;
  }
  locals[to] = (vm_obj_t){.num = res};
  goto *ptrs[ops[index++]];
}
do_static_call : {
  int32_t outreg = ops[index++];
  int32_t next_func = ops[index++];
  int32_t nargs = ops[index++];
  vm_obj_t *next_locals = locals + frame->nlocals;
  for (int32_t argno = 1; argno <= nargs; argno++) {
    int32_t regno = ops[index++];
    next_locals[argno] = locals[regno];
  }
  locals = next_locals;
  frame->index = index;
  frame->outreg = outreg;
  frame++;
  frame->nlocals = ops[next_func - 1];
  index = next_func;
  goto *ptrs[ops[index++]];
}
do_putchar : {
  int32_t from = ops[index++];
  int32_t val = locals[from].num;
  printf("%c", val);
  goto *ptrs[ops[index++]];
}
do_string_new : {
  int32_t outreg = ops[index++];
  int32_t nargs = ops[index++];
  vm_gc_entry_t *str = vm_gc_static_array_new(nargs);
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
do_index_get : {
  int32_t outreg = ops[index++];
  vm_obj_t vec = locals[ops[index++]];
  vm_obj_t oindex = locals[ops[index++]];
  locals[outreg] = vec.ptr->arr[oindex.num];
  goto *ptrs[ops[index++]];
}
do_index_set : {
  vm_obj_t vec = locals[ops[index++]];
  vm_obj_t oindex = locals[ops[index++]];
  vm_obj_t value = locals[ops[index++]];
  vec.ptr->arr[oindex.num] = value;
  goto *ptrs[ops[index++]];
}
do_dump : {
  int32_t namreg = ops[index++];
  int32_t inreg = ops[index++];

  vm_gc_entry_t *sname = locals[namreg].ptr;
  int32_t slen = sname->len;
  char *name = GC_malloc(sizeof(char) * (slen + 1));
  for (int32_t i = 0; i < slen; i++) {
    vm_obj_t obj = sname->arr[i];
    name[i] = obj.num;
  }
  name[slen] = '\0';

  vm_gc_entry_t *ent = locals[inreg].ptr;
  size_t size = sizeof(int32_t);
  int32_t xlen = ent->len;
  FILE *out = fopen(name, "wb");
  fwrite(&size, 1, 1, out);
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
  int32_t namereg = ops[index++];
  vm_gc_entry_t *sname = locals[namereg].ptr;
  int32_t slen = sname->len;
  char *name = GC_malloc(sizeof(char) * (slen + 1));
  for (int32_t i = 0; i < slen; i++) {
    vm_obj_t obj = sname->arr[i];
    name[i] = obj.num;
  }
  name[slen] = '\0';
  int32_t where = 0;
  int32_t nalloc = 64;
  FILE *in = fopen(name, "rb");
  if (in == (void *)0) {
    locals[outreg] = (vm_obj_t){.ptr = vm_gc_static_array_new(0)};
    goto *ptrs[ops[index++]];
  }
  char *str = GC_malloc(sizeof(char) * nalloc);
  for (;;) {
    char buf[2048];
    int32_t n = fread(buf, 1, 2048, in);
    for (int32_t i = 0; i < n; i++) {
      if (where + 4 >= nalloc) {
        nalloc = 4 + nalloc * 2;
        str = GC_realloc(str, sizeof(char) * nalloc);
      }
      str[where] = buf[i];
      where += 1;
    }
    if (n < 2048) {
      break;
    }
  }
  fclose(in);
  vm_gc_entry_t *ent = vm_gc_static_array_new(where);
  for (int32_t i = 0; i < where; i++) {
    ent->arr[i] = (vm_obj_t){.num = str[i]};
  }
  locals[outreg] = (vm_obj_t){.ptr = ent};
  goto *ptrs[ops[index++]];
}
do_write : {
  int32_t outreg = ops[index++];
  int32_t inreg = ops[index++];
  vm_gc_entry_t *sname = locals[outreg].ptr;
  int32_t slen = sname->len;
  char *name = GC_malloc(sizeof(char) * (slen + 1));
  for (int32_t i = 0; i < slen; i++) {
    vm_obj_t obj = sname->arr[i];
    name[i] = obj.num;
  }
  name[slen] = '\0';
  vm_gc_entry_t *ent = locals[inreg].ptr;
  int32_t xlen = ent->len;
  FILE *out = fopen(name, "wb");
  for (int32_t i = 0; i < xlen; i++) {
    vm_obj_t obj = ent->arr[i];
    char op = obj.num;
    fwrite(&op, 1, sizeof(char), out);
  }
  fclose(out);
  goto *ptrs[ops[index++]];
}
do_static_array_new : {
  int32_t outreg = ops[index++];
  int32_t nargs = ops[index++];
  vm_gc_entry_t *vec = vm_gc_static_array_new(nargs);
  for (int32_t i = 0; i < nargs; i++) {
    int32_t vreg = ops[index++];
    vec->arr[i] = locals[vreg];
  }
  locals[outreg] = (vm_obj_t){.ptr = vec};
  goto *ptrs[ops[index++]];
}
do_static_concat : {
  int32_t to = ops[index++];
  vm_gc_entry_t *left = locals[ops[index++]].ptr;
  vm_gc_entry_t *right = locals[ops[index++]].ptr;
  vm_gc_entry_t *ent = vm_gc_static_array_new(left->len + right->len);
  for (int32_t i = 0; i < left->len; i++) {
    ent->arr[i] = left->arr[i];
  }
  for (int32_t i = 0; i < right->len; i++) {
    ent->arr[left->len + i] = right->arr[i];
  }
  locals[to] = (vm_obj_t){.ptr = ent};
  goto *ptrs[ops[index++]];
}
do_branch_equal : {
  vm_obj_t lhs = locals[ops[index++]];
  vm_obj_t rhs = locals[ops[index++]];
  if ((lhs).num == (rhs).num) {
    index = ops[index + 1];
    goto *ptrs[ops[index++]];
  } else {
    index = ops[index];
    goto *ptrs[ops[index++]];
  }
}
do_branch_less : {
  vm_obj_t lhs = locals[ops[index++]];
  vm_obj_t rhs = locals[ops[index++]];
  if ((lhs).num < (rhs).num) {
    index = ops[index + 1];
    goto *ptrs[ops[index++]];
  } else {
    index = ops[index];
    goto *ptrs[ops[index++]];
  }
}
do_branch_less_than_equal : {
  vm_obj_t lhs = locals[ops[index++]];
  vm_obj_t rhs = locals[ops[index++]];
  if ((lhs).num <= (rhs).num) {
    index = ops[index + 1];
    goto *ptrs[ops[index++]];
  } else {
    index = ops[index];
    goto *ptrs[ops[index++]];
  }
}
do_branch_bool : {
  int32_t from = ops[index++];
  if (locals[from].log) {
    index = ops[index + 1];
    goto *ptrs[ops[index++]];
  } else {
    index = ops[index];
    goto *ptrs[ops[index++]];
  }
}
do_inc : {
  int32_t to = ops[index++];
  int32_t lhs = ops[index++];
  int32_t rhs = ops[index++];
  locals[to] = (vm_obj_t){.num = locals[lhs].num + (rhs)};
  goto *ptrs[ops[index++]];
}
}

int main(int argc, const char **argv) {
  GC_init();
  if (argc < 2) {
    printf("cannot run vm: not enough args\n");
    return 1;
  }
  FILE *file = fopen(argv[1], "rb");
  if (file == (void *)0) {
    printf("cannot run vm: file to run could not be read\n");
    return 2;
  }
  unsigned char nver = 0;
  fread(&nver, 1, 1, file);
  if (nver != 4) {
    printf("cannot run vm: bytecode file header wanted 4: got %hhu\n", nver);
    return 3;
  }
  int32_t *ops = GC_malloc(sizeof(int32_t) * (1 << 20));
  size_t nops = 0;
  for (;;) {
    int32_t op = 0;
    size_t size = fread(&op, sizeof(int32_t), 1, file);
    if (size == 0) {
      break;
    }
    ops[nops++] = op;
  }
  vm_run(ops, argc-2, argv+2);
}
