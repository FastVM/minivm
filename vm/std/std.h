#if !defined(VM_HEADER_STD_STD)
#define VM_HEADER_STD_STD

#include "../obj.h"
#include "./io.h"

void vm_value_buffer_tostring(vm_io_buffer_t *buf, vm_std_value_t value);
vm_table_t *vm_std_new(void);

#endif
