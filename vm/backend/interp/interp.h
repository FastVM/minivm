
#if !defined(VM_HEADER_BACKEND_INTERP)
#define VM_HEADER_BACKEND_INTERP

#include "../../ir/ir.h"
#include "../../obj.h"

vm_std_value_t vm_interp_run(vm_config_t *config, vm_block_t *entry, vm_blocks_t *blocks, vm_table_t *std);

#endif
