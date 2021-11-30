#pragma once

#include "nanbox.h"
typedef nanbox_t vm_obj_t;
typedef char vm_char_t;
typedef int vm_int_t;
typedef int vm_loc_t;
// typedef double vm_number_t;
typedef double vm_number_t;

#if defined(VM_OS)
extern uint8_t *os_mem_base;
#endif

enum
{
    VM_TYPE_NONE = 1,
    VM_TYPE_BOOL = 2,
    VM_TYPE_NUMBER = 3,
    VM_TYPE_ARRAY = 4,
    VM_TYPE_STRING = 5,
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
    return nanbox_is_double(obj);
}

static inline bool vm_obj_is_ptr(vm_obj_t obj)
{
#if defined(VM_OS)
    return obj.as_bits.tag == 0x00010000;
#else
    return nanbox_is_pointer(obj);
#endif
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

static inline vm_obj_t vm_obj_of_ptr(void *obj)
{
#if defined(VM_OS)
    nanbox_t ret;
    ret.as_bits.tag = 0x00010000;
    ret.as_bits.payload = (uint8_t *) obj - os_mem_base;
    return ret;
#else
    return nanbox_from_pointer(obj);
#endif
}

// obj to c

static inline bool vm_obj_to_bool(vm_obj_t obj)
{
    return !nanbox_is_false(obj);
}

static inline vm_int_t vm_obj_to_int(vm_obj_t obj)
{
    if (!vm_obj_is_num(obj))
    {
#if defined(VM_DEBUG_OBJ)
        __builtin_trap();
#else
        __builtin_unreachable(); 
#endif
    }
    return (vm_int_t)nanbox_to_double(obj);
}

static inline vm_number_t vm_obj_to_num(vm_obj_t obj)
{
    if (!vm_obj_is_num(obj))
    {
#if defined(VM_DEBUG_OBJ)
        __builtin_trap();
#else
        __builtin_unreachable(); 
#endif
    }
    return nanbox_to_double(obj);
}

static inline void *vm_obj_to_ptr(vm_obj_t obj)
{
    if (!vm_obj_is_ptr(obj))
    {
#if defined(VM_DEBUG_OBJ)
        __builtin_trap();
#else
        __builtin_unreachable(); 
#endif
    }
#if defined(VM_OS)
    return &os_mem_base[obj.as_bits.payload];
#else
    return nanbox_to_pointer(obj);
#endif
}
