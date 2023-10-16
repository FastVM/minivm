
#include "./opt.h"

void vm_opt_info(vm_block_t *block) {
    vm_opt_pass(block, .final = &vm_block_info);
}
