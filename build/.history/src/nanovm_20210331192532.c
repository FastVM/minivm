
#ifdef __COSMO__
#include <cosmopolitan.h>
#else
#include <ctype.h>
#include <execinfo.h>
#include <gc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
  array_t *mem;
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

struct func_t {
  func_t *parent;
  array_t *bytecode;
  array_t *constants;
  array_t *local_names;
  array_t *local_flags;
  int stack_used;
  int locals_used;
};

enum opcode {
  OPCODE_RETURN,
  OPCODE_EXIT,
  OPCODE_PUSH,
  OPCODE_POP,
  OPCODE_ARG,
  OPCODE_STORE,
  OPCODE_LOAD,
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
  OPCODE_MAX1,
};

array_t *array_new(int elem_size) {
  array_t *ret = malloc(sizeof(array_t) + elem_size * 4);
  ret->length = 0;
  ret->alloc = 4;
  return ret;
}

void array_ensure(int elem_size, array_t **arr, int index) {
  if (index + 1 >= (*arr)->alloc) {
    int next_alloc = (*arr)->alloc * 4;
    array_t *new_array = malloc(sizeof(array_t) + elem_size * next_alloc);
    new_array->length = (*arr)->length;
    new_array->alloc = next_alloc;
    memcpy(new_array->values, (*arr)->values, elem_size * next_alloc);
    *arr = new_array;
  }
}

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
  array_ensure(elem_size, arr, (*arr)->length);
  memcpy((*arr)->values + (*arr)->length * elem_size, value, elem_size);
  (*arr)->length++;
}

void *array_pop(int elem_size, array_t **arr_ptr) {
  array_t *arr = *arr_ptr;
  arr->length--;
  return arr->values + arr->length * elem_size;
}

