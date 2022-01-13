#include "state.h"
#include "config.h"

static inline size_t vm_state_strlen(const vm_char_t *str) {
  size_t ret = 0;
  while (*str != '\0') {
    ret += 1;
    str += 1;
  }
  return ret;
}

vm_obj_t vm_state_global_from(size_t len, const vm_char_t **args) {
  vm_gc_entry_t *global = vm_gc_static_array_new(len);
  for (size_t i = 0; i < len; i++) {
    vm_gc_entry_t *ent = vm_gc_static_array_new(vm_state_strlen(args[i]));
    for (const vm_char_t *src = args[i]; *src != '\0'; src++) {
      ent->arr[src - args[i]] = (vm_obj_t){.num = *src};
    }
    global->arr[i] = (vm_obj_t){.ptr = ent};
  }
  return (vm_obj_t){.ptr = global};
}

vm_state_t *vm_state_new(size_t len, const vm_char_t **args) {
  vm_state_t *state = vm_malloc(sizeof(vm_state_t));
  state->frames = vm_malloc(sizeof(vm_stack_frame_t) * VM_FRAMES_UNITS);
  state->globals = vm_malloc(sizeof(vm_obj_t) * VM_LOCALS_UNITS);
  state->globals[0] = vm_state_global_from(len, args);

  state->framenum = 0;
  state->nlocals = 0;

  state->frames[state->framenum].nlocals = 0;
  state->framenum += 1;
  state->frames[state->framenum].nlocals = 256;

  state->index = 0;
  state->nops = 0;

  return state;
}

void vm_state_set_ops(vm_state_t *state, size_t nops, const vm_opcode_t *ops) {
  state->nops = nops;
  state->ops = ops;
}

void vm_state_del(vm_state_t *state) {
  vm_free((void *)state->ops);
  vm_free(state->frames);
  vm_free(state->globals);
  vm_free(state);
}
