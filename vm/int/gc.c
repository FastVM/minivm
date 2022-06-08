#include "gc.h"

void vm_gc_init(vm_gc_t *restrict out)
{
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
    for (size_t i = 0; i < gc.arr_used; i++) {
        vm_free(gc.arr_buf[i]);
    }
    vm_free(gc.arr_buf);
    vm_free(gc.arr_lens);
    vm_free(gc.arr_marks);
}

void vm_gc_mark(vm_gc_t *restrict gc, vm_value_t val)
{
    if (VM_VALUE_IS_ARR(val))
    {
        size_t nth = VM_VALUE_GET_ARR(val);
        if (nth < gc->arr_used)
        {
            if (gc->arr_marks[nth] != 1)
            {
                gc->arr_marks[nth] = 1;
                size_t len = gc->arr_lens[nth];
                for (size_t i = 0; i < len; i++)
                {
                    vm_gc_mark(gc, gc->arr_buf[nth][i]);
                }
            }
        }
    }
}

void vm_gc_move_arr(vm_gc_t *restrict gc, vm_value_t *set)
{
    if (VM_VALUE_IS_ARR(*set))
    {
        size_t nth = VM_VALUE_GET_ARR(*set);
        if (nth < gc->arr_used && gc->arr_marks[nth] != 0)
        {
            gc->arr_marks[nth] = 0;
            size_t newpos = gc->move_buf[nth];
            *set = VM_VALUE_SET_ARR(newpos);
            size_t len = gc->arr_lens[newpos];
            for (size_t i = 0; i < len; i++)
            {
                vm_gc_move_arr(gc, &gc->arr_buf[newpos][i]);
            }
        }
    }
}

void vm_gc_run(vm_gc_t *restrict gc)
{
    if (gc->count < gc->max) {
        return;
    }
    for (size_t i = 0; i < gc->nregs; i++)
    {
        vm_gc_mark(gc, gc->regs[i]);
    }
    {
        if (gc->arr_alloc > gc->move_alloc)
        {
            gc->move_alloc = gc->arr_alloc * 2;
            gc->move_buf = vm_realloc(gc->move_buf, sizeof(size_t) * gc->move_alloc);
        }
        size_t arr_used = 0;
        for (size_t i = 0; i < gc->arr_used; i++)
        {
            if (gc->arr_marks[i] == 1)
            {
                {
                    vm_value_t *tmp = gc->arr_buf[arr_used];
                    gc->arr_buf[arr_used] = gc->arr_buf[i];
                    gc->arr_buf[i] = tmp;
                }
                {
                    size_t tmp = gc->arr_lens[arr_used];
                    gc->arr_lens[arr_used] = gc->arr_lens[i];
                    gc->arr_lens[i] = tmp;
                }
                gc->move_buf[i] = arr_used;
                arr_used += 1;
            }
            else
            {
                vm_free(gc->arr_buf[i]);
            }
        }
        for (size_t i = 0; i < gc->nregs; i++)
        {
            vm_gc_move_arr(gc, &gc->regs[i]);
        }
        gc->arr_used = arr_used;
    }
    gc->count = 0;
    gc->max = (gc->str_alloc + gc->arr_alloc) >> 1;
    if (gc->max < VM_GC_MIN)
    {
        gc->max = VM_GC_MIN;
    }
}

size_t vm_gc_arr(vm_gc_t *restrict gc, size_t size)
{
    size_t next_head = gc->arr_used + 1;
    if (gc->arr_alloc <= next_head)
    {
        size_t next_alloc = next_head * 2;
        gc->arr_buf = vm_realloc(gc->arr_buf, sizeof(vm_value_t *) * next_alloc);
        gc->arr_lens = vm_realloc(gc->arr_lens, sizeof(size_t) * next_alloc);
        gc->arr_marks = vm_realloc(gc->arr_marks, sizeof(uint8_t) * next_alloc);
        for (size_t i = gc->arr_alloc; i < next_alloc; i++)
        {
            gc->arr_buf[i] = NULL;
            gc->arr_marks[i] = 0;
        }
        gc->arr_alloc = next_alloc;
    }
    size_t ret = gc->arr_used;
    gc->arr_lens[ret] = (size_t)size;
    gc->arr_buf[ret] = vm_malloc(sizeof(vm_value_t) * size);
    gc->arr_used = next_head;
    gc->count += 1;
    return ret;
}

size_t vm_gc_len(vm_gc_t *restrict gc, vm_value_t val)
{
    return gc->arr_lens[VM_VALUE_GET_ARR(val)];
}

vm_value_t vm_gc_get_v(vm_gc_t *restrict gc, vm_value_t ptr, vm_value_t index)
{
    return gc->arr_buf[VM_VALUE_GET_ARR(ptr)][vm_value_to_int(gc, index)];
}

void vm_gc_set_vv(vm_gc_t *restrict gc, vm_value_t obj, vm_value_t index, vm_value_t value)
{
    gc->arr_buf[VM_VALUE_GET_ARR(obj)][vm_value_to_int(gc, index)] = value;
}
