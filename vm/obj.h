#pragma once

#include "lib.h"

union vm_obj_t;
typedef union vm_obj_t vm_obj_t;

union vm_obj_t {
    size_t num;
    vm_obj_t *pair;
};

#define vm_obj_num(n_) ((vm_obj_t){.num = n_})
#define vm_obj_pair(p_) ((vm_obj_t){.pair = p_})

#define vm_obj_to_num(o_) ((o_).num)
#define vm_obj_to_pair(o_) ((o_).pair)

#define vm_obj_car(o_) ((o_).pair[0])
#define vm_obj_cdr(o_) ((o_).pair[1])

