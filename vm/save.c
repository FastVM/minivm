
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
  vm_save_byte(save, (val >> 24) & 0xFF);
  vm_save_byte(save, (val >> 16) & 0xFF);
  vm_save_byte(save, (val >> 8) & 0xFF);
  vm_save_byte(save, (val >> 0) & 0xFF);
}

void vm_save_state(vm_save_t *save, vm_state_t *state) {
  vm_save_gc(save, state->gc);
  vm_save_uint(save, state->index);
  vm_save_uint(save, state->nops);
  for (size_t i = 0; i < state->nops; i++) {
    vm_save_uint(save, state->ops[i]);
  }
  vm_save_uint(save, state->framenum);
  for (size_t i = 0; i < state->framenum + 1; i++) {
    vm_save_uint(save, state->frames[i].outreg);
    vm_save_uint(save, state->frames[i].index);
    vm_save_uint(save, state->frames[i].nlocals);
  }
  vm_save_uint(save, state->nlocals);
  size_t max = state->nlocals + 512;
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
  vm_save_byte(save, gc.up);
  vm_save_uint(save, gc.len);
  vm_save_uint(save, gc.max);
  for (size_t i = 0; i < gc.len; i++) {
    vm_save_byte(save, gc.mem[i]);
  }
}

uint8_t vm_save_get_byte(vm_save_t *save) { return save->str[save->len++]; }

uint32_t vm_save_get_uint(vm_save_t *save) {
  uint32_t h3 = ((uint32_t)vm_save_get_byte(save)) << 24;
  uint32_t h2 = ((uint32_t)vm_save_get_byte(save)) << 16;
  uint32_t h1 = ((uint32_t)vm_save_get_byte(save)) << 8;
  uint32_t h0 = ((uint32_t)vm_save_get_byte(save)) << 0;
  return h3 + h2 + h1 + h0;
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
  for (size_t i = 0; i < state->framenum + 1; i++) {
    state->frames[i].outreg = vm_save_get_uint(save);
    state->frames[i].index = vm_save_get_uint(save);
    state->frames[i].nlocals = vm_save_get_uint(save);
  }
  state->nlocals = vm_save_get_uint(save);
  size_t max = state->nlocals + 512;
  for (size_t i = 0; i < max; i++) {
    state->globals[i] = vm_save_get_obj(save);
  }
}

vm_obj_t vm_save_get_obj(vm_save_t *save) {
  uint8_t tag = vm_save_get_byte(save);
  bool inv = vm_save_get_byte(save);
  uint32_t val = vm_save_get_uint(save);
  if (inv == 1) {
    return (vm_obj_t){
        .type = tag,
        .value = 0 - (int32_t)val,
    };
  } else {
    return (vm_obj_t){
        .type = tag,
        .value = (int32_t)val,
    };
  }
}

void vm_save_get_gc(vm_save_t *save, vm_gc_t *gc) {
  gc->up = vm_save_get_byte(save);
  gc->len = vm_save_get_uint(save);
  gc->max = vm_save_get_uint(save);
  gc->xmem = (&gc->base[!gc->up * gc->alloc]);
  gc->mem = (&gc->base[gc->up * gc->alloc]);
  for (size_t i = 0; i < gc->len; i++) {
    gc->mem[i] = vm_save_get_byte(save);
  }
}
