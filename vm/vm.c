#include "vm.h"
#include "gc.h"
#include "obj.h"

#if defined(VM_DEBUG)
#define VM_TYPE_CHECK(V) ({       \
    vm_obj_t val = V;        \
    if (vm_obj_is_dead(val)) \
    {                        \
        vm_puts("bad type"); \
        goto do_exit;        \
    }                        \
    val;                     \
})
#endif

#define VM_GLOBALS_NUM ((255))

#if defined(VM_DEBUG_OPCODE)
int printf(const char *fmt, ...);
#define run_next_op                                                   \
    printf("(%i -> %i)\n", (int)cur_index, (int)basefunc[cur_index]); \
    goto *next_op;
#else
#define run_next_op \
    goto *next_op;
#endif

#define cur_bytecode_next(Type)                       \
    (                                                 \
        {                                             \
            Type ret = *(Type *)&basefunc[cur_index]; \
            cur_index += sizeof(Type);                \
            ret;                                      \
        })

#define next_op (cur_index += 1, next_op_value)
#define vm_fetch (next_op_value = ptrs[basefunc[cur_index]])

#define get_byte(index) (*(uint8_t *)&basefunc[(cur_index) + (index)])
#define read_byte (cur_bytecode_next(uint8_t))
#define read_reg (cur_bytecode_next(uint8_t))
#define read_int (cur_bytecode_next(int))
#define read_loc (cur_bytecode_next(int))

