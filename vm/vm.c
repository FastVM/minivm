#include "vm.h"
#include "gc.h"
#include "math.h"
#include "obj.h"
#include "effect.h"
#include "obj/map.h"

#define VM_GLOBALS_NUM (255)

#if defined(VM_DEBUG_OPCODE)
#define run_next_op                                                   \
    printf("(%i -> %i)\n", (int)cur_index, (int)basefunc[cur_index]); \
    goto *next_op;
#else
#define run_next_op \
    goto *next_op;
#endif

typedef struct
{
    vm_obj_t handler;
    int depth;
    int max;
} find_handler_pair_t;

static inline int find_handler_worker(void *state, vm_obj_t key, vm_obj_t val)
{
    find_handler_pair_t *pair = state;
    int ikey = vm_obj_to_int(key);
    if (ikey > pair->max)
    {
        return 0;
    }
    if (ikey >= pair->depth)
    {
        pair->handler = val;
        pair->depth = ikey;
    }
    return 0;
}

static inline find_handler_pair_t find_handler(vm_gc_entry_t *handlers, vm_obj_t effect, size_t max_level)
{
    vm_obj_t res1 = vm_gc_get_index(handlers, effect);
    vm_gc_entry_map_t *map = vm_obj_to_ptr(res1);
    find_handler_pair_t pair = (find_handler_pair_t){
        .max = max_level,
        .depth = -1,
    };
    vm_map_for_pairs(map->map, &pair, &find_handler_worker);
    return pair;
}

#define run_next_op_after_effect(outreg_, effect)                                                \
    ({                                                                                           \
        vm_reg_t copy_outreg = outreg_;                                                          \
        vm_obj_t copy_effect = effect;                                                           \
        vm_obj_t *next_locals = cur_frame->locals;                                               \
        find_handler_pair_t pair = find_handler(handlers, copy_effect, cur_frame - frames_base); \
        if (pair.depth == -1)                                                                    \
        {                                                                                        \
            __builtin_trap();                                                                    \
        }                                                                                        \
        vm_obj_t funcv = pair.handler;                                                           \
        int level = pair.depth;                                                                  \
        for (int i = 0; vm_obj_is_ptr(funcv); i++)                                               \
        {                                                                                        \
            next_locals[i] = funcv;                                                              \
            funcv = vm_gc_get_index(vm_obj_to_ptr(funcv), vm_obj_of_int(0));                     \
        }                                                                                        \
        vm_loc_t next_func = vm_obj_to_fun(funcv);                                               \
        cur_locals = next_locals;                                                                \
        cur_frame->index = cur_index;                                                            \
        cur_frame->func = cur_func;                                                              \
        cur_frame->outreg = copy_outreg;                                                         \
        cur_frame++;                                                                             \
        cur_index = next_func;                                                                   \
        cur_func = next_func;                                                                    \
        cur_frame->locals = cur_locals + get_byte(-1);                                           \
        cur_effect->resume = cur_frame - frames_base;                                            \
        cur_effect->exit = level;                                                                \
        cur_effect++;                                                                            \
        vm_fetch;                                                                                \
    });                                                                                          \
    run_next_op;

#define gc_once ({                                           \
    if (raw_gc.len >= raw_gc.max)                            \
    {                                                        \
        vm_gc_run1(&raw_gc, locals_base, cur_frame->locals); \
    }                                                        \
})

#define gc_new(TYPE, ...) ({                                 \
    if (raw_gc.len >= raw_gc.max)                            \
    {                                                        \
        vm_gc_run1(&raw_gc, locals_base, cur_frame->locals); \
    }                                                        \
    vm_gc_##TYPE##_new(__VA_ARGS__);                         \
})

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

