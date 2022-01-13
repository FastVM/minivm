#pragma once

#if defined(VM_USE_FLOAT)
#include "nanbox.h"
typedef char vm_char_t;
typedef int vm_int_t;
typedef int vm_loc_t;
typedef double vm_number_t;
typedef vm_nanbox_t vm_obj_t;
#else
#include "libc.h"
typedef char vm_char_t;
typedef int vm_int_t;
typedef int vm_loc_t;
typedef int vm_number_t;

enum {
    VM_TYPE_NUM = 0,
    VM_TYPE_PTR = 1,
    VM_TYPE_BOOL = 2,
    VM_TYPE_NONE = 3,
};

typedef union {
    struct {
        uint8_t itype: 2;
        int32_t ival: 30;
    };
    struct {
        uint8_t ptype: 2;
        uint32_t pval: 30;
    };
} vm_obj_t;
#endif