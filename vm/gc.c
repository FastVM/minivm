#include "gc.h"
#include "obj.h"

void vm_gc_mark_ptr(vm_gc_entry_t *ent)
{
    if (ent->keep)
    {
        return;
    }
    ent->keep = true;
    switch (ent->type)
    {
    case VM_GC_ENTRY_TYPE_STATIC_ARRAY:
    {
        for (size_t cur = 0; cur < ent->len; cur++)
        {
            vm_obj_t obj = ent->arr[cur];
            if (vm_obj_is_ptr(obj))
            {
                vm_gc_mark_ptr(vm_obj_to_ptr(obj));
            }
        }
        break;
    }
    case VM_GC_ENTRY_TYPE_ARRAY:
    {
        for (size_t cur = 0; cur < ent->len; cur++)
        {
            vm_obj_t obj = ent->ptr[cur];
            if (vm_obj_is_ptr(obj))
            {
                vm_gc_mark_ptr(vm_obj_to_ptr(obj));
            }
        }
        break;
    }
    }
}

void vm_gc_run1(vm_gc_t *gc, vm_obj_t *low, vm_obj_t *high)
{
    if (gc->remain > 0)
    {
        return;
    }
    for (vm_obj_t *base = low; base < high; base++)
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
            switch (ent->type)
            {
            case VM_GC_ENTRY_TYPE_STATIC_ARRAY:
            {
                break;
            }
            case VM_GC_ENTRY_TYPE_ARRAY:
            {
                vm_free(ent->ptr);
                break;
            }
            }
            vm_free(ent);
            no++;
        }
    }
    gc->remain = yes * 1;
    gc->first = last;
}

void vm_gc_start(vm_gc_t *gc)
{
    gc->remain = 1;
    gc->first = NULL;
}

void vm_gc_stop(vm_gc_t *gc)
{
    vm_gc_entry_t *first = gc->first;
    while (first != NULL)
    {
        vm_gc_entry_t *next = first->next;
        switch (first->type)
        {
        case VM_GC_ENTRY_TYPE_STATIC_ARRAY:
        {
            break;
        }
        case VM_GC_ENTRY_TYPE_ARRAY:
        {
            vm_free(first->ptr);
            break;
        }
        }
        vm_free(first);
        first = next;
    }
}

vm_gc_entry_t *vm_gc_array_new(vm_gc_t *gc, size_t size)
{
    vm_gc_entry_t *entry = vm_malloc(sizeof(vm_gc_entry_t) + sizeof(vm_obj_t *));
    *entry = (vm_gc_entry_t){
        .next = gc->first,
        .keep = false,
        .alloc = size,
        .type = VM_GC_ENTRY_TYPE_STATIC_ARRAY,
        .len = size,
        .ptr = vm_malloc(sizeof(vm_obj_t) * size),
    };
    gc->first = entry;
    return entry;
}


vm_gc_entry_t *vm_gc_static_array_new(vm_gc_t *gc, size_t size)
{
    vm_gc_entry_t *entry = vm_malloc(sizeof(vm_gc_entry_base_t) + sizeof(vm_obj_t) * size);
    *entry = (vm_gc_entry_t){
        .next = gc->first,
        .keep = false,
        .alloc = size,
        .type = VM_GC_ENTRY_TYPE_ARRAY,
        .len = size,
    };
    gc->first = entry;
    return entry;
}

vm_obj_t vm_gc_get_index(vm_gc_entry_t *ptr, int index)
{
    if (index < 0) {
        index += ptr->len;
    }
    if (index >= ptr->len) {
        __builtin_trap();
    }
    switch (ptr->type)
    {
    case VM_GC_ENTRY_TYPE_STATIC_ARRAY:
    {
        return ptr->arr[index];
    }
    case VM_GC_ENTRY_TYPE_ARRAY:
    {
        return ptr->ptr[index];
    }
    }
    __builtin_trap();
}

void vm_gc_set_index(vm_gc_entry_t *ptr, int index, vm_obj_t value)
{
    if (index < 0) {
        index += ptr->len;
    }
    if (index >= ptr->len) {
        __builtin_trap();
    }
    switch (ptr->type)
    {
    case VM_GC_ENTRY_TYPE_STATIC_ARRAY:
    {
        ptr->arr[index] = value;
        return;
    }
    case VM_GC_ENTRY_TYPE_ARRAY:
    {
        ptr->ptr[index] = value;
        return;
    }
    }
}

