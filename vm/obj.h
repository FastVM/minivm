#pragma once

#if defined(VM_USE_NAN)
#if VM_USE_NAN
#define VM_NANBOX 1
#else
#define VM_NANBOX 0
#endif
#else
#define VM_NANBOX 1
#endif

#if VM_NANBOX
#include "nanbox.h"
typedef nanbox_t vm_obj_t;
typedef char vm_char_t;
typedef int vm_int_t;
typedef int vm_loc_t;
// typedef double vm_number_t;
typedef double vm_number_t;

enum
{
    VM_TYPE_NONE = 1,
    VM_TYPE_BOOL = 2,
    VM_TYPE_NUMBER = 3,
    VM_TYPE_ARRAY = 4,
};

#include "gc.h"

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
    return nanbox_is_double(obj);
}

static inline bool vm_obj_is_ptr(vm_obj_t obj)
{
    return nanbox_is_pointer(obj);
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
#endif