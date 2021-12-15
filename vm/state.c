#include "state.h"
#include "config.h"

static void vm_state_putchar_default(vm_state_t *state, vm_char_t chr)
{
    vm_putchar(chr);
}

static inline size_t vm_state_strlen(const vm_char_t *str)
{
    size_t ret = 0;
    while (*str != '\0')
    {
        ret += 1;
        str += 1;
    }
    return ret;
}

vm_obj_t vm_state_global_from(vm_gc_t *gc, size_t len, const vm_char_t **args)
{
    vm_gc_entry_t *global = vm_gc_static_array_new(gc, len);
    for (size_t i = 0; i < len; i++) {
        vm_gc_entry_t *ent = vm_gc_static_array_new(gc, vm_state_strlen(args[i]));
        for (const vm_char_t *src = args[i]; *src != '\0'; src++)
        {
            vm_gc_set_index(gc, ent, src - args[i], vm_obj_of_int(*src));
        }
        vm_gc_set_index(gc, global, i, vm_obj_of_ptr(gc, ent));
    }
    return vm_obj_of_ptr(gc, global);
}

vm_state_t *vm_state_new(size_t len, const vm_char_t **args)
{
    vm_state_t *state = vm_malloc(sizeof(vm_state_t));
    vm_gc_start(&state->gc);
    state->frames = vm_malloc(sizeof(vm_stack_frame_t) * VM_FRAMES_UNITS);
    state->globals = vm_malloc(sizeof(vm_obj_t) * VM_LOCALS_UNITS);
    state->xops = vm_malloc(sizeof(vm_opcode_t) * VM_OPS_UNITS);
    state->putchar = &vm_state_putchar_default;
    state->globals[0] = vm_state_global_from(&state->gc, len, args);

    state->frame = state->frames;
    state->nlocals = 0;
    
    state->frame->nlocals = 0;
    state->frame += 1;
    state->frame->nlocals = 256;

    state->index = 0;
    state->nops = 0;
    state->ops = NULL;

    return state;
}

void vm_state_set_ops(vm_state_t *state, size_t nops, const vm_opcode_t *ops)
{
    state->nops = nops;
    state->ops = ops;
}

void vm_state_del(vm_state_t *state)
{
    vm_gc_stop(&state->gc);
    vm_free(state->frames);
    vm_free(state->globals);
    vm_free(state->xops);
    vm_free(state);
}
