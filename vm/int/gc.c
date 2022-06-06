#include "gc.h"

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
    out->running = false;
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
        if (nth < gc->num_used)
        {
            gc->num_marks[nth] = 1;
        }
    }
    if (VM_VALUE_IS_STR(val))
    {
        uint32_t nth = VM_VALUE_GET_INT(val);
        if (nth < gc->str_used)
        {
            gc->str_marks[nth] = 1;
        }
    }
    if (VM_VALUE_IS_ARR(val))
    {
        uint32_t nth = VM_VALUE_GET_INT(val);
        if (nth < gc->arr_used)
        {
            if (gc->arr_marks[nth] == 0)
            {
                gc->arr_marks[nth] = 1;
                uint32_t len = gc->arr_lens[nth];
                for (uint32_t i = 0; i < len; i++)
                {
                    vm_gc_mark(gc, gc->arr_buf[nth][i]);
                }
            }
        }
    }
}

void vm_gc_move_big(vm_gc_t *restrict gc, vm_value_t *set)
{
    if (VM_VALUE_IS_BIG(*set))
    {
        uint32_t nth = VM_VALUE_GET_INT(*set);
        if (nth < gc->num_used && gc->num_marks[nth] != 0) {
            gc->num_marks[nth] = 0;
            uint32_t newpos = gc->move_buf[nth];
            *set = VM_VALUE_SET_ARR(newpos);
            MP_INT tmp = gc->num_buf[newpos];
            gc->num_buf[newpos] = gc->num_buf[nth];
            gc->num_buf[nth] = tmp;
        }
    }
    if (VM_VALUE_IS_ARR(*set))
    {
        uint32_t nth = VM_VALUE_GET_INT(*set);
        if (nth < gc->arr_used && gc->arr_marks[nth] != 0)
        {
            gc->arr_marks[nth] = 0;
            uint32_t len = gc->arr_lens[nth];
            for (uint32_t i = 0; i < len; i++)
            {
                vm_gc_move_big(gc, &gc->arr_buf[nth][i]);
            }
            gc->arr_marks[nth] = 1;
        }
    }
}

void vm_gc_move_str(vm_gc_t *restrict gc, vm_value_t *set)
{
    if (VM_VALUE_IS_STR(*set))
    {
        uint32_t nth = VM_VALUE_GET_INT(*set);
        if (nth < gc->str_used && gc->str_marks[nth] != 0) {
            gc->str_marks[nth] = 0;
            uint32_t newpos = gc->move_buf[nth];
            *set = VM_VALUE_SET_STR(newpos);
            char *tmp = gc->str_buf[newpos];
            gc->str_buf[newpos] = gc->str_buf[nth];
            gc->str_buf[nth] = tmp;
            gc->str_lens[newpos] = gc->str_lens[nth];
        }
    }
    if (VM_VALUE_IS_ARR(*set))
    {
        uint32_t nth = VM_VALUE_GET_INT(*set);
        if (nth < gc->arr_used && gc->arr_marks[nth] != 0)
        {
            gc->arr_marks[nth] = 0;
            uint32_t len = gc->arr_lens[nth];
            for (uint32_t i = 0; i < len; i++)
            {
                vm_gc_move_str(gc, &gc->arr_buf[nth][i]);
            }
            gc->arr_marks[nth] = 1;
        }
    }
}

void vm_gc_move_arr(vm_gc_t *restrict gc, vm_value_t *set)
{
    if (VM_VALUE_IS_ARR(*set))
    {
        uint32_t nth = VM_VALUE_GET_INT(*set);
        if (nth < gc->arr_used && gc->arr_marks[nth] != 0)
        {
            gc->arr_marks[nth] = 0;
            uint32_t len = gc->arr_lens[nth];
            for (uint32_t i = 0; i < len; i++)
            {
                vm_gc_move_str(gc, &gc->arr_buf[nth][i]);
            }
            gc->arr_marks[nth] = 0;
            uint32_t newpos = gc->move_buf[nth];
            *set = VM_VALUE_SET_ARR(newpos);
            vm_value_t *tmp = gc->arr_buf[newpos];
            gc->arr_buf[newpos] = gc->arr_buf[nth];
            gc->arr_buf[nth] = tmp;
            gc->arr_lens[newpos] = gc->arr_lens[nth];
        }
    }
}

