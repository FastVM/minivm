
#ifdef __COSMO__
#include <cosmopolitan.h>
#else
#include <ctype.h>
#include <gc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define malloc(n) (GC_malloc((n)))
#define calloc(e, n) (GC_malloc((e) * (n)))
#define realloc(ptr, n) (GC_realloc((ptr), (n)))
#define free(n) (GC_free((n)))
#endif

struct obj_t;
struct func_t;
struct vm_t;
union value_t;
enum type_t;

typedef struct obj_t obj_t;
typedef struct array_t array_t;
typedef struct vm_t vm_t;
typedef struct func_t func_t;
typedef enum type_t type_t;
typedef enum form_t form_t;

obj_t run(vm_t *vm, func_t func, size_t argc, obj_t *argv);

enum type_t {
  TYPE_NONE,
  TYPE_BOOLEAN,
  TYPE_NUMBER,
  TYPE_POINTER,
  TYPE_FUNCTION,
};

enum form_t {
  FORM_NONE,
  FORM_ADD,
  FORM_SUB,
  FORM_MUL,
  FORM_DIV,
  FORM_MOD,
  FORM_LT,
  FORM_GT,
  FORM_LTE,
  FORM_GTE,
  FORM_EQ,
  FORM_NEQ,
  FORM_IF,
  FORM_REC,
  FORM_BLOCK,
  FORM_PRINT,
  FORM_DEFINE,
  FORM_FUNCTION,
};

struct vm_t {
  array_t *frames;
  array_t *linear;
};

struct array_t {
  int length;
  int alloc;
  char values[0];
};

struct obj_t {
  type_t type;
  union {
    bool boolean;
    double number;
    struct {
      array_t *pointer;
    };
    func_t *function;
  };
};

enum local_flags_t {
  LOCAL_FLAGS_NONE = 0,
  LOCAL_FLAGS_ARG = 1,
};

enum capture_from_t {
  CAPTURE_FROM_LOCAL,
  CAPTURE_FROM_ARGS,
  CAPTURE_FROM_CAPTURE,
};

struct func_t {
  array_t *bytecode;
  array_t *constants;
  func_t *parent;
  array_t *local_names;
  array_t *local_flags;
  array_t *capture_names;
  array_t *capture_from;
  array_t *capture_flags;
  int stack_used;
  int locals_used;
  array_t *captured;
};

enum opcode {
  OPCODE_RETURN,
  OPCODE_EXIT,
  OPCODE_PUSH,
  OPCODE_POP,
  OPCODE_ARG,
  OPCODE_STORE,
  OPCODE_LOAD,
  OPCODE_LOADC,
  OPCODE_ADD,
  OPCODE_SUB,
  OPCODE_MUL,
  OPCODE_DIV,
  OPCODE_MOD,
  OPCODE_NEG,
  OPCODE_LT,
  OPCODE_GT,
  OPCODE_LTE,
  OPCODE_GTE,
  OPCODE_EQ,
  OPCODE_NEQ,
  OPCODE_PRINT,
  OPCODE_JUMP,
  OPCODE_IFTRUE,
  OPCODE_IFFALSE,
  OPCODE_CALL,
  OPCODE_REC,
  OPCODE_FUNC,
  OPCODE_MAX1,
};

array_t *array_new(int elem_size) {
  array_t *ret = malloc(sizeof(array_t) + elem_size * 4 + 4);
  ret->length = 0;
  ret->alloc = 4;
  return ret;
}

void array_ensure(int elem_size, array_t **arr, int index) {
  if (((*arr)->length + index) * elem_size >= (*arr)->alloc) {
    (*arr)->alloc *= 4;
    *arr = realloc(*arr, sizeof(array_t) + elem_size * (*arr)->alloc + 4);
  }
}

void *no = NULL;
#define seg (*(void *)1)

void *array_index(int elem_size, array_t *arr, int index) {
  if (index < 0) {
    index += arr->length;
  }
  if (index >= arr->length) {
    fprintf(stderr, "bounds error: %x\n", index);
    exit(1);
  }
  return (void *)(arr->values + elem_size * index);
}

