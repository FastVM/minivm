
#include "./opt.h"

static void vm_opt_jump_from(vm_block_t *block) {
    if (block->branch.op == VM_BOP_JUMP) {
        vm_block_t *next = block->branch.targets[0];
        block->instrs = vm_realloc(block->instrs, sizeof(vm_instr_t) * (block->len + next->len));
        for (size_t i = 0; i < next->len; i++) {
            block->instrs[block->len++] = next->instrs[i];
        }
        block->branch = next->branch;
    }
}

void vm_opt_jump(vm_block_t *block) {
    vm_opt_pass(block, .post = &vm_opt_jump_from);
}
