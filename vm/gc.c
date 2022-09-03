#include "gc.h"

void vm_gc_init(vm_gc_t* restrict out)
{
    out->buf = NULL;
    out->buf_used = 0;
    out->buf_alloc = 0;

    out->move_buf = 0;
    out->move_alloc = 0;

    out->count = 0;
    out->max = VM_GC_MIN;
    out->running = false;
}

void vm_gc_stop(vm_gc_t gc)
{
    vm_free(gc.buf);
    vm_free(gc.move_buf);
}

void vm_gc_mark(vm_gc_t* restrict gc, vm_value_t val)
{
    if (VM_VALUE_IS_ARR(val))
    {
        vm_int_t n = VM_VALUE_GET_ARR(val);
        vm_gc_data_t* data = &gc->buf[n - 1];
        if (0 < n && n < gc->buf_used && data->header.mark != 1)
        {
            data->header.mark = 1;
            vm_int_t len = data->header.len;
            for (vm_int_t i = 1; i <= len; i++)
            {
                vm_gc_mark(gc, data[i].value);
            }
        }
    }
}

void vm_gc_move_arr(vm_gc_t* restrict gc, vm_value_t* set)
{
    if (VM_VALUE_IS_ARR(*set))
    {
        vm_int_t n = VM_VALUE_GET_ARR(*set);
        if (0 < n && n < gc->buf_used)
        {
            vm_int_t newpos = gc->move_buf[n];
            *set = VM_VALUE_SET_ARR(newpos);
        }
    }
}

void vm_gc_run(vm_gc_t* restrict gc)
{
    if (gc->count < gc->max) {
        return;
    }
    for (vm_int_t i = 0; i < gc->nregs; i++)
    {
        vm_gc_mark(gc, gc->regs[i]);
    }
    {
        if (gc->buf_used > gc->move_alloc)
        {
            gc->move_alloc = gc->buf_used * 2;
            gc->move_buf = vm_realloc(gc->move_buf, sizeof(vm_int_t) * gc->move_alloc);
        }
    }
    vm_int_t used = 0;
    {
        vm_int_t i = 0;
        while (i < gc->buf_used)
        {
            vm_gc_data_t data = gc->buf[i++];
            if (data.header.mark == 1)
            {
                data.header.mark = 0;
                gc->buf[used++] = data;
                gc->move_buf[i] = used;
                for (vm_int_t n = 1; n <= data.header.len; n++)
                {
                    gc->buf[used++] = gc->buf[i++];
                }
            }
            else
            {
                i += data.header.len;
            }
        }
    }
    {
        for (vm_int_t i = 0; i < gc->nregs; i++)
        {
            vm_gc_move_arr(gc, &gc->regs[i]);
        }
        vm_int_t i = 0;
        while (i < used)
        {
            vm_gc_data_t data = gc->buf[i++];
            for (vm_int_t n = 1; n <= data.header.len; n++)
            {
                vm_gc_move_arr(gc, &gc->buf[i++].value);
            }
        }
    }
    gc->buf_used = used;
    gc->count = 0;
    gc->max = used * 2;
    if (gc->max < VM_GC_MIN)
    {
        gc->max = VM_GC_MIN;
    }
}

vm_int_t vm_gc_arr(vm_gc_t* restrict gc, vm_int_t size)
{
    vm_int_t next_head = gc->buf_used + size + 1;
    if (gc->buf_alloc <= next_head + 4)
    {
        vm_int_t next_alloc = (next_head + 4) * 2;
        gc->buf = vm_realloc(gc->buf, sizeof(vm_gc_data_t) * next_alloc);
        for (vm_int_t i = gc->buf_alloc; i < next_alloc; i++) {
            gc->buf[i].value = VM_VALUE_SET_INT(0);
        }
        gc->buf_alloc = next_alloc;
    }
    vm_int_t ret = gc->buf_used;
    gc->buf[ret].header.mark = 0;
    gc->buf[ret].header.len = size;
    gc->buf[ret].header.type = 0;
    gc->count += size + 1;
    gc->buf_used = next_head;
    return ret + 1;
}

vm_value_t vm_gc_get(vm_gc_t *restrict gc, vm_value_t obj, vm_int_t index)
{
    return gc->buf[VM_VALUE_GET_ARR(obj) + index].value;
}

void vm_gc_set(vm_gc_t *restrict gc, vm_value_t obj, vm_int_t index, vm_value_t value)
{
    gc->buf[VM_VALUE_GET_ARR(obj) + index].value = value;
}

vm_int_t vm_gc_len(vm_gc_t *restrict gc, vm_value_t obj)
{
    return gc->buf[VM_VALUE_GET_ARR(obj) - 1].header.len;
}
