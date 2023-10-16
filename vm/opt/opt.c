
#include "./opt.h"

static void vm_opt_pass_by_from(vm_opt_jump_done_t *state, vm_block_t *block) {
    if (block == NULL) {
        return;
    }
    for (size_t i = 0; i < state->nblocks; i++) {
        if (state->blocks[i] == block) {
            return;
        }
    }
    if (state->nblocks + 2 >= state->blocks_alloc) {
        state->blocks_alloc = (state->nblocks + 2) * 2;
        state->blocks = vm_realloc(state->blocks, sizeof(vm_block_t *) * state->blocks_alloc);
    }
    state->blocks[state->nblocks++] = block;
    if (state->pass.pre != NULL) {
        state->pass.pre(block);
    }
    for (size_t i = 0; i < 2; i++) {
        vm_opt_pass_by_from(state, block->branch.targets[i]);
    }
    if (state->pass.post != NULL) {
        state->pass.post(block);
    }
}

void vm_opt_pass_by(vm_block_t *block, vm_opt_pass_t pass) {
    vm_opt_jump_done_t state = (vm_opt_jump_done_t) {
        .nblocks = 0,
        .blocks = NULL,
        .blocks_alloc = 0,
        .pass = pass,
    };
    vm_opt_pass_by_from(&state, block);
    if (pass.final != NULL) {
        pass.final(state.nblocks, state.blocks);
    }
    vm_free(state.blocks);
}

void vm_opt_do_pass(const char *pass, vm_block_t *block) {
    if (!strcmp(pass, "3")) {
        vm_opt_jump(block);
        vm_opt_inline(block);
        vm_opt_unused(block);
    } else if (!strcmp(pass, "2")) {
        vm_opt_jump(block);
        vm_opt_inline(block);
    } else if (!strcmp(pass, "1")) {
        vm_opt_jump(block);
    } else if (!strcmp(pass, "0")) {
    }
}

void vm_opt_do_passes(const char *passes, vm_block_t *block) {
    char buf[256];
    size_t head = 0;
    for (size_t i = 0; passes[i] != '\0'; i++) {
        char got = passes[i];
        if (got == ',') {
            buf[head++] = '\0';
            vm_opt_do_pass(buf, block);
            head = 0;
        } else {
            buf[head++] = got;
        }
    }
    buf[head++] = '\0';
    vm_opt_do_pass(buf, block);
    vm_opt_info(block);
}
