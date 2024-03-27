
#include "./tb.h"

#include "../../vendor/cuik/tb/include/tb.h"

#define VM_TB_CC TB_CDECL
#define VM_TB_TYPE_VALUE TB_TYPE_I64

#include "tb_dyn.h"
#include "tb_ver.h"

vm_std_value_t vm_tb_run_main(vm_config_t *config, vm_block_t *entry, vm_blocks_t *blocks, vm_table_t *std) {
    vm_std_value_t val = vm_tb_run_repl(config, entry, blocks, std);
    if (vm_type_eq(val.tag, VM_TYPE_ERROR)) {
        fprintf(stderr, "error: %s\n", val.value.str);
    }
    return val;
}

vm_std_value_t vm_tb_run_repl(vm_config_t *config, vm_block_t *entry, vm_blocks_t *blocks, vm_table_t *std) {
    vm_std_value_t value;
    if (config->tb_use_lbbv) {
        vm_tb_ver_state_t *state = vm_malloc(sizeof(vm_tb_ver_state_t));
        state->std = std;
        state->config = config;
        state->blocks = blocks;
        state->module = NULL;
        state->regs = NULL;

        vm_tb_ver_func_t *fn = (vm_tb_ver_func_t *)vm_tb_ver_full_comp(state, entry);

#if defined(_WIN32)
        state->vm_caller(&value, fn);
#else
        value = fn();
#endif
    } else {
    }

    for (size_t i = 0; i < blocks->len; i++) {
        vm_block_t *block = blocks->blocks[i];
        // for (size_t j = 0; j < block->cache.len; j++) {
        //     vm_rblock_reset(block->cache.keys[j]);
        //     vm_free_block_sub(block->cache.values[j]);
        // }
        block->cache.len = 0;
    }

    return *(vm_std_value_t *)&value;
}
