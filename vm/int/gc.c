#include "gc.h"
#include "../gc.h"

void vm_gc_init(vm_gc_t *restrict out)
{
    out->num_buf = NULL;
    out->num_marks = NULL;
    out->num_used = 0;
    out->num_alloc = 0;

    out->str_root = 0;
    out->str_buf = NULL;
    out->str_lens = NULL;
    out->str_marks = NULL;
    out->str_used = 0;
    out->str_alloc = 0;

    out->arr_buf = NULL;
    out->arr_lens = NULL;
    out->arr_marks = NULL;
    out->arr_used = 0;
    out->arr_alloc = 0;

    out->move_buf = 0;
    out->move_alloc = 0;

    out->count = 0;
    out->max = VM_GC_MIN;
}

void vm_gc_stop(vm_gc_t gc)
{
    // TODO: free memory
}

mpz_ptr vm_gc_num_get(vm_gc_t *restrict gc, uint32_t ptr)
{
    return &gc->num_buf[ptr];
}

void vm_gc_mark(vm_gc_t *restrict gc, vm_value_t val)
{
    if (VM_VALUE_IS_BIG(val))
    {
        uint32_t nth = VM_VALUE_GET_INT(val);
        if (nth < gc->num_alloc)
        {
            gc->num_marks[nth] = 1;
        }
    }
    if (VM_VALUE_IS_STR(val))
    {
        uint32_t nth = VM_VALUE_GET_INT(val);
        if (nth < gc->str_alloc)
        {
            gc->str_marks[nth] = 1;
        }
    }
    if (VM_VALUE_IS_ARR(val))
    {
        uint32_t nth = VM_VALUE_GET_INT(val);
        if (nth < gc->str_alloc)
        {
            gc->arr_marks[nth] = 1;
        }
    }
}

void vm_gc_run(vm_gc_t *restrict gc, size_t nregs, void *vregs)
{
    gc->running = true;
    vm_value_t *regs = vregs;
    for (uint32_t i = 0; i < nregs; i++)
    {
        vm_value_t reg = regs[i];
    }
    {
        if (gc->num_alloc > gc->move_alloc)
        {
            gc->move_alloc = gc->num_alloc * 2;
            gc->move_buf = vm_realloc(gc->move_buf, gc->move_alloc);
        }
        uint32_t num_used = 0;
        for (uint32_t i = 0; i < gc->num_alloc; i++)
        {
            if (gc->num_marks[i] != 0)
            {
                gc->num_marks[i] = 0;
                if (i != num_used)
                {
                    MP_INT tmp = gc->num_buf[num_used];
                    gc->num_buf[num_used] = gc->num_buf[i];
                    gc->num_buf[i] = tmp;
                }
                gc->move_buf[i] = num_used;
                num_used += 1;
            }
        }
        for (uint32_t i = 0; i < nregs; i++)
        {
            vm_value_t reg = regs[i];
            if (VM_VALUE_IS_BIG(reg))
            {
                uint32_t nth = VM_VALUE_GET_INT(reg);
                if (nth < gc->num_alloc)
                {
                    regs[i] = VM_VALUE_SET_BIG(gc->move_buf[nth]);
                }
            }
        }
        gc->num_used = num_used;
    }
    {
        if (gc->str_alloc > gc->move_alloc)
        {
            gc->move_alloc = gc->str_alloc * 2;
            gc->move_buf = vm_realloc(gc->move_buf, gc->move_alloc);
        }
        uint32_t str_used = 0;
        for (uint32_t i = 0; i < gc->str_alloc; i++)
        {
            if (gc->str_marks[i] != 0 || i < gc->str_root)
            {
                gc->str_marks[i] = 0;
                if (i != str_used)
                {
                    {
                        char *tmp = gc->str_buf[str_used];
                        gc->str_buf[str_used] = gc->str_buf[i];
                        gc->str_buf[i] = tmp;
                    }
                    {
                        uint32_t tmp = gc->str_lens[str_used];
                        gc->str_lens[str_used] = gc->str_lens[i];
                        gc->str_lens[i] = tmp;
                    }
                }
                gc->move_buf[i] = str_used;
                str_used += 1;
            }
        }
        for (uint32_t i = 0; i < nregs; i++)
        {
            vm_value_t reg = regs[i];
            if (VM_VALUE_IS_STR(reg))
            {
                uint32_t nth = VM_VALUE_GET_INT(reg);
                if (nth < gc->str_alloc)
                {
                    regs[i] = VM_VALUE_SET_STR(gc->move_buf[nth]);
                }
            }
        }
        gc->str_used = str_used;
    }
    gc->count = 0;
    gc->max = gc->num_alloc - (gc->num_alloc >> 3);
    if (gc->max < VM_GC_MIN)
    {
        gc->max = VM_GC_MIN;
    }
}

