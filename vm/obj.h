#if !defined(VM_HEADER_OBJ)
#define VM_HEADER_OBJ

#include "vm.h"

vm_obj_t vm_obj_of_string(vm_t *vm, const char *str);
uint32_t vm_obj_hash(vm_obj_t value);

vm_obj_table_t *vm_table_new(vm_t *vm);
void vm_table_set(vm_obj_table_t *table, vm_obj_t key, vm_obj_t value);
vm_obj_t vm_table_get(vm_obj_table_t *table, vm_obj_t key);

#endif
