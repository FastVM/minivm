#pragma once

#include "lib.h"

#include <gmp.h>

typedef int64_t vm_int_t;
typedef double vm_float_t;

struct vm_value_t;
typedef struct vm_value_t vm_value_t;

struct vm_value_t {
  int64_t intval;
};

#define vm_value_from_int(n) ((vm_value_t) {.intval = (n)})
#define vm_value_from_func(n) ((vm_value_t) {.intval = (n)})

#define vm_value_to_bool(x) (vm_value_to_int(x) != 0)
#define vm_value_to_int(n) ((n).intval)
#define vm_value_to_func(n) ((n).intval)

#define vm_value_add(x, y) vm_value_from_int(vm_value_to_int(x) + vm_value_to_int(y))
#define vm_value_sub(x, y) vm_value_from_int(vm_value_to_int(x) - vm_value_to_int(y))
#define vm_value_mul(x, y) vm_value_from_int(vm_value_to_int(x) * vm_value_to_int(y))
#define vm_value_div(x, y) vm_value_from_int(vm_value_to_int(x) / vm_value_to_int(y))
#define vm_value_mod(x, y) vm_value_from_int(vm_value_to_int(x) % vm_value_to_int(y))

#define vm_value_is_equal(x, y) (vm_value_to_int(x) == vm_value_to_int(y))
#define vm_value_is_less(x, y) (vm_value_to_int(x) < vm_value_to_int(y))
