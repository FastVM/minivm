
#include "backend.h"
#include "interp/interp.h"

vm_std_value_t vm_run_main(vm_config_t *config, vm_block_t *entry, vm_blocks_t *blocks, vm_table_t *std) {
    vm_std_value_t val = vm_run_repl(config, entry, blocks, std);
    if (val.tag == VM_TAG_ERROR) {
        fprintf(stderr, "error: %s\n", val.value.str);
    }
    return val;
}

vm_std_value_t vm_run_repl(vm_config_t *config, vm_block_t *entry, vm_blocks_t *blocks, vm_table_t *std) {
    return vm_interp_run(config, entry, blocks, std);
}
