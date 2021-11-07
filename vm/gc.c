#include "gc.h"
#include "obj.h"
#include "obj/map.h"

void vm_gc_mark_ptr(vm_gc_entry_t *ent);
void vm_gc_free(vm_gc_entry_t *ent);

static inline int vm_gc_mark_map_entry(void *state, vm_obj_t key, vm_obj_t val)
{
    if (vm_obj_is_ptr(key))
    {
        vm_gc_mark_ptr(vm_obj_to_ptr(key));
    }
    if (vm_obj_is_ptr(val))
    {
        vm_gc_mark_ptr(vm_obj_to_ptr(val));
    }
    return 0;
}

void vm_gc_mark_ptr(vm_gc_entry_t *ent)
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
            vm_gc_mark_ptr(vm_obj_to_ptr(obj));
        }
        break;
    }
    case VM_TYPE_ARRAY:
    {
        vm_gc_entry_array_t *arr_ent = (vm_gc_entry_array_t *)ent;
        for (size_t cur = 0; cur < arr_ent->len; cur++)
        {
            vm_obj_t obj = ((vm_obj_t *)arr_ent->obj)[cur];
            if (vm_obj_is_ptr(obj))
            {
                vm_gc_mark_ptr(vm_obj_to_ptr(obj));
            }
        }
        break;
    }
    case VM_TYPE_MAP:
    {
        vm_gc_entry_map_t *map_ent = (vm_gc_entry_map_t *)ent;
        vm_map_for_pairs(map_ent->map, NULL, vm_gc_mark_map_entry);
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
        vm_gc_entry_map_t *map = (vm_gc_entry_map_t *)ent;
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

void vm_gc_run1(vm_gc_t *gc)
{
    if (gc->low == NULL)
    {
        return;
    }
    for (vm_obj_t *base = gc->low; base < gc->high; base++)
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
    size_t n = 0;
    while (first != NULL)
    {
        vm_gc_entry_t *ent = first;
        first = first->next;
        if (ent->keep)
        {
            ent->keep = false;
            ent->next = last;
            last = ent;
            n++;
        }
        else
        {
            vm_gc_free(ent);
        }
    }
    gc->remain = (gc->high - gc->low) + n;
    gc->first = last;
}


void *vm_gc_run(void *arg) 
{
    vm_gc_t *gc = arg;
    return NULL;
}

void vm_gc_start(vm_gc_t *gc)
{
    gc->remain = 1000;
    gc->first = NULL;
    gc->low = NULL;
    gc->high = NULL;
}

void vm_gc_stop(vm_gc_t *gc)
{
    vm_gc_entry_t *first = gc->first;
    while (first != NULL)
    {
        vm_gc_entry_t *next = first->next;
        vm_gc_free(first);
        first = next;
    }
}

vm_gc_entry_t *vm_gc_map_new(vm_gc_t *gc)
{
    gc->remain--;
    if (gc->remain == 0) {
        vm_gc_run1(gc);
    }
    vm_gc_entry_map_t *entry = vm_malloc(sizeof(vm_gc_entry_map_t));
    *entry = (vm_gc_entry_map_t){
        .next = gc->first,
        .keep = false,
        .type = VM_TYPE_MAP,
        .map = vm_map_new(),
    };
    vm_gc_entry_t *obj = (vm_gc_entry_t *)entry;
    gc->first = (vm_gc_entry_t *) entry;
    return obj;
}

vm_gc_entry_t *vm_gc_array_new(vm_gc_t *gc, size_t size)
{
    gc->remain--;
    if (gc->remain == 0) {
        vm_gc_run1(gc);
    }
    vm_gc_entry_array_t *entry = vm_malloc(sizeof(vm_gc_entry_array_t));
    *entry = (vm_gc_entry_array_t){
        .next = gc->first,
        .keep = false,
        .type = VM_TYPE_ARRAY,
        .len = size,
        .alloc = size,
        .obj = vm_malloc(sizeof(vm_obj_t) * size),
    };
    vm_gc_entry_t *obj = (vm_gc_entry_t *)entry;
    gc->first = (vm_gc_entry_t *) entry;
    return obj;
}

vm_gc_entry_t *vm_gc_string_new(vm_gc_t *gc, size_t size)
{
    gc->remain--;
    if (gc->remain == 0) {
        vm_gc_run1(gc);
    }
    vm_gc_entry_string_t *entry = vm_malloc(sizeof(vm_gc_entry_string_t) + sizeof(char) * (size + 1));
    *entry = (vm_gc_entry_string_t){
        .next = gc->first,
        .keep = false,
        .type = VM_TYPE_STRING,
        .len = size,
    };
    vm_gc_entry_t *obj = (vm_gc_entry_t *)entry;
    gc->first = (vm_gc_entry_t *) entry;
    return obj;
}

vm_gc_entry_t *vm_gc_box_new(vm_gc_t *gc)
{
    gc->remain--;
    if (gc->remain == 0) {
        vm_gc_run1(gc);
    }
    vm_gc_entry_box_t *entry = vm_malloc(sizeof(vm_gc_entry_box_t));
    *entry = (vm_gc_entry_box_t){
        .next = gc->first,
        .keep = false,
        .type = VM_TYPE_BOX,
    };
    vm_gc_entry_t *obj = (vm_gc_entry_t *)entry;
    gc->first = (vm_gc_entry_t *) entry;
    return obj;
}

vm_gc_entry_t *vm_gc_ref_new(vm_gc_t *gc, vm_obj_t *value)
{
    gc->remain--;
    if (gc->remain == 0) {
        vm_gc_run1(gc);
    }
    vm_gc_entry_ref_t *entry = vm_malloc(sizeof(vm_gc_entry_ref_t));
    *entry = (vm_gc_entry_ref_t){
        .next = gc->first,
        .keep = false,
        .type = VM_TYPE_REF,
        .ref = value,
    };
    vm_gc_entry_t *obj = (vm_gc_entry_t *)entry;
    gc->first = (vm_gc_entry_t *) entry;
    return obj;
}

vm_obj_t vm_gc_get_box(vm_gc_entry_t *box)
{
    return ((vm_gc_entry_box_t *)box)->obj;
}

vm_obj_t *vm_gc_get_ref(vm_gc_entry_t *box)
{
    return ((vm_gc_entry_ref_t *)box)->ref;
}

void vm_gc_set_ref(vm_gc_entry_t *box, vm_obj_t value)
{
    *((vm_gc_entry_ref_t *)box)->ref = value;
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
        if (!vm_obj_is_num(index)) {
            return vm_obj_of_dead();
        }
        vm_gc_entry_array_t *ent = (vm_gc_entry_array_t *)ptr;
        int iind = vm_obj_to_int(index);
        if (iind < 0) {
            iind += ent->len;
        }
        if (iind >= ent->len) {
            return vm_obj_of_dead();
        }
        return ent->obj[iind];
    }
    case VM_TYPE_MAP:
    {
        return vm_map_get_index(((vm_gc_entry_map_t *)ptr)->map, index);
    }
    case VM_TYPE_STRING:
    {
        if (!vm_obj_is_num(index)) {
            return vm_obj_of_dead();
        }
        vm_gc_entry_string_t *ent = (vm_gc_entry_string_t *)ptr;
        int iind = vm_obj_to_int(index);
        if (iind < 0) {
            iind += ent->len;
        }
        if (iind >= ent->len) {
            return vm_obj_of_dead();
        }
        return vm_obj_of_int(ent->obj[iind]);
    }
    default:
    {
        return vm_obj_of_dead();
    }
    }
}