void array_push(int elem_size, array_t **arr, void *value) {
  array_ensure(elem_size, arr, 1);
  memcpy((*arr)->values + (*arr)->length * elem_size, value, elem_size);
  (*arr)->length++;
}

void *array_pop(int elem_size, array_t **arr_ptr) {
  array_t *arr = *arr_ptr;
  arr->length--;
  return arr->values + arr->length * elem_size;
}

#define array_new(type) array_new(sizeof(type))
#define array_ensure(type, arr, ...)                                           \
  array_ensure(sizeof(type), &(arr), (__VA_ARGS__))
#define array_push(type, arr, ...)                                             \
  ({                                                                           \
    type pushval = (__VA_ARGS__);                                              \
    array_push(sizeof(type), &(arr), &(pushval));                              \
  })
#define array_pop(type, arr) ((type *)array_pop(sizeof(type), &(arr)))
#define array_index(type, arr, index)                                          \
  ((type *)array_index(sizeof(type), (arr), (index)))
#define array_ptr(type, arr) ((type *)(arr)->values)

void print(obj_t arg) {
  switch (arg.type) {
  default:
    fprintf(stderr, "type error: unknown type");
    exit(1);
  case TYPE_NONE:
    printf("none");
    break;
  case TYPE_BOOLEAN:
    if (arg.boolean) {
      printf("true");
    } else {
      printf("false");
    }
    break;
  case TYPE_NUMBER:
    printf("%lg", arg.number);
    break;
  case TYPE_POINTER:
    printf("%zx", (size_t)arg.pointer);
    break;
  case TYPE_FUNCTION:
    printf("%p", arg.function);
    break;
  }
}

#ifdef VM_DEBUG
#define debug_op                                                               \
  printf("%zu: %i\n", cur.index, array_ptr(int, cur.func.bytecode)[cur.index])
#define run_next_op                                                            \
  debug_op;                                                                    \
  goto *ptrs[array_ptr(int, cur.func.bytecode)[cur.index++]];
#else
#define run_next_op goto *ptrs[array_ptr(int, cur.func.bytecode)[cur.index++]];
#endif

typedef struct {
  size_t index;
  func_t func;
  size_t argc;
  obj_t *argv;
  obj_t *stack;
  obj_t *locals;
} stack_frame_t;

