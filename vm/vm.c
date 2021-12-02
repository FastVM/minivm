#include "vm.h"
#include "gc.h"
#include "math.h"
#include "obj.h"
#include "libc.h"
#include "state.h"

#if defined(VM_OS)
void os_putn(size_t n);
void os_puts(const char *str);
#endif

#if defined(VM_DEBUG_OPCODE)
int printf(const char *, ...);
#define vm_run_next_op() \
    ({ \
        vm_opcode_t op = vm_read(); \
        printf("%i: %i\n", (int) (index - 1), (int) op); \
        goto *ptrs[op]; \
    })
#else
#define vm_run_next_op() \
    goto *ptrs[vm_read()]
#endif

#define vm_run_op(index_) \
    index = index_; \
    vm_run_next_op();

#define vm_read() (ops[index++])
#define vm_read_at(index_) (*(vm_opcode_t *)&ops[(index_)])

void vm_run_state(vm_state_t *state) {
    vm_run_some(state);
}

void vm_run(vm_state_t *state)
{
    vm_run_state(state);
}

void vm_run_some(vm_state_t *state)
{
    const vm_opcode_t *ops = state->ops;
    vm_obj_t *globals = state->globals;
    vm_obj_t *locals = state->locals;
    size_t index = state->index;
    vm_stack_frame_t *frame = state->frame;
    vm_gc_t *gc = &state->gc;
    void *ptrs[] = {
        [VM_OPCODE_EXIT] = &&do_exit,
        [VM_OPCODE_STORE_REG] = &&do_store_reg,
        [VM_OPCODE_STORE_NONE] = &&do_store_none,
        [VM_OPCODE_STORE_BOOL] = &&do_store_bool,
        [VM_OPCODE_STORE_INT] = &&do_store_int,
        [VM_OPCODE_EQUAL] = &&do_equal,
        [VM_OPCODE_NOT_EQUAL] = &&do_not_equal,
        [VM_OPCODE_LESS] = &&do_less,
        [VM_OPCODE_GREATER] = &&do_greater,
        [VM_OPCODE_LESS_THAN_EQUAL] = &&do_less_than_equal,
        [VM_OPCODE_GREATER_THAN_EQUAL] = &&do_greater_than_equal,
        [VM_OPCODE_JUMP] = &&do_jump,
        [VM_OPCODE_BRANCH_TRUE] = &&do_branch_true,
        [VM_OPCODE_ADD] = &&do_add,
        [VM_OPCODE_SUB] = &&do_sub,
        [VM_OPCODE_MUL] = &&do_mul,
        [VM_OPCODE_DIV] = &&do_div,
        [VM_OPCODE_MOD] = &&do_mod,
        [VM_OPCODE_CONCAT] = &&do_concat,
        [VM_OPCODE_STATIC_CALL] = &&do_static_call,
        [VM_OPCODE_RETURN] = &&do_return,
        [VM_OPCODE_PUTCHAR] = &&do_putchar,
        [VM_OPCODE_STRING_NEW] = &&do_string_new,
        [VM_OPCODE_ARRAY_NEW] = &&do_array_new,
        [VM_OPCODE_LENGTH] = &&do_length,
        [VM_OPCODE_INDEX_GET] = &&do_index_get,
        [VM_OPCODE_INDEX_SET] = &&do_index_set,
        [VM_OPCODE_EXEC] = &&do_exec,
        [VM_OPCODE_TYPE] = &&do_type,
        [VM_OPCODE_EXTEND] = &&do_extend,
        [VM_OPCODE_PUSH] = &&do_push,
        [VM_OPCODE_DUMP] = &&do_dump,
        [VM_OPCODE_WRITE] = &&do_write,
        [VM_OPCODE_READ] = &&do_read,
        [VM_OPCODE_LOAD_GLOBAL] = &&do_load_global,
        [VM_OPCODE_DYNAMIC_CALL] = &&do_dynamic_call,
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
        [VM_OPCODE_INC] = &&do_inc,
        [VM_OPCODE_DEC] = &&do_dec,
        [VM_OPCODE_BRANCH_EQUAL_INT] = &&do_branch_equal_int,
        [VM_OPCODE_BRANCH_NOT_EQUAL_INT] = &&do_branch_not_equal_int,
        [VM_OPCODE_BRANCH_LESS_INT] = &&do_branch_less_int,
        [VM_OPCODE_BRANCH_GREATER_INT] = &&do_branch_greater_int,
        [VM_OPCODE_BRANCH_LESS_THAN_EQUAL_INT] = &&do_branch_less_than_equal_int,
        [VM_OPCODE_BRANCH_GREATER_THAN_EQUAL_INT] = &&do_branch_greater_than_equal_int,
    };
    vm_run_next_op();
do_exit:
{
    vm_state_del(state);
    return;
}
do_return:
{
    vm_reg_t from = vm_read();
    vm_obj_t val = locals[from];
    frame--;
    locals = (frame - 1)->locals;
    vm_reg_t outreg = frame->outreg;
    locals[outreg] = val;
    vm_run_op(frame->index);
}
do_store_none:
{
    vm_reg_t to = vm_read();
    locals[to] = vm_obj_of_none();
    vm_run_next_op();
}
do_store_bool:
{
    vm_reg_t to = vm_read();
    vm_int_t from = (int)vm_read();
    locals[to] = vm_obj_of_bool((bool)from);
    vm_run_next_op();
}
do_store_reg:
{
    vm_reg_t to = vm_read();
    vm_reg_t from = vm_read();
    locals[to] = locals[from];
    vm_run_next_op();
}
do_store_int:
{
    vm_reg_t to = vm_read();
    vm_int_t from = vm_read();
    locals[to] = vm_obj_of_int(from);
    vm_run_next_op();
}
do_equal:
{
    vm_reg_t to = vm_read();
    vm_reg_t lhs = vm_read();
    vm_reg_t rhs = vm_read();
    locals[to] = vm_obj_of_bool(vm_obj_eq(locals[lhs], locals[rhs]));
    vm_run_next_op();
}
do_not_equal:
{
    vm_reg_t to = vm_read();
    vm_reg_t lhs = vm_read();
    vm_reg_t rhs = vm_read();
    locals[to] = vm_obj_of_bool(vm_obj_neq(locals[lhs], locals[rhs]));
    vm_run_next_op();
}
do_less:
{
    vm_reg_t to = vm_read();
    vm_reg_t lhs = vm_read();
    vm_reg_t rhs = vm_read();
    locals[to] = vm_obj_of_bool(vm_obj_lt(locals[lhs], locals[rhs]));
    vm_run_next_op();
}
do_greater:
{
    vm_reg_t to = vm_read();
    vm_reg_t lhs = vm_read();
    vm_reg_t rhs = vm_read();
    locals[to] = vm_obj_of_bool(vm_obj_gt(locals[lhs], locals[rhs]));
    vm_run_next_op();
}
do_less_than_equal:
{
    vm_reg_t to = vm_read();
    vm_reg_t lhs = vm_read();
    vm_reg_t rhs = vm_read();
    locals[to] = vm_obj_of_bool(vm_obj_lte(locals[lhs], locals[rhs]));
    vm_run_next_op();
}
do_greater_than_equal:
{
    vm_reg_t to = vm_read();
    vm_reg_t lhs = vm_read();
    vm_reg_t rhs = vm_read();
    locals[to] = vm_obj_of_bool(vm_obj_gte(locals[lhs], locals[rhs]));
    vm_run_next_op();
}
do_jump:
{
    vm_loc_t to = vm_read();
    vm_run_op(to);
}
do_branch_true:
{
    vm_loc_t to1 = vm_read();
    vm_loc_t to2 = vm_read();
    vm_reg_t from = vm_read();
    if (vm_obj_to_bool(locals[from]))
    {
        vm_run_op(to1);
    }
    else
    {
        vm_run_op(to2);
    }
}
do_add:
{
    vm_reg_t to = vm_read();
    vm_reg_t lhs = vm_read();
    vm_reg_t rhs = vm_read();
    locals[to] = vm_obj_num_add(locals[lhs], locals[rhs]);
    vm_run_next_op();
}
do_sub:
{
    vm_reg_t to = vm_read();
    vm_reg_t lhs = vm_read();
    vm_reg_t rhs = vm_read();
    locals[to] = vm_obj_num_sub(locals[lhs], locals[rhs]);
    vm_run_next_op();
}
do_mul:
{
    vm_reg_t to = vm_read();
    vm_reg_t lhs = vm_read();
    vm_reg_t rhs = vm_read();
    locals[to] = vm_obj_num_mul(locals[lhs], locals[rhs]);
    vm_run_next_op();
}
do_div:
{
    vm_reg_t to = vm_read();
    vm_reg_t lhs = vm_read();
    vm_reg_t rhs = vm_read();
    locals[to] = vm_obj_num_div(locals[lhs], locals[rhs]);
    vm_run_next_op();
}
do_mod:
{
    vm_reg_t to = vm_read();
    vm_reg_t lhs = vm_read();
    vm_reg_t rhs = vm_read();
    locals[to] = vm_obj_num_mod(locals[lhs], locals[rhs]);
    vm_run_next_op();
}
do_concat:
{
    vm_gc_run1(gc, globals, frame->locals);
    vm_reg_t to = vm_read();
    vm_reg_t lhs = vm_read();
    vm_reg_t rhs = vm_read();
    vm_obj_t o1 = locals[lhs];
    vm_obj_t o2 = locals[rhs];
    locals[to] = vm_gc_concat(gc, o1, o2);
    vm_run_next_op();
}
do_static_call:
{
    vm_reg_t outreg = vm_read();
    vm_loc_t next_func = vm_read();
    vm_int_t nargs = vm_read();
    vm_obj_t *next_locals = frame->locals;
    for (vm_int_t argno = 1; argno <= nargs; argno++)
    {
        vm_reg_t regno = vm_read();
        next_locals[argno] = locals[regno];
    }
    locals = next_locals;
    frame->index = index;
    frame->outreg = outreg;
    frame++;
    frame->locals = locals + vm_read_at(next_func - 1);
    vm_run_op(next_func);
}
do_putchar:
{
    vm_reg_t from = vm_read();
    vm_int_t val = vm_obj_to_int(locals[from]);
    state->putchar(state, val);
    vm_run_next_op();
}
do_string_new:
{
    vm_reg_t outreg = vm_read();
    vm_int_t nargs = vm_read();
    vm_gc_entry_t *str = vm_gc_array_new(gc, nargs);
    for (size_t i = 0; i < nargs; i++)
    {
        vm_number_t num = vm_read();
        vm_gc_set_index(str, i, vm_obj_of_int(num));
    }
    locals[outreg] = vm_obj_of_ptr(str);
    vm_run_next_op();
}
do_array_new:
{
    vm_gc_run1(gc, globals, frame->locals);
    vm_reg_t outreg = vm_read();
    vm_int_t nargs = vm_read();
    vm_gc_entry_t *vec = vm_gc_array_new(gc, nargs);
    for (vm_int_t i = 0; i < nargs; i++)
    {
        vm_reg_t vreg = vm_read();
        vm_gc_set_index(vec, i, locals[vreg]);
    }
    locals[outreg] = vm_obj_of_ptr(vec);
    vm_run_next_op();
}
do_length:
{
    vm_reg_t outreg = vm_read();
    vm_reg_t reg = vm_read();
    vm_obj_t vec = locals[reg];
    locals[outreg] = vm_obj_of_int(vm_gc_sizeof(vm_obj_to_ptr(vec)));
    vm_run_next_op();
}
do_index_get:
{
    vm_reg_t outreg = vm_read();
    vm_reg_t reg = vm_read();
    vm_reg_t ind = vm_read();
    vm_obj_t vec = locals[reg];
    vm_obj_t oindex = locals[ind];
    locals[outreg] = vm_gc_get_index(vm_obj_to_ptr(vec), vm_obj_to_int(oindex));
    vm_run_next_op();
}
do_index_set:
{
    vm_reg_t reg = vm_read();
    vm_reg_t ind = vm_read();
    vm_reg_t val = vm_read();
    vm_obj_t vec = locals[reg];
    vm_obj_t oindex = locals[ind];
    vm_obj_t value = locals[val];
    vm_gc_set_index(vm_obj_to_ptr(vec), vm_obj_to_int(oindex), value);
    vm_run_next_op();
}
do_type:
{
    vm_reg_t outreg = vm_read();
    vm_reg_t valreg = vm_read();
    vm_obj_t obj = locals[valreg];
    double num = -1;
    if (vm_obj_is_none(obj))
    {
        num = VM_TYPE_NONE;
    }
    if (vm_obj_is_bool(obj))
    {
        num = VM_TYPE_BOOL;
    }
    if (vm_obj_is_num(obj))
    {
        num = VM_TYPE_NUMBER;
    }
    if (vm_obj_is_ptr(obj))
    {
        num = VM_TYPE_ARRAY;
    }
    locals[outreg] = vm_obj_of_num(num);
    vm_run_next_op();
}
do_exec:
{
    vm_reg_t in = vm_read();
    vm_reg_t argreg = vm_read();
    vm_gc_entry_t *ent = vm_obj_to_ptr(locals[in]);
    vm_int_t xlen = vm_gc_sizeof(ent);
    vm_opcode_t *xops = vm_malloc(sizeof(vm_opcode_t) * xlen);
    for (vm_int_t i = 0; i < xlen; i++)
    {
        vm_obj_t obj = vm_gc_get_index(ent, i);
        double n = vm_obj_to_num(obj);
        xops[i] = (vm_opcode_t) n;
    }
    vm_state_t *xstate = vm_state_new(0, NULL);
    xstate->globals[0] = vm_gc_dup(&xstate->gc, locals[argreg]);
    vm_state_set_ops(xstate, xlen, xops);
    vm_run_state(xstate);
    vm_run_next_op();
}
do_extend:
{
    vm_reg_t toreg = vm_read();
    vm_reg_t fromreg = vm_read();
    vm_obj_t to = locals[toreg];
    vm_obj_t from = locals[fromreg];
    vm_gc_extend(vm_obj_to_ptr(to), vm_obj_to_ptr(from));
    vm_run_next_op();
}
do_push:
{
    vm_reg_t toreg = vm_read();
    vm_reg_t fromreg = vm_read();
    vm_obj_t to = locals[toreg];
    vm_obj_t from = locals[fromreg];
    vm_gc_entry_t *ptr = vm_obj_to_ptr(to);
    vm_gc_push(vm_obj_to_ptr(to), from);
    vm_run_next_op();
}
do_dump:
{
    vm_reg_t namreg = vm_read();
    vm_reg_t inreg = vm_read();
    vm_gc_entry_t *sname = vm_obj_to_ptr(locals[namreg]);
    vm_int_t slen = vm_gc_sizeof(sname);
    vm_char_t *name = vm_malloc(sizeof(vm_char_t) * (slen + 1));
    for (vm_int_t i = 0; i < slen; i++)
    {
        vm_obj_t obj = vm_gc_get_index(sname, i);
        name[i] = vm_obj_to_num(obj);
    }
    name[slen] = '\0';
    vm_gc_entry_t *ent = vm_obj_to_ptr(locals[inreg]);
    uint8_t size = sizeof(vm_opcode_t); 
    vm_int_t xlen = vm_gc_sizeof(ent);
    FILE *out = fopen(name, "wb");
    fwrite(&size, 1, 1, out);
    vm_free(name);
    for (vm_int_t i = 0; i < xlen; i++)
    {
        vm_obj_t obj = vm_gc_get_index(ent, i);
        vm_opcode_t op = vm_obj_to_int(obj);
        fwrite(&op, sizeof(vm_opcode_t), 1,  out);
    }
    fclose(out);
    vm_run_next_op();
}
do_read:
{
    vm_gc_run1(gc, globals, frame->locals);
    vm_reg_t outreg = vm_read();
    vm_reg_t namereg = vm_read();
    vm_gc_entry_t *sname = vm_obj_to_ptr(locals[namereg]);
    vm_int_t slen = vm_gc_sizeof(sname);
    vm_char_t *name = vm_malloc(sizeof(vm_char_t) * (slen + 1));
    for (vm_int_t i = 0; i < slen; i++)
    {
        vm_obj_t obj = vm_gc_get_index(sname, i);
        name[i] = vm_obj_to_num(obj);
    }
    name[slen] = '\0';
    vm_int_t where = 0;
    vm_int_t nalloc = 64;
    FILE *in = fopen(name, "rb");
    vm_free(name);
    if (in == NULL) {
        locals[outreg] = vm_obj_of_none();
        vm_run_next_op();
    }
    vm_char_t *str = vm_malloc(sizeof(vm_char_t) * nalloc);
    while (true)
    {
        uint8_t buf[2048];
        vm_int_t n = fread(buf, 1, 2048, in);
        for (vm_int_t i = 0; i < n; i++)
        {
            if (where + 4 >= nalloc)
            {
                nalloc = 4 + nalloc * 2;
                str = vm_realloc(str, sizeof(vm_char_t) * nalloc);
            }
            str[where] = buf[i];
            where += 1;
        }
        if (n < 2048)
        {
            break;
        }
    }
    fclose(in);
    vm_gc_entry_t *ent = vm_gc_array_new(gc, where);
    for (vm_int_t i = 0; i < where; i++)
    {
        vm_gc_set_index(ent, i, vm_obj_of_int(str[i]));
    }
    vm_free(str);
    locals[outreg] = vm_obj_of_ptr(ent);
    vm_run_next_op();
}
do_write:
{
    vm_reg_t outreg = vm_read();
    vm_reg_t inreg = vm_read();
    vm_gc_entry_t *sname = vm_obj_to_ptr(locals[outreg]);
    vm_int_t slen = vm_gc_sizeof(sname);
    vm_char_t *name = vm_malloc(sizeof(vm_char_t) * (slen + 1));
    for (vm_int_t i = 0; i < slen; i++)
    {
        vm_obj_t obj = vm_gc_get_index(sname, i);
        name[i] = vm_obj_to_num(obj);
    }
    name[slen] = '\0';
    vm_gc_entry_t *ent = vm_obj_to_ptr(locals[inreg]);
    vm_int_t xlen = vm_gc_sizeof(ent);
    FILE *out = fopen(name, "wb");
    vm_free(name);
    for (vm_int_t i = 0; i < xlen; i++)
    {
        vm_obj_t obj = vm_gc_get_index(ent, i);
        uint8_t op = vm_obj_to_num(obj);
        fwrite(&op, 1, sizeof(uint8_t), out);
    }
    fclose(out);
    vm_run_next_op();
}
do_load_global:
{
    vm_reg_t out = vm_read();
    vm_reg_t global = vm_read();
    locals[out] = locals[global];
    vm_run_next_op();
}
do_dynamic_call:
{
    vm_reg_t outreg = vm_read();
    vm_reg_t funcreg = vm_read();
    vm_int_t nargs = vm_read();
    vm_obj_t *next_locals = frame->locals;
    for (vm_int_t argno = 1; argno <= nargs; argno++)
    {
        vm_reg_t regno = vm_read();
        next_locals[argno] = locals[regno];
    }
    vm_int_t next_func = vm_obj_to_int(locals[funcreg]);
    locals = next_locals;
    frame->index = index;
    frame->outreg = outreg;
    frame++;
    frame->locals = locals + vm_read_at(next_func - 1);
    vm_run_op(next_func);
}
do_static_array_new:
{
    vm_gc_run1(gc, globals, frame->locals);
    vm_reg_t outreg = vm_read();
    vm_int_t nargs = vm_read();
    vm_gc_entry_t *vec = vm_gc_static_array_new(gc, nargs);
    for (vm_int_t i = 0; i < nargs; i++)
    {
        vm_reg_t vreg = vm_read();
        vm_gc_set_index(vec, i, locals[vreg]);
    }
    locals[outreg] = vm_obj_of_ptr(vec);
    vm_run_next_op();
}
do_static_concat:
{
    vm_gc_run1(gc, globals, frame->locals);
    vm_reg_t to = vm_read();
    vm_reg_t lhs = vm_read();
    vm_reg_t rhs = vm_read();
    vm_obj_t o1 = locals[lhs];
    vm_obj_t o2 = locals[rhs];
    locals[to] = vm_gc_static_concat(gc, o1, o2);
    vm_run_next_op();
}
do_static_call0:
{
    vm_reg_t outreg = vm_read();
    vm_loc_t next_func = vm_read();
    vm_obj_t *next_locals = frame->locals;
    locals = next_locals;
    frame->index = index;
    frame->outreg = outreg;
    frame++;
    frame->locals = locals + vm_read_at(next_func - 1);
    vm_run_op(next_func);
}
do_static_call1:
{
    vm_reg_t outreg = vm_read();
    vm_loc_t next_func = vm_read();
    vm_obj_t *next_locals = frame->locals;
    next_locals[1] = locals[vm_read()];
    locals = next_locals;
    frame->index = index;
    frame->outreg = outreg;
    frame++;
    frame->locals = locals + vm_read_at(next_func - 1);
    vm_run_op(next_func);
}
do_static_call2:
{
    vm_reg_t outreg = vm_read();
    vm_loc_t next_func = vm_read();
    vm_obj_t *next_locals = frame->locals;
    next_locals[1] = locals[vm_read()];
    next_locals[2] = locals[vm_read()];
    locals = next_locals;
    frame->index = index;
    frame->outreg = outreg;
    frame++;
    frame->locals = locals + vm_read_at(next_func - 1);
    vm_run_op(next_func);
}
do_static_call3:
{
    vm_reg_t outreg = vm_read();
    vm_loc_t next_func = vm_read();
    vm_obj_t *next_locals = frame->locals;
    next_locals[1] = locals[vm_read()];
    next_locals[2] = locals[vm_read()];
    next_locals[3] = locals[vm_read()];
    locals = next_locals;
    frame->index = index;
    frame->outreg = outreg;
    frame++;
    frame->locals = locals + vm_read_at(next_func - 1);
    vm_run_op(next_func);
}
do_branch_equal:
{
    vm_obj_t lhs = locals[vm_read()];
    vm_obj_t rhs = locals[vm_read()];
    if (vm_obj_eq(lhs, rhs))
    {
        vm_loc_t jt = vm_read_at(index + 1);
        vm_run_op(jt);
    }
    else
    {
        vm_loc_t jf = vm_read_at(index);
        vm_run_op(jf);
    }
}
do_branch_not_equal:
{
    vm_obj_t lhs = locals[vm_read()];
    vm_obj_t rhs = locals[vm_read()];
    if (vm_obj_neq(lhs, rhs))
    {
        vm_loc_t jt = vm_read_at(index + 1);
        vm_run_op(jt);
    }
    else
    {
        vm_loc_t jf = vm_read_at(index);
        vm_run_op(jf);
    }
}
do_branch_less:
{
    vm_obj_t lhs = locals[vm_read()];
    vm_obj_t rhs = locals[vm_read()];
    if (vm_obj_lt(lhs, rhs))
    {
        vm_loc_t jt = vm_read_at(index + 1);
        vm_run_op(jt);
    }
    else
    {
        vm_loc_t jf = vm_read_at(index);
        vm_run_op(jf);
    }
}
do_branch_greater:
{
    vm_obj_t lhs = locals[vm_read()];
    vm_obj_t rhs = locals[vm_read()];
    if (vm_obj_gt(lhs, rhs))
    {
        vm_loc_t jt = vm_read_at(index + 1);
        vm_run_op(jt);
    }
    else
    {
        vm_loc_t jf = vm_read_at(index);
        vm_run_op(jf);
    }
}
do_branch_less_than_equal:
{
    vm_obj_t lhs = locals[vm_read()];
    vm_obj_t rhs = locals[vm_read()];
    if (vm_obj_lte(lhs, rhs))
    {
        vm_loc_t jt = vm_read_at(index + 1);
        vm_run_op(jt);
    }
    else
    {
        vm_loc_t jf = vm_read_at(index);
        vm_run_op(jf);
    }
}
do_branch_greater_than_equal:
{
    vm_obj_t lhs = locals[vm_read()];
    vm_obj_t rhs = locals[vm_read()];
    if (vm_obj_gte(lhs, rhs))
    {
        vm_loc_t jt = vm_read_at(index + 1);
        vm_run_op(jt);
    }
    else
    {
        vm_loc_t jf = vm_read_at(index);
        vm_run_op(jf);
    }
}
do_branch_bool:
{
    vm_reg_t from = vm_read();
    if (vm_obj_to_bool(locals[from]))
    {
        vm_loc_t jt = vm_read_at(index + 1);
        vm_run_op(jt);
    }
    else
    {
        vm_loc_t jf = vm_read_at(index);
        vm_run_op(jf);
    }
}
do_inc:
{
    vm_reg_t to = vm_read();
    vm_reg_t lhs = vm_read();
    vm_int_t rhs = vm_read();
    locals[to] = vm_obj_num_addc(locals[lhs], rhs);
    vm_run_next_op();
}
do_dec:
{
    vm_reg_t to = vm_read();
    vm_reg_t lhs = vm_read();
    vm_int_t rhs = vm_read();
    locals[to] = vm_obj_num_subc(locals[lhs], rhs);
    vm_run_next_op();
}
do_branch_equal_int:
{
    vm_obj_t lhs = locals[vm_read()];
    vm_int_t rhs = vm_read();
    if (vm_obj_ieq(lhs, rhs))
    {
        vm_loc_t jt = vm_read_at(index + 1);
        vm_run_op(jt);
    }
    else
    {
        vm_loc_t jf = vm_read_at(index);
        vm_run_op(jf);
    }
}
do_branch_not_equal_int:
{
    vm_obj_t lhs = locals[vm_read()];
    vm_int_t rhs = vm_read();
    if (vm_obj_ineq(lhs, rhs))
    {
        vm_loc_t jt = vm_read_at(index + 1);
        vm_run_op(jt);
    }
    else
    {
        vm_loc_t jf = vm_read_at(index);
        vm_run_op(jf);
    }
}
do_branch_less_int:
{
    vm_obj_t lhs = locals[vm_read()];
    vm_int_t rhs = vm_read();
    if (vm_obj_ilt(lhs, rhs))
    {
        vm_loc_t jt = vm_read_at(index + 1);
        vm_run_op(jt);
    }
    else
    {
        vm_loc_t jf = vm_read_at(index);
        vm_run_op(jf);
    }
}
do_branch_greater_int:
{
    vm_obj_t lhs = locals[vm_read()];
    vm_int_t rhs = vm_read();
    if (vm_obj_igt(lhs, rhs))
    {
        vm_loc_t jt = vm_read_at(index + 1);
        vm_run_op(jt);
    }
    else
    {
        vm_loc_t jf = vm_read_at(index);
        vm_run_op(jf);
    }
}
do_branch_less_than_equal_int:
{
    vm_obj_t lhs = locals[vm_read()];
    vm_int_t rhs = vm_read();
    if (vm_obj_ilte(lhs, rhs))
    {
        vm_loc_t jt = vm_read_at(index + 1);
        vm_run_op(jt);
    }
    else
    {
        vm_loc_t jf = vm_read_at(index);
        vm_run_op(jf);
    }
}
do_branch_greater_than_equal_int:
{
    vm_obj_t lhs = locals[vm_read()];
    vm_int_t rhs = vm_read();
    if (vm_obj_igte(lhs, rhs))
    {
        vm_loc_t jt = vm_read_at(index + 1);
        vm_run_op(jt);
    }
    else
    {
        vm_loc_t jf = vm_read_at(index);
        vm_run_op(jf);
    }
}
}