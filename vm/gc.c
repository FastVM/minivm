#include "gc.h"
#include "obj.h"
#include "io.h"
#include "obj/map.h"

void *vm_mem_grow(size_t size)
{
    return vm_calloc(1, size);
}

void vm_mem_reset(void *ptr)
{
    vm_free(ptr);
}

static inline void vm_gc_mark_map_entry(void *gc, vm_obj_t key, vm_obj_t val)
{
    if (vm_obj_is_ptr(key))
    {
        vm_gc_mark_ptr(gc, vm_obj_to_ptr(key));
    }
    if (vm_obj_is_ptr(val))
    {
        vm_gc_mark_ptr(gc, vm_obj_to_ptr(val));
    }
}

void vm_gc_mark_ptr(vm_gc_t *gc, vm_gc_entry_t *ent)
{
    if (ent->keep)
    {
        return;
    }
    ent->keep = true;
    switch (ent->type)
    {
    case VM_TYPE_BOX:
    {
        vm_gc_entry_box_t *box_ent = (vm_gc_entry_box_t *)ent;
        vm_obj_t obj = box_ent->obj;
        if (vm_obj_is_ptr(obj))
        {
            vm_gc_mark_ptr(gc, vm_obj_to_ptr(obj));
        }
    }
    case VM_TYPE_ARRAY:
    {
        vm_gc_entry_array_t *arr_ent = (vm_gc_entry_array_t *)ent;
        for (size_t cur = 0; cur < arr_ent->len / sizeof(vm_obj_t); cur++)
        {
            vm_obj_t obj = ((vm_obj_t *)arr_ent->obj)[cur];
            if (vm_obj_is_ptr(obj))
            {
                vm_gc_mark_ptr(gc, vm_obj_to_ptr(obj));
            }
        }
        break;
    }
    case VM_TYPE_MAP:
    {
        vm_gc_entry_map_t *map_ent = (vm_gc_entry_map_t *)ent;
        vm_map_for_pairs(map_ent->map, gc, vm_gc_mark_map_entry);
        break;
    }
    case VM_TYPE_STRING:
    {
        break;
    }
    }
}

void vm_gc_free(vm_gc_entry_t *ent)
{
    switch (vm_gc_type(ent))
    {
    case VM_TYPE_MAP:
    {
        vm_gc_entry_map_t *map = (vm_gc_entry_map_t*) ent;
        vm_map_del(map->map);
        break;
    }
    default: 
    {
        break;
    }
    }
    vm_free(ent);
}

void vm_gc_run1(vm_gc_t *gc, vm_obj_t *low, vm_obj_t *high)
{
    for (vm_obj_t *base = low; base < high; base++)
    {
        vm_obj_t cur = *base;
        if (vm_obj_is_ptr(cur))
        {
            vm_gc_entry_t *ptr = vm_obj_to_ptr(cur);
            vm_gc_mark_ptr(gc, ptr);
        }
    }
    size_t begin = 0;
    for (size_t index = 0; index < gc->len; index++)
    {
        vm_gc_entry_t *ent = gc->objs[index];
        if (ent != NULL && ent->keep)
        {
            ent->keep = false;
            gc->objs[begin++] = ent;
        }
        else
        {
            vm_gc_free(ent);
            gc->objs[index] = NULL;
        }
    }
    gc->len = begin;
    size_t newmax = 4 + begin * 2;
    if (gc->max < newmax)
    {
        gc->max = newmax;
    }
    if (newmax + 4 >= gc->alloc)
    {
        gc->alloc = 4 + gc->alloc * 2;
        gc->objs = vm_realloc(gc->objs, sizeof(vm_gc_entry_t *) * gc->alloc);
    }
}

void vm_gc_start(vm_gc_t *gc)
{
    gc->len = 0;
    gc->max = 8;
    gc->alloc = 16;
    gc->objs = vm_malloc(sizeof(vm_gc_entry_t *) * gc->alloc);
}

void vm_gc_stop(vm_gc_t *gc)
{
    for (size_t index = 0; index < gc->len; index++)
    {
        vm_gc_free(gc->objs[index]);
    }
    vm_free(gc->objs);
}

vm_gc_entry_t *vm_gc_map_new(vm_gc_t *gc)
{
    vm_gc_entry_map_t *entry = vm_malloc(sizeof(vm_gc_entry_map_t));
    *entry = (vm_gc_entry_map_t){
        .keep = false,
        .type = VM_TYPE_MAP,
        .map = vm_map_new(),
    };
    vm_gc_entry_t *obj = (vm_gc_entry_t *)entry;
    gc->objs[gc->len++] = obj;
    return obj;
}

vm_gc_entry_t *vm_gc_array_new(vm_gc_t *gc, size_t size)
{
    vm_gc_entry_array_t *entry = vm_malloc(sizeof(vm_gc_entry_array_t) + sizeof(vm_obj_t) * size);
    *entry = (vm_gc_entry_array_t){
        .keep = false,
        .type = VM_TYPE_ARRAY,
        .len = size * sizeof(vm_obj_t),
    };
    vm_gc_entry_t *obj = (vm_gc_entry_t *)entry;
    gc->objs[gc->len++] = obj;
    return obj;
}