obj_t run(vm_t *vm, func_t basefunc, size_t argc, obj_t *argv) {
  stack_frame_t next = (stack_frame_t){
      .func = basefunc,
      .argc = argc,
      .argv = argv,
  };
  stack_frame_t cur = (stack_frame_t){
      .stack = NULL,
      .locals = NULL,
  };
  static void *ptrs[OPCODE_MAX1];
  ptrs[OPCODE_RETURN] = &&do_return;
  ptrs[OPCODE_EXIT] = &&do_exit;
  ptrs[OPCODE_PUSH] = &&do_push;
  ptrs[OPCODE_POP] = &&do_pop;
  ptrs[OPCODE_ARG] = &&do_arg;
  ptrs[OPCODE_STORE] = &&do_store;
  ptrs[OPCODE_LOAD] = &&do_load;
  ptrs[OPCODE_LOADC] = &&do_loadc;
  ptrs[OPCODE_ADD] = &&do_add;
  ptrs[OPCODE_SUB] = &&do_sub;
  ptrs[OPCODE_MUL] = &&do_mul;
  ptrs[OPCODE_DIV] = &&do_div;
  ptrs[OPCODE_MOD] = &&do_mod;
  ptrs[OPCODE_NEG] = &&do_neg;
  ptrs[OPCODE_LT] = &&do_lt;
  ptrs[OPCODE_GT] = &&do_gt;
  ptrs[OPCODE_LTE] = &&do_lte;
  ptrs[OPCODE_GTE] = &&do_gte;
  ptrs[OPCODE_EQ] = &&do_eq;
  ptrs[OPCODE_NEQ] = &&do_neq;
  ptrs[OPCODE_PRINT] = &&do_print;
  ptrs[OPCODE_JUMP] = &&do_jump;
  ptrs[OPCODE_IFTRUE] = &&do_iftrue;
  ptrs[OPCODE_IFFALSE] = &&do_iffalse;
  ptrs[OPCODE_CALL] = &&do_call;
  ptrs[OPCODE_REC] = &&do_rec;
  ptrs[OPCODE_FUNC] = &&do_func;
rec_call:
  ((stack_frame_t *)vm->frames->values)[vm->frames->length++] = cur;
  cur = next;
  cur.stack =
      (obj_t *)(vm->linear->values + vm->linear->length * sizeof(obj_t));
  vm->linear->length += cur.func.stack_used * sizeof(obj_t);
  cur.locals =
      (obj_t *)(vm->linear->values + vm->linear->length * sizeof(obj_t));
  vm->linear->length += cur.func.locals_used * sizeof(obj_t);
  cur.index = 0;
  run_next_op;
do_return : {
  obj_t retval = *cur.stack;
  vm->linear->length -= cur.func.locals_used * sizeof(obj_t) +
                        cur.func.stack_used * sizeof(obj_t);
  cur = *array_pop(stack_frame_t, vm->frames);
  *cur.stack = retval;
  run_next_op;
}
do_exit : {
  obj_t retval = *cur.stack;
  vm->linear->length -= cur.func.locals_used * sizeof(obj_t) +
                        cur.func.stack_used * sizeof(obj_t);
  cur = *array_pop(stack_frame_t, vm->frames);
  return retval;
}
do_push : {
  *(++cur.stack) = array_ptr(
      obj_t,
      cur.func.constants)[array_ptr(int, cur.func.bytecode)[cur.index++]];
  run_next_op;
}
do_pop : {
  cur.stack--;
  run_next_op;
}
do_arg : {
  *(++cur.stack) = cur.argv[array_ptr(int, cur.func.bytecode)[cur.index++]];
  run_next_op;
}
do_store : {
  cur.locals[array_ptr(int, cur.func.bytecode)[cur.index++]] = *cur.stack;
  run_next_op;
}
do_load : {
  *(++cur.stack) = cur.locals[array_ptr(int, cur.func.bytecode)[cur.index++]];
  run_next_op;
}
do_loadc : {
  *(++cur.stack) = array_ptr(
      obj_t, cur.func.captured)[array_ptr(int, cur.func.bytecode)[cur.index++]];
  run_next_op;
}
do_add : {
  double rhs = (cur.stack--)->number;
  cur.stack->number += rhs;
  run_next_op;
}
do_sub : {
  double rhs = (cur.stack--)->number;
  cur.stack->number -= rhs;
  run_next_op;
}
do_mul : {
  double rhs = (cur.stack--)->number;
  cur.stack->number *= rhs;
  run_next_op;
}
do_div : {
  double rhs = (cur.stack--)->number;
  cur.stack->number /= rhs;
  run_next_op;
}
do_mod : {
  double rhs = (cur.stack--)->number;
  cur.stack->number *= rhs;
  run_next_op;
}
do_neg : {
  cur.stack->number *= -1;
  run_next_op;
}
do_lt : {
  double rhs = (cur.stack--)->number;
  *cur.stack = (obj_t){
      .type = TYPE_BOOLEAN,
      .boolean = cur.stack->number < rhs,
  };
  run_next_op;
}
do_gt : {
  double rhs = (cur.stack--)->number;
  *cur.stack = (obj_t){
      .type = TYPE_BOOLEAN,
      .boolean = cur.stack->number > rhs,
  };
  run_next_op;
}
do_lte : {
  double rhs = (cur.stack--)->number;
  *cur.stack = (obj_t){
      .type = TYPE_BOOLEAN,
      .boolean = cur.stack->number <= rhs,
  };
  run_next_op;
}
do_gte : {
  double rhs = (cur.stack--)->number;
  *cur.stack = (obj_t){
      .type = TYPE_BOOLEAN,
      .boolean = cur.stack->number >= rhs,
  };
  run_next_op;
}
do_eq : {
  double rhs = (cur.stack--)->number;
  *cur.stack = (obj_t){
      .type = TYPE_BOOLEAN,
      .boolean = cur.stack->number == rhs,
  };
  run_next_op;
}
do_neq : {
  double rhs = (cur.stack--)->number;
  *cur.stack = (obj_t){
      .type = TYPE_BOOLEAN,
      .boolean = cur.stack->number != rhs,
  };
  run_next_op;
}
do_print : {
  print(*cur.stack);
  printf("\n");
  cur.stack->type = TYPE_NONE;
  run_next_op;
}
do_jump : {
  cur.index = array_ptr(int, cur.func.bytecode)[cur.index];
  run_next_op;
}
do_iftrue : {
  if ((cur.stack--)->boolean) {
    cur.index = array_ptr(int, cur.func.bytecode)[cur.index];
  } else {
    cur.index++;
  }
  run_next_op;
}
do_iffalse : {
  if (!(cur.stack--)->boolean) {
    cur.index = array_ptr(int, cur.func.bytecode)[cur.index];
  } else {
    cur.index++;
  }
  run_next_op;
}
do_call : {
  int nargs = array_ptr(int, cur.func.bytecode)[cur.index++];
  cur.stack -= nargs;
  next = (stack_frame_t){
      .func = *cur.stack->function,
      .argc = nargs,
      .argv = cur.stack + 1,
  };
  goto rec_call;
}
do_rec : {
  int nargs = array_ptr(int, cur.func.bytecode)[cur.index++];
  cur.stack -= nargs - 1;
  next = (stack_frame_t){
      .argc = nargs,
      .argv = cur.stack,
      .func = cur.func,
  };
  goto rec_call;
}
do_func : {
  func_t *old_func = cur.stack->function;
  func_t *new_func = malloc(sizeof(func_t));
  new_func->bytecode = old_func->bytecode;
  new_func->constants = old_func->constants;
  new_func->parent = old_func->parent;
  new_func->local_names = old_func->local_names;
  new_func->local_flags = old_func->local_flags;
  new_func->capture_names = old_func->capture_names;
  new_func->capture_from = old_func->capture_from;
  new_func->capture_flags = old_func->capture_flags;
  new_func->stack_used = old_func->stack_used;
  new_func->locals_used = old_func->locals_used;
  new_func->captured = array_new(obj_t);
  for (int index = 0; index < new_func->capture_from->length; index++) {
    int flags = array_ptr(int, new_func->capture_flags)[index];
    int from = array_ptr(int, new_func->capture_from)[index];
    switch (flags) {
    case CAPTURE_FROM_LOCAL:
      array_push(obj_t, new_func->captured, cur.locals[from]);
      break;
    case CAPTURE_FROM_ARGS:
      array_push(obj_t, new_func->captured, cur.argv[from]);
      break;
    case CAPTURE_FROM_CAPTURE:
      array_push(obj_t, new_func->captured, array_ptr(obj_t, cur.func.captured)[from]);
      break;
    }
  }
  cur.stack->function = new_func;
  run_next_op;
}
}

