#include "int3.h"

vm_state_t *vm_state_init(size_t nregs) {
    vm_state_t *ret = vm_malloc(sizeof(vm_state_t));
    ret->framesize = 256;
    ret->nlocals = nregs;
    ret->locals = vm_malloc(ret->nlocals);
    ret->head = vm_malloc(ret->nlocals / ret->framesize);
    return ret;
}

void vm_state_deinit(vm_state_t *state) {
    vm_free(state->head);
    vm_free(state->locals);
    vm_free(state);
}

void vm_run(vm_state_t *state, vm_block_t *block) {
    
}

