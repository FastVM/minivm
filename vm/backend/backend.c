
#include "backend.h"
#include "tb/tb.h"
#include "interp/interp.h"

vm_std_value_t vm_run_main(vm_config_t *config, vm_block_t *entry, vm_blocks_t *blocks, vm_table_t *std) {
    vm_std_value_t val = vm_run_repl(config, entry, blocks, std);
    if (vm_type_eq(val.tag, VM_TAG_ERROR)) {
        fprintf(stderr, "error: %s\n", val.value.str);
    }
    return val;
}

vm_std_value_t vm_run_repl(vm_config_t *config, vm_block_t *entry, vm_blocks_t *blocks, vm_table_t *std) {
    switch (config->target) {
        // interp based
        case VM_TARGET_INTERP: {
            return vm_interp_run(config, entry, blocks, std);
        }
        // tb based
        case VM_TARGET_TB:
    #if defined(EMSCRIPTEN)
        case VM_TARGET_TB_EMCC:
    #else
    #if defined(VM_USE_TCC)
        case VM_TARGET_TB_TCC:
    #endif
        case VM_TARGET_TB_CC:
        case VM_TARGET_TB_GCC:
        case VM_TARGET_TB_CLANG:
    #endif
        {
            return vm_tb_run(config, entry, blocks, std);
        }
    }
}
