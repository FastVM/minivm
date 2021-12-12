#include "gc.h"
#include "obj.h"

bool vm_gc_owns(vm_gc_t *gc, vm_gc_entry_t *ent)
{
    return (void *) &gc->mem[0] <= (void *) ent && (void *) ent < (void *) &gc->mem[gc->alloc];
}

bool vm_gc_xowns(vm_gc_t *gc, vm_gc_entry_t *ent)
{
    // printf("%p in [%p .. %p]\n", ent, &gc->xmem[0], &gc->xmem[gc->alloc]);
    return (void *) &gc->xmem[0] <= (void *) ent && (void *) ent < (void *) &gc->xmem[gc->alloc];
}

void vm_gc_mark_ptr(vm_gc_t *gc, vm_gc_entry_t *ent)
{
    if (ent->keep)
    {
        return;
    }
    ent->keep = true;
    for (size_t cur = 0; cur < ent->len; cur++)
    {
        vm_obj_t obj = ent->arr[cur];
        if (vm_obj_is_ptr(obj))
        {
            vm_gc_mark_ptr(gc, vm_obj_to_ptr(gc, obj));
        }
    }
}

void vm_gc_run1_mark(vm_gc_t *gc, vm_obj_t *low, vm_obj_t *high)
{
    for (vm_obj_t *base = low; base < low + VM_LOCALS_UNITS; base++)
    {
        vm_obj_t obj = *base;
        if (vm_obj_is_ptr(obj))
        {
            vm_gc_entry_t *ptr = vm_obj_to_ptr(gc, obj);
            if (!vm_gc_owns(gc, ptr))
            {
                continue;
            }
            vm_gc_mark_ptr(gc, ptr);
        }
    }
}

void vm_gc_update_any(vm_gc_t *gc, vm_obj_t *obj)
{
    if (!vm_obj_is_ptr(*obj))
    {
        return;
    }
    vm_gc_entry_t *ent = vm_obj_to_ptr(gc, *obj);
    if (vm_gc_xowns(gc, ent))
    {
        return;
    }
    vm_gc_entry_t *put = (void *) &gc->xmem[ent->ptr];
    *obj = vm_obj_of_ptr(gc, put);
    for (size_t cur = 0; cur < put->len; cur++)
    {
        vm_gc_update_any(gc, &put->arr[cur]);
    }
}

void vm_gc_run1_update(vm_gc_t *gc, vm_obj_t *low, vm_obj_t *high)
{
    for (vm_obj_t *base = low; base < low + VM_LOCALS_UNITS; base++)
    {
        vm_gc_update_any(gc, base);
    }
}

size_t vm_gc_run1_move(vm_gc_t *gc)
{
    size_t max = gc->len;
    size_t pos = 0;
    size_t out = 0;
    size_t yes = 0;
    size_t no = 0;
    while (pos < max)
    {
        vm_gc_entry_t *ent = (void *) &gc->mem[pos];
        size_t count = sizeof(vm_gc_entry_t) + sizeof(vm_obj_t) * ent->len;
        if (ent->keep)
        // if (true)
        {
            ent->ptr = out;
            vm_gc_entry_t *put = (void *) &gc->xmem[out];
            put->keep = false;
            put->len = ent->len;
            for (size_t i = 0; i < ent->len; i++)
            {
                put->arr[i] = ent->arr[i];
            }
            yes += 1;
            out += count;
        }
        else
        {
            no += 1;
        }
        pos += count;
    }
    return out;
}

void vm_gc_run1(vm_gc_t *gc, vm_obj_t *low, vm_obj_t *high)
{
    if (gc->max > gc->len)
    {
        return;
    }
    vm_gc_run1_mark(gc, low, high);
    size_t len = vm_gc_run1_move(gc);
    vm_gc_run1_update(gc, low, high);
    uint8_t *old_mem = gc->mem;
    uint8_t *old_xmem = gc->xmem;
    // printf("%zu %zu\n", gc->len, len);
    gc->len = len;
    gc->xmem = old_mem;
    gc->mem = old_xmem;
    if (gc->len * 2 > gc->max)
    {
        gc->max = gc->len * 2;
    }
}

void vm_gc_start(vm_gc_t *gc)
{
    gc->max = 0;
    gc->alloc = (1 << 10) * (1 << 10) * 64;
    gc->len = 0;
    gc->mem = vm_malloc(gc->alloc);
    gc->xmem = vm_malloc(gc->alloc);
}

void vm_gc_stop(vm_gc_t *gc)
{
    vm_free(gc->mem);
    vm_free(gc->xmem);
}

vm_gc_entry_t *vm_gc_static_array_new(vm_gc_t *gc, size_t size)
{
    vm_gc_entry_t *entry = (vm_gc_entry_t *) &gc->mem[gc->len];
    *entry = (vm_gc_entry_t){
        .keep = false,
        .len = size,
    };
    gc->len += sizeof(vm_gc_entry_t) + sizeof(vm_obj_t) * size;
    return entry;
}

vm_obj_t vm_gc_get_index(vm_gc_t *gc, vm_gc_entry_t *ptr, vm_int_t index)
{
    return ptr->arr[index];
}

void vm_gc_set_index(vm_gc_t *gc, vm_gc_entry_t *ptr, vm_int_t index, vm_obj_t value)
{
    ptr->arr[index] = value;
}

vm_int_t vm_gc_sizeof(vm_gc_t *gc, vm_gc_entry_t *ptr)
{
    return ptr->len;
}

vm_obj_t vm_gc_static_concat(vm_gc_t *gc, vm_obj_t lhs, vm_obj_t rhs)
{
    vm_gc_entry_t *left = vm_obj_to_ptr(gc, lhs);
    vm_gc_entry_t *right = vm_obj_to_ptr(gc, rhs);
    vm_int_t llen = left->len;
    vm_int_t rlen = right->len;
    vm_gc_entry_t *ent = vm_gc_static_array_new(gc, llen + rlen);
    for (vm_int_t i = 0; i < llen; i++)
    {
        ent->arr[i] = left->arr[i];
    }
    for (vm_int_t i = 0; i < rlen; i++)
    {
        ent->arr[llen + i] = right->arr[i];
    }
    return vm_obj_of_ptr(gc, ent);
}

vm_obj_t vm_gc_dup(vm_gc_t *gc, vm_obj_t obj)
{
    if (!vm_obj_is_ptr(obj))
    {
        return obj;
    }
    vm_gc_entry_t *ent = vm_obj_to_ptr(gc, obj);
    vm_gc_entry_t *ret = vm_gc_static_array_new(gc, ent->len);
    for (size_t i = 0; i < ent->len; i++)
    {
        ret->arr[i] = vm_gc_dup(gc, ent->arr[i]);
    }
    return vm_obj_of_ptr(gc, ret);
}
