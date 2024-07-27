#if !defined(VM_HEADER_OBJ)
#define VM_HEADER_OBJ

#include "lib.h"

struct vm_table_pair_t;
typedef struct vm_table_pair_t vm_table_pair_t;

bool vm_obj_eq(vm_obj_t lhs, vm_obj_t rhs);
bool vm_obj_is_int(vm_obj_t val);

vm_obj_table_t *vm_table_new(vm_t *vm);
void vm_table_set(vm_obj_table_t *table, vm_obj_t key, vm_obj_t value);
vm_obj_t vm_table_get(vm_obj_table_t *table, vm_obj_t key);

#endif
