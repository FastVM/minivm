#include "state.h"
#include "config.h"

static void vm_state_putchar_default(vm_state_t *state, vm_char_t chr) {
  vm_putchar(chr);
}

static inline size_t vm_state_strlen(const vm_char_t *str) {
  size_t ret = 0;
  while (*str != '\0') {
    ret += 1;
    str += 1;
  }
  return ret;
}

vm_obj_t vm_state_global_from(vm_gc_t *gc, size_t len, const vm_char_t **args) {
  vm_gc_entry_t *global = vm_gc_static_array_new(gc, len);
  for (size_t i = 0; i < len; i++) {
    vm_gc_entry_t *ent = vm_gc_static_array_new(gc, vm_state_strlen(args[i]));
    for (const vm_char_t *src = args[i]; *src != '\0'; src++) {
      vm_gc_set_index(gc, ent, src - args[i], vm_obj_of_num(*src));
    }
    vm_gc_set_index(gc, global, i, vm_obj_of_ptr(gc, ent));
  }
  return vm_obj_of_ptr(gc, global);
}

vm_state_t *vm_state_new(size_t len, const vm_char_t **args) {
  vm_state_t *state = vm_malloc(sizeof(vm_state_t));
  vm_gc_start(&state->gc);
  state->frames = vm_malloc(sizeof(vm_stack_frame_t) * VM_FRAMES_UNITS);
  state->globals = vm_malloc(sizeof(vm_obj_t) * VM_LOCALS_UNITS);
  state->globals[0] = vm_state_global_from(&state->gc, len, args);

  state->framenum = 0;
  state->nlocals = 0;

  state->frames[state->framenum].nlocals = 0;
  state->framenum += 1;
  state->frames[state->framenum].nlocals = 256;

  state->index = 0;
  state->nops = 0;
  state->ops = NULL;

  state->tmphead = 0;
  state->tmpsize = 16;
  state->tmpbuf = vm_malloc(sizeof(vm_obj_t) * state->tmpsize);

  return state;
}

void vm_state_set_ops(vm_state_t *state, size_t nops, const vm_opcode_t *ops) {
  state->jumps = NULL;
  state->nops = nops;
  state->ops = ops;
}

void vm_state_ptrs(vm_state_t *state, void **ptrs) {
  state->jumps = vm_malloc(sizeof(void *) * state->nops);
  for (size_t i = 0; i < state->nops; i++) {
    vm_opcode_t op = state->ops[i];
    if (op >= 0 && op < VM_OPCODE_MAX1 && ptrs[op] != NULL) {
      state->jumps[i] = ptrs[op];
    }
  }
}

void vm_state_del(vm_state_t *state) {
  vm_gc_stop(&state->gc);
  vm_free(state->jumps);
  vm_free((void *)state->ops);
  vm_free(state->frames);
  vm_free(state->globals);
  vm_free(state);
}
