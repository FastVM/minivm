
#include "gc.h"

vm_value_t vm_gc_new(vm_int_t slots) {
    vm_value_array_t *arr =
        vm_malloc(sizeof(vm_value_array_t) + sizeof(vm_value_data_t) * slots + sizeof(uint8_t) * slots);
    arr->alloc = slots;
    arr->len = slots;
    arr->datas = (vm_value_data_t *)&arr[1];
    arr->types = (uint8_t *)&arr->datas[slots];
    return vm_value_from_static(arr);
}

vm_value_t vm_gc_get(vm_value_t obj, vm_value_t index) {
    vm_value_t val;
    val.data = vm_value_to_array(obj)->datas[(size_t)vm_value_to_int(index)];
    val.type = vm_value_to_array(obj)->types[(size_t)vm_value_to_int(index)];
    return val;
}

void vm_gc_set(vm_value_t obj, vm_value_t index, vm_value_t value) {
    vm_value_to_array(obj)->datas[(size_t)vm_value_to_int(index)] = value.data;
    vm_value_to_array(obj)->types[(size_t)vm_value_to_int(index)] = value.type;
}

vm_int_t vm_gc_len(vm_value_t obj) { return (vm_int_t)vm_value_to_array(obj)->len; }
