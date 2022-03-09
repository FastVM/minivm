#pragma once
#include "lib.h"

typedef int64_t vm_number_t;
typedef size_t vm_counter_t;

union vm_obj_t;
typedef union vm_obj_t vm_obj_t;

#define vm_obj_num(v_) ((vm_obj_t){.num = (v_) * 2})
#define vm_obj_to_num(o_) ((o_).num / 2)

union vm_obj_t {
  vm_number_t num;
  size_t ptr;
};