#define array_new(type) array_new(sizeof(type))
#define array_ensure(type, arr, ...) array_ensure(sizeof(type), &(arr), (count))
#define array_push(type, arr, ...)                                             \
  ({                                                                           \
    type pushval = (__VA_ARGS__);                                              \
    array_push(sizeof(type), &(arr), &pushval);                                \
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

inline void *vm_alloca(vm_t *vm, int n) {
  void *ret = (void *)(vm->mem->values + vm->mem->length);
  vm->mem->length += n;
  return ret;
}

inline void *vm_dealloca(vm_t *vm, int n) {
  vm->mem->length -= n;
  void *ret = (void *)(vm->mem->values + vm->mem->length);
  return ret;
}

#define run_next_op goto *ptrs[array_ptr(int, func.bytecode)[index++]];

obj_t run(vm_t *vm, func_t func, size_t argc, obj_t *argv) {
  size_t index = 0;
  func_t func_last = func;
  size_t argc_last = 0;
  obj_t *argv_last = NULL;
  obj_t *stack = NULL;
  obj_t *locals = NULL;
  static void *ptrs[OPCODE_MAX1] = {
      &&do_return, &&do_exit,   &&do_push,    &&do_pop,  &&do_arg,
      &&do_store,  &&do_load,   &&do_add,     &&do_sub,  &&do_mul,
      &&do_div,    &&do_mod,    &&do_neg,     &&do_lt,   &&do_gt,
      &&do_lte,    &&do_gte,    &&do_eq,      &&do_neq,  &&do_print,
      &&do_jump,   &&do_iftrue, &&do_iffalse, &&do_call, &&do_rec};
rec_call:
  *(size_t *)vm_alloca(vm, sizeof(size_t)) = argc_last;
  *(obj_t **)vm_alloca(vm, sizeof(obj_t *)) = argv_last;
  *(func_t *)vm_alloca(vm, sizeof(func_t)) = func_last;
  *(size_t *)vm_alloca(vm, sizeof(size_t)) = index;
  *(obj_t **)vm_alloca(vm, sizeof(obj_t *)) = stack;
  *(obj_t **)vm_alloca(vm, sizeof(obj_t *)) = locals;
  index = 0;
  stack = vm_alloca(vm, func.stack_used * sizeof(obj_t));
  locals = vm_alloca(vm, func.locals_used * sizeof(obj_t));
  run_next_op;
do_return : {
  obj_t retval = *stack;
  vm_dealloca(vm, func.locals_used * sizeof(obj_t));
  vm_dealloca(vm, func.stack_used * sizeof(obj_t));
  locals = *(obj_t **)vm_dealloca(vm, sizeof(obj_t *));
  stack = *(obj_t **)vm_dealloca(vm, sizeof(obj_t *));
  index = *(size_t *)vm_dealloca(vm, sizeof(size_t));
  func = *(func_t *)vm_dealloca(vm, sizeof(func_t));
  argv = *(obj_t **)vm_dealloca(vm, sizeof(obj_t *));
  argc = *(size_t *)vm_dealloca(vm, sizeof(size_t));
  *stack = retval;
  run_next_op;
}
do_exit : {
  obj_t retval = *stack;
  vm_dealloca(vm, func.locals_used * sizeof(obj_t));
  vm_dealloca(vm, func.stack_used * sizeof(obj_t));
  func = *(func_t *)vm_dealloca(vm, sizeof(func_t));
  locals = *(obj_t **)vm_dealloca(vm, sizeof(obj_t *));
  stack = *(obj_t **)vm_dealloca(vm, sizeof(obj_t *));
  index = *(size_t *)vm_dealloca(vm, sizeof(size_t));
  argv = *(obj_t **)vm_dealloca(vm, sizeof(obj_t *));
  argc = *(size_t *)vm_dealloca(vm, sizeof(size_t));
  return retval;
}
do_push : {
  *(++stack) =
      array_ptr(obj_t, func.constants)[array_ptr(int, func.bytecode)[index++]];
  run_next_op;
}
do_pop : {
  stack--;
  run_next_op;
}
do_arg : {
  *(++stack) = argv[array_ptr(int, func.bytecode)[index++]];
  run_next_op;
}
do_store : {
  locals[array_ptr(int, func.bytecode)[index++]] = *stack;
  run_next_op;
}
do_load : {
  *(++stack) = locals[array_ptr(int, func.bytecode)[index++]];
  run_next_op;
}
do_add : {
  double rhs = (stack--)->number;
  stack->number += rhs;
  run_next_op;
}
do_sub : {
  double rhs = (stack--)->number;
  stack->number -= rhs;
  run_next_op;
}
do_mul : {
  double rhs = (stack--)->number;
  stack->number *= rhs;
  run_next_op;
}
do_div : {
  double rhs = (stack--)->number;
  stack->number /= rhs;
  run_next_op;
}
do_mod : {
  double rhs = (stack--)->number;
  stack->number *= rhs;
  run_next_op;
}
do_neg : {
  stack->number *= -1;
  run_next_op;
}
do_lt : {
  double rhs = (stack--)->number;
  *stack = (obj_t){
      .type = TYPE_BOOLEAN,
      .boolean = stack->number < rhs,
  };
  run_next_op;
}
do_gt : {
  double rhs = (stack--)->number;
  *stack = (obj_t){
      .type = TYPE_BOOLEAN,
      .boolean = stack->number > rhs,
  };
  run_next_op;
}
do_lte : {
  double rhs = (stack--)->number;
  *stack = (obj_t){
      .type = TYPE_BOOLEAN,
      .boolean = stack->number <= rhs,
  };
  run_next_op;
}
do_gte : {
  double rhs = (stack--)->number;
  *stack = (obj_t){
      .type = TYPE_BOOLEAN,
      .boolean = stack->number >= rhs,
  };
  run_next_op;
}
do_eq : {
  double rhs = (stack--)->number;
  *stack = (obj_t){
      .type = TYPE_BOOLEAN,
      .boolean = stack->number == rhs,
  };
  run_next_op;
}
do_neq : {
  double rhs = (stack--)->number;
  *stack = (obj_t){
      .type = TYPE_BOOLEAN,
      .boolean = stack->number != rhs,
  };
  run_next_op;
}
do_print : {
  print(*stack);
  printf("\n");
  stack->type = TYPE_NONE;
  run_next_op;
}
do_jump : {
  index = array_ptr(int, func.bytecode)[index];
  run_next_op;
}
do_iftrue : {
  if ((stack--)->boolean) {
    index = array_ptr(int, func.bytecode)[index];
  } else {
    index++;
  }
  run_next_op;
}
do_iffalse : {
  if (!(stack--)->boolean) {
    index = array_ptr(int, func.bytecode)[index];
  } else {
    index++;
  }
  run_next_op;
}
do_call : {
  func_last = func;
  argc_last = argc;
  argv_last = argv;
  int nargs = array_ptr(int, func.bytecode)[index++];
  stack -= nargs;
  func = *stack->function;
  argc = nargs;
  argv = stack + 1;
  goto rec_call;
}
do_rec : {
  func_last = func;
  argc_last = argc;
  argv_last = argv;
  int nargs = array_ptr(int, func.bytecode)[index++];
  stack -= nargs - 1;
  argc = nargs;
  argv = stack;
  goto rec_call;
}
}

void strip(char **code) {
  while (**code == ' ' || **code == '\t' || **code == '\n' || **code == '\r') {
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

int main(int argc, char **argv) {
  array_t *mem = malloc(sizeof(array_t) + (1 << 20));
  mem->length = 0;
  mem->alloc = 1 << 16;
  vm_t *vm = &(vm_t){
      .mem = mem,
  };
  for (int arg = 1; arg < argc; arg++) {
    obj_t res = vm_run(vm, strdup(argv[arg]));
    if (res.type != TYPE_NONE) {
      print(res);
      printf("\n");
    }
  }
  if (argc == 1) {
    int conts = 0;
    while (true) {
      printf(">>> ");
      fflush(stdout);
      size_t bufsize = 256;
      char *code = malloc(sizeof(char) * bufsize);
      int read = getline(&code, &bufsize, stdin);
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
