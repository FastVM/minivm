
#include "sys.h"

vm_obj_t vm_sys_call(void *sys, vm_obj_t arg)
{
	return vm_obj_of_num(0);
}

void vm_sys_mark(void *sys)
{
}

void *vm_sys_init(vm_gc_t *gc)
{
	return NULL;
}

void vm_sys_deinit(void *obj)
{
}
