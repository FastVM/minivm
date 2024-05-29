
#if !defined(VM_HEADER_RBLOCK)
#define VM_HEADER_RBLOCK

#include "ir.h"

void vm_rblock_reset(vm_rblock_t *rblock);
vm_block_t *vm_rblock_version(vm_config_t *config, vm_blocks_t *blocks, vm_rblock_t *rblock);

#endif
