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

#define vm_run_exit(ret_) \
    state->locals = locals; \
    state->index = index; \
    state->gas = gas; \
    return (ret_);

#define vm_run_next_op() \
    goto *ptrs[vm_read()]

#if !defined(VM_NO_SCHEDULE)
#define vm_run_op(index_) \
    index = index_; \
    if (gas-- == 0) { vm_run_exit(((vm_run_some_t) {.tag = VM_RUN_SOME_OUT_OF_GAS})); } \
    else { vm_run_next_op(); }
#else
#define vm_run_op(index_) \
    index = index_; \
    vm_run_next_op()
#endif

#define vm_read() (ops[index++])
#define vm_read_at(index_) (*(vm_opcode_t *)&ops[(index_)])

static inline vm_state_t *vm_run_some_rec(vm_state_t *cur)
{
    if (cur == NULL)
    {
        return NULL;
    }
    vm_state_t *ret = vm_run_some_rec(cur->next);
    cur->next = ret;
    cur->gas = 256;
    vm_run_some_t res = vm_run_some(cur);
    if (res.tag == VM_RUN_SOME_DEAD) {
        vm_state_t *next = cur->next;
        vm_state_del(cur);
        return next;
    }
    return cur;
}

void vm_run(vm_state_t *state)
{
    while (state != NULL)
    {
        state = vm_run_some_rec(state);
    }
}

vm_run_some_t vm_run_some(vm_state_t *state)
{
    ptrdiff_t gas = state->gas;
    const vm_opcode_t *ops = state->ops;
    vm_obj_t *locals = state->locals;
    size_t index = state->index;
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
    vm_run_next_op();
do_exit:
{
    vm_run_exit(((vm_run_some_t) {
        .tag = VM_RUN_SOME_DEAD,
    }));
}
do_load_global:
{
    vm_reg_t out = vm_read();
    vm_reg_t global = vm_read();
    locals[out] = locals[global];
    vm_run_next_op();
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
    vm_gc_run1(state->gc, state->globals, state->frame->locals);
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
    vm_gc_entry_t *ent = vm_gc_array_new(state->gc, where);
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
#endif
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
    xstate->globals[0] = locals[argreg];
    vm_state_set_ops(xstate, xlen, xops);
    xstate->next = state->next;
    state->next = xstate;
    vm_run_next_op();
}
do_return:
{
    vm_reg_t from = vm_read();
    vm_obj_t val = locals[from];
    state->frame--;
    locals = (state->frame - 1)->locals;
    vm_reg_t outreg = state->frame->outreg;
    locals[outreg] = val;
    vm_run_op(state->frame->index);
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
do_extend:
{
    vm_reg_t toreg = vm_read();
    vm_reg_t fromreg = vm_read();
    vm_obj_t to = locals[toreg];
    vm_obj_t from = locals[fromreg];
    vm_gc_extend(vm_obj_to_ptr(to), vm_obj_to_ptr(from));
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
do_string_new:
{
    vm_reg_t outreg = vm_read();
    vm_int_t nargs = vm_read();
    vm_gc_entry_t *str = vm_gc_array_new(state->gc, nargs);
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
    vm_gc_run1(state->gc, state->globals, state->frame->locals);
    vm_reg_t outreg = vm_read();
    vm_int_t nargs = vm_read();
    vm_gc_entry_t *vec = vm_gc_array_new(state->gc, nargs);
    for (vm_int_t i = 0; i < nargs; i++)
    {
        vm_reg_t vreg = vm_read();
        vm_gc_set_index(vec, i, locals[vreg]);
    }
    locals[outreg] = vm_obj_of_ptr(vec);
    vm_run_next_op();
}
do_static_array_new:
{
    vm_gc_run1(state->gc, state->globals, state->frame->locals);
    vm_reg_t outreg = vm_read();
    vm_int_t nargs = vm_read();
    vm_gc_entry_t *vec = vm_gc_static_array_new(state->gc, nargs);
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
do_static_call:
{
    vm_reg_t outreg = vm_read();
    vm_loc_t next_func = vm_read();
    vm_int_t nargs = vm_read();
    vm_obj_t *next_locals = state->frame->locals;
    for (vm_int_t argno = 1; argno <= nargs; argno++)
    {
        vm_reg_t regno = vm_read();
        next_locals[argno] = locals[regno];
    }
    locals = next_locals;
    state->frame->index = index;
    state->frame->outreg = outreg;
    state->frame++;
    state->frame->locals = locals + vm_read_at(next_func - 1);
    vm_run_op(next_func);
}
do_dynamic_call:
{
    vm_reg_t outreg = vm_read();
    vm_reg_t funcreg = vm_read();
    vm_int_t nargs = vm_read();
    vm_obj_t *next_locals = state->frame->locals;
    for (vm_int_t argno = 1; argno <= nargs; argno++)
    {
        vm_reg_t regno = vm_read();
        next_locals[argno] = locals[regno];
    }
    vm_int_t next_func = vm_obj_to_int(locals[funcreg]);
    locals = next_locals;
    state->frame->index = index;
    state->frame->outreg = outreg;
    state->frame++;
    state->frame->locals = locals + vm_read_at(next_func - 1);
    vm_run_op(next_func);
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
do_mul:
{
    vm_reg_t to = vm_read();
    vm_reg_t lhs = vm_read();
    vm_reg_t rhs = vm_read();
    locals[to] = vm_obj_num_mul(locals[lhs], locals[rhs]);
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
    vm_gc_run1(state->gc, state->globals, state->frame->locals);
    vm_reg_t to = vm_read();
    vm_reg_t lhs = vm_read();
    vm_reg_t rhs = vm_read();
    vm_obj_t o1 = locals[lhs];
    vm_obj_t o2 = locals[rhs];
    locals[to] = vm_gc_concat(state->gc, o1, o2);
    vm_run_next_op();
}
do_static_concat:
{
    vm_gc_run1(state->gc, state->globals, state->frame->locals);
    vm_reg_t to = vm_read();
    vm_reg_t lhs = vm_read();
    vm_reg_t rhs = vm_read();
    vm_obj_t o1 = locals[lhs];
    vm_obj_t o2 = locals[rhs];
    locals[to] = vm_gc_static_concat(state->gc, o1, o2);
    vm_run_next_op();
}
do_putchar:
{
    vm_reg_t from = vm_read();
    vm_int_t val = vm_obj_to_int(locals[from]);
    state->putchar(state, val);
    vm_run_next_op();
}
}