uint32_t vm_gc_num(vm_gc_t *restrict gc)
{
    uint32_t next_head = gc->num_used + 1;
    if (gc->num_alloc <= next_head)
    {
        uint32_t next_alloc = next_head * 2;
        gc->num_buf = vm_realloc(gc->num_buf, sizeof(MP_INT) * next_alloc);
        gc->num_marks = vm_realloc(gc->num_marks, sizeof(uint8_t) * next_alloc);
        for (uint32_t i = gc->num_alloc; i < next_alloc; i++)
        {
            mpz_init(&gc->num_buf[i]);
            gc->num_marks[i] = 0;
        }
        gc->num_alloc = next_alloc;
    }
    return gc->num_used++;
}

uint32_t vm_gc_str(vm_gc_t *restrict gc, size_t size)
{
    uint32_t next_head = gc->str_used + 1;
    if (gc->str_alloc <= next_head)
    {
        uint32_t next_alloc = next_head * 2;
        gc->str_buf = vm_realloc(gc->str_buf, sizeof(char *) * next_alloc);
        gc->str_lens = vm_realloc(gc->str_lens, sizeof(uint32_t) * next_alloc);
        gc->str_marks = vm_realloc(gc->str_marks, sizeof(uint8_t) * next_alloc);
        for (uint32_t i = gc->str_alloc; i < next_alloc; i++)
        {
            gc->str_buf[i] = NULL;
            gc->str_marks[i] = 0;
        }
        gc->str_alloc = next_alloc;
    }
    uint32_t ret = gc->str_used;
    gc->str_lens[ret] = (uint32_t) size;
    gc->str_buf[ret] = vm_malloc(sizeof(char) * size);
    gc->str_used = next_head;
    if (!gc->running) {
        gc->str_root++;
    }
    return ret;
}

uint32_t vm_gc_len(vm_gc_t *restrict gc, vm_value_t val)
{
    if (VM_VALUE_IS_STR(val))
    {
        return gc->str_lens[VM_VALUE_GET_INT(val)];
    }
    __builtin_trap();
} 

uint32_t vm_gc_arr(vm_gc_t *restrict gc, size_t size)
{
    __builtin_trap();
}

void vm_gc_set_char(vm_gc_t *restrict gc, uint32_t ptr, vm_int_t index, char chr)
{
    gc->str_buf[ptr][index] = chr;
}

vm_value_t vm_gc_get_v(vm_gc_t *restrict gc, vm_value_t ptr, vm_value_t index)
{
    if (VM_VALUE_IS_STR(ptr))
    {
        return vm_value_from_int(gc, gc->str_buf[VM_VALUE_GET_INT(ptr)][vm_value_to_int(gc, index)]);
    }
    __builtin_trap();
}

vm_value_t vm_gc_get_i(vm_gc_t *restrict gc, vm_value_t ptr, vm_int_t index)
{
    if (VM_VALUE_IS_STR(ptr))
    {
        return vm_value_from_int(gc, gc->str_buf[VM_VALUE_GET_INT(ptr)][index]);
    }
    __builtin_trap();
}

void vm_gc_set_vv(vm_gc_t *restrict gc, vm_value_t obj, vm_value_t ptr, vm_value_t value)
{
    __builtin_trap();
}

void vm_gc_set_vi(vm_gc_t *restrict gc, vm_value_t obj, vm_value_t ptr, vm_int_t value)
{
    __builtin_trap();
}

void vm_gc_set_iv(vm_gc_t *restrict gc, vm_value_t obj, vm_int_t ptr, vm_value_t value)
{
    __builtin_trap();
}

void vm_gc_set_ii(vm_gc_t *restrict gc, vm_value_t obj, vm_int_t ptr, vm_int_t value)
{
    __builtin_trap();
}