int vm_gc_sizeof(vm_gc_entry_t *ptr)
{
    return ((vm_gc_entry_t *)ptr)->len;
}

void vm_gc_extend(vm_gc_entry_t *ato, vm_gc_entry_t *afrom)
{
    if (ato->len + afrom->len + 1 >= ato->alloc)
    {
        int alloc = (ato->len + afrom->len) * 2;
        ato->ptr = vm_realloc(ato->ptr, sizeof(vm_obj_t) * alloc);
        ato->alloc = alloc;
    }
    switch (afrom->type)
    {
    case VM_GC_ENTRY_TYPE_STATIC_ARRAY:
    {
        for (size_t i = 0; i < afrom->len; i++)
        {
            ato->ptr[ato->len++] = afrom->arr[i];
        }
        return;
    }
    case VM_GC_ENTRY_TYPE_ARRAY:
    {
        for (size_t i = 0; i < afrom->len; i++)
        {
            ato->ptr[ato->len++] = afrom->ptr[i];
        }
        return;
    }
    }
}

void vm_gc_push(vm_gc_entry_t *ato, vm_obj_t from)
{
    if (ato->len + 2 >= ato->alloc)
    {
        int alloc = (ato->len + 1) * 2;
        ato->ptr = vm_realloc(ato->ptr, sizeof(vm_obj_t) * alloc);
        ato->alloc = alloc;
    }
    ato->ptr[ato->len++] = from;
}

vm_obj_t vm_gc_concat(vm_gc_t *gc, vm_obj_t lhs, vm_obj_t rhs)
{
    vm_gc_entry_t *left = vm_obj_to_ptr(lhs);
    vm_gc_entry_t *right = vm_obj_to_ptr(rhs);
    int llen = left->len;
    int rlen = right->len;
    vm_gc_entry_t *ent = vm_gc_array_new(gc, llen + rlen);
    
    switch (left->type)
    {
    case VM_GC_ENTRY_TYPE_STATIC_ARRAY:
    {
        for (int i = 0; i < llen; i++)
        {
            ent->ptr[i] = left->arr[i];
        }
        break;
    }
    case VM_GC_ENTRY_TYPE_ARRAY:
    {
        for (int i = 0; i < llen; i++)
        {
            ent->ptr[i] = left->ptr[i];
        }
        break;
    }
    }
    switch (right->type)
    {
    case VM_GC_ENTRY_TYPE_STATIC_ARRAY:
    {
        for (int i = 0; i < rlen; i++)
        {
            ent->ptr[llen + i] = right->arr[i];
        }
        break;
    }
    case VM_GC_ENTRY_TYPE_ARRAY:
    {
        for (int i = 0; i < rlen; i++)
        {
            ent->ptr[llen + i] = right->ptr[i];
        }
        break;
    }
    }
    return vm_obj_of_ptr(ent);
}

vm_obj_t vm_gc_static_concat(vm_gc_t *gc, vm_obj_t lhs, vm_obj_t rhs)
{
    vm_gc_entry_t *left = vm_obj_to_ptr(lhs);
    vm_gc_entry_t *right = vm_obj_to_ptr(rhs);
    int llen = left->len;
    int rlen = right->len;
    vm_gc_entry_t *ent = vm_gc_static_array_new(gc, llen + rlen);
    switch (left->type)
    {
    case VM_GC_ENTRY_TYPE_STATIC_ARRAY:
    {
        for (int i = 0; i < llen; i++)
        {
            ent->ptr[i] = left->arr[i];
        }
        break;
    }
    case VM_GC_ENTRY_TYPE_ARRAY:
    {
        for (int i = 0; i < llen; i++)
        {
            ent->ptr[i] = left->ptr[i];
        }
        break;
    }
    }
    switch (right->type)
    {
    case VM_GC_ENTRY_TYPE_STATIC_ARRAY:
    {
        for (int i = 0; i < rlen; i++)
        {
            ent->ptr[llen + i] = right->arr[i];
        }
        break;
    }
    case VM_GC_ENTRY_TYPE_ARRAY:
    {
        for (int i = 0; i < rlen; i++)
        {
            ent->ptr[llen + i] = right->ptr[i];
        }
        break;
    }
    }
    return vm_obj_of_ptr(ent);
}
