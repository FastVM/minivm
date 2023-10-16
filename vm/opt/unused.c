
#include "./opt.h"

void vm_opt_unused_block(vm_block_t *block) {
    bool used[256] = {0};
    vm_branch_t branch = block->branch;
    for (size_t i = 0; i < 2; i++) {
        if (block->branch.targets[i] != NULL) {
            for (size_t j = 0; j < block->branch.targets[i]->nargs; j++) {
                used[block->branch.targets[i]->args[j].reg] = true;
            }
        }
    }
    for (size_t i = 0; block->branch.args[i].type != VM_ARG_NONE; i++) {
        if (block->branch.args[i].type == VM_ARG_REG) {
            used[block->branch.args[i].reg] = true;
        }
    }
    // for (size_t j = 0; j < 256; j++) {
    //     if (used[j]) {
    //         fprintf(stderr, "[%zu] ", j);
    //     }
    // }
    // vm_print_branch(stderr, block->branch);
    // fprintf(stderr, "\n");
    for (ptrdiff_t i = (ptrdiff_t) block->len - 1; i >= 0; i--) {
        vm_instr_t *instr = &block->instrs[i];
        // for (size_t j = 0; j < 256; j++) {
        //     if (used[j]) {
        //         fprintf(stderr, "[%zu] ", j);
        //     }
        // }
        // vm_print_instr(stderr, *instr);
        // fprintf(stderr, "\n");
        if (instr->out.type == VM_ARG_REG) {
            if (!used[instr->out.reg]) {
                *instr = (vm_instr_t) {
                    .op = VM_IOP_NOP,
                };
                continue;
            }
        }
        for (size_t j = 0; instr->args[j].type != VM_ARG_NONE; j++) {
            if (instr->args[j].type == VM_ARG_REG) {
                used[instr->args[j].reg] = true;
            }  
        }
    }
    // fprintf(stderr, "---\n");
}

void vm_opt_unused_remove_nop(vm_block_t *block) {
    size_t write = 0;
    for (size_t read = 0; read < block->len; read++) {
        if (block->instrs[read].op != VM_IOP_NOP) {
            block->instrs[write++] = block->instrs[read];
        }
    }
    block->len = write;
}

void vm_opt_unused(vm_block_t *block) {
    vm_opt_info(block);
    vm_opt_pass(block, .post = &vm_opt_unused_block);
    vm_opt_pass(block, .post = &vm_opt_unused_remove_nop);
}
