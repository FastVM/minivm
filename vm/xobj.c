#include "obj.h"

int vm_xobj_to_int(vm_obj_t obj)
{
	return (int)nanbox_to_double(obj);
}

vm_number_t vm_xobj_to_num(vm_obj_t obj)
{
	return nanbox_to_double(obj);
}

vm_obj_t vm_xobj_of_int(int obj)
{
	return nanbox_from_double((vm_number_t)obj);
}

vm_obj_t vm_xobj_of_num(vm_number_t obj)
{
	return nanbox_from_double(obj);
}

void* vm_xobj_to_ptr(vm_obj_t obj)
{
	return nanbox_to_pointer(obj);
}

int vm_xobj_to_fun(vm_obj_t obj)
{
	return nanbox_to_int(obj);
}

vm_obj_t vm_xobj_of_ptr(void* obj)
{
	return nanbox_from_pointer(obj);
}

vm_obj_t vm_xobj_of_fun(int obj)
{
	return nanbox_from_int(obj);
}

vm_obj_t vm_xobj_of_dead()
{
	return nanbox_empty();
}

bool vm_xobj_is_num(vm_obj_t obj)
{
	return nanbox_is_number(obj);
}

bool vm_xobj_is_ptr(vm_obj_t obj)
{
	return nanbox_is_pointer(obj);
}

bool vm_xobj_is_dead(vm_obj_t obj)
{
	return nanbox_is_empty(obj);
}

vm_obj_t vm_xobj_num_add(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_xobj_of_num(vm_xobj_to_num(lhs) + vm_xobj_to_num(rhs));
}

vm_obj_t vm_xobj_num_addc(vm_obj_t lhs, int rhs)
{
	return vm_xobj_of_num(vm_xobj_to_num(lhs) + rhs);
}

vm_obj_t vm_xobj_num_sub(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_xobj_of_num(vm_xobj_to_num(lhs) - vm_xobj_to_num(rhs));
}

vm_obj_t vm_xobj_num_subc(vm_obj_t lhs, int rhs)
{
	return vm_xobj_of_num(vm_xobj_to_num(lhs) - rhs);
}

vm_obj_t vm_xobj_num_mul(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_xobj_of_num(vm_xobj_to_num(lhs) * vm_xobj_to_num(rhs));
}

vm_obj_t vm_xobj_num_mulc(vm_obj_t lhs, int rhs)
{
	return vm_xobj_of_num(vm_xobj_to_num(lhs) * rhs);
}

vm_obj_t vm_xobj_num_div(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_xobj_of_num(vm_xobj_to_num(lhs) / vm_xobj_to_num(rhs));
}

vm_obj_t vm_xobj_num_divc(vm_obj_t lhs, int rhs)
{
	return vm_xobj_of_num(vm_xobj_to_num(lhs) / rhs);
}

vm_obj_t vm_xobj_num_mod(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_xobj_of_num(fmod(vm_xobj_to_num(lhs), vm_xobj_to_num(rhs)));
}

vm_obj_t vm_xobj_num_modc(vm_obj_t lhs, int rhs)
{
	return vm_xobj_of_num(fmod(vm_xobj_to_num(lhs), rhs));
}

bool vm_xobj_lt(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_xobj_to_num(lhs) < vm_xobj_to_num(rhs);
}

bool vm_xobj_ilt(vm_obj_t lhs, int rhs)
{
	return vm_xobj_to_num(lhs) < rhs;
}

bool vm_xobj_gt(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_xobj_to_num(lhs) > vm_xobj_to_num(rhs);
}

bool vm_xobj_igt(vm_obj_t lhs, int rhs)
{
	return vm_xobj_to_num(lhs) > rhs;
}

bool vm_xobj_lte(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_xobj_to_num(lhs) <= vm_xobj_to_num(rhs);
}

bool vm_xobj_ilte(vm_obj_t lhs, int rhs)
{
	return vm_xobj_to_num(lhs) <= rhs;
}

bool vm_xobj_gte(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_xobj_to_num(lhs) >= vm_xobj_to_num(rhs);
}

bool vm_xobj_igte(vm_obj_t lhs, int rhs)
{
	return vm_xobj_to_num(lhs) >= rhs;
}

bool vm_xobj_eq(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_xobj_to_num(lhs) == vm_xobj_to_num(rhs);
}

bool vm_xobj_ieq(vm_obj_t lhs, int rhs)
{
	return vm_xobj_to_num(lhs) == rhs;
}

bool vm_xobj_neq(vm_obj_t lhs, vm_obj_t rhs)
{
	return vm_xobj_to_num(lhs) != vm_xobj_to_num(rhs);
}

bool vm_xobj_ineq(vm_obj_t lhs, int rhs)
{
	return vm_xobj_to_num(lhs) != rhs;
}