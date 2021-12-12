#include "gc.h"
#include "obj.h"

void vm_gc_mark_ptr(vm_gc_entry_t *ent)
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
            vm_gc_mark_ptr(vm_obj_to_ptr(obj));
        }
    }
}

void vm_gc_run1_impl(vm_gc_t *gc, vm_obj_t *low, vm_obj_t *high)
{
    for (vm_obj_t *base = low; base < low + VM_LOCALS_UNITS; base++)
    {
        vm_obj_t cur = *base;
        if (vm_obj_is_ptr(cur))
        {
            vm_gc_entry_t *ptr = vm_obj_to_ptr(cur);
            vm_gc_mark_ptr(ptr);
        }
    }
    size_t begin = 0;
    vm_gc_entry_t *first = gc->first;
    vm_gc_entry_t *last = NULL;
    size_t yes = 0;
    size_t no = 0;
    while (first != NULL)
    {
        vm_gc_entry_t *ent = first;
        first = first->next;
        if (ent->keep)
        {
            ent->keep = false;
            ent->next = last;
            last = ent;
            yes++;
        }
        else
        {
            vm_free(ent);
            no++;
        }
    }
    gc->remain = yes * 2;
    gc->first = last;
}

void vm_gc_run1(vm_gc_t *gc, vm_obj_t *low, vm_obj_t *high) {
    if (gc->remain > 0)
    {
        return;
    }
    vm_gc_run1_impl(gc, low, high);
}

void vm_gc_start(vm_gc_t *gc)
{
    gc->remain = 100;
    gc->first = NULL;
}

void vm_gc_stop(vm_gc_t *gc)
{
    vm_gc_entry_t *first = gc->first;
    while (first != NULL)
    {
        vm_gc_entry_t *next = first->next;
        vm_free(first);
        first = next;
    }
}

vm_gc_entry_t *vm_gc_static_array_new(vm_gc_t *gc, size_t size)
{
    gc->remain -= 1;
    vm_gc_entry_t *entry = vm_malloc(sizeof(vm_gc_entry_t) + sizeof(vm_obj_t) * size);
    *entry = (vm_gc_entry_t){
        .next = gc->first,
        .keep = false,
        .alloc = size,
        .len = size,
    };
    gc->first = entry;
    return entry;
}

vm_obj_t vm_gc_get_index(vm_gc_entry_t *ptr, vm_int_t index)
{
    if (index < 0) {
        index += ptr->len;
    }
    if (index >= ptr->len) {
        __builtin_trap();
    }
    return ptr->arr[index];
}

void vm_gc_set_index(vm_gc_entry_t *ptr, vm_int_t index, vm_obj_t value)
{
    if (index < 0) {
        index += ptr->len;
    }
    if (index >= ptr->len) {
        __builtin_trap();
    }
    ptr->arr[index] = value;
}

vm_int_t vm_gc_sizeof(vm_gc_entry_t *ptr)
{
    return ptr->len;
}

vm_obj_t vm_gc_static_concat(vm_gc_t *gc, vm_obj_t lhs, vm_obj_t rhs)
{
    vm_gc_entry_t *left = vm_obj_to_ptr(lhs);
    vm_gc_entry_t *right = vm_obj_to_ptr(rhs);
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
    return vm_obj_of_ptr(ent);
}

vm_obj_t vm_gc_dup(vm_gc_t *gc, vm_obj_t obj)
{
    if (!vm_obj_is_ptr(obj))
    {
        return obj;
    }
    vm_gc_entry_t *ent = vm_obj_to_ptr(obj);
    vm_gc_entry_t *ret = vm_gc_static_array_new(gc, ent->len);
    for (size_t i = 0; i < ent->len; i++)
    {
        ret->arr[i] = vm_gc_dup(gc, ent->arr[i]);
    }
    return vm_obj_of_ptr(ret);
}
