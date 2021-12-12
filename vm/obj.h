#pragma once

#include "type.h"
#include "gc.h"

#if VM_NANBOX

// type check

static inline bool vm_obj_is_none(vm_obj_t obj)
{
    return nanbox_is_null(obj);
}

static inline bool vm_obj_is_bool(vm_obj_t obj)
{
    return nanbox_is_boolean(obj);
}

static inline bool vm_obj_is_num(vm_obj_t obj)
{
    return nanbox_is_double(obj);
}

static inline bool vm_obj_is_ptr(vm_obj_t obj)
{
    return nanbox_is_pointer(obj);
}

// c to obj

static inline vm_obj_t vm_obj_of_none(void)
{
    return nanbox_null();
}

static inline vm_obj_t vm_obj_of_bool(bool obj)
{
    return nanbox_from_boolean(obj);
}

static inline vm_obj_t vm_obj_of_int(int obj)
{
    return nanbox_from_double((vm_number_t)obj);
}

static inline vm_obj_t vm_obj_of_num(vm_number_t obj)
{
    return nanbox_from_double(obj);
}

static inline vm_obj_t vm_obj_of_ptr(vm_gc_t *gc, void *obj)
{
    return nanbox_from_pointer(obj);
}

// obj to c

static inline bool vm_obj_to_bool(vm_obj_t obj)
{
    return !nanbox_is_false(obj);
}

static inline vm_int_t vm_obj_to_int(vm_obj_t obj)
{
    return (vm_int_t)nanbox_to_double(obj);
}

static inline vm_number_t vm_obj_to_num(vm_obj_t obj)
{
    return nanbox_to_double(obj);
}

static inline void *vm_obj_to_ptr(vm_gc_t *gc, vm_obj_t obj)
{
    return nanbox_to_pointer(obj);
}

#else

static inline bool vm_obj_is_none(vm_obj_t obj)
{
    return obj.type == VM_TYPE_NONE;
}

static inline bool vm_obj_is_bool(vm_obj_t obj)
{
    return obj.type == VM_TYPE_BOOL;
}

static inline bool vm_obj_is_num(vm_obj_t obj)
{
    return obj.type == VM_TYPE_NUMBER;
}

static inline bool vm_obj_is_ptr(vm_obj_t obj)
{
    return obj.type == VM_TYPE_ARRAY;
}

// c to obj

static inline vm_obj_t vm_obj_of_none(void)
{
    return (vm_obj_t) {
        .type = VM_TYPE_NONE,
    };
}

static inline vm_obj_t vm_obj_of_bool(bool obj)
{
    return (vm_obj_t) {
        .type = VM_TYPE_BOOL,
        .value = obj, 
    };
}

static inline vm_obj_t vm_obj_of_int(int obj)
{
    return (vm_obj_t) {
        .type = VM_TYPE_NUMBER,
        .value = obj,
    };
}

static inline vm_obj_t vm_obj_of_num(vm_number_t obj)
{
    return (vm_obj_t) {
        .type = VM_TYPE_NUMBER,
        .value = obj,
    };
}

static inline vm_obj_t vm_obj_of_ptr(vm_gc_t *gc, void *obj)
{
    return (vm_obj_t) {
        .type = VM_TYPE_ARRAY,
        .value = (uint8_t *)obj - gc->base,
    };
}

// obj to c

static inline bool vm_obj_to_bool(vm_obj_t obj)
{
    return obj.value;
}

static inline vm_int_t vm_obj_to_int(vm_obj_t obj)
{
    return obj.value;
}

static inline vm_number_t vm_obj_to_num(vm_obj_t obj)
{
    return obj.value;
}

static inline void *vm_obj_to_ptr(vm_gc_t *gc, vm_obj_t obj)
{
    return gc->base + obj.value;
}

#endif
