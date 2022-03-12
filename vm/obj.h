#pragma once

#include "lib.h"

typedef int32_t vm_number_t;
typedef size_t vm_counter_t;

typedef int32_t vm_obj_t;

#define vm_obj_num(v_) ((v_) * 2)
#define vm_obj_to_num(o_) ((o_) / 2)
