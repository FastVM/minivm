#include "state.h"

static void vm_state_putchar_default(vm_state_t *state, char chr)
{
    vm_putchar(chr);
}

vm_state_t *vm_state_new(void)
{
    vm_state_t *state = vm_calloc(sizeof(vm_state_t));
    vm_gc_t *gc = vm_calloc(sizeof(vm_gc_t));
    vm_gc_start(gc);
    state->gc = gc;
    state->global = vm_gc_map_new(gc);
    state->putchar = &vm_state_putchar_default;
    return state;
}

void vm_state_del(vm_state_t *state)
{
    vm_gc_stop(state->gc);
    vm_free(state->gc);
    vm_free(state);
}
