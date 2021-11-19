#include "vm.h"
#include "gc.h"
#include "math.h"
#include "obj.h"
#include "libc.h"
#include "state.h"

#define VM_GLOBALS_NUM (1 << 8)

#if defined(VM_OS)
void os_putn(size_t n);
void os_puts(const char *str);
#endif

#if defined(VM_DEBUG_OPCODE)
#if !defined(VM_OS)
#define run_next_op                                                   \
    printf("%i -> %i\n", (int)cur_index, (int)basefunc[cur_index]); \
    goto *next_op;
#else
#define run_next_op                                                   \
    os_putn(cur_index);                                              \
    os_puts("\n");                                                   \
    goto *next_op;
#endif
#else
#define run_next_op \
    goto *next_op;
#endif

#define vm_read                              \
    (                                                        \
        {                                                    \
            vm_opcode_t ret = basefunc[cur_index];        \
            cur_index += 1; \
            ret;                                             \
        })

#define vm_read_reg vm_read

#define next_op (cur_index += 1, next_op_value)
#define vm_fetch (next_op_value = ptrs[basefunc[cur_index]])

#define vm_read_ahead(index) (*(vm_opcode_t *)&basefunc[(cur_index) + (index)])

vm_stack_frame_t vm_frames_base[VM_FRAMES_UNITS];
vm_obj_t vm_locals_base[VM_LOCALS_UNITS + VM_GLOBALS_NUM];
vm_opcode_t vm_xops[1 << 16];