vm_obj_t vm_gc_extend(vm_gc_entry_t *to, vm_gc_entry_t *from)
{
    if (to->type != VM_TYPE_ARRAY)
    {
        return vm_obj_of_dead();
    }
    if (from->type != VM_TYPE_ARRAY)
    {
        return vm_obj_of_dead();
    }
    switch (to->type)
    {
    default: 
    {
        return vm_obj_of_dead();
    }
    case VM_TYPE_ARRAY:
    {
        vm_gc_entry_array_t *ato = (vm_gc_entry_array_t *)to;
        vm_gc_entry_array_t *afrom = (vm_gc_entry_array_t *)from;
        if (ato->len + afrom->len >= ato->alloc)
        {
            int alloc = 4 + (ato->len + afrom->len) * 2;
            ato->obj = vm_realloc(ato->obj, sizeof(vm_obj_t) * alloc);
            ato->alloc = alloc;
        }
        for (size_t i = 0; i < afrom->len; i++)
        {
            ato->obj[ato->len++] = afrom->obj[i];
        }
        return vm_obj_of_none();
    }
    }
}

vm_obj_t vm_gc_set_index(vm_gc_entry_t *ptr, vm_obj_t index, vm_obj_t value)
{
    switch (ptr->type)
    {
    case VM_TYPE_ARRAY:
    {
        vm_gc_entry_array_t *arr = (vm_gc_entry_array_t *)ptr;
        int i = vm_obj_to_int(index);
        if (i >= arr->alloc)
        {
            int alloc = 4 + i * 2;
            arr->obj = vm_realloc(arr->obj, sizeof(vm_obj_t) * alloc);
            arr->alloc = alloc;
        }
        if (i >= arr->len)
        {
            arr->len = i + 1;
        }
        arr->obj[i] = value;
        return vm_obj_of_none();
    }
    case VM_TYPE_MAP:
    {
        vm_map_set_index(((vm_gc_entry_map_t *)ptr)->map, index, value);
        return vm_obj_of_none();
    }
    case VM_TYPE_STRING:
    {
        ((vm_gc_entry_string_t *)ptr)->obj[vm_obj_to_int(index)] = vm_obj_to_int(value);
        return vm_obj_of_none();
    }
    default:
    {
        return vm_obj_of_dead();
    }
    }
}

vm_obj_t vm_gc_sizeof(vm_gc_entry_t *ptr)
{
    switch (ptr->type)
    {
    case VM_TYPE_ARRAY:
    {
        return vm_obj_of_int(((vm_gc_entry_array_t *)ptr)->len);
    }
    case VM_TYPE_MAP:
    {
        return vm_obj_of_int(vm_map_sizeof(((vm_gc_entry_map_t *)ptr)->map));
    }
    case VM_TYPE_STRING:
    {
        return vm_obj_of_int(((vm_gc_entry_string_t *)ptr)->len);
    }
    default:
    {
        return vm_obj_of_dead();
    }
    }
}

vm_obj_t vm_gc_concat(vm_gc_t *gc, vm_obj_t lhs, vm_obj_t rhs)
{
    vm_gc_entry_t *left = vm_obj_to_ptr(lhs);
    vm_gc_entry_t *right = vm_obj_to_ptr(rhs);
    int ret_type = vm_gc_type(left);
    if (ret_type != vm_gc_type(right))
    {
        return vm_obj_of_dead();
    }
    switch (ret_type)
    {
    default:
    {
        return vm_obj_of_dead();
    }
    case VM_TYPE_ARRAY:
    {
        vm_gc_entry_array_t *arr_left = (vm_gc_entry_array_t *)left;
        vm_gc_entry_array_t *arr_right = (vm_gc_entry_array_t *)right;
        int llen = arr_left->len;
        int rlen = arr_right->len;
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
