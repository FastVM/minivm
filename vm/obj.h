#if !defined(VM_HEADER_OBJ)
#define VM_HEADER_OBJ

#include "vm.h"

bool vm_obj_is_nil(vm_obj_t obj);
bool vm_obj_is_boolean(vm_obj_t obj);
bool vm_obj_is_number(vm_obj_t obj);
bool vm_obj_is_buffer(vm_obj_t obj);
bool vm_obj_is_table(vm_obj_t obj);
bool vm_obj_is_closure(vm_obj_t obj);
bool vm_obj_is_ffi(vm_obj_t obj);
bool vm_obj_is_block(vm_obj_t obj);
bool vm_obj_is_error(vm_obj_t obj);

bool vm_obj_get_boolean(vm_obj_t obj);
double vm_obj_get_number(vm_obj_t obj);
vm_io_buffer_t *vm_obj_get_buffer(vm_obj_t obj);
vm_obj_table_t *vm_obj_get_table(vm_obj_t obj);
vm_obj_closure_t *vm_obj_get_closure(vm_obj_t obj);
vm_ffi_t *vm_obj_get_ffi(vm_obj_t obj);
vm_ir_block_t *vm_obj_get_block(vm_obj_t obj);
vm_error_t *vm_obj_get_error(vm_obj_t obj);

vm_obj_t vm_obj_of_nil(void);
vm_obj_t vm_obj_of_boolean(bool from);
vm_obj_t vm_obj_of_number(double from);
vm_obj_t vm_obj_of_buffer(vm_io_buffer_t *from);
vm_obj_t vm_obj_of_table(vm_obj_table_t *from);
vm_obj_t vm_obj_of_closure(vm_obj_closure_t *from);
vm_obj_t vm_obj_of_ffi(vm_ffi_t *from);
vm_obj_t vm_obj_of_block(vm_ir_block_t *from);
vm_obj_t vm_obj_of_error(vm_error_t *from);

// extras
vm_obj_t vm_obj_of_string(vm_t *vm, const char *str);
uint32_t vm_obj_hash(vm_obj_t value);

#endif
