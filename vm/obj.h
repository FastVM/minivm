#pragma once

#include <vm/nanbox.h>
typedef nanbox_t vm_obj_t;
typedef int vm_loc_t;
typedef double vm_number_t;

static inline int vm_obj_to_int(vm_obj_t obj)
{
	return (int) nanbox_to_double(obj);
}

static inline vm_number_t vm_obj_to_num(vm_obj_t obj)
{
	return nanbox_to_double(obj);
}

static inline vm_obj_t vm_obj_of_int(int obj)
{
	return nanbox_from_double((vm_number_t)obj);
}

static inline vm_obj_t vm_obj_of_num(vm_number_t obj)
{
	return nanbox_from_double(obj);
}

static inline long vm_obj_to_ptr(vm_obj_t obj)
{
	return obj.as_int64 - NANBOX_MIN_AUX;
}

static inline int vm_obj_to_fun(vm_obj_t obj)
{
	return (int) nanbox_to_double(obj);
}

static inline vm_obj_t vm_obj_of_ptr(long obj)
{
	vm_obj_t ret;
	ret.as_int64 = obj + NANBOX_MIN_AUX;
	return ret;
}

static inline vm_obj_t vm_obj_of_fun(int obj)
{
	return nanbox_from_double((double) obj);
}

static inline vm_obj_t vm_obj_of_dead()
{
	return nanbox_empty();
}

static inline bool vm_obj_is_num(vm_obj_t obj)
{
	return nanbox_is_number(obj);
}

static inline bool vm_obj_is_ptr(vm_obj_t obj)
{
	return obj.as_int64 >= NANBOX_MIN_AUX && obj.as_int64 <= NANBOX_MAX_AUX;
}

static inline bool vm_obj_is_dead(vm_obj_t obj)
{
	return nanbox_is_empty(obj);
}

static inline vm_obj_t vm_obj_num_add(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_obj_of_num(vm_obj_to_num(lhs) + vm_obj_to_num(rhs));
}

static inline vm_obj_t vm_obj_num_addc(vm_obj_t lhs, int rhs)
{
	return vm_obj_of_num(vm_obj_to_num(lhs) + rhs);
}

static inline vm_obj_t vm_obj_num_sub(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_obj_of_num(vm_obj_to_num(lhs) - vm_obj_to_num(rhs));
}

static inline vm_obj_t vm_obj_num_subc(vm_obj_t lhs, int rhs)
{
	return vm_obj_of_num(vm_obj_to_num(lhs) - rhs);
}

static inline vm_obj_t vm_obj_num_mul(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_obj_of_num(vm_obj_to_num(lhs) * vm_obj_to_num(rhs));
}

static inline vm_obj_t vm_obj_num_mulc(vm_obj_t lhs, int rhs)
{
	return vm_obj_of_num(vm_obj_to_num(lhs) * rhs);
}

static inline vm_obj_t vm_obj_num_div(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_obj_of_num(vm_obj_to_num(lhs) / vm_obj_to_num(rhs));
}

static inline vm_obj_t vm_obj_num_divc(vm_obj_t lhs, int rhs)
{
	return vm_obj_of_num(vm_obj_to_num(lhs) / rhs);
}

static inline vm_obj_t vm_obj_num_mod(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_obj_of_num(fmod(vm_obj_to_num(lhs), vm_obj_to_num(rhs)));
}

static inline vm_obj_t vm_obj_num_modc(vm_obj_t lhs, int rhs)
{
	return vm_obj_of_num(fmod(vm_obj_to_num(lhs), rhs));
}

static inline bool vm_obj_lt(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_obj_to_num(lhs) < vm_obj_to_num(rhs);
}

static inline bool vm_obj_ilt(vm_obj_t lhs, int rhs)
{
	return vm_obj_to_num(lhs) < rhs;
}

static inline bool vm_obj_gt(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_obj_to_num(lhs) > vm_obj_to_num(rhs);
}

static inline bool vm_obj_igt(vm_obj_t lhs, int rhs)
{
	return vm_obj_to_num(lhs) > rhs;
}

static inline bool vm_obj_lte(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_obj_to_num(lhs) <= vm_obj_to_num(rhs);
}

static inline bool vm_obj_ilte(vm_obj_t lhs, int rhs)
{
	return vm_obj_to_num(lhs) <= rhs;
}

static inline bool vm_obj_gte(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_obj_to_num(lhs) >= vm_obj_to_num(rhs);
}

static inline bool vm_obj_igte(vm_obj_t lhs, int rhs)
{
	return vm_obj_to_num(lhs) >= rhs;
}

static inline bool vm_obj_eq(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_obj_to_num(lhs) == vm_obj_to_num(rhs);
}

static inline bool vm_obj_ieq(vm_obj_t lhs, int rhs)
{
	return vm_obj_to_num(lhs) == rhs;
}

static inline bool vm_obj_neq(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_obj_to_num(lhs) != vm_obj_to_num(rhs);
}

static inline bool vm_obj_ineq(vm_obj_t lhs, int rhs)
{
	return vm_obj_to_num(lhs) != rhs;
}