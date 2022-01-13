#pragma once

#include "libc.h"
typedef char vm_char_t;
typedef int32_t vm_int_t;
typedef int32_t vm_loc_t;
typedef int32_t vm_number_t;

typedef union {
    bool log;
    int32_t num;
    uint32_t ptr;
} vm_obj_t;