void strip(char **code) {
  while (**code == ' ' || **code == '\t' || **code == '\n' || **code == '\r') {
    if (**code == '\0') {
      break;
    }
    *code += 1;
  }
}

char *parse_name(char **code, bool *is_number) {
  strip(code);
  array_t *name = array_new(char);
  *is_number = true;
  while (**code != '\0' && **code != ' ' && **code != '\t' && **code != '\n' &&
         **code != '(' && **code != ')' && **code != '\0' && isprint(**code)) {
    if (!('0' <= **code && **code <= '9')) {
      *is_number = false;
    }
    if (isprint(**code)) {
      array_push(char, name, **code);
    }
    *code += 1;
  }
  array_push(char, name, '\0');
  return name->values;
}

form_t parse(char **code, func_t *func, int depth) {
  if (depth != 0 && **code == '\0') {
    fprintf(stderr, "unmatched parentheses\n");
    exit(1);
  }
  strip(code);
  if (depth == 0) {
    array_push(int, func->bytecode, OPCODE_PUSH);
    array_push(int, func->bytecode, func->constants->length);
    array_push(obj_t, func->constants, (obj_t){.type = TYPE_NONE});
  }
  form_t ret = FORM_NONE;
  if (**code == '(') {
    *code += 1;
    form_t form = parse(code, func, depth + 1);
    switch (form) {
    default:
      break;
    case FORM_IF: {
      parse(code, func, depth + 1);
      array_push(int, func->bytecode, OPCODE_IFFALSE);
      int false_jump = func->bytecode->length;
      array_push(int, func->bytecode, -1);
      parse(code, func, depth + 1);
      array_push(int, func->bytecode, OPCODE_JUMP);
      int exit_jump = func->bytecode->length;
      array_push(int, func->bytecode, -1);
      *array_index(int, func->bytecode, false_jump) = func->bytecode->length;
      no = array_index(int, func->bytecode, false_jump);
      strip(code);
      if (**code != ')') {
        parse(code, func, depth + 1);
      } else {
        array_push(int, func->bytecode, OPCODE_PUSH);
        array_push(int, func->bytecode, func->constants->length);
        array_push(obj_t, func->constants, (obj_t){.type = TYPE_NONE});
      }
      strip(code);
      if (**code != ')') {
        fprintf(stderr, "unmatched parentheses\n");
        fprintf(stderr, "code: %s\n", *code);
        exit(1);
      }
      *code += 1;
      *array_index(int, func->bytecode, exit_jump) = func->bytecode->length;
      goto done;
    };
    case FORM_FUNCTION: {
      func_t *sub_func = malloc(sizeof(func_t));
      *sub_func = (func_t){
          .parent = func,
          .bytecode = array_new(int),
          .constants = array_new(obj_t),
          .local_names = array_new(char *),
          .local_flags = array_new(int),
          .capture_names = array_new(char *),
          .capture_from = array_new(int),
          .capture_flags = array_new(int),
          .stack_used = 16,
          .locals_used = 16,
      };
      strip(code);
      if (**code != '(') {
        fprintf(stderr, "error: function arugment expected\n");
        exit(1);
      }
      *code += 1;
      int nargs = 0;
      while (**code != ')') {
        if (**code == '\0') {
          fprintf(stderr, "unmatched parentheses\n");
          fprintf(stderr, "code: %s\n", *code);
          exit(1);
        }
        bool is_number = true;
        char *arg_name = parse_name(code, &is_number);
        array_push(char *, sub_func->local_names, arg_name);
        array_push(int, sub_func->local_flags, LOCAL_FLAGS_ARG);
        strip(code);
        nargs++;
      }
      *code += 1;
      parse(code, sub_func, depth + 1);
      array_push(int, sub_func->bytecode, OPCODE_RETURN);
      array_push(int, func->bytecode, OPCODE_PUSH);
      array_push(int, func->bytecode, func->constants->length);
      array_push(obj_t, func->constants,
                 (obj_t){.type = TYPE_FUNCTION, .function = sub_func});
      if (sub_func->capture_names->length != 0) {
        array_push(int, func->bytecode, OPCODE_FUNC);
      }
      strip(code);
      *code += 1;
      goto done;
    }
    case FORM_DEFINE: {
      bool is_number = true;
      char *str_value = parse_name(code, &is_number);
      if (is_number) {
        fprintf(stderr, "error: define needs an identifier not %s\n",
                str_value);
        exit(1);
      }
      parse(code, func, depth + 1);
      int argno = 0;
      for (int i = func->local_names->length - 1; i >= 0; i--) {
        int flags = *array_index(int, func->local_flags, i);
        if (flags & LOCAL_FLAGS_ARG) {
          argno++;
        }
        if (!strcmp(str_value, *array_index(char *, func->local_names, i))) {
          array_push(int, func->bytecode, OPCODE_STORE);
          array_push(int, func->bytecode, i - argno);
          goto endef;
        }
      }
      array_push(int, func->bytecode, OPCODE_STORE);
      array_push(int, func->bytecode, func->local_names->length);
      array_push(char *, func->local_names, str_value);
      array_push(int, func->local_flags, LOCAL_FLAGS_NONE);
    endef:
      strip(code);
      *code += 1;
      goto done;
    }
    }
    int args_count = 0;
    while (**code != ')') {
      if (args_count != 0 && form == FORM_BLOCK) {
        array_push(int, func->bytecode, OPCODE_POP);
      }
      parse(code, func, depth + 1);
      strip(code);
      args_count++;
    }
    *code += 1;
    switch (form) {
    case FORM_DEFINE:
      fprintf(stderr, "internal error: define should be handled already\n");
      exit(1);
    case FORM_FUNCTION:
      fprintf(stderr, "internal error: function should be handled already\n");
      exit(1);
    case FORM_IF:
      fprintf(stderr, "internal error: if should be handled already\n");
      exit(1);
    case FORM_REC:
      array_push(int, func->bytecode, OPCODE_REC);
      array_push(int, func->bytecode, args_count);
      goto done;
    case FORM_BLOCK:
      goto done;
    case FORM_PRINT:
      if (args_count != 1) {
        fprintf(stderr, "error: needs one argument: print\n");
        exit(1);
      }
      array_push(int, func->bytecode, OPCODE_PRINT);
      goto done;
    case FORM_NONE:
      array_push(int, func->bytecode, OPCODE_CALL);
      array_push(int, func->bytecode, args_count);
      goto done;
    case FORM_ADD:
      if (args_count == 0) {
        array_push(int, func->bytecode, OPCODE_PUSH);
        array_push(int, func->bytecode, func->constants->length);
        array_push(obj_t, func->constants,
                   (obj_t){.type = TYPE_NUMBER, .number = 0});
      }
      for (int i = 1; i < args_count; i++) {
        array_push(int, func->bytecode, OPCODE_ADD);
      }
      goto done;
    case FORM_SUB:
      if (args_count == 0) {
        fprintf(stderr, "error: too few arguments to: minus\n");
        exit(1);
      }
      if (args_count == 1) {
        array_push(int, func->bytecode, OPCODE_NEG);
      }
      for (int i = 1; i < args_count; i++) {
        array_push(int, func->bytecode, OPCODE_SUB);
      }
      goto done;
    case FORM_MUL:
      if (args_count == 0) {
        array_push(int, func->bytecode, OPCODE_PUSH);
        array_push(int, func->bytecode, func->constants->length);
        array_push(obj_t, func->constants,
                   (obj_t){.type = TYPE_NUMBER, .number = 1});
      }
      for (int i = 1; i < args_count; i++) {
        array_push(int, func->bytecode, OPCODE_MUL);
      }
      goto done;
    case FORM_DIV:
      if (args_count <= 1) {
        fprintf(stderr, "error: too few arguments to: devide\n");
        exit(1);
      }
      for (int i = 1; i < args_count; i++) {
        array_push(int, func->bytecode, OPCODE_DIV);
      }
      goto done;
    case FORM_MOD:
      if (args_count <= 1) {
        fprintf(stderr, "error: too few arguments to: mod\n");
        exit(1);
      }
      for (int i = 1; i < args_count; i++) {
        array_push(int, func->bytecode, OPCODE_MOD);
      }
      goto done;
    case FORM_LT:
      array_push(int, func->bytecode, OPCODE_LT);
      goto done;
    case FORM_GT:
      array_push(int, func->bytecode, OPCODE_GT);
      goto done;
    case FORM_LTE:
      array_push(int, func->bytecode, OPCODE_LTE);
      goto done;
    case FORM_GTE:
      array_push(int, func->bytecode, OPCODE_GTE);
      goto done;
    case FORM_EQ:
      array_push(int, func->bytecode, OPCODE_EQ);
      goto done;
    case FORM_NEQ:
      array_push(int, func->bytecode, OPCODE_NEQ);
      goto done;
    }
  } else {
    bool is_number = true;
    char *str_value = parse_name(code, &is_number);
    if (is_number) {
      array_push(int, func->bytecode, OPCODE_PUSH);
      array_push(int, func->bytecode, func->constants->length);
      array_push(obj_t, func->constants,
                 (obj_t){
                     .type = TYPE_NUMBER,
                     .number = atof(str_value),
                 });
      goto done;
    }
    if (strcmp(str_value, "+") == 0) {
      return FORM_ADD;
    }
    if (strcmp(str_value, "-") == 0) {
      return FORM_SUB;
    }
    if (strcmp(str_value, "*") == 0) {
      return FORM_MUL;
    }
    if (strcmp(str_value, "/") == 0) {
      return FORM_DIV;
    }
    if (strcmp(str_value, "%") == 0) {
      return FORM_MOD;
    }
    if (strcmp(str_value, "<") == 0) {
      return FORM_LT;
    }
    if (strcmp(str_value, ">") == 0) {
      return FORM_GT;
    }
    if (strcmp(str_value, "<=") == 0) {
      return FORM_LTE;
    }
    if (strcmp(str_value, ">=") == 0) {
      return FORM_GTE;
    }
    if (strcmp(str_value, "=") == 0) {
      return FORM_EQ;
    }
    if (strcmp(str_value, "!=") == 0) {
      return FORM_NEQ;
    }
    if (strcmp(str_value, "+") == 0) {
      return FORM_ADD;
    }
    if (strcmp(str_value, "if") == 0) {
      return FORM_IF;
    }
    if (strcmp(str_value, "rec") == 0) {
      return FORM_REC;
    }
    if (strcmp(str_value, "block") == 0) {
      return FORM_BLOCK;
    }
    if (strcmp(str_value, "print") == 0) {
      return FORM_PRINT;
    }
    if (strcmp(str_value, "define") == 0) {
      return FORM_DEFINE;
    }
    if (strcmp(str_value, "function") == 0) {
      return FORM_FUNCTION;
    }
    if (strcmp(str_value, "true") == 0) {
      array_push(int, func->bytecode, OPCODE_PUSH);
      array_push(int, func->bytecode, func->constants->length);
      array_push(obj_t, func->constants,
                 (obj_t){
                     .type = TYPE_BOOLEAN,
                     .boolean = true,
                 });
      goto done;
    }
    if (strcmp(str_value, "false") == 0) {
      array_push(int, func->bytecode, OPCODE_PUSH);
      array_push(int, func->bytecode, func->constants->length);
      array_push(obj_t, func->constants,
                 (obj_t){
                     .type = TYPE_BOOLEAN,
                     .boolean = false,
                 });
      goto done;
    }
    if (strcmp(str_value, "none") == 0) {
      array_push(int, func->bytecode, OPCODE_PUSH);
      array_push(int, func->bytecode, func->constants->length);
      array_push(obj_t, func->constants,
                 (obj_t){
                     .type = TYPE_NONE,
                 });
      goto done;
    }
    int argno = 0;
    for (int i = func->local_names->length - 1; i >= 0; i--) {
      int flags = *array_index(int, func->local_flags, i);
      if (flags & LOCAL_FLAGS_ARG) {
        argno++;
      }
      if (strcmp(str_value, *array_index(char *, func->local_names, i)) == 0) {
        if (flags & LOCAL_FLAGS_ARG) {
          array_push(int, func->bytecode, OPCODE_ARG);
          array_push(int, func->bytecode, i);
        } else {
          array_push(int, func->bytecode, OPCODE_LOAD);
          array_push(int, func->bytecode, i - argno);
        }
        goto done;
      }
    }
    int capno = func->capture_names->length;
    func_t *cur = func;
    while (cur->parent != NULL) {
      func_t *last = cur;
      cur = cur->parent;
      int argno = 0;
      for (int i = cur->local_names->length - 1; i >= 0; i--) {
        int flags = *array_index(int, cur->local_flags, i);
        if (flags & LOCAL_FLAGS_ARG) {
          argno++;
        }
        if (strcmp(str_value, *array_index(char *, cur->local_names, i)) == 0) {
          if (flags & LOCAL_FLAGS_ARG) {
            array_push(char *, last->capture_names, str_value);
            array_push(int, last->capture_from, i);
            array_push(int, last->capture_flags, CAPTURE_FROM_ARGS);
          } else {
            array_push(char *, last->capture_names, str_value);
            array_push(int, last->capture_from, i - argno);
            array_push(int, last->capture_flags, CAPTURE_FROM_LOCAL);
          }
          array_push(int, func->bytecode, OPCODE_LOADC);
          array_push(int, func->bytecode, capno);
          goto done;
        }
      }
      array_push(char *, last->capture_names, str_value);
      array_push(int, last->capture_from, last->parent->capture_names->length);
      array_push(int, last->capture_flags, CAPTURE_FROM_CAPTURE);
    }
    fprintf(stderr, "undefined variable: %s\n", str_value);
    exit(1);
  }
  fprintf(stderr, "bad char: %c (code: %hhi)\n", **code, **code);
  exit(1);
done:
  if (depth == 0) {
    array_push(int, func->bytecode, OPCODE_EXIT);
  }
  return ret;
}

