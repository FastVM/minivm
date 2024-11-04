
#include "vm.h"

vm_obj_table_t *vm_table_new(vm_t *vm);
void vm_table_set(vm_obj_table_t *table, vm_obj_t key, vm_obj_t value);
vm_obj_t vm_table_get(vm_obj_table_t *table, vm_obj_t key);

#include "primes.inc"
