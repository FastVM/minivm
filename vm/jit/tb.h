
#if !defined(VM_HEADER_BE_TB)
#define VM_HEADER_BE_TB

#include "../ir.h"
#include "../lib.h"
#include "../obj.h"

vm_std_value_t vm_x64_run(vm_block_t *block, vm_table_t *std, vm_std_value_t *args);

#endif