void vm_gc_run(vm_gc_t *restrict gc)
{
    if (gc->count++ < gc->max) {
        return;
    }
    for (uint32_t i = 0; i < gc->nregs; i++)
    {
        vm_gc_mark(gc, gc->regs[i]);
    }
    {
        if (gc->num_alloc > gc->move_alloc)
        {
            gc->move_alloc = gc->num_alloc * 2;
            gc->move_buf = vm_realloc(gc->move_buf, sizeof(uint32_t) * gc->move_alloc);
        }
        uint32_t num_used = 0;
        for (uint32_t i = 0; i < gc->num_used; i++)
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
        for (uint32_t i = 0; i < gc->nregs; i++)
        {
            vm_gc_move_big(gc, &gc->regs[i]);
        }
        gc->num_used = num_used;
    }
    {
        if (gc->str_alloc > gc->move_alloc)
        {
            gc->move_alloc = gc->str_alloc * 2;
            gc->move_buf = vm_realloc(gc->move_buf, sizeof(uint32_t) * gc->move_alloc);
        }
        uint32_t str_used = 0;
        for (uint32_t i = 0; i < gc->str_used; i++)
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
        for (uint32_t i = 0; i < gc->nregs; i++)
        {
            vm_gc_move_str(gc, &gc->regs[i]);
        }
        gc->str_used = str_used;
    }
    {
        if (gc->arr_alloc > gc->move_alloc)
        {
            gc->move_alloc = gc->arr_alloc * 2;
            gc->move_buf = vm_realloc(gc->move_buf, sizeof(uint32_t) * gc->move_alloc);
        }
        uint32_t arr_used = 0;
        for (uint32_t i = 0; i < gc->arr_used; i++)
        {
            if (gc->arr_marks[i] != 0)
            {
                gc->arr_marks[i] = 0;
                if (i != arr_used)
                {
                    {
                        vm_value_t *tmp = gc->arr_buf[arr_used];
                        gc->arr_buf[arr_used] = gc->arr_buf[i];
                        gc->arr_buf[i] = tmp;
                    }
                    {
                        uint32_t tmp = gc->arr_lens[arr_used];
                        gc->arr_lens[arr_used] = gc->arr_lens[i];
                        gc->arr_lens[i] = tmp;
                    }
                }
                gc->move_buf[i] = arr_used;
                arr_used += 1;
            }
        }
        for (uint32_t i = 0; i < gc->nregs; i++)
        {
            vm_gc_move_arr(gc, &gc->regs[i]);
        }
        gc->arr_used = arr_used;
    }
    gc->count = 0;
    gc->max = (gc->num_alloc + gc->str_alloc + gc->arr_alloc) >> 2;
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
    gc->str_lens[ret] = (uint32_t)size;
    gc->str_buf[ret] = vm_malloc(sizeof(char) * size);
    gc->str_used = next_head;
    if (!gc->running)
    {
        gc->str_root++;
    }
    return ret;
}

uint32_t vm_gc_arr(vm_gc_t *restrict gc, size_t size)
{
    uint32_t next_head = gc->arr_used + 1;
    if (gc->arr_alloc <= next_head)
    {
        uint32_t next_alloc = next_head * 2;
        gc->arr_buf = vm_realloc(gc->arr_buf, sizeof(vm_value_t *) * next_alloc);
        gc->arr_lens = vm_realloc(gc->arr_lens, sizeof(uint32_t) * next_alloc);
        gc->arr_marks = vm_realloc(gc->arr_marks, sizeof(uint8_t) * next_alloc);
        for (uint32_t i = gc->arr_alloc; i < next_alloc; i++)
        {
            gc->arr_buf[i] = NULL;
            gc->arr_marks[i] = 0;
        }
        gc->arr_alloc = next_alloc;
    }
    uint32_t ret = gc->arr_used;
    gc->arr_lens[ret] = (uint32_t)size;
    gc->arr_buf[ret] = vm_malloc(sizeof(vm_value_t) * size);
    gc->arr_used = next_head;
    return ret;
}

uint32_t vm_gc_len(vm_gc_t *restrict gc, vm_value_t val)
{
    if (VM_VALUE_IS_STR(val))
    {
        return gc->str_lens[VM_VALUE_GET_INT(val)];
    }
    if (VM_VALUE_IS_ARR(val))
    {
        return gc->arr_lens[VM_VALUE_GET_INT(val)];
    }
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
    if (VM_VALUE_IS_ARR(ptr))
    {
        return gc->arr_buf[VM_VALUE_GET_INT(ptr)][vm_value_to_int(gc, index)];
    }
    __builtin_trap();
}

vm_value_t vm_gc_get_i(vm_gc_t *restrict gc, vm_value_t ptr, vm_int_t index)
{
    if (VM_VALUE_IS_STR(ptr))
    {
        return vm_value_from_int(gc, gc->str_buf[VM_VALUE_GET_INT(ptr)][index]);
    }
    if (VM_VALUE_IS_ARR(ptr))
    {
        return gc->arr_buf[VM_VALUE_GET_INT(ptr)][index];
    }
    __builtin_trap();
}

void vm_gc_set_vv(vm_gc_t *restrict gc, vm_value_t obj, vm_value_t index, vm_value_t value)
{
    if (VM_VALUE_IS_ARR(obj))
    {
        gc->arr_buf[VM_VALUE_GET_INT(obj)][vm_value_to_int(gc, index)] = value;
        return;
    }
    __builtin_trap();
}

void vm_gc_set_vi(vm_gc_t *restrict gc, vm_value_t obj, vm_value_t index, vm_int_t value)
{
    if (VM_VALUE_IS_ARR(obj))
    {
        gc->arr_buf[VM_VALUE_GET_INT(obj)][vm_value_to_int(gc, index)] = vm_value_from_int(gc, value);
        return;
    }
    __builtin_trap();
}

void vm_gc_set_iv(vm_gc_t *restrict gc, vm_value_t obj, vm_int_t index, vm_value_t value)
{
    if (VM_VALUE_IS_ARR(obj))
    {
        gc->arr_buf[VM_VALUE_GET_INT(obj)][index] = value;
        return;
    }
    __builtin_trap();
}

void vm_gc_set_ii(vm_gc_t *restrict gc, vm_value_t obj, vm_int_t index, vm_int_t value)
{
    if (VM_VALUE_IS_ARR(obj))
    {
        gc->arr_buf[VM_VALUE_GET_INT(obj)][index] = vm_value_from_int(gc, value);
        return;
    }
    __builtin_trap();
}
