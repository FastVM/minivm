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