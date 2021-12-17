#pragma once

#include "libc.h"

typedef char vm_char_t;
typedef int vm_int_t;
typedef int vm_loc_t;
typedef int32_t vm_number_t;

typedef struct {
  uint8_t type : 2;
  int32_t value : 30;
} vm_obj_t;

enum {
  VM_TYPE_NONE = 3,
  VM_TYPE_BOOL = 2,
  VM_TYPE_ARRAY = 1,
  VM_TYPE_NUMBER = 0,
};