void vm_run(size_t len, const vm_opcode_t *basefunc, size_t start)
{
    vm_gc_t raw_gc;
    vm_gc_start(&raw_gc);

    vm_stack_frame_t *frames_base = vm_mem_grow(VM_FRAMES_UNITS * sizeof(vm_stack_frame_t));
    vm_obj_t *locals_base = vm_mem_grow(VM_LOCALS_UNITS * sizeof(vm_obj_t));
    vm_handler_frame_t *effects_base = vm_mem_grow(256 * sizeof(vm_stack_frame_t));

    vm_stack_frame_t *cur_frame = frames_base;
    vm_obj_t *cur_locals = locals_base;
    vm_handler_frame_t *cur_effect = effects_base;

    vm_loc_t cur_index = start;
    vm_loc_t cur_func = start;

    vm_gc_entry_t *handlers = gc_new(map, &raw_gc);
    // vm_gc_entry_t *levels = gc_new(map, &raw_gc);
    cur_locals[0] = vm_obj_of_ptr(handlers);
    cur_locals += 1;
    // cur_locals[1] = vm_obj_of_ptr(levels);
    // cur_locals += 1;

    void *next_op_value;
    void *ptrs[VM_OPCODE_MAX2P] = {};
    ptrs[VM_OPCODE_EXIT] = &&do_exit;
    ptrs[VM_OPCODE_STORE_REG] = &&do_store_reg;
    ptrs[VM_OPCODE_STORE_NONE] = &&do_store_none;
    ptrs[VM_OPCODE_STORE_BOOL] = &&do_store_bool;
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
    ptrs[VM_OPCODE_REF_NEW] = &&do_ref_new;
    ptrs[VM_OPCODE_BOX_NEW] = &&do_box_new;
    ptrs[VM_OPCODE_STRING_NEW] = &&do_string_new;
    ptrs[VM_OPCODE_ARRAY_NEW] = &&do_array_new;
    ptrs[VM_OPCODE_MAP_NEW] = &&do_map_new;
    ptrs[VM_OPCODE_REF_GET] = &&do_ref_get;
    ptrs[VM_OPCODE_BOX_GET] = &&do_get_box;
    ptrs[VM_OPCODE_BOX_SET] = &&do_set_box;
    ptrs[VM_OPCODE_LENGTH] = &&do_length;
    ptrs[VM_OPCODE_INDEX_GET] = &&do_index_get;
    ptrs[VM_OPCODE_INDEX_SET] = &&do_index_set;
    ptrs[VM_OPCODE_SET_HANDLER] = &&do_set_handler;
    ptrs[VM_OPCODE_CALL_HANDLER] = &&do_call_handler;
    ptrs[VM_OPCODE_RETURN_HANDLER] = &&do_return_handler;
    ptrs[VM_OPCODE_EXIT_HANDLER] = &&do_exit_handler;
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
    vm_mem_reset(effects_base);
    return;
}
do_set_handler:
{
    vm_reg_t sym = read_reg;
    vm_reg_t handler = read_reg;
    vm_fetch;
    vm_obj_t xmap = vm_gc_get_index(handlers, cur_locals[sym]);
    if (vm_obj_is_dead(xmap))
    {
        xmap = vm_obj_of_ptr(gc_new(map, &raw_gc));
        vm_gc_set_index(handlers, cur_locals[sym], xmap);
    }
    vm_obj_t my_frame = vm_obj_of_int(cur_frame - frames_base);
    vm_gc_set_index(vm_obj_to_ptr(xmap), my_frame, cur_locals[handler]);
    run_next_op;
}
do_call_handler:
{
    vm_reg_t outreg = read_reg;
    vm_reg_t effect = read_reg;
    run_next_op_after_effect(outreg, cur_locals[effect]);
}
do_return_handler:
{
    vm_reg_t from = read_reg;
    cur_effect--;
    cur_frame = frames_base + cur_effect->resume;
    vm_obj_t val = cur_locals[from];
    cur_frame--;
    cur_locals = (cur_frame - 1)->locals;
    cur_func = cur_frame->func;
    cur_index = cur_frame->index;
    vm_reg_t outreg = cur_frame->outreg;
    cur_locals[outreg] = val;
    vm_fetch;
    run_next_op;
}
do_exit_handler:
{
    vm_reg_t from = read_reg;
    cur_effect--;
    cur_frame = 1 + frames_base + cur_effect->exit;
    vm_obj_t val = cur_locals[from];
    cur_frame--;
    cur_locals = (cur_frame - 1)->locals;
    cur_func = cur_frame->func;
    cur_index = cur_frame->index;
    vm_reg_t outreg = cur_frame->outreg;
    cur_locals[outreg] = val;
    vm_fetch;
    run_next_op;
}
do_return:
{
    vm_reg_t from = read_reg;
    vm_obj_t val = cur_locals[from];
    cur_frame--;
    cur_locals = (cur_frame - 1)->locals;
    cur_func = cur_frame->func;
    cur_index = cur_frame->index;
    vm_reg_t outreg = cur_frame->outreg;
    cur_locals[outreg] = val;
    vm_fetch;
    run_next_op;
}
do_type:
{
    vm_reg_t outreg = read_reg;
    vm_reg_t valreg = read_reg;
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
    vm_reg_t outreg = read_reg;
    int nargs = read_byte;
    vm_gc_entry_t *str = gc_new(string, &raw_gc, nargs);
    for (size_t i = 0; i < nargs; i++)
    {
        vm_gc_set_index(str, vm_obj_of_int(i), vm_obj_of_int(read_byte));
    }
    vm_fetch;
    cur_locals[outreg] = vm_obj_of_ptr(str);
    run_next_op;
}
do_array_new:
{
    vm_reg_t outreg = read_reg;
    int nargs = read_byte;
    vm_gc_entry_t *vec = gc_new(array, &raw_gc, nargs);
    for (int i = 0; i < nargs; i++)
    {
        vm_reg_t vreg = read_reg;
        vm_gc_set_index(vec, vm_obj_of_int(i), cur_locals[vreg]);
    }
    vm_fetch;
    cur_locals[outreg] = vm_obj_of_ptr(vec);
    run_next_op;
}
do_ref_new:
{
    vm_reg_t outreg = read_reg;
    vm_reg_t inreg = read_reg;
    vm_fetch;
    // printf("%p\n", &cur_locals[inreg] - locals_base);
    // printf("%lf\n", vm_obj_to_num(cur_locals[inreg]));
    vm_gc_entry_t *box = gc_new(ref, &raw_gc, &cur_locals[inreg]);
    cur_locals[outreg] = vm_obj_of_ptr(box);
    run_next_op;
}
do_box_new:
{
    vm_reg_t outreg = read_reg;
    vm_reg_t inreg = read_reg;
    vm_fetch;
    vm_gc_entry_t *box = gc_new(box, &raw_gc);
    vm_gc_set_box(box, cur_locals[inreg]);
    cur_locals[outreg] = vm_obj_of_ptr(box);
    run_next_op;
}
do_map_new:
{
    vm_reg_t outreg = read_reg;
    vm_fetch;
    vm_gc_entry_t *map = gc_new(map, &raw_gc);
    cur_locals[outreg] = vm_obj_of_ptr(map);
    run_next_op;
}
do_set_box:
{
    vm_reg_t targetreg = read_reg;
    vm_reg_t valreg = read_reg;
    vm_fetch;
    vm_gc_set_box(vm_obj_to_ptr(cur_locals[targetreg]), cur_locals[valreg]);
    run_next_op;
}
do_ref_get:
{
    vm_reg_t outreg = read_reg;
    vm_reg_t inreg = read_reg;
    vm_fetch;
    vm_obj_t *ref = vm_gc_get_ref(vm_obj_to_ptr(cur_locals[inreg]));
    // printf("%li\n", ref - locals_base);
    // printf("%lf\n", vm_obj_to_num(cur_locals[outreg]));
    cur_locals[outreg] = *ref;
    run_next_op;
}
do_get_box:
{
    vm_reg_t outreg = read_reg;
    vm_reg_t inreg = read_reg;
    vm_fetch;
    cur_locals[outreg] = vm_gc_get_box(vm_obj_to_ptr(cur_locals[inreg]));
    run_next_op;
}
do_length:
{
    vm_reg_t outreg = read_reg;
    vm_reg_t reg = read_reg;
    vm_fetch;
    vm_obj_t vec = cur_locals[reg];
    cur_locals[outreg] = vm_gc_sizeof(vm_obj_to_ptr(vec));
    run_next_op;
}
do_index_get:
{
    vm_reg_t outreg = read_reg;
    vm_reg_t reg = read_reg;
    vm_reg_t ind = read_reg;
    vm_fetch;
    vm_obj_t vec = cur_locals[reg];
    vm_obj_t index = cur_locals[ind];
    cur_locals[outreg] = vm_gc_get_index(vm_obj_to_ptr(vec), index);
    if (vm_obj_is_dead(cur_locals[outreg]))
    {
        run_next_op_after_effect(outreg, vm_obj_of_num(VM_EFFECT_BOUNDS));
    }
    run_next_op;
}
do_index_set:
{
    vm_reg_t reg = read_reg;
    vm_reg_t ind = read_reg;
    vm_reg_t val = read_reg;
    vm_fetch;
    vm_obj_t vec = cur_locals[reg];
    vm_obj_t index = cur_locals[ind];
    vm_obj_t value = cur_locals[val];
    vm_gc_set_index(vm_obj_to_ptr(vec), index, value);
    run_next_op;
}
do_call0:
{
    vm_reg_t outreg = read_reg;
    vm_reg_t func = read_reg;
    vm_obj_t *next_locals = cur_frame->locals;
    vm_obj_t funcv = cur_locals[func];
    if (vm_obj_is_ptr(funcv))
    {
        next_locals[0] = funcv;
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
    vm_reg_t outreg = read_reg;
    vm_reg_t func = read_reg;
    vm_obj_t *next_locals = cur_frame->locals;
    next_locals[1] = cur_locals[read_reg];
    vm_obj_t funcv = cur_locals[func];
    if (vm_obj_is_ptr(funcv))
    {
        next_locals[0] = funcv;
        funcv = vm_gc_get_index(vm_obj_to_ptr(funcv), vm_obj_of_int(0));
    }
    else
    {
        next_locals[0] = funcv;
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
    vm_reg_t outreg = read_reg;
    vm_reg_t func = read_reg;
    vm_obj_t *next_locals = cur_frame->locals;
    next_locals[1] = cur_locals[read_reg];
    next_locals[2] = cur_locals[read_reg];
    vm_obj_t funcv = cur_locals[func];
    if (vm_obj_is_ptr(funcv))
    {
        next_locals[0] = funcv;
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
    vm_reg_t outreg = read_reg;
    vm_reg_t func = read_reg;
    int nargs = read_byte;
    vm_obj_t *next_locals = cur_frame->locals;
    for (int argno = 1; argno <= nargs; argno++)
    {
        vm_reg_t regno = read_reg;
        next_locals[argno] = cur_locals[regno];
    }
    vm_obj_t funcv = cur_locals[func];
    if (vm_obj_is_ptr(funcv))
    {
        next_locals[0] = funcv;
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
    vm_reg_t outreg = read_reg;
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
    vm_reg_t outreg = read_reg;
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
    vm_reg_t outreg = read_reg;
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
    vm_reg_t outreg = read_reg;
    int next_func = read_loc;
    int nargs = read_byte;
    vm_obj_t *next_locals = cur_frame->locals;
    for (int argno = 0; argno < nargs; argno++)
    {
        vm_reg_t regno = read_reg;
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
    vm_reg_t outreg = read_reg;
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
    vm_reg_t outreg = read_reg;
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
    vm_reg_t outreg = read_reg;
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
    vm_reg_t outreg = read_reg;
    int nargs = read_byte;
    vm_obj_t *next_locals = cur_frame->locals;
    for (int argno = 0; argno < nargs; argno++)
    {
        vm_reg_t regno = read_reg;
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
do_store_none:
{
    vm_reg_t to = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_none();
    run_next_op;
}
do_store_bool:
{
    vm_reg_t to = read_reg;
    int from = (int)read_byte;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool((bool)from);
    run_next_op;
}
do_store_reg:
{
    vm_reg_t to = read_reg;
    vm_reg_t from = read_reg;
    vm_fetch;
    cur_locals[to] = cur_locals[from];
    run_next_op;
}
do_store_byte:
{
    vm_reg_t to = read_reg;
    int from = (int)read_byte;
    vm_fetch;
    cur_locals[to] = vm_obj_of_int((int)from);
    run_next_op;
}
do_store_int:
{
    vm_reg_t to = read_reg;
    int from = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_of_int(from);
    run_next_op;
}
do_store_fun:
{
    vm_reg_t to = read_reg;
    vm_loc_t func_end = read_loc;
    vm_loc_t head = cur_index;
    cur_index = func_end;
    vm_fetch;
    cur_locals[to] = vm_obj_of_fun(head + 1);
    run_next_op;
}
do_equal:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    vm_reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool(vm_obj_eq(cur_locals[lhs], cur_locals[rhs]));
    run_next_op;
}
do_equal_num:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool(vm_obj_ieq(cur_locals[lhs], rhs));
    run_next_op;
}
do_not_equal:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    vm_reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool(vm_obj_neq(cur_locals[lhs], cur_locals[rhs]));
    run_next_op;
}
do_not_equal_num:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool(vm_obj_ineq(cur_locals[lhs], rhs));
    run_next_op;
}
do_less:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    vm_reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool(vm_obj_lt(cur_locals[lhs], cur_locals[rhs]));
    run_next_op;
}
do_less_num:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool(vm_obj_ilt(cur_locals[lhs], rhs));
    run_next_op;
}
do_greater:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    vm_reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool(vm_obj_gt(cur_locals[lhs], cur_locals[rhs]));
    run_next_op;
}
do_greater_num:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool(vm_obj_igt(cur_locals[lhs], rhs));
    run_next_op;
}
do_less_than_equal:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    vm_reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool(vm_obj_lte(cur_locals[lhs], cur_locals[rhs]));
    run_next_op;
}
do_less_than_equal_num:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool(vm_obj_ilte(cur_locals[lhs], rhs));
    run_next_op;
}
do_greater_than_equal:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    vm_reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool(vm_obj_gte(cur_locals[lhs], cur_locals[rhs]));
    run_next_op;
}
do_greater_than_equal_num:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_of_bool(vm_obj_igte(cur_locals[lhs], rhs));
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
    vm_reg_t from = read_reg;
    if (!vm_obj_to_bool(cur_locals[from]))
    {
        cur_index = to;
    }
    vm_fetch;
    run_next_op;
}
do_jump_if_true:
{
    vm_loc_t to = read_loc;
    vm_reg_t from = read_reg;
    if (vm_obj_to_bool(cur_locals[from]))
    {
        cur_index = to;
    }
    vm_fetch;
    run_next_op;
}
do_jump_if_equal:
{
    vm_loc_t to = read_loc;
    vm_reg_t lhs = read_reg;
    vm_reg_t rhs = read_reg;
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
    vm_reg_t lhs = read_reg;
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
    vm_reg_t lhs = read_reg;
    vm_reg_t rhs = read_reg;
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
    vm_reg_t lhs = read_reg;
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
    vm_reg_t lhs = read_reg;
    vm_reg_t rhs = read_reg;
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
    vm_reg_t lhs = read_reg;
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
    vm_reg_t lhs = read_reg;
    vm_reg_t rhs = read_reg;
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
    vm_reg_t lhs = read_reg;
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
    vm_reg_t lhs = read_reg;
    vm_reg_t rhs = read_reg;
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
    vm_reg_t lhs = read_reg;
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
    vm_reg_t lhs = read_reg;
    vm_reg_t rhs = read_reg;
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
    vm_reg_t lhs = read_reg;
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
    vm_reg_t target = read_reg;
    vm_reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[target] = vm_obj_num_add(cur_locals[target], cur_locals[rhs]);
    run_next_op;
}
do_inc_num:
{
    vm_reg_t target = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[target] = vm_obj_num_addc(cur_locals[target], rhs);
    run_next_op;
}
do_dec:
{
    vm_reg_t target = read_reg;
    vm_reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[target] = vm_obj_num_sub(cur_locals[target], cur_locals[rhs]);
    run_next_op;
}
do_dec_num:
{
    vm_reg_t target = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[target] = vm_obj_num_subc(cur_locals[target], rhs);
    run_next_op;
}
do_add:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    vm_reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_num_add(cur_locals[lhs], cur_locals[rhs]);
    run_next_op;
}
do_add_num:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_num_addc(cur_locals[lhs], rhs);
    run_next_op;
}
do_mul:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    vm_reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_num_mul(cur_locals[lhs], cur_locals[rhs]);
    run_next_op;
}
do_mul_num:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_num_mulc(cur_locals[lhs], rhs);
    run_next_op;
}
do_sub:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    vm_reg_t rhs = read_reg;
    vm_fetch;
    cur_locals[to] = vm_obj_num_sub(cur_locals[lhs], cur_locals[rhs]);
    run_next_op;
}
do_sub_num:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    cur_locals[to] = vm_obj_num_subc(cur_locals[lhs], rhs);
    run_next_op;
}
do_div:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    vm_reg_t rhs = read_reg;
    vm_fetch;
    if (vm_obj_to_num(cur_locals[rhs]) == 0)
    {
        run_next_op_after_effect(to, vm_obj_of_num(VM_EFFECT_MATH_DIV));
    }
    cur_locals[to] = vm_obj_num_div(cur_locals[lhs], cur_locals[rhs]);
    run_next_op;
}
do_div_num:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    if (rhs == 0)
    {
        run_next_op_after_effect(to, vm_obj_of_num(VM_EFFECT_MATH_DIV));
    }
    cur_locals[to] = vm_obj_num_divc(cur_locals[lhs], rhs);
    run_next_op;
}
do_mod:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    vm_reg_t rhs = read_reg;
    vm_fetch;
    if (vm_obj_to_num(cur_locals[rhs]) == 0)
    {
        run_next_op_after_effect(to, vm_obj_of_num(VM_EFFECT_MATH_MOD));
    }
    cur_locals[to] = vm_obj_num_mod(cur_locals[lhs], cur_locals[rhs]);
    run_next_op;
}
do_mod_num:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    int rhs = read_int;
    vm_fetch;
    if (rhs == 0)
    {
        run_next_op_after_effect(to, vm_obj_of_num(VM_EFFECT_MATH_MOD));
    }
    cur_locals[to] = vm_obj_num_modc(cur_locals[lhs], rhs);
    run_next_op;
}
do_concat:
{
    vm_reg_t to = read_reg;
    vm_reg_t lhs = read_reg;
    vm_reg_t rhs = read_reg;
    vm_fetch;
    gc_once;
    cur_locals[to] = vm_gc_concat(&raw_gc, cur_locals[lhs], cur_locals[rhs]);
    if (vm_obj_is_dead(cur_locals[to]))
    {
        run_next_op_after_effect(to, vm_obj_of_num(VM_EFFECT_TYPE_CONCAT));
    }
    run_next_op;
}
do_putchar:
{
    vm_reg_t from = read_reg;
    vm_fetch;
    int val = vm_obj_to_int(cur_locals[from]);
    vm_putchar(val);
    run_next_op;
}
}