vm_gc_entry_t *vm_gc_string_new(vm_gc_t *gc, size_t size)
{
    vm_gc_entry_string_t *entry = vm_malloc(sizeof(vm_gc_entry_string_t) + sizeof(char) * size);
    *entry = (vm_gc_entry_string_t){
        .keep = false,
        .type = VM_TYPE_STRING,
        .len = size,
    };
    vm_gc_entry_t *obj = (vm_gc_entry_t *)entry;
    gc->objs[gc->len++] = obj;
    return obj;
}

vm_gc_entry_t *vm_gc_box_new(vm_gc_t *gc)
{
    vm_gc_entry_box_t *entry = vm_malloc(sizeof(vm_gc_entry_box_t));
    *entry = (vm_gc_entry_box_t){
        .keep = false,
        .type = VM_TYPE_BOX,
    };
    vm_gc_entry_t *obj = (vm_gc_entry_t *)entry;
    gc->objs[gc->len++] = obj;
    return obj;
}

vm_obj_t vm_gc_get_box(vm_gc_entry_t *box)
{
    return ((vm_gc_entry_box_t *)box)->obj;
}

void vm_gc_set_box(vm_gc_entry_t *box, vm_obj_t value)
{
    ((vm_gc_entry_box_t *)box)->obj = value;
}

int vm_gc_type(vm_gc_entry_t *ent)
{
    return ent->type;
}

vm_obj_t vm_gc_get_index(vm_gc_entry_t *ptr, vm_obj_t index)
{
    switch (ptr->type)
    {
    case VM_TYPE_ARRAY:
    {
        return ((vm_gc_entry_array_t *)ptr)->obj[vm_obj_to_int(index)];
    }
    case VM_TYPE_MAP:
    {
        return vm_map_get_index(((vm_gc_entry_map_t *)ptr)->map, index);
    }
    case VM_TYPE_STRING:
    {
        return vm_obj_of_int(((vm_gc_entry_string_t *)ptr)->obj[vm_obj_to_int(index)]);
    }
    }
    __builtin_trap();
}

void vm_gc_set_index(vm_gc_entry_t *ptr, vm_obj_t index, vm_obj_t value)
{
    switch (ptr->type)
    {
    case VM_TYPE_ARRAY:
    {
        ((vm_gc_entry_array_t *)ptr)->obj[vm_obj_to_int(index)] = value;
        return;
    }
    case VM_TYPE_MAP:
    {
        vm_map_set_index(((vm_gc_entry_map_t *)ptr)->map, index, value);
        return;
    }
    case VM_TYPE_STRING:
    {
        ((vm_gc_entry_string_t *)ptr)->obj[vm_obj_to_int(index)] = vm_obj_to_int(value);
        return;
    }
    }
    __builtin_trap();
}

size_t vm_gc_sizeof(vm_gc_entry_t *ptr)
{
    switch (ptr->type)
    {
    case VM_TYPE_ARRAY:
    {
        return ((vm_gc_entry_array_t *)ptr)->len / sizeof(vm_obj_t);
    }
    case VM_TYPE_MAP:
    {
        return vm_map_sizeof(((vm_gc_entry_map_t *)ptr)->map);
    }
    case VM_TYPE_STRING:
    {
        return ((vm_gc_entry_string_t *)ptr)->len;
    }
    }
    __builtin_trap();
}

vm_obj_t vm_gc_concat(vm_gc_t *gc, vm_obj_t lhs, vm_obj_t rhs)
{
    vm_gc_entry_t *left = vm_obj_to_ptr(lhs);
    vm_gc_entry_t *right = vm_obj_to_ptr(rhs);
    int ret_type = vm_gc_type(left);
    switch (ret_type)
    {
    default:
    {
        __builtin_trap();
    }
    case VM_TYPE_ARRAY:
    {
        vm_gc_entry_array_t *arr_left = (vm_gc_entry_array_t *)left;
        vm_gc_entry_array_t *arr_right = (vm_gc_entry_array_t *)right;
        int llen = vm_gc_sizeof(left);
        int rlen = vm_gc_sizeof(right);
        vm_gc_entry_array_t *ent = (vm_gc_entry_array_t *)vm_gc_array_new(gc, llen + rlen);
        for (int i = 0; i < llen; i++)
        {
            ((vm_obj_t *)ent->obj)[i] = arr_left->obj[i];
        }
        for (int i = 0; i < rlen; i++)
        {
            ((vm_obj_t *)ent->obj)[llen + i] = arr_right->obj[i];
        }
        return vm_obj_of_ptr(ent);
    }
    case VM_TYPE_STRING:
    {
        vm_gc_entry_string_t *str_left = (vm_gc_entry_string_t *)left;
        vm_gc_entry_string_t *str_right = (vm_gc_entry_string_t *)right;
        int llen = str_left->len;
        int rlen = str_right->len;
        vm_gc_entry_string_t *ent = (vm_gc_entry_string_t *)vm_gc_string_new(gc, llen + rlen);
        for (int i = 0; i < llen; i++)
        {
            ((char *)ent->obj)[i] = ((char *)str_left->obj)[i];
        }
        for (int i = 0; i < rlen; i++)
        {
            ((char *)ent->obj)[llen + i] = ((char *)str_right->obj)[i];
        }
        return vm_obj_of_ptr(ent);
    }
    }
}
