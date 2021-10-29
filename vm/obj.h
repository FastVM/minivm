#pragma once

#include "nanbox.h"
typedef nanbox_t vm_obj_t;
typedef int vm_loc_t;
typedef double vm_number_t;

enum
{
    VM_TYPE_NONE = 0,
    VM_TYPE_BOOL = 1,
    VM_TYPE_NUMBER = 2,
    VM_TYPE_FUNCTION = 3,
    VM_TYPE_ARRAY = 4,
    VM_TYPE_STRING = 5,
    VM_TYPE_BOX = 6,
    VM_TYPE_MAP = 7,
    VM_TYPE_REF = 8,
};

// type check

static inline bool vm_obj_is_dead(vm_obj_t obj)
{
    return nanbox_is_empty(obj);
}

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
    return nanbox_is_number(obj);
}

static inline bool vm_obj_is_ptr(vm_obj_t obj)
{
    return nanbox_is_pointer(obj);
}

static inline bool vm_obj_is_fun(vm_obj_t obj)
{
    return nanbox_is_int(obj);
}


// c to obj

static inline vm_obj_t vm_obj_of_dead(void)
{
    return nanbox_empty();
}

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

static inline vm_obj_t vm_obj_of_fun(int obj)
{
    return nanbox_from_int(obj);
}

static inline vm_obj_t vm_obj_of_ptr(void *obj)
{
    return nanbox_from_pointer(obj);
}

// obj to c

static inline bool vm_obj_to_bool(vm_obj_t obj)
{
    return !nanbox_is_false(obj);
}

static inline int vm_obj_to_int(vm_obj_t obj)
{
    return (int)nanbox_to_double(obj);
}

static inline vm_number_t vm_obj_to_num(vm_obj_t obj)
{
    return nanbox_to_double(obj);
}

static inline int vm_obj_to_fun(vm_obj_t obj)
{
    return nanbox_to_int(obj);
}

static inline void *vm_obj_to_ptr(vm_obj_t obj)
{
    return nanbox_to_pointer(obj);
}
