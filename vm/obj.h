#if !defined(VM_HEADER_OBJ)
#define VM_HEADER_OBJ

#include "lib.h"

struct vm_table_pair_t;
typedef struct vm_table_pair_t vm_table_pair_t;

bool vm_obj_eq(vm_obj_t lhs, vm_obj_t rhs);
bool vm_value_is_int(vm_obj_t val);
int64_t vm_value_to_i64(vm_obj_t arg);
double vm_value_to_f64(vm_obj_t arg);
bool vm_value_can_to_n64(vm_obj_t val);

vm_table_t *vm_table_new(vm_t *vm);
vm_table_pair_t *vm_table_lookup(vm_table_t *table, vm_obj_t key);
void vm_table_iset(vm_table_t *table, uint64_t key_ival, uint64_t val_ival, vm_tag_t key_tag, vm_tag_t val_tag);
void vm_table_set(vm_table_t *table, vm_obj_t key, vm_obj_t value);
void vm_table_set_pair(vm_table_t *table, vm_table_pair_t *pair);
void vm_table_get_pair(vm_table_t *table, vm_table_pair_t *pair);

#endif
