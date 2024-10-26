
#if !defined(VM_HEADER_MATH)
#define VM_HEADER_MATH

#include "vm.h"

bool vm_obj_unsafe_eq(vm_obj_t v1, vm_obj_t v2);
bool vm_obj_unsafe_lt(vm_obj_t v1, vm_obj_t v2);
bool vm_obj_unsafe_le(vm_obj_t v1, vm_obj_t v2);

vm_obj_t vm_obj_eq(vm_t *vm, vm_obj_t v1, vm_obj_t v2);
vm_obj_t vm_obj_lt(vm_t *vm, vm_obj_t v1, vm_obj_t v2);

#endif
