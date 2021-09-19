#pragma once

#include <vm/nanbox.h>
typedef nanbox_t vm_obj_t;
typedef double vm_number_t;
typedef int vm_loc_t;

#define vm_obj_to_num(obj) (nanbox_to_double(obj))
#define vm_obj_of_num(obj) (nanbox_from_double(obj))
#define vm_obj_is_num(obj) (nanbox_is_double(obj))

#define vm_obj_to_ptr(obj) (nanbox_to_pointer(obj))
#define vm_obj_of_ptr(ptr) (nanbox_from_pointer(ptr))
#define vm_obj_is_ptr(obj) (nanbox_is_pointer(obj))

#define vm_obj_to_fun(obj) (nanbox_to_int(obj))
#define vm_obj_of_fun(fun) (nanbox_from_int(fun))

#define vm_obj_of_dead() (nanbox_empty())
#define vm_obj_is_dead(val) (nanbox_is_empty(val))

#define vm_obj_num_add(lhs, rhs) (vm_obj_of_num(vm_obj_to_num(lhs) + vm_obj_to_num(rhs)))
#define vm_obj_num_sub(lhs, rhs) (vm_obj_of_num(vm_obj_to_num(lhs) - vm_obj_to_num(rhs)))
#define vm_obj_num_mul(lhs, rhs) (vm_obj_of_num(vm_obj_to_num(lhs) * vm_obj_to_num(rhs)))
#define vm_obj_num_div(lhs, rhs) (vm_obj_of_num(vm_obj_to_num(lhs) / vm_obj_to_num(rhs)))
#define vm_obj_num_mod(lhs, rhs) (vm_obj_of_num(fmod(vm_obj_to_num(lhs), vm_obj_to_num(rhs))))

#define vm_obj_num_addc(lhs, rhs) (vm_obj_of_num(vm_obj_to_num(lhs) + (rhs)))
#define vm_obj_num_subc(lhs, rhs) (vm_obj_of_num(vm_obj_to_num(lhs) - (rhs)))
#define vm_obj_num_mulc(lhs, rhs) (vm_obj_of_num(vm_obj_to_num(lhs) * (rhs)))
#define vm_obj_num_divc(lhs, rhs) (vm_obj_of_num(vm_obj_to_num(lhs) / (rhs)))
#define vm_obj_num_modc(lhs, rhs) (vm_obj_of_num(fmod(vm_obj_to_num(lhs), (rhs))))
