#include <vm/vm.h>

#define VM_FRAME_NUM ((16))

#ifdef __clang__
#define vm_assume(expr) (__builtin_assume(expr))
#else
#define vm_assume(expr) (__builtin_expect(expr, true))
#endif

#define vm_fetch (next_op_value = ptrs[basefunc[cur_index]])
#define next_op (cur_index++, next_op_value)

typedef struct
{
  int index;
  int func;
  int bytecode;
  int outreg;
  int nregs;
} stack_frame_t;

#define vm_set_frame(frame_arg)          \
  (                                      \
      {                                  \
        stack_frame_t frame = frame_arg; \
        cur_index = frame.index;         \
        cur_func = frame.func;           \
      })

#define run_next_op goto *next_op;
#define cur_bytecode_next(Type)                   \
  (                                               \
      {                                           \
        Type ret = *(Type *)&basefunc[cur_index]; \
        cur_index += sizeof(Type);                \
        ret;                                      \
      })

#define read_reg (cur_bytecode_next(int))
#define read_bool (cur_bytecode_next(bool))
#define read_int (cur_bytecode_next(int))
#define read_num (cur_bytecode_next(int))
#define read_loc (cur_bytecode_next(int))

void vm_run(opcode_t *basefunc)
{
  int allocn = VM_FRAME_NUM;
  stack_frame_t *frames_base = calloc(1, sizeof(stack_frame_t) * allocn);
  int locals_allocated = 16 * VM_FRAME_NUM;
  nanbox_t *locals_base = calloc(1, sizeof(nanbox_t) * locals_allocated);

  stack_frame_t *cur_frame = frames_base;
  nanbox_t *cur_locals = locals_base;
  int cur_index = 0;
  int cur_func = 0;

  void *next_op_value;
  void *ptrs[OPCODE_MAX2P] = {NULL};
  ptrs[OPCODE_EXIT] = &&do_exit;
  ptrs[OPCODE_STORE_REG] = &&do_store_reg;
  ptrs[OPCODE_STORE_LOG] = &&do_store_log;
  ptrs[OPCODE_STORE_NUM] = &&do_store_num;
  ptrs[OPCODE_STORE_FUN] = &&do_store_fun;
  ptrs[OPCODE_EQUAL] = &&do_equal;
  ptrs[OPCODE_EQUAL_NUM] = &&do_equal_num;
  ptrs[OPCODE_NOT_EQUAL] = &&do_not_equal;
  ptrs[OPCODE_NOT_EQUAL_NUM] = &&do_not_equal_num;
  ptrs[OPCODE_LESS] = &&do_less;
  ptrs[OPCODE_LESS_NUM] = &&do_less_num;
  ptrs[OPCODE_GREATER] = &&do_greater;
  ptrs[OPCODE_GREATER_NUM] = &&do_greater_num;
  ptrs[OPCODE_LESS_THAN_EQUAL] = &&do_less_than_equal;
  ptrs[OPCODE_LESS_THAN_EQUAL_NUM] = &&do_less_than_equal_num;
  ptrs[OPCODE_GREATER_THAN_EQUAL] = &&do_greater_than_equal;
  ptrs[OPCODE_GREATER_THAN_EQUAL_NUM] = &&do_greater_than_equal_num;
  ptrs[OPCODE_JUMP_ALWAYS] = &&do_jump_always;
  ptrs[OPCODE_JUMP_IF_FALSE] = &&do_jump_if_false;
  ptrs[OPCODE_JUMP_IF_TRUE] = &&do_jump_if_true;
  ptrs[OPCODE_JUMP_IF_EQUAL] = &&do_jump_if_equal;
  ptrs[OPCODE_JUMP_IF_EQUAL_NUM] = &&do_jump_if_equal_num;
  ptrs[OPCODE_JUMP_IF_NOT_EQUAL] = &&do_jump_if_not_equal;
  ptrs[OPCODE_JUMP_IF_NOT_EQUAL_NUM] = &&do_jump_if_not_equal_num;
  ptrs[OPCODE_JUMP_IF_LESS] = &&do_jump_if_less;
  ptrs[OPCODE_JUMP_IF_LESS_NUM] = &&do_jump_if_less_num;
  ptrs[OPCODE_JUMP_IF_GREATER] = &&do_jump_if_greater;
  ptrs[OPCODE_JUMP_IF_GREATER_NUM] = &&do_jump_if_greater_num;
  ptrs[OPCODE_JUMP_IF_LESS_THAN_EQUAL] = &&do_jump_if_less_than_equal;
  ptrs[OPCODE_JUMP_IF_LESS_THAN_EQUAL_NUM] = &&do_jump_if_less_than_equal_num;
  ptrs[OPCODE_JUMP_IF_GREATER_THAN_EQUAL] = &&do_jump_if_greater_than_equal;
  ptrs[OPCODE_JUMP_IF_GREATER_THAN_EQUAL_NUM] = &&do_jump_if_greater_than_equal_num;
  ptrs[OPCODE_INC] = &&do_inc;
  ptrs[OPCODE_INC_NUM] = &&do_inc_num;
  ptrs[OPCODE_DEC] = &&do_dec;
  ptrs[OPCODE_DEC_NUM] = &&do_dec_num;
  ptrs[OPCODE_ADD] = &&do_add;
  ptrs[OPCODE_ADD_NUM] = &&do_add_num;
  ptrs[OPCODE_SUB] = &&do_sub;
  ptrs[OPCODE_SUB_NUM] = &&do_sub_num;
  ptrs[OPCODE_MUL] = &&do_mul;
  ptrs[OPCODE_MUL_NUM] = &&do_mul_num;
  ptrs[OPCODE_DIV] = &&do_div;
  ptrs[OPCODE_DIV_NUM] = &&do_div_num;
  ptrs[OPCODE_MOD] = &&do_mod;
  ptrs[OPCODE_MOD_NUM] = &&do_mod_num;
  ptrs[OPCODE_CALL] = &&do_call;
  ptrs[OPCODE_REC] = &&do_rec;
  ptrs[OPCODE_TAIL_CALL] = &&do_tail_call;
  ptrs[OPCODE_TAIL_REC] = &&do_tail_rec;
  ptrs[OPCODE_RETURN] = &&do_return;
  ptrs[OPCODE_PRINTLN] = &&do_println;
  ptrs[OPCODE_ALLOCA] = &&do_alloca;
  cur_frame->nregs = 256;
  vm_fetch;
  run_next_op;
do_exit:
{
  free(locals_base);
  free(frames_base);
  return;
}
do_alloca:
{
  int nregs = read_int;
  vm_fetch;
  cur_frame->nregs += nregs;
  run_next_op;
}
do_return:
{
  reg_t from = read_reg;
  nanbox_t val = cur_locals[from];
  int outreg = cur_frame->outreg;
  cur_func = cur_frame->func;
  cur_index = cur_frame->index;
  cur_frame--;
  cur_locals -= cur_frame->nregs;
  cur_locals[outreg] = val;
  vm_fetch;
  run_next_op;
}
do_call:
{
  if (cur_frame - frames_base + 1 >= allocn)
  {
    int len = cur_frame - frames_base;
    int alloc = allocn * 2;
    frames_base = realloc(frames_base, sizeof(stack_frame_t) * alloc);
    cur_frame = frames_base + len;
    allocn = alloc - 4;
  }
  if (cur_locals - locals_base + cur_frame->nregs >= locals_allocated)
  {
    int len = cur_locals - locals_base;
    int alloc = locals_allocated * 2;
    locals_base = realloc(locals_base, sizeof(nanbox_t) * alloc);
    cur_locals = locals_base + len;
    locals_allocated = alloc;
  }
  reg_t outreg = read_reg;
  reg_t func = read_reg;
  reg_t nargs = read_reg;
  nanbox_t *next_locals = cur_locals + cur_frame->nregs;
  for (int argno = 0; argno < nargs; argno++)
  {
    reg_t regno = read_reg;
    next_locals[argno] = cur_locals[regno];
  }
  int next_func = nanbox_to_int(cur_locals[func]);
  cur_locals = next_locals;
  cur_frame++;
  cur_frame->index = cur_index;
  cur_frame->func = cur_func;
  cur_frame->outreg = outreg;
  cur_frame->nregs = nargs + 8;
  cur_index = next_func;
  cur_func = next_func;
  vm_fetch;
  run_next_op;
}
do_rec:
{
  if (cur_frame - frames_base + 1 >= allocn)
  {
    int len = cur_frame - frames_base;
    int alloc = allocn * 2;
    frames_base = realloc(frames_base, sizeof(stack_frame_t) * alloc);
    cur_frame = frames_base + len;
    allocn = alloc - 4;
  }
  if (cur_locals - locals_base + cur_frame->nregs >= locals_allocated)
  {
    int len = cur_locals - locals_base;
    int alloc = locals_allocated * 2;
    locals_base = realloc(locals_base, sizeof(nanbox_t) * alloc);
    cur_locals = locals_base + len;
    locals_allocated = alloc;
  }
  reg_t outreg = read_reg;
  reg_t nargs = read_reg;
  nanbox_t *next_locals = cur_locals + cur_frame->nregs;
  for (int argno = 0; argno < nargs; argno++)
  {
    reg_t regno = read_reg;
    next_locals[argno] = cur_locals[regno];
  }
  cur_locals = next_locals;
  cur_frame++;
  cur_frame->index = cur_index;
  cur_frame->func = cur_func;
  cur_frame->outreg = outreg;
  cur_frame->nregs = nargs + 8;
  cur_index = cur_func;
  vm_fetch;
  run_next_op;
}
do_tail_call:
{
  reg_t func = read_reg;
  reg_t nargs = read_reg;
  nanbox_t *next_locals = cur_locals - cur_frame->nregs + nargs;
  for (int argno = 0; argno < nargs; argno++)
  {
    reg_t from = read_reg;
    next_locals[argno] = cur_locals[from];
  }
  int next_func = nanbox_to_int(cur_locals[func]);
  cur_index = next_func;
  cur_locals = next_locals;
  cur_func = next_func;
  vm_fetch;
  run_next_op;
}
do_tail_rec:
{
  reg_t nargs = read_reg;
  nanbox_t *next_locals = cur_locals - cur_frame->nregs + nargs;
  for (int argno = 0; argno < nargs; argno++)
  {
    reg_t from = read_reg;
    next_locals[argno] = cur_locals[from];
  }
  cur_index = cur_func;
  cur_locals = next_locals;
  vm_fetch;
  run_next_op;
}
do_store_reg:
{
  reg_t to = read_reg;
  reg_t from = read_reg;
  vm_fetch;
  cur_locals[to] = cur_locals[from];
  run_next_op;
}
do_store_num:
{
  reg_t to = read_reg;
  number_t from = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(from);
  run_next_op;
}
do_store_log:
{
  reg_t to = read_reg;
  bool from = read_bool;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(from);
  run_next_op;
}
do_store_fun:
{
  reg_t to = read_reg;
  int func_end = read_loc;
  int head = cur_index;
  cur_index = func_end;
  vm_fetch;
  cur_locals[to] = nanbox_from_int(head);
  run_next_op;
}
do_equal:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) == nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_equal_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) == rhs);
  run_next_op;
}
do_not_equal:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) != nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_not_equal_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) != rhs);
  run_next_op;
}
do_less:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) < nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_less_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) < rhs);
  run_next_op;
}
do_greater:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) > nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_greater_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) > rhs);
  run_next_op;
}
do_less_than_equal:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) <= nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_less_than_equal_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) <= rhs);
  run_next_op;
}
do_greater_than_equal:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) >= nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_greater_than_equal_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) >= rhs);
  run_next_op;
}
do_jump_always:
{
  int to = read_loc;
  cur_index = to;
  vm_fetch;
  run_next_op;
}
do_jump_if_false:
{
  int to = read_loc;
  reg_t from = read_reg;
  if (nanbox_to_boolean(cur_locals[from]) == false)
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_true:
{
  int to = read_loc;
  reg_t from = read_reg;
  if (nanbox_to_boolean(cur_locals[from]) == true)
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_equal:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  if (nanbox_to_double(cur_locals[lhs]) == nanbox_to_double(cur_locals[rhs]))
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_equal_num:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  if (nanbox_to_double(cur_locals[lhs]) == rhs)
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_not_equal:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  if (nanbox_to_double(cur_locals[lhs]) != nanbox_to_double(cur_locals[rhs]))
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_not_equal_num:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  if (nanbox_to_double(cur_locals[lhs]) != rhs)
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_less:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  if (nanbox_to_double(cur_locals[lhs]) < nanbox_to_double(cur_locals[rhs]))
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_less_num:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  if (nanbox_to_double(cur_locals[lhs]) < rhs)
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_less_than_equal:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  if (nanbox_to_double(cur_locals[lhs]) <= nanbox_to_double(cur_locals[rhs]))
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_less_than_equal_num:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  if (nanbox_to_double(cur_locals[lhs]) <= rhs)
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_greater:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  if (nanbox_to_double(cur_locals[lhs]) > nanbox_to_double(cur_locals[rhs]))
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_greater_num:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  if (nanbox_to_double(cur_locals[lhs]) >= rhs)
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_greater_than_equal:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  if (nanbox_to_double(cur_locals[lhs]) >= nanbox_to_double(cur_locals[rhs]))
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_greater_than_equal_num:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  if (nanbox_to_double(cur_locals[lhs]) >= rhs)
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_inc:
{
  reg_t target = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[target] = nanbox_from_double(nanbox_to_double(cur_locals[target]) + nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_inc_num:
{
  reg_t target = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[target] = nanbox_from_double(nanbox_to_double(cur_locals[target]) + rhs);
  run_next_op;
}
do_dec:
{
  reg_t target = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[target] = nanbox_from_double(nanbox_to_double(cur_locals[target]) - nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_dec_num:
{
  reg_t target = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[target] = nanbox_from_double(nanbox_to_double(cur_locals[target]) - rhs);
  run_next_op;
}
do_add:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(nanbox_to_double(cur_locals[lhs]) + nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_add_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(nanbox_to_double(cur_locals[lhs]) + rhs);
  run_next_op;
}
do_mul:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(nanbox_to_double(cur_locals[lhs]) * nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_mul_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(nanbox_to_double(cur_locals[lhs]) * rhs);
  run_next_op;
}
do_sub:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(nanbox_to_double(cur_locals[lhs]) - nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_sub_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(nanbox_to_double(cur_locals[lhs]) - rhs);
  run_next_op;
}
do_div:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(nanbox_to_double(cur_locals[lhs]) / nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_div_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(nanbox_to_double(cur_locals[lhs]) / rhs);
  run_next_op;
}
do_mod:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(fmod(nanbox_to_double(cur_locals[lhs]), nanbox_to_double(cur_locals[rhs])));
  run_next_op;
}
do_mod_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(fmod(nanbox_to_double(cur_locals[lhs]), rhs));
  run_next_op;
}
do_println:
{
  reg_t from = read_reg;
  nanbox_t val = cur_locals[from];
  if (nanbox_is_boolean(val))
  {
    bool log = nanbox_to_boolean(val);
    if (log)
    {
      printf("true\n");
    }
    else
    {
      printf("false\n");
    }
  }
  else if (nanbox_is_double(val))
  {
    number_t num = nanbox_to_double(val);
    if (fmod(num, 1) == 0)
    {
      printf("%.0f\n", num);
    }
    else
    {
      printf("%f\n", num);
    }
  }
  else
  {
    printf("unk\n");
  }
  vm_fetch;
  run_next_op;
}
}