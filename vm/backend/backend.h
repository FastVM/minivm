
#if !defined(VM_HEADER_BACKEND)
#define VM_HEADER_BACKEND

#include "../ir/ir.h"
#include "../obj.h"
#include "../std/std.h"

vm_std_value_t vm_run_main(vm_config_t *config, vm_block_t *entry, vm_blocks_t *blocks, vm_table_t *std);
vm_std_value_t vm_run_repl(vm_config_t *config, vm_block_t *entry, vm_blocks_t *blocks, vm_table_t *std);

#endif
