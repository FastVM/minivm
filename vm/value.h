#pragma once

#include "lib.h"

typedef int64_t vm_int_t;
typedef double vm_float_t;

union vm_value_t;
typedef union vm_value_t vm_value_t;

union vm_value_t {
  vm_int_t s;
  vm_float_t f;
};

#define vm_value_add(x, y) ()
