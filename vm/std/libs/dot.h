
#if !defined(VM_HEADER_STD_LIBS_DOT)
#define VM_HEADER_STD_LIBS_DOT
#include "../std.h"
#include "../../ir.h"

void vm_dot_block(FILE *file, vm_block_t *block);

vm_std_value_t vm_std_dot_parse(vm_std_value_t *args);
vm_std_value_t vm_std_dot_file(vm_std_value_t *args);

#endif
