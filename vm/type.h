#pragma once

#include "libc.h"
typedef char vm_char_t;
typedef int32_t vm_int_t;
typedef int32_t vm_loc_t;
typedef int32_t vm_number_t;

typedef union {
    bool log;
    intptr_t num;
    vm_gc_entry_t *ptr;
} vm_obj_t;