void vm_run(vm_state_t *state, const vm_opcode_t *basefunc)
{
vm_reboot:;
    vm_loc_t cur_index = 0;

    vm_stack_frame_t *frames_base = &vm_frames_base[0];
    vm_obj_t *locals_base = &vm_locals_base[0];
  
    vm_stack_frame_t *cur_frame = frames_base;
    vm_obj_t *cur_locals = locals_base;

    vm_gc_t *gc = state->gc;

    cur_locals[0] = state->global;

    void *next_op_value;
    void *ptrs[VM_OPCODE_MAX1];
    ptrs[VM_OPCODE_EXIT] = &&do_exit;
    ptrs[VM_OPCODE_STORE_REG] = &&do_store_reg;
    ptrs[VM_OPCODE_STORE_NONE] = &&do_store_none;
    ptrs[VM_OPCODE_STORE_BOOL] = &&do_store_bool;
    ptrs[VM_OPCODE_STORE_INT] = &&do_store_int;
    ptrs[VM_OPCODE_EQUAL] = &&do_equal;
    ptrs[VM_OPCODE_NOT_EQUAL] = &&do_not_equal;
    ptrs[VM_OPCODE_LESS] = &&do_less;
    ptrs[VM_OPCODE_GREATER] = &&do_greater;
    ptrs[VM_OPCODE_LESS_THAN_EQUAL] = &&do_less_than_equal;
    ptrs[VM_OPCODE_GREATER_THAN_EQUAL] = &&do_greater_than_equal;
    ptrs[VM_OPCODE_JUMP] = &&do_jump;
    ptrs[VM_OPCODE_BRANCH_TRUE] = &&do_branch_true;
    ptrs[VM_OPCODE_ADD] = &&do_add;
    ptrs[VM_OPCODE_SUB] = &&do_sub;
    ptrs[VM_OPCODE_MUL] = &&do_mul;
    ptrs[VM_OPCODE_DIV] = &&do_div;
    ptrs[VM_OPCODE_MOD] = &&do_mod;
    ptrs[VM_OPCODE_CONCAT] = &&do_concat;
    ptrs[VM_OPCODE_STATIC_CALL] = &&do_static_call;
    ptrs[VM_OPCODE_RETURN] = &&do_return;
    ptrs[VM_OPCODE_PUTCHAR] = &&do_putchar;
    ptrs[VM_OPCODE_STRING_NEW] = &&do_string_new;
    ptrs[VM_OPCODE_ARRAY_NEW] = &&do_array_new;
    ptrs[VM_OPCODE_LENGTH] = &&do_length;
    ptrs[VM_OPCODE_INDEX_GET] = &&do_index_get;
    ptrs[VM_OPCODE_INDEX_SET] = &&do_index_set;
    ptrs[VM_OPCODE_EXEC] = &&do_exec;
    ptrs[VM_OPCODE_TYPE] = &&do_type;
    ptrs[VM_OPCODE_EXTEND] = &&do_extend;
    ptrs[VM_OPCODE_PUSH] = &&do_push;
#if defined(VM_OS)
    ptrs[VM_OPCODE_DUMP] = &&do_os_error;
    ptrs[VM_OPCODE_WRITE] = &&do_os_error;
    ptrs[VM_OPCODE_READ] = &&do_os_error;
#else
    ptrs[VM_OPCODE_DUMP] = &&do_dump;
    ptrs[VM_OPCODE_WRITE] = &&do_write;
    ptrs[VM_OPCODE_READ] = &&do_read;
#endif
    ptrs[VM_OPCODE_LOAD_GLOBAL] = &&do_load_global;
    ptrs[VM_OPCODE_DYNAMIC_CALL] = &&do_dynamic_call;
    ptrs[VM_OPCODE_STATIC_ARRAY_NEW] = &&do_static_array_new;
    ptrs[VM_OPCODE_STATIC_CONCAT] = &&do_static_concat;
    cur_frame->locals = cur_locals;
    cur_frame += 1;
    cur_frame->locals = cur_locals + VM_GLOBALS_NUM;
    vm_fetch;
    run_next_op;
do_exit:
{
    gc->first = NULL;
    return;
}
do_load_global:
{
    vm_reg_t out = vm_read_reg;
    vm_reg_t global = vm_read_reg;
    vm_fetch;
    cur_locals[out] = locals_base[global];
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
    vm_reg_t namreg = vm_read_reg;
    vm_reg_t inreg = vm_read_reg;
    vm_fetch;
    vm_gc_entry_t *sname = vm_obj_to_ptr(cur_locals[namreg]);
    int slen = vm_gc_sizeof(sname);
    char *name = vm_malloc(sizeof(char) * (slen + 1));
    for (int i = 0; i < slen; i++)
    {
        vm_obj_t obj = vm_gc_get_index(sname, i);
        name[i] = vm_obj_to_num(obj);
    }
    name[slen] = '\0';
    vm_gc_entry_t *ent = vm_obj_to_ptr(cur_locals[inreg]);
    uint8_t size = sizeof(vm_opcode_t); 
    int xlen = vm_gc_sizeof(ent);
    FILE *out = fopen(name, "wb");
    fwrite(&size, 1, 1, out);
    vm_free(name);
    for (int i = 0; i < xlen; i++)
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
    vm_reg_t outreg = vm_read_reg;
    vm_reg_t namereg = vm_read_reg;
    vm_fetch;
    vm_gc_entry_t *sname = vm_obj_to_ptr(cur_locals[namereg]);
    int slen = vm_gc_sizeof(sname);
    char *name = vm_malloc(sizeof(char) * (slen + 1));
    for (int i = 0; i < slen; i++)
    {
        vm_obj_t obj = vm_gc_get_index(sname, i);
        name[i] = vm_obj_to_num(obj);
    }
    name[slen] = '\0';
    int where = 0;
    int nalloc = 64;
    FILE *in = fopen(name, "rb");
    vm_free(name);
    if (in == NULL) {
        cur_locals[outreg] = vm_obj_of_none();
        run_next_op;
    }
    char *str = vm_malloc(sizeof(char) * nalloc);
    while (true)
    {
        uint8_t buf[2048];
        int n = fread(buf, 1, 2048, in);
        for (int i = 0; i < n; i++)
        {
            if (where + 4 >= nalloc)
            {
                nalloc = 4 + nalloc * 2;
                str = vm_realloc(str, sizeof(char) * nalloc);
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
    for (int i = 0; i < where; i++)
    {
        vm_gc_set_index(ent, i, vm_obj_of_int(str[i]));
    }
    vm_free(str);
    cur_locals[outreg] = vm_obj_of_ptr(ent);
    run_next_op;
}
do_write:
{
    vm_reg_t outreg = vm_read_reg;
    vm_reg_t inreg = vm_read_reg;
    vm_fetch;
    vm_gc_entry_t *sname = vm_obj_to_ptr(cur_locals[outreg]);
    int slen = vm_gc_sizeof(sname);
    char *name = vm_malloc(sizeof(char) * (slen + 1));
    for (int i = 0; i < slen; i++)
    {
        vm_obj_t obj = vm_gc_get_index(sname, i);
        name[i] = vm_obj_to_num(obj);
    }
    name[slen] = '\0';
    vm_gc_entry_t *ent = vm_obj_to_ptr(cur_locals[inreg]);
    int xlen = vm_gc_sizeof(ent);
    FILE *out = fopen(name, "wb");
    vm_free(name);
    for (int i = 0; i < xlen; i++)
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
    vm_reg_t in = vm_read_reg;
    vm_reg_t argreg = vm_read_reg;
    vm_fetch;
    vm_gc_entry_t *ent = vm_obj_to_ptr(cur_locals[in]);
    int xlen = vm_gc_sizeof(ent);
    vm_opcode_t *xops = &vm_xops[0];
    for (int i = 0; i < xlen; i++)
    {
        vm_obj_t obj = vm_gc_get_index(ent, i);
        double n = vm_obj_to_num(obj);
        xops[i] = (vm_opcode_t) n;
    }
    state->global = cur_locals[argreg];
    basefunc = xops;
    goto vm_reboot;
    run_next_op;
}
do_return:
{
    vm_reg_t from = vm_read_reg;
    vm_obj_t val = cur_locals[from];
    cur_frame--;
    cur_locals = (cur_frame - 1)->locals;
    cur_index = cur_frame->index;
    vm_reg_t outreg = cur_frame->outreg;
    cur_locals[outreg] = val;
    vm_fetch;
    run_next_op;
}
do_push:
{
    vm_reg_t toreg = vm_read_reg;
    vm_reg_t fromreg = vm_read_reg;
    vm_fetch;
    vm_obj_t to = cur_locals[toreg];
    vm_obj_t from = cur_locals[fromreg];
    vm_gc_entry_t *ptr = vm_obj_to_ptr(to);
    vm_gc_push(vm_obj_to_ptr(to), from);
    run_next_op;
}
do_extend:
{
    vm_reg_t toreg = vm_read_reg;
    vm_reg_t fromreg = vm_read_reg;
    vm_fetch;
    vm_obj_t to = cur_locals[toreg];
    vm_obj_t from = cur_locals[fromreg];
    vm_gc_extend(vm_obj_to_ptr(to), vm_obj_to_ptr(from));
    run_next_op;
}
do_type:
{
    vm_reg_t outreg = vm_read_reg;
    vm_reg_t valreg = vm_read_reg;
    vm_fetch;
    vm_obj_t obj = cur_locals[valreg];
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
    cur_locals[outreg] = vm_obj_of_num(num);
    run_next_op;
}
do_string_new:
{
    vm_reg_t outreg = vm_read_reg;
    int nargs = vm_read;
    vm_gc_entry_t *str = vm_gc_array_new(gc, nargs);
    for (size_t i = 0; i < nargs; i++)
    {
        vm_gc_set_index(str, i, vm_obj_of_int(vm_read));
    }
    vm_fetch;
    cur_locals[outreg] = vm_obj_of_ptr(str);
    run_next_op;
}
do_array_new:
{
    vm_gc_run1(gc, locals_base, cur_frame->locals);
    vm_reg_t outreg = vm_read_reg;
    int nargs = vm_read;
    vm_gc_entry_t *vec = vm_gc_array_new(gc, nargs);
    for (int i = 0; i < nargs; i++)
    {
        vm_reg_t vreg = vm_read_reg;
        vm_gc_set_index(vec, i, cur_locals[vreg]);
    }
    vm_fetch;
    cur_locals[outreg] = vm_obj_of_ptr(vec);
    run_next_op;
}
do_static_array_new:
{
    vm_gc_run1(gc, locals_base, cur_frame->locals);
    vm_reg_t outreg = vm_read_reg;
    int nargs = vm_read;
    vm_gc_entry_t *vec = vm_gc_static_array_new(gc, nargs);
    for (int i = 0; i < nargs; i++)
    {
        vm_reg_t vreg = vm_read_reg;
        vm_gc_set_index(vec, i, cur_locals[vreg]);
    }
    vm_fetch;
    cur_locals[outreg] = vm_obj_of_ptr(vec);
    run_next_op;
}
do_length:
{
    vm_reg_t outreg = vm_read_reg;
    vm_reg_t reg = vm_read_reg;
    vm_fetch;
    vm_obj_t vec = cur_locals[reg];
    cur_locals[outreg] = vm_obj_of_int(vm_gc_sizeof(vm_obj_to_ptr(vec)));
    run_next_op;
}
do_index_get:
{
    vm_reg_t outreg = vm_read_reg;
    vm_reg_t reg = vm_read_reg;
    vm_reg_t ind = vm_read_reg;
    vm_fetch;
    vm_obj_t vec = cur_locals[reg];
    vm_obj_t index = cur_locals[ind];
    cur_locals[outreg] = vm_gc_get_index(vm_obj_to_ptr(vec), vm_obj_to_int(index));
    run_next_op;
}
do_index_set:
{
    vm_reg_t reg = vm_read_reg;
    vm_reg_t ind = vm_read_reg;
    vm_reg_t val = vm_read_reg;
    vm_fetch;
    vm_obj_t vec = cur_locals[reg];
    vm_obj_t index = cur_locals[ind];
    vm_obj_t value = cur_locals[val];
    vm_gc_set_index(vm_obj_to_ptr(vec), vm_obj_to_int(index), value);
    run_next_op;
}
do_static_call:
{
    vm_reg_t outreg = vm_read_reg;
    vm_loc_t next_func = vm_read;
    int nargs = vm_read;
    vm_obj_t *next_locals = cur_frame->locals;
    for (int argno = 1; argno <= nargs; argno++)
    {
        vm_reg_t regno = vm_read_reg;
        next_locals[argno] = cur_locals[regno];
    }
    cur_locals = next_locals;
    cur_frame->index = cur_index;
    cur_frame->outreg = outreg;
    cur_frame++;
    cur_index = next_func;
    cur_frame->locals = cur_locals + vm_read_ahead(-1);
    vm_fetch;
    run_next_op;
}
do_dynamic_call:
{
    vm_reg_t outreg = vm_read_reg;
    vm_reg_t funcreg = vm_read;
    int nargs = vm_read;
    vm_obj_t *next_locals = cur_frame->locals;
    for (int argno = 1; argno <= nargs; argno++)
    {
        vm_reg_t regno = vm_read_reg;
        next_locals[argno] = cur_locals[regno];
    }
    vm_obj_t next_func = cur_locals[funcreg];
    cur_locals = next_locals;
    cur_frame->index = cur_index;
    cur_frame->outreg = outreg;
    cur_frame++;
    cur_index = (vm_loc_t) vm_obj_to_int(next_func);
    cur_frame->locals = cur_locals + vm_read_ahead(-1);
    vm_fetch;
    run_next_op;
}
do_store_none:
{
    vm_reg_t to = vm_read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_none();
    run_next_op;
}
do_store_bool:
{
    vm_reg_t to = vm_read_reg;
    int from = (int)vm_read;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool((bool)from);
    run_next_op;
}
do_store_reg:
{
    vm_reg_t to = vm_read_reg;
    vm_reg_t from = vm_read_reg;
    vm_fetch;
    cur_locals[to] = cur_locals[from];
    run_next_op;
}
do_store_int:
{
    vm_reg_t to = vm_read_reg;
    int from = vm_read;
    vm_fetch;
    cur_locals[to] = vm_obj_of_int(from);
    run_next_op;
}
do_equal:
{
    vm_reg_t to = vm_read_reg;
    vm_reg_t lhs = vm_read_reg;
    vm_reg_t rhs = vm_read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool(vm_obj_eq(cur_locals[lhs], cur_locals[rhs]));
    run_next_op;
}
do_not_equal:
{
    vm_reg_t to = vm_read_reg;
    vm_reg_t lhs = vm_read_reg;
    vm_reg_t rhs = vm_read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool(vm_obj_neq(cur_locals[lhs], cur_locals[rhs]));
    run_next_op;
}
do_less:
{
    vm_reg_t to = vm_read_reg;
    vm_reg_t lhs = vm_read_reg;
    vm_reg_t rhs = vm_read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool(vm_obj_lt(cur_locals[lhs], cur_locals[rhs]));
    run_next_op;
}
do_greater:
{
    vm_reg_t to = vm_read_reg;
    vm_reg_t lhs = vm_read_reg;
    vm_reg_t rhs = vm_read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool(vm_obj_gt(cur_locals[lhs], cur_locals[rhs]));
    run_next_op;
}
do_less_than_equal:
{
    vm_reg_t to = vm_read_reg;
    vm_reg_t lhs = vm_read_reg;
    vm_reg_t rhs = vm_read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool(vm_obj_lte(cur_locals[lhs], cur_locals[rhs]));
    run_next_op;
}
do_greater_than_equal:
{
    vm_reg_t to = vm_read_reg;
    vm_reg_t lhs = vm_read_reg;
    vm_reg_t rhs = vm_read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool(vm_obj_gte(cur_locals[lhs], cur_locals[rhs]));
    run_next_op;
}
do_jump:
{
    vm_loc_t to = vm_read;
    cur_index = to;
    vm_fetch;
    run_next_op;
}
do_branch_true:
{
    vm_loc_t to1 = vm_read;
    vm_loc_t to2 = vm_read;
    vm_reg_t from = vm_read_reg;
    if (vm_obj_to_bool(cur_locals[from]))
    {
        cur_index = to1;
    }
    else
    {
        cur_index = to2;
    }
    vm_fetch;
    run_next_op;
}
do_add:
{
    vm_reg_t to = vm_read_reg;
    vm_reg_t lhs = vm_read_reg;
    vm_reg_t rhs = vm_read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_num_add(cur_locals[lhs], cur_locals[rhs]);
    run_next_op;
}
do_mul:
{
    vm_reg_t to = vm_read_reg;
    vm_reg_t lhs = vm_read_reg;
    vm_reg_t rhs = vm_read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_num_mul(cur_locals[lhs], cur_locals[rhs]);
    run_next_op;
}
do_sub:
{
    vm_reg_t to = vm_read_reg;
    vm_reg_t lhs = vm_read_reg;
    vm_reg_t rhs = vm_read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_num_sub(cur_locals[lhs], cur_locals[rhs]);
    run_next_op;
}
do_div:
{
    vm_reg_t to = vm_read_reg;
    vm_reg_t lhs = vm_read_reg;
    vm_reg_t rhs = vm_read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_num_div(cur_locals[lhs], cur_locals[rhs]);
    run_next_op;
}
do_mod:
{
    vm_reg_t to = vm_read_reg;
    vm_reg_t lhs = vm_read_reg;
    vm_reg_t rhs = vm_read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_num_mod(cur_locals[lhs], cur_locals[rhs]);
    run_next_op;
}
do_concat:
{
    vm_gc_run1(gc, locals_base, cur_frame->locals);
    vm_reg_t to = vm_read_reg;
    vm_reg_t lhs = vm_read_reg;
    vm_reg_t rhs = vm_read_reg;
    vm_fetch;
    vm_obj_t o1 = cur_locals[lhs];
    vm_obj_t o2 = cur_locals[rhs];
    cur_locals[to] = vm_gc_concat(gc, o1, o2);
    run_next_op;
}
do_static_concat:
{
    vm_gc_run1(gc, locals_base, cur_frame->locals);
    vm_reg_t to = vm_read_reg;
    vm_reg_t lhs = vm_read_reg;
    vm_reg_t rhs = vm_read_reg;
    vm_fetch;
    vm_obj_t o1 = cur_locals[lhs];
    vm_obj_t o2 = cur_locals[rhs];
    cur_locals[to] = vm_gc_static_concat(gc, o1, o2);
    run_next_op;
}
do_putchar:
{
    vm_reg_t from = vm_read_reg;
    vm_fetch;
    int val = vm_obj_to_int(cur_locals[from]);
    state->putchar(state, val);
    run_next_op;
}
}