void vm_run(int len, const vm_opcode_t *basefunc)
{
    vm_stack_frame_t *frames_base = vm_mem_grow(VM_FRAMES_UNITS * sizeof(vm_stack_frame_t));
    vm_obj_t *locals_base = vm_mem_grow(VM_LOCALS_UNITS * sizeof(vm_obj_t));

    vm_stack_frame_t *cur_frame = frames_base;
    vm_obj_t *cur_locals = locals_base;
    vm_loc_t cur_index = 0;
    vm_loc_t cur_func = 0;

    vm_gc_t raw_gc;
    vm_gc_start(&raw_gc);

    void *next_op_value;
    void *ptrs[VM_OPCODE_MAX2P] = {};
    ptrs[VM_OPCODE_EXIT] = &&do_exit;
    ptrs[VM_OPCODE_STORE_REG] = &&do_store_reg;
    ptrs[VM_OPCODE_STORE_BYTE] = &&do_store_byte;
    ptrs[VM_OPCODE_STORE_INT] = &&do_store_int;
    ptrs[VM_OPCODE_STORE_FUN] = &&do_store_fun;
    ptrs[VM_OPCODE_EQUAL] = &&do_equal;
    ptrs[VM_OPCODE_EQUAL_NUM] = &&do_equal_num;
    ptrs[VM_OPCODE_NOT_EQUAL] = &&do_not_equal;
    ptrs[VM_OPCODE_NOT_EQUAL_NUM] = &&do_not_equal_num;
    ptrs[VM_OPCODE_LESS] = &&do_less;
    ptrs[VM_OPCODE_LESS_NUM] = &&do_less_num;
    ptrs[VM_OPCODE_GREATER] = &&do_greater;
    ptrs[VM_OPCODE_GREATER_NUM] = &&do_greater_num;
    ptrs[VM_OPCODE_LESS_THAN_EQUAL] = &&do_less_than_equal;
    ptrs[VM_OPCODE_LESS_THAN_EQUAL_NUM] = &&do_less_than_equal_num;
    ptrs[VM_OPCODE_GREATER_THAN_EQUAL] = &&do_greater_than_equal;
    ptrs[VM_OPCODE_GREATER_THAN_EQUAL_NUM] = &&do_greater_than_equal_num;
    ptrs[VM_OPCODE_JUMP_ALWAYS] = &&do_jump_always;
    ptrs[VM_OPCODE_JUMP_IF_FALSE] = &&do_jump_if_false;
    ptrs[VM_OPCODE_JUMP_IF_TRUE] = &&do_jump_if_true;
    ptrs[VM_OPCODE_JUMP_IF_EQUAL] = &&do_jump_if_equal;
    ptrs[VM_OPCODE_JUMP_IF_EQUAL_NUM] = &&do_jump_if_equal_num;
    ptrs[VM_OPCODE_JUMP_IF_NOT_EQUAL] = &&do_jump_if_not_equal;
    ptrs[VM_OPCODE_JUMP_IF_NOT_EQUAL_NUM] = &&do_jump_if_not_equal_num;
    ptrs[VM_OPCODE_JUMP_IF_LESS] = &&do_jump_if_less;
    ptrs[VM_OPCODE_JUMP_IF_LESS_NUM] = &&do_jump_if_less_num;
    ptrs[VM_OPCODE_JUMP_IF_GREATER] = &&do_jump_if_greater;
    ptrs[VM_OPCODE_JUMP_IF_GREATER_NUM] = &&do_jump_if_greater_num;
    ptrs[VM_OPCODE_JUMP_IF_LESS_THAN_EQUAL] = &&do_jump_if_less_than_equal;
    ptrs[VM_OPCODE_JUMP_IF_LESS_THAN_EQUAL_NUM] = &&do_jump_if_less_than_equal_num;
    ptrs[VM_OPCODE_JUMP_IF_GREATER_THAN_EQUAL] = &&do_jump_if_greater_than_equal;
    ptrs[VM_OPCODE_JUMP_IF_GREATER_THAN_EQUAL_NUM] = &&do_jump_if_greater_than_equal_num;
    ptrs[VM_OPCODE_INC] = &&do_inc;
    ptrs[VM_OPCODE_INC_NUM] = &&do_inc_num;
    ptrs[VM_OPCODE_DEC] = &&do_dec;
    ptrs[VM_OPCODE_DEC_NUM] = &&do_dec_num;
    ptrs[VM_OPCODE_ADD] = &&do_add;
    ptrs[VM_OPCODE_ADD_NUM] = &&do_add_num;
    ptrs[VM_OPCODE_SUB] = &&do_sub;
    ptrs[VM_OPCODE_SUB_NUM] = &&do_sub_num;
    ptrs[VM_OPCODE_MUL] = &&do_mul;
    ptrs[VM_OPCODE_MUL_NUM] = &&do_mul_num;
    ptrs[VM_OPCODE_DIV] = &&do_div;
    ptrs[VM_OPCODE_DIV_NUM] = &&do_div_num;
    ptrs[VM_OPCODE_MOD] = &&do_mod;
    ptrs[VM_OPCODE_MOD_NUM] = &&do_mod_num;
    ptrs[VM_OPCODE_CONCAT] = &&do_concat;
    ptrs[VM_OPCODE_CALL0] = &&do_call0;
    ptrs[VM_OPCODE_CALL1] = &&do_call1;
    ptrs[VM_OPCODE_CALL2] = &&do_call2;
    ptrs[VM_OPCODE_CALL] = &&do_call;
    ptrs[VM_OPCODE_STATIC_CALL0] = &&do_static_call0;
    ptrs[VM_OPCODE_STATIC_CALL1] = &&do_static_call1;
    ptrs[VM_OPCODE_STATIC_CALL2] = &&do_static_call2;
    ptrs[VM_OPCODE_STATIC_CALL] = &&do_static_call;
    ptrs[VM_OPCODE_REC0] = &&do_rec0;
    ptrs[VM_OPCODE_REC1] = &&do_rec1;
    ptrs[VM_OPCODE_REC2] = &&do_rec2;
    ptrs[VM_OPCODE_REC] = &&do_rec;
    ptrs[VM_OPCODE_RETURN] = &&do_return;
    ptrs[VM_OPCODE_PUTCHAR] = &&do_putchar;
    ptrs[VM_OPCODE_ARRAY_NEW] = &&do_array_new;
    ptrs[VM_OPCODE_STRING_NEW] = &&do_string_new;
    ptrs[VM_OPCODE_LENGTH] = &&do_length;
    ptrs[VM_OPCODE_INDEX_GET] = &&do_index_get;
    ptrs[VM_OPCODE_INDEX_SET] = &&do_index_set;
    ptrs[VM_OPCODE_TYPE] = &&do_type;
    cur_frame->locals = cur_locals;
    cur_frame += 1;
    cur_frame->locals = cur_locals + VM_GLOBALS_NUM;
    vm_fetch;
    run_next_op;
do_exit:
{
    vm_gc_stop(&raw_gc);
    vm_mem_reset(frames_base);
    vm_mem_reset(locals_base);
    return;
}
do_return:
{
    reg_t from = read_reg;
    vm_obj_t val = cur_locals[from];
    cur_frame--;
    cur_locals = (cur_frame - 1)->locals;
    cur_func = cur_frame->func;
    cur_index = cur_frame->index;
    reg_t outreg = cur_frame->outreg;
    cur_locals[outreg] = val;
    vm_fetch;
    run_next_op;
}
do_type:
{
    reg_t outreg = read_reg;
    reg_t valreg = read_reg;
    vm_fetch;
    vm_obj_t obj = cur_locals[valreg];
    double num = -1;
    if (vm_obj_is_num(obj))
    {
        num = VM_TYPE_NUMBER;
    }
    if (vm_obj_is_fun(obj))
    {
        num = VM_TYPE_FUNCTION;
    }
    if (vm_obj_is_ptr(obj))
    {
        num = vm_gc_type(vm_obj_to_ptr(obj));
    }
    cur_locals[outreg] = vm_obj_of_num(num);
    run_next_op;
}
do_string_new:
{
    reg_t outreg = read_reg;
    int nargs = read_byte;
    if (raw_gc.len >= raw_gc.max)
    {
        vm_gc_run1(&raw_gc, locals_base, cur_frame->locals);
    }
    vm_gc_entry_t *str = vm_gc_string_new(&raw_gc, nargs);
    for (size_t i = 0; i < nargs; i++)
    {
        ((char*) str->obj)[i] = read_byte;
    }
    vm_fetch;
    cur_locals[outreg] = vm_obj_of_ptr(str);
    run_next_op;
}
do_array_new:
{
    reg_t outreg = read_reg;
    int nargs = read_byte;
    if (raw_gc.len >= raw_gc.max)
    {
        vm_gc_run1(&raw_gc, locals_base, cur_frame->locals);
    }
    vm_gc_entry_t *vec = vm_gc_array_new(&raw_gc, nargs);
    for (int i = 0; i < nargs; i++)
    {
        reg_t vreg = read_reg;
        vm_gc_set_index(vec, vm_obj_of_int(i), cur_locals[vreg]);
    }
    vm_fetch;
    cur_locals[outreg] = vm_obj_of_ptr(vec);
    run_next_op;
}
do_length:
{
    reg_t outreg = read_reg;
    reg_t reg = read_reg;
    vm_fetch;
    vm_obj_t vec = cur_locals[reg];
    cur_locals[outreg] = vm_obj_of_int(vm_gc_sizeof(vm_obj_to_ptr(vec)));
    run_next_op;
}
do_index_get:
{
    reg_t outreg = read_reg;
    reg_t reg = read_reg;
    reg_t ind = read_reg;
    vm_fetch;
    vm_obj_t vec = cur_locals[reg];
    vm_obj_t index = cur_locals[ind];
    cur_locals[outreg] = vm_gc_get_index(vm_obj_to_ptr(vec), index);
    run_next_op;
}
do_index_set:
{
    reg_t reg = read_reg;
    reg_t ind = read_reg;
    reg_t val = read_reg;
    vm_fetch;
    vm_obj_t vec = cur_locals[reg];
    vm_obj_t index = cur_locals[ind];
    vm_obj_t value = cur_locals[val];
    vm_gc_set_index(vm_obj_to_ptr(vec), index, value);
    run_next_op;
}
do_call0:
{
    reg_t outreg = read_reg;
    reg_t func = read_reg;
    vm_obj_t *next_locals = cur_frame->locals;
    vm_obj_t funcv = cur_locals[func];
    for (int i = 0; vm_obj_is_ptr(funcv); i++)
    {
        next_locals[i] = funcv;
        funcv = vm_gc_get_index(vm_obj_to_ptr(funcv), vm_obj_of_int(0));
    }
    vm_loc_t next_func = vm_obj_to_fun(funcv);
    cur_locals = next_locals;
    cur_frame->index = cur_index;
    cur_frame->func = cur_func;
    cur_frame->outreg = outreg;
    cur_frame++;
    cur_index = next_func;
    cur_func = next_func;
    cur_frame->locals = cur_locals + get_byte(-1);
    vm_fetch;
    run_next_op;
}
do_call1:
{
    reg_t outreg = read_reg;
    reg_t func = read_reg;
    vm_obj_t *next_locals = cur_frame->locals;
    next_locals[0] = cur_locals[read_reg];
    vm_obj_t funcv = cur_locals[func];
    for (int i = 1; vm_obj_is_ptr(funcv); i++)
    {
        next_locals[i] = funcv;
        funcv = vm_gc_get_index(vm_obj_to_ptr(funcv), vm_obj_of_int(0));
    }
    vm_loc_t next_func = vm_obj_to_fun(funcv);
    cur_locals = next_locals;
    cur_frame->index = cur_index;
    cur_frame->func = cur_func;
    cur_frame->outreg = outreg;
    cur_frame++;
    cur_index = next_func;
    cur_func = next_func;
    cur_frame->locals = cur_locals + get_byte(-1);
    vm_fetch;
    run_next_op;
}
do_call2:
{
    reg_t outreg = read_reg;
    reg_t func = read_reg;
    vm_obj_t *next_locals = cur_frame->locals;
    next_locals[0] = cur_locals[read_reg];
    next_locals[1] = cur_locals[read_reg];
    vm_obj_t funcv = cur_locals[func];
    for (int i = 2; vm_obj_is_ptr(funcv); i++)
    {
        next_locals[i] = funcv;
        funcv = vm_gc_get_index(vm_obj_to_ptr(funcv), vm_obj_of_int(0));
    }
    vm_loc_t next_func = vm_obj_to_fun(funcv);
    cur_locals = next_locals;
    cur_frame->index = cur_index;
    cur_frame->func = cur_func;
    cur_frame->outreg = outreg;
    cur_frame++;
    cur_index = next_func;
    cur_func = next_func;
    cur_frame->locals = cur_locals + get_byte(-1);
    vm_fetch;
    run_next_op;
}
do_call:
{
    reg_t outreg = read_reg;
    reg_t func = read_reg;
    int nargs = read_byte;
    vm_obj_t *next_locals = cur_frame->locals;
    for (int argno = 0; argno < nargs; argno++)
    {
        reg_t regno = read_reg;
        next_locals[argno] = cur_locals[regno];
    }
    vm_obj_t funcv = cur_locals[func];
    for (int i = nargs; vm_obj_is_ptr(funcv); i++)
    {
        next_locals[i] = funcv;
        funcv = vm_gc_get_index(vm_obj_to_ptr(funcv), vm_obj_of_int(0));
    }
    vm_loc_t next_func = vm_obj_to_fun(funcv);
    cur_locals = next_locals;
    cur_frame->index = cur_index;
    cur_frame->func = cur_func;
    cur_frame->outreg = outreg;
    cur_frame++;
    cur_index = next_func;
    cur_func = next_func;
    cur_frame->locals = cur_locals + get_byte(-1);
    vm_fetch;
    run_next_op;
}
do_static_call0:
{
    reg_t outreg = read_reg;
    vm_loc_t next_func = read_loc;
    vm_obj_t *next_locals = cur_frame->locals;
    cur_locals = next_locals;
    cur_frame->index = cur_index;
    cur_frame->func = cur_func;
    cur_frame->outreg = outreg;
    cur_frame++;
    cur_index = next_func;
    cur_func = next_func;
    cur_frame->locals = cur_locals + get_byte(-1);
    vm_fetch;
    run_next_op;
}
do_static_call1:
{
    reg_t outreg = read_reg;
    vm_loc_t next_func = read_loc;
    vm_obj_t *next_locals = cur_frame->locals;
    next_locals[0] = cur_locals[read_reg];
    cur_locals = next_locals;
    cur_frame->index = cur_index;
    cur_frame->func = cur_func;
    cur_frame->outreg = outreg;
    cur_frame++;
    cur_index = next_func;
    cur_func = next_func;
    cur_frame->locals = cur_locals + get_byte(-1);
    vm_fetch;
    run_next_op;
}
do_static_call2:
{
    reg_t outreg = read_reg;
    vm_loc_t next_func = read_loc;
    vm_obj_t *next_locals = cur_frame->locals;
    next_locals[0] = cur_locals[read_reg];
    next_locals[1] = cur_locals[read_reg];
    cur_locals = next_locals;
    cur_frame->index = cur_index;
    cur_frame->func = cur_func;
    cur_frame->outreg = outreg;
    cur_frame++;
    cur_index = next_func;
    cur_func = next_func;
    cur_frame->locals = cur_locals + get_byte(-1);
    vm_fetch;
    run_next_op;
}
do_static_call:
{
    reg_t outreg = read_reg;
    int next_func = read_loc;
    int nargs = read_byte;
    vm_obj_t *next_locals = cur_frame->locals;
    for (int argno = 0; argno < nargs; argno++)
    {
        reg_t regno = read_reg;
        next_locals[argno] = cur_locals[regno];
    }
    cur_locals = next_locals;
    cur_frame->index = cur_index;
    cur_frame->func = cur_func;
    cur_frame->outreg = outreg;
    cur_frame++;
    cur_index = next_func;
    cur_func = next_func;
    cur_frame->locals = cur_locals + get_byte(-1);
    vm_fetch;
    run_next_op;
}
do_rec0:
{
    reg_t outreg = read_reg;
    vm_obj_t *next_locals = cur_frame->locals;
    next_locals[0] = cur_locals[0];
    cur_locals = next_locals;
    cur_frame->index = cur_index;
    cur_frame->func = cur_func;
    cur_frame->outreg = outreg;
    cur_frame++;
    cur_index = cur_func;
    cur_frame->locals = cur_locals + get_byte(-1);
    vm_fetch;
    run_next_op;
}
do_rec1:
{
    reg_t outreg = read_reg;
    vm_obj_t *next_locals = cur_frame->locals;
    next_locals[0] = cur_locals[read_reg];
    next_locals[1] = cur_locals[1];
    cur_locals = next_locals;
    cur_frame->index = cur_index;
    cur_frame->func = cur_func;
    cur_frame->outreg = outreg;
    cur_frame++;
    cur_index = cur_func;
    cur_frame->locals = cur_locals + get_byte(-1);
    vm_fetch;
    run_next_op;
}
do_rec2:
{
    reg_t outreg = read_reg;
    vm_obj_t *next_locals = cur_frame->locals;
    next_locals[0] = cur_locals[read_reg];
    next_locals[1] = cur_locals[read_reg];
    next_locals[2] = cur_locals[2];
    cur_locals = next_locals;
    cur_frame->index = cur_index;
    cur_frame->func = cur_func;
    cur_frame->outreg = outreg;
    cur_frame++;
    cur_index = cur_func;
    cur_frame->locals = cur_locals + get_byte(-1);
    vm_fetch;
    run_next_op;
}
do_rec:
{
    reg_t outreg = read_reg;
    int nargs = read_byte;
    vm_obj_t *next_locals = cur_frame->locals;
    for (int argno = 0; argno < nargs; argno++)
    {
        reg_t regno = read_reg;
        next_locals[argno] = cur_locals[regno];
    }
    next_locals[nargs] = cur_locals[nargs];
    cur_locals = next_locals;
    cur_frame->index = cur_index;
    cur_frame->func = cur_func;
    cur_frame->outreg = outreg;
    cur_frame++;
    cur_index = cur_func;
    cur_frame->locals = cur_locals + get_byte(-1);
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
do_store_byte:
{
    reg_t to = read_reg;
    int from = (int)read_byte;
    vm_fetch;
    cur_locals[to] = vm_obj_of_int((int)from);
    run_next_op;
}
do_store_int:
{
    reg_t to = read_reg;
    int from = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_of_int(from);
    run_next_op;
}
do_store_fun:
{
    reg_t to = read_reg;
    vm_loc_t func_end = read_loc;
    vm_loc_t head = cur_index;
    cur_index = func_end;
    vm_fetch;
    cur_locals[to] = vm_obj_of_fun(head);
    run_next_op;
}
do_equal:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_int(vm_obj_eq(cur_locals[lhs], cur_locals[rhs]));
    run_next_op;
}
do_equal_num:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_of_int(vm_obj_ieq(cur_locals[lhs], rhs));
    run_next_op;
}
do_not_equal:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_int(vm_obj_neq(cur_locals[lhs], cur_locals[rhs]));
    run_next_op;
}
do_not_equal_num:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_of_int(vm_obj_ineq(cur_locals[lhs], rhs));
    run_next_op;
}
do_less:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_int(vm_obj_lt(cur_locals[lhs], cur_locals[rhs]));
    run_next_op;
}
do_less_num:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_of_int(vm_obj_ilt(cur_locals[lhs], rhs));
    run_next_op;
}
do_greater:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_int(vm_obj_gt(cur_locals[lhs], cur_locals[rhs]));
    run_next_op;
}
do_greater_num:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_of_int(vm_obj_igt(cur_locals[lhs], rhs));
    run_next_op;
}
do_less_than_equal:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_int(vm_obj_lte(cur_locals[lhs], cur_locals[rhs]));
    run_next_op;
}
do_less_than_equal_num:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_of_int(vm_obj_ilte(cur_locals[lhs], rhs));
    run_next_op;
}
do_greater_than_equal:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_int(vm_obj_gte(cur_locals[lhs], cur_locals[rhs]));
    run_next_op;
}
do_greater_than_equal_num:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_of_int(vm_obj_igte(cur_locals[lhs], rhs));
    run_next_op;
}
do_jump_always:
{
    vm_loc_t to = read_loc;
    cur_index = to;
    vm_fetch;
    run_next_op;
}
do_jump_if_false:
{
    vm_loc_t to = read_loc;
    reg_t from = read_reg;
    if (vm_obj_is_zero(cur_locals[from]))
    {
        cur_index = to;
    }
    vm_fetch;
    run_next_op;
}
do_jump_if_true:
{
    vm_loc_t to = read_loc;
    reg_t from = read_reg;
    if (vm_obj_is_nonzero(cur_locals[from]))
    {
        cur_index = to;
    }
    vm_fetch;
    run_next_op;
}
do_jump_if_equal:
{
    vm_loc_t to = read_loc;
    reg_t lhs = read_reg;
    reg_t rhs = read_reg;
    if (vm_obj_eq(cur_locals[lhs], cur_locals[rhs]))
    {
        cur_index = to;
    }
    vm_fetch;
    run_next_op;
}
do_jump_if_equal_num:
{
    vm_loc_t to = read_loc;
    reg_t lhs = read_reg;
    int rhs = read_int;
    if (vm_obj_ieq(cur_locals[lhs], rhs))
    {
        cur_index = to;
    }
    vm_fetch;
    run_next_op;
}
do_jump_if_not_equal:
{
    vm_loc_t to = read_loc;
    reg_t lhs = read_reg;
    reg_t rhs = read_reg;
    if (vm_obj_neq(cur_locals[lhs], cur_locals[rhs]))
    {
        cur_index = to;
    }
    vm_fetch;
    run_next_op;
}
do_jump_if_not_equal_num:
{
    vm_loc_t to = read_loc;
    reg_t lhs = read_reg;
    int rhs = read_int;
    if (vm_obj_ineq(cur_locals[lhs], rhs))
    {
        cur_index = to;
    }
    vm_fetch;
    run_next_op;
}
do_jump_if_less:
{
    vm_loc_t to = read_loc;
    reg_t lhs = read_reg;
    reg_t rhs = read_reg;
    if (vm_obj_lt(cur_locals[lhs], cur_locals[rhs]))
    {
        cur_index = to;
    }
    vm_fetch;
    run_next_op;
}
do_jump_if_less_num:
{
    vm_loc_t to = read_loc;
    reg_t lhs = read_reg;
    int rhs = read_int;
    if (vm_obj_ilt(cur_locals[lhs], rhs))
    {
        cur_index = to;
    }
    vm_fetch;
    run_next_op;
}
do_jump_if_less_than_equal:
{
    vm_loc_t to = read_loc;
    reg_t lhs = read_reg;
    reg_t rhs = read_reg;
    if (vm_obj_lte(cur_locals[lhs], cur_locals[rhs]))
    {
        cur_index = to;
    }
    vm_fetch;
    run_next_op;
}
do_jump_if_less_than_equal_num:
{
    vm_loc_t to = read_loc;
    reg_t lhs = read_reg;
    int rhs = read_int;
    if (vm_obj_ilte(cur_locals[lhs], rhs))
    {
        cur_index = to;
    }
    vm_fetch;
    run_next_op;
}
do_jump_if_greater:
{
    vm_loc_t to = read_loc;
    reg_t lhs = read_reg;
    reg_t rhs = read_reg;
    if (vm_obj_gt(cur_locals[lhs], cur_locals[rhs]))
    {
        cur_index = to;
    }
    vm_fetch;
    run_next_op;
}
do_jump_if_greater_num:
{
    vm_loc_t to = read_loc;
    reg_t lhs = read_reg;
    int rhs = read_int;
    if (vm_obj_igt(cur_locals[lhs], rhs))
    {
        cur_index = to;
    }
    vm_fetch;
    run_next_op;
}
do_jump_if_greater_than_equal:
{
    vm_loc_t to = read_loc;
    reg_t lhs = read_reg;
    reg_t rhs = read_reg;
    if (vm_obj_gte(cur_locals[lhs], cur_locals[rhs]))
    {
        cur_index = to;
    }
    vm_fetch;
    run_next_op;
}
do_jump_if_greater_than_equal_num:
{
    vm_loc_t to = read_loc;
    reg_t lhs = read_reg;
    int rhs = read_int;
    if (vm_obj_igte(cur_locals[lhs], rhs))
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
    cur_locals[target] = vm_obj_num_add(cur_locals[target], cur_locals[rhs]);
    run_next_op;
}
do_inc_num:
{
    reg_t target = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[target] = vm_obj_num_addc(cur_locals[target], rhs);
    run_next_op;
}
do_dec:
{
    reg_t target = read_reg;
    reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[target] = vm_obj_num_sub(cur_locals[target], cur_locals[rhs]);
    run_next_op;
}
do_dec_num:
{
    reg_t target = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[target] = vm_obj_num_subc(cur_locals[target], rhs);
    run_next_op;
}
do_add:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_num_add(cur_locals[lhs], cur_locals[rhs]);
    run_next_op;
}
do_add_num:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_num_addc(cur_locals[lhs], rhs);
    run_next_op;
}
do_mul:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_num_mul(cur_locals[lhs], cur_locals[rhs]);
    run_next_op;
}
do_mul_num:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_num_mulc(cur_locals[lhs], rhs);
    run_next_op;
}
do_sub:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_num_sub(cur_locals[lhs], cur_locals[rhs]);
    run_next_op;
}
do_sub_num:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_num_subc(cur_locals[lhs], rhs);
    run_next_op;
}
do_div:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_num_div(cur_locals[lhs], cur_locals[rhs]);
    run_next_op;
}
do_div_num:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_num_divc(cur_locals[lhs], rhs);
    run_next_op;
}
do_mod:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_num_mod(cur_locals[lhs], cur_locals[rhs]);
    run_next_op;
}
do_mod_num:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_num_modc(cur_locals[lhs], rhs);
    run_next_op;
}
do_concat:
{
    reg_t to = read_reg;
    reg_t lhs = read_reg;
    reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_gc_concat(&raw_gc, cur_locals[lhs], cur_locals[rhs]);
    run_next_op;
}
do_putchar:
{
    reg_t from = read_reg;
    vm_fetch;
    int val = vm_obj_to_int(cur_locals[from]);
    vm_putchar(val);
    run_next_op;
}
}