obj_t vm_run(vm_t *vm, char *code) {
  strip(&code);
  char *last = NULL;
  obj_t res = (obj_t){.type = TYPE_NONE};
  while (*code != '\0') {
    func_t func = (func_t){
        .stack_used = 16,
        .locals_used = 16,
        .bytecode = array_new(int),
        .constants = array_new(obj_t),
        .local_names = array_new(char *),
        .local_flags = array_new(int),
        .capture_names = array_new(char *),
        .capture_from = array_new(int),
        .capture_flags = array_new(int),
    };
    parse(&code, &func, 0);
    if (last == code) {
      if (strlen(code) == 0) {
        break;
      }
      fprintf(stderr, "parse error: (remains: `%s`)", code);
      exit(1);
    }
    last = code;
    res = run(vm, func, 0, NULL);
    strip(&code);
  }
  return res;
}

char *slurp(const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    fprintf(stderr, "error: cannot open file: %s\n", filename);
  }
  array_t *buffer = array_new(char);
  while (!feof(fp)) {
    char got = fgetc(fp);
    if (isprint(got)) {
      array_push(char, buffer, got);
    }
  }
  array_push(char, buffer, '\0');
slurp_exit:
  fclose(fp);
  return buffer->values;
}

int main(int argc, char **argv) {
  vm_t *vm = &(vm_t){
      .frames = calloc(1, 1 << 24),
      .linear = calloc(1, 1 << 24),
  };
  int times = 1;
  for (int argno = 1; argno < argc; argno++) {
    char *arg = argv[argno];
    if (arg[0] == '*') {
      times = atof(arg + 1);
      continue;
    }
    char *data = slurp(arg);
    obj_t res = (obj_t){.type = TYPE_NONE};
    for (int i = 0; i < times; i++) {
      res = vm_run(vm, data);
    }
  }
  if (argc == 1) {
    int conts = 0;
    while (true) {
      printf(">>> ");
      fflush(stdout);
      size_t bufsize = 256;
      char *code = malloc(sizeof(char) * bufsize);
      char *ret = fgets(code, bufsize, stdin);
      if (ret == NULL) {
        fprintf(stderr, "internal error: buffer");
        exit(1);
      }
      if (code[0] == '\0') {
        printf("Ctrl-D (exit)");
        return 0;
      }
      obj_t res = vm_run(vm, code);
      if (res.type != TYPE_NONE) {
        print(res);
        printf("\n");
      }
    }
  }
  return 0;
}
