#include "gc.h"

void vm_gc_init(vm_gc_t *gc)
{
}

void vm_gc_deinit(vm_gc_t *gc)
{
}

void vm_gc_mark(vm_value_t val)
{
}

void vm_gc_run(vm_gc_t* restrict gc)
{
}

vm_value_t vm_gc_new(vm_gc_t *gc, vm_int_t slots) {
    vm_value_array_t *arr =
        vm_alloc0(sizeof(vm_value_array_t) + sizeof(vm_value_t) * slots);
    arr->tag = VM_TYPE_ARRAY;
    arr->alloc = slots;
    arr->len = slots;
    arr->data = (vm_value_t *)&arr[1];
    return vm_value_from_array(arr);
}

vm_value_t vm_gc_get(vm_gc_t *gc, vm_value_t obj, vm_value_t index) {
    return vm_value_to_array(obj)->data[(size_t)vm_value_to_int(index)];
}

void vm_gc_set(vm_gc_t *gc, vm_value_t obj, vm_value_t index, vm_value_t value) {
    vm_value_to_array(obj)->data[(size_t)vm_value_to_int(index)] = value;
}

vm_int_t vm_gc_len(vm_gc_t *gc, vm_value_t obj) { return (vm_int_t)vm_value_to_array(obj)->len; }
