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

#define run_next_op \
    goto *ptrs[vm_read(state)];

#define vm_read(state) (state->ops[state->index++])
#define vm_read_ahead(index_) (*(vm_opcode_t *)&state->ops[(state->index) + (index_)])

void vm_run(vm_state_t *restrict state)
{
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
    };
    run_next_op;
do_exit:
{
    return;
}
do_load_global:
{
    vm_reg_t out = vm_read(state);
    vm_reg_t global = vm_read(state);
    state->locals[out] = state->locals[global];
    run_next_op;
}
#if defined(VM_OS)
do_os_error:
{
    putchar('?');
    putchar('?');
    putchar('?');
    putchar('\n');
}
#else
do_dump:
{
    vm_reg_t namreg = vm_read(state);
    vm_reg_t inreg = vm_read(state);
    vm_gc_entry_t *sname = vm_obj_to_ptr(state->locals[namreg]);
    vm_int_t slen = vm_gc_sizeof(sname);
    vm_char_t *name = vm_malloc(sizeof(vm_char_t) * (slen + 1));
    for (vm_int_t i = 0; i < slen; i++)
    {
        vm_obj_t obj = vm_gc_get_index(sname, i);
        name[i] = vm_obj_to_num(obj);
    }
    name[slen] = '\0';
    vm_gc_entry_t *ent = vm_obj_to_ptr(state->locals[inreg]);
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
    run_next_op;
}
do_read:
{
    vm_gc_run1(state->gc, state->globals, state->frame->locals);
    vm_reg_t outreg = vm_read(state);
    vm_reg_t namereg = vm_read(state);
    vm_gc_entry_t *sname = vm_obj_to_ptr(state->locals[namereg]);
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
        state->locals[outreg] = vm_obj_of_none();
        run_next_op;
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
    vm_gc_entry_t *ent = vm_gc_array_new(state->gc, where);
    for (vm_int_t i = 0; i < where; i++)
    {
        vm_gc_set_index(ent, i, vm_obj_of_int(str[i]));
    }
    vm_free(str);
    state->locals[outreg] = vm_obj_of_ptr(ent);
    run_next_op;
}
do_write:
{
    vm_reg_t outreg = vm_read(state);
    vm_reg_t inreg = vm_read(state);
    vm_gc_entry_t *sname = vm_obj_to_ptr(state->locals[outreg]);
    vm_int_t slen = vm_gc_sizeof(sname);
    vm_char_t *name = vm_malloc(sizeof(vm_char_t) * (slen + 1));
    for (vm_int_t i = 0; i < slen; i++)
    {
        vm_obj_t obj = vm_gc_get_index(sname, i);
        name[i] = vm_obj_to_num(obj);
    }
    name[slen] = '\0';
    vm_gc_entry_t *ent = vm_obj_to_ptr(state->locals[inreg]);
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
    run_next_op;
}
#endif
do_exec:
{
    vm_reg_t in = vm_read(state);
    vm_reg_t argreg = vm_read(state);
    vm_gc_entry_t *ent = vm_obj_to_ptr(state->locals[in]);
    vm_int_t xlen = vm_gc_sizeof(ent);
    vm_opcode_t *xops = vm_malloc(sizeof(vm_opcode_t) * xlen);
    for (vm_int_t i = 0; i < xlen; i++)
    {
        vm_obj_t obj = vm_gc_get_index(ent, i);
        double n = vm_obj_to_num(obj);
        xops[i] = (vm_opcode_t) n;
    }
    vm_state_t *xstate = vm_state_new(0, NULL);
    xstate->globals[0] = state->locals[argreg];
    vm_state_set_ops(xstate, xlen, xops);
    vm_run(xstate);
    vm_state_del(xstate);
    run_next_op;
}
do_return:
{
    vm_reg_t from = vm_read(state);
    vm_obj_t val = state->locals[from];
    state->frame--;
    state->locals = (state->frame - 1)->locals;
    state->index = state->frame->index;
    vm_reg_t outreg = state->frame->outreg;
    state->locals[outreg] = val;
    run_next_op;
}
do_push:
{
    vm_reg_t toreg = vm_read(state);
    vm_reg_t fromreg = vm_read(state);
    vm_obj_t to = state->locals[toreg];
    vm_obj_t from = state->locals[fromreg];
    vm_gc_entry_t *ptr = vm_obj_to_ptr(to);
    vm_gc_push(vm_obj_to_ptr(to), from);
    run_next_op;
}
do_extend:
{
    vm_reg_t toreg = vm_read(state);
    vm_reg_t fromreg = vm_read(state);
    vm_obj_t to = state->locals[toreg];
    vm_obj_t from = state->locals[fromreg];
    vm_gc_extend(vm_obj_to_ptr(to), vm_obj_to_ptr(from));
    run_next_op;
}
do_type:
{
    vm_reg_t outreg = vm_read(state);
    vm_reg_t valreg = vm_read(state);
    vm_obj_t obj = state->locals[valreg];
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
    state->locals[outreg] = vm_obj_of_num(num);
    run_next_op;
}
do_string_new:
{
    vm_reg_t outreg = vm_read(state);
    vm_int_t nargs = vm_read(state);
    vm_gc_entry_t *str = vm_gc_array_new(state->gc, nargs);
    for (size_t i = 0; i < nargs; i++)
    {
        vm_number_t num = vm_read(state);
        vm_gc_set_index(str, i, vm_obj_of_int(num));
    }
    state->locals[outreg] = vm_obj_of_ptr(str);
    run_next_op;
}
do_array_new:
{
    vm_gc_run1(state->gc, state->globals, state->frame->locals);
    vm_reg_t outreg = vm_read(state);
    vm_int_t nargs = vm_read(state);
    vm_gc_entry_t *vec = vm_gc_array_new(state->gc, nargs);
    for (vm_int_t i = 0; i < nargs; i++)
    {
        vm_reg_t vreg = vm_read(state);
        vm_gc_set_index(vec, i, state->locals[vreg]);
    }
    state->locals[outreg] = vm_obj_of_ptr(vec);
    run_next_op;
}
do_static_array_new:
{
    vm_gc_run1(state->gc, state->globals, state->frame->locals);
    vm_reg_t outreg = vm_read(state);
    vm_int_t nargs = vm_read(state);
    vm_gc_entry_t *vec = vm_gc_static_array_new(state->gc, nargs);
    for (vm_int_t i = 0; i < nargs; i++)
    {
        vm_reg_t vreg = vm_read(state);
        vm_gc_set_index(vec, i, state->locals[vreg]);
    }
    state->locals[outreg] = vm_obj_of_ptr(vec);
    run_next_op;
}
do_length:
{
    vm_reg_t outreg = vm_read(state);
    vm_reg_t reg = vm_read(state);
    vm_obj_t vec = state->locals[reg];
    state->locals[outreg] = vm_obj_of_int(vm_gc_sizeof(vm_obj_to_ptr(vec)));
    run_next_op;
}
do_index_get:
{
    vm_reg_t outreg = vm_read(state);
    vm_reg_t reg = vm_read(state);
    vm_reg_t ind = vm_read(state);
    vm_obj_t vec = state->locals[reg];
    vm_obj_t index = state->locals[ind];
    state->locals[outreg] = vm_gc_get_index(vm_obj_to_ptr(vec), vm_obj_to_int(index));
    run_next_op;
}
do_index_set:
{
    vm_reg_t reg = vm_read(state);
    vm_reg_t ind = vm_read(state);
    vm_reg_t val = vm_read(state);
    vm_obj_t vec = state->locals[reg];
    vm_obj_t index = state->locals[ind];
    vm_obj_t value = state->locals[val];
    vm_gc_set_index(vm_obj_to_ptr(vec), vm_obj_to_int(index), value);
    run_next_op;
}
do_static_call:
{
    vm_reg_t outreg = vm_read(state);
    vm_loc_t next_func = vm_read(state);
    vm_int_t nargs = vm_read(state);
    vm_obj_t *next_locals = state->frame->locals;
    for (vm_int_t argno = 1; argno <= nargs; argno++)
    {
        vm_reg_t regno = vm_read(state);
        next_locals[argno] = state->locals[regno];
    }
    state->locals = next_locals;
    state->frame->index = state->index;
    state->frame->outreg = outreg;
    state->frame++;
    state->index = next_func;
    state->frame->locals = state->locals + vm_read_ahead(-1);
    run_next_op;
}
do_dynamic_call:
{
    vm_reg_t outreg = vm_read(state);
    vm_reg_t funcreg = vm_read(state);
    vm_int_t nargs = vm_read(state);
    vm_obj_t *next_locals = state->frame->locals;
    for (vm_int_t argno = 1; argno <= nargs; argno++)
    {
        vm_reg_t regno = vm_read(state);
        next_locals[argno] = state->locals[regno];
    }
    vm_obj_t next_func = state->locals[funcreg];
    state->locals = next_locals;
    state->frame->index = state->index;
    state->frame->outreg = outreg;
    state->frame++;
    state->index = (vm_loc_t) vm_obj_to_int(next_func);
    state->frame->locals = state->locals + vm_read_ahead(-1);
    run_next_op;
}
do_store_none:
{
    vm_reg_t to = vm_read(state);
    state->locals[to] = vm_obj_of_none();
    run_next_op;
}
do_store_bool:
{
    vm_reg_t to = vm_read(state);
    vm_int_t from = (int)vm_read(state);
    state->locals[to] = vm_obj_of_bool((bool)from);
    run_next_op;
}
do_store_reg:
{
    vm_reg_t to = vm_read(state);
    vm_reg_t from = vm_read(state);
    state->locals[to] = state->locals[from];
    run_next_op;
}
do_store_int:
{
    vm_reg_t to = vm_read(state);
    vm_int_t from = vm_read(state);
    state->locals[to] = vm_obj_of_int(from);
    run_next_op;
}
do_equal:
{
    vm_reg_t to = vm_read(state);
    vm_reg_t lhs = vm_read(state);
    vm_reg_t rhs = vm_read(state);
    state->locals[to] = vm_obj_of_bool(vm_obj_eq(state->locals[lhs], state->locals[rhs]));
    run_next_op;
}
do_not_equal:
{
    vm_reg_t to = vm_read(state);
    vm_reg_t lhs = vm_read(state);
    vm_reg_t rhs = vm_read(state);
    state->locals[to] = vm_obj_of_bool(vm_obj_neq(state->locals[lhs], state->locals[rhs]));
    run_next_op;
}
do_less:
{
    vm_reg_t to = vm_read(state);
    vm_reg_t lhs = vm_read(state);
    vm_reg_t rhs = vm_read(state);
    state->locals[to] = vm_obj_of_bool(vm_obj_lt(state->locals[lhs], state->locals[rhs]));
    run_next_op;
}
do_greater:
{
    vm_reg_t to = vm_read(state);
    vm_reg_t lhs = vm_read(state);
    vm_reg_t rhs = vm_read(state);
    state->locals[to] = vm_obj_of_bool(vm_obj_gt(state->locals[lhs], state->locals[rhs]));
    run_next_op;
}
do_less_than_equal:
{
    vm_reg_t to = vm_read(state);
    vm_reg_t lhs = vm_read(state);
    vm_reg_t rhs = vm_read(state);
    state->locals[to] = vm_obj_of_bool(vm_obj_lte(state->locals[lhs], state->locals[rhs]));
    run_next_op;
}
do_greater_than_equal:
{
    vm_reg_t to = vm_read(state);
    vm_reg_t lhs = vm_read(state);
    vm_reg_t rhs = vm_read(state);
    state->locals[to] = vm_obj_of_bool(vm_obj_gte(state->locals[lhs], state->locals[rhs]));
    run_next_op;
}
do_jump:
{
    vm_loc_t to = vm_read(state);
    state->index = to;
    run_next_op;
}
do_branch_true:
{
    vm_loc_t to1 = vm_read(state);
    vm_loc_t to2 = vm_read(state);
    vm_reg_t from = vm_read(state);
    if (vm_obj_to_bool(state->locals[from]))
    {
        state->index = to1;
    }
    else
    {
        state->index = to2;
    }
    run_next_op;
}
do_add:
{
    vm_reg_t to = vm_read(state);
    vm_reg_t lhs = vm_read(state);
    vm_reg_t rhs = vm_read(state);
    state->locals[to] = vm_obj_num_add(state->locals[lhs], state->locals[rhs]);
    run_next_op;
}
do_mul:
{
    vm_reg_t to = vm_read(state);
    vm_reg_t lhs = vm_read(state);
    vm_reg_t rhs = vm_read(state);
    state->locals[to] = vm_obj_num_mul(state->locals[lhs], state->locals[rhs]);
    run_next_op;
}
do_sub:
{
    vm_reg_t to = vm_read(state);
    vm_reg_t lhs = vm_read(state);
    vm_reg_t rhs = vm_read(state);
    state->locals[to] = vm_obj_num_sub(state->locals[lhs], state->locals[rhs]);
    run_next_op;
}
do_div:
{
    vm_reg_t to = vm_read(state);
    vm_reg_t lhs = vm_read(state);
    vm_reg_t rhs = vm_read(state);
    state->locals[to] = vm_obj_num_div(state->locals[lhs], state->locals[rhs]);
    run_next_op;
}
do_mod:
{
    vm_reg_t to = vm_read(state);
    vm_reg_t lhs = vm_read(state);
    vm_reg_t rhs = vm_read(state);
    state->locals[to] = vm_obj_num_mod(state->locals[lhs], state->locals[rhs]);
    run_next_op;
}
do_concat:
{
    vm_gc_run1(state->gc, state->globals, state->frame->locals);
    vm_reg_t to = vm_read(state);
    vm_reg_t lhs = vm_read(state);
    vm_reg_t rhs = vm_read(state);
    vm_obj_t o1 = state->locals[lhs];
    vm_obj_t o2 = state->locals[rhs];
    state->locals[to] = vm_gc_concat(state->gc, o1, o2);
    run_next_op;
}
do_static_concat:
{
    vm_gc_run1(state->gc, state->globals, state->frame->locals);
    vm_reg_t to = vm_read(state);
    vm_reg_t lhs = vm_read(state);
    vm_reg_t rhs = vm_read(state);
    vm_obj_t o1 = state->locals[lhs];
    vm_obj_t o2 = state->locals[rhs];
    state->locals[to] = vm_gc_static_concat(state->gc, o1, o2);
    run_next_op;
}
do_putchar:
{
    vm_reg_t from = vm_read(state);
    vm_int_t val = vm_obj_to_int(state->locals[from]);
    state->putchar(state, val);
    run_next_op;
}
}