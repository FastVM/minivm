
#if !defined(VM_HEADER_BE_TB)
#define VM_HEADER_BE_TB

#include "../ir/ir.h"
#include "../lib.h"
#include "../obj.h"
#include "../std/io.h"
#include "../std/std.h"
#include "../ir/type.h"

vm_std_value_t vm_tb_run_main(vm_config_t *config, vm_block_t *entry, vm_blocks_t *blocks, vm_table_t *std);
vm_std_value_t vm_tb_run_repl(vm_config_t *config, vm_block_t *entry, vm_blocks_t *blocks, vm_table_t *std);

#endif
