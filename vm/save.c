
#include "save.h"

void vm_save_init(vm_save_t *out) {
  *out = (vm_save_t){
      .len = 0,
      .alloc = 0,
      .str = NULL,
  };
}

void vm_save_deinit(vm_save_t *del) { vm_free(del->str); }

void vm_save_rewind(vm_save_t *save) { save->len = 0; }

void vm_save_byte(vm_save_t *save, uint8_t val) {
  if (save->len + 1 >= save->alloc) {
    save->alloc = save->alloc * 4 + 4;
    save->str = vm_realloc(save->str, save->alloc);
  }
  save->str[save->len++] = val;
}

void vm_save_uint(vm_save_t *save, uint32_t val) {
  vm_save_byte(save, val >> 24);
  vm_save_byte(save, val >> 16);
  vm_save_byte(save, val >> 8);
  vm_save_byte(save, val);
}

void vm_save_state(vm_save_t *save, vm_state_t *state) {
  vm_save_gc(save, state->gc);
  vm_save_uint(save, state->index);
  vm_save_uint(save, state->nops);
  for (size_t i = 0; i < state->nops; i++) {
    vm_save_uint(save, state->ops[i]);
  }
  vm_save_uint(save, state->framenum);
  for (size_t i = 0; i < state->framenum; i++) {
    vm_save_uint(save, state->frames[i].outreg);
    vm_save_uint(save, state->frames[i].index);
    vm_save_uint(save, state->frames[i].nlocals);
  }
  vm_save_uint(save, state->nlocals);
  size_t max = state->nlocals + state->frames[state->framenum].nlocals;
  for (size_t i = 0; i < max; i++) {
    vm_save_obj(save, state->globals[i]);
  }
}

void vm_save_obj(vm_save_t *save, vm_obj_t obj) {
  vm_save_byte(save, obj.type);
  if (obj.value < 0) {
    vm_save_byte(save, 1);
    vm_save_uint(save, 0 - obj.value);
  } else {
    vm_save_byte(save, 0);
    vm_save_uint(save, obj.value);
  }
}

void vm_save_gc(vm_save_t *save, vm_gc_t gc) {
  vm_save_uint(save, gc.mem - gc.base);
  vm_save_uint(save, gc.xmem - gc.base);
  vm_save_uint(save, gc.len);
  vm_save_uint(save, gc.max);
  vm_save_byte(save, gc.up);
  for (size_t i = 0; i < gc.len; i++) {
    vm_save_byte(save, gc.mem[i]);
  }
}

uint8_t vm_save_get_byte(vm_save_t *save) { return save->str[save->len++]; }

uint32_t vm_save_get_uint(vm_save_t *save) {
  return ((uint32_t) vm_save_get_byte(save) << 24) + ((uint32_t)vm_save_get_byte(save) << 16) +
         ((uint32_t)vm_save_get_byte(save) << 8) + (uint32_t)vm_save_get_byte(save);
}

void vm_save_get_state(vm_save_t *save, vm_state_t *state) {
  vm_save_get_gc(save, &state->gc);
  state->index = vm_save_get_uint(save);
  state->nops = vm_save_get_uint(save);
  vm_opcode_t *ops = vm_malloc(sizeof(vm_opcode_t) * state->nops);
  for (size_t i = 0; i < state->nops; i++) {
    ops[i] = vm_save_get_uint(save);
  }
  vm_free((void *)state->ops);
  state->ops = ops;
  state->framenum = vm_save_get_uint(save);
  for (size_t i = 0; i < state->framenum; i++) {
    state->frames[i].outreg = vm_save_get_uint(save);
    state->frames[i].index = vm_save_get_uint(save);
    state->frames[i].nlocals = vm_save_get_uint(save);
  }
  state->nlocals = vm_save_get_uint(save);
  size_t max = state->nlocals + state->frames[state->framenum].nlocals;
  for (size_t i = 0; i < max; i++) {
    state->globals[i] = vm_save_get_obj(save);
  }
}

vm_obj_t vm_save_get_obj(vm_save_t *save) {
  uint8_t tag = vm_save_get_byte(save);
  if (vm_save_get_byte(save)) {
    return (vm_obj_t){
        .type = tag,
        .value = 0 - vm_save_get_uint(save),
    };
  } else {
    return (vm_obj_t){
        .type = tag,
        .value = vm_save_get_uint(save),
    };
  }
}

void vm_save_get_gc(vm_save_t *save, vm_gc_t *gc) {
  gc->mem = gc->base + vm_save_get_uint(save);
  gc->xmem = gc->base + vm_save_get_uint(save);
  gc->len = vm_save_get_uint(save);
  gc->max = vm_save_get_uint(save);
  gc->up = vm_save_get_byte(save);
  for (size_t i = 0; i < gc->len; i++) {
    gc->mem[i] = vm_save_get_byte(save);
  }
}