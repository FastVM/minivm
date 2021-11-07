#include "state.h"

static void vm_state_putchar_default(vm_state_t *state, char chr)
{
    vm_putchar(chr);
}

static inline size_t vm_state_strlen(const char *str)
{
    size_t ret = 0;
    while (*str != '\0')
    {
        ret += 1;
        str += 1;
    }
    return ret;
}

vm_gc_entry_t *vm_state_global_from(vm_gc_t *gc, size_t len, const char **args)
{
    vm_gc_entry_t *global = vm_gc_array_new(gc, len);
    for (size_t i = 0; i < len; i++) {
        vm_gc_entry_t *ent = vm_gc_string_new(gc, vm_state_strlen(args[i]));
        for (const char *src = args[i]; *src != '\0'; src++)
        {
            vm_gc_set_index(ent, vm_obj_of_int(src - args[i]), vm_obj_of_int(*src));
        }
        vm_gc_set_index(global, vm_obj_of_int(i), vm_obj_of_ptr(ent));
    }
    return global;
}

vm_state_t *vm_state_new(size_t len, const char **args)
{
    vm_state_t *state = vm_calloc(sizeof(vm_state_t));
    vm_gc_t *gc = vm_calloc(sizeof(vm_gc_t));
    vm_gc_start(gc);
    state->gc = gc;
    state->global = vm_state_global_from(state->gc, len, args);
    state->putchar = &vm_state_putchar_default;
    return state;
}

void vm_state_del(vm_state_t *state)
{
    vm_gc_stop(state->gc);
    vm_free(state->gc);
    vm_free(state);
}
