
#include "save.h"
#include "config.h"

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

void vm_save_uint_len(vm_save_t *save, uint8_t len, uint32_t val) {
  if (len == 1) {
    vm_save_byte(save, (val >> 0) & 0xFF);
  } else if (len == 2) {
    vm_save_byte(save, (val >> 8) & 0xFF);
    vm_save_byte(save, (val >> 0) & 0xFF);
  } else if (len == 3) {
    vm_save_byte(save, (val >> 16) & 0xFF);
    vm_save_byte(save, (val >> 8) & 0xFF);
    vm_save_byte(save, (val >> 0) & 0xFF);
  } else {
    vm_save_byte(save, (val >> 24) & 0xFF);
    vm_save_byte(save, (val >> 16) & 0xFF);
    vm_save_byte(save, (val >> 8) & 0xFF);
    vm_save_byte(save, (val >> 0) & 0xFF);
  }
}

void vm_save_uint(vm_save_t *save, uint32_t val) {
  if (val < 252) {
    vm_save_byte(save, val);
  } else {
    uint8_t len = 0;
    val -= 252;
    if (val < (1 << 8)) {
      len = 1;
      vm_save_byte(save, 252);
    } else if (val < (1 << 16)) {
      len = 2;
      vm_save_byte(save, 253);
    } else if (val < (1 << 24)) {
      len = 3;
      vm_save_byte(save, 254);
    } else {
      len = 4;
      vm_save_byte(save, 255);
    }
    vm_save_uint_len(save, len, val);
  }
}

void vm_save_state(vm_save_t *save, vm_state_t *state) {
  vm_save_gc(save, &state->gc);
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
  size_t max = state->nlocals + state->frames[state->framenum].nlocals;
  while (max > 0 && vm_obj_is_none(state->globals[max - 1])) {
    max -= 1;
  }
  vm_save_uint(save, max);
  for (size_t i = 0; i < max; i++) {
    vm_save_obj(save, &state->gc, state->globals[i]);
  }
}

void vm_save_obj(vm_save_t *save, vm_gc_t *gc, vm_obj_t obj) {
  if (vm_obj_is_none(obj)) {
    vm_save_byte(save, 255);
  } else if (vm_obj_is_bool(obj)) {
    if (vm_obj_to_bool(obj) == false) {
      vm_save_byte(save, 254);
    } else {
      vm_save_byte(save, 253);
    }
  } else if (vm_obj_is_num(obj)) {
    vm_number_t num = vm_obj_to_num(obj);
    if (num < 0) {
      num = -num;
      uint8_t len = 0;
      if (num < (1 << 8)) {
        len = 1;
        vm_save_byte(save, 252);
      } else if (num < (1 << 16)) {
        len = 2;
        vm_save_byte(save, 251);
      } else if (num < (1 << 24)) {
        len = 3;
        vm_save_byte(save, 250);
      } else {
        len = 4;
        vm_save_byte(save, 249);
      }
      vm_save_uint_len(save, len, num);
    } else if (num < 244) {
      vm_save_byte(save, num);
    } else {
      num -= 243;
      uint8_t len = 0;
      if (num < (1 << 8)) {
        len = 1;
        vm_save_byte(save, 248);
      } else if (num < (1 << 16)) {
        len = 2;
        vm_save_byte(save, 247);
      } else if (num < (1 << 24)) {
        len = 3;
        vm_save_byte(save, 246);
      } else {
        len = 4;
        vm_save_byte(save, 245);
      }
      vm_save_uint_len(save, len, num);
    }
  } else if (vm_obj_is_ptr(obj)) {
    vm_gc_entry_t *ptr = vm_obj_to_ptr(gc, obj);
    vm_save_byte(save, 244);
    uint32_t v = (uint8_t *)ptr - gc->mem;
    vm_save_uint(save, v);
  } else {
    __builtin_trap();
  }
}

void vm_save_gc(vm_save_t *save, vm_gc_t *gc) {
  vm_save_uint(save, gc->len);
  size_t pos = 0;
  while (pos < gc->len) {
    vm_gc_entry_t *ent = (vm_gc_entry_t *)&gc->mem[pos];
    size_t count = sizeof(vm_gc_entry_t) + sizeof(vm_obj_t) * ent->data;
    vm_save_uint(save, ent->data);
    for (size_t i = 0; i < ent->data; i++) {
      vm_save_obj(save, gc, ent->arr[i]);
    }
    pos += count;
  }
}

uint8_t vm_save_get_byte(vm_save_t *save) { return save->str[save->len++]; }

uint32_t vm_save_get_uint_len(vm_save_t *save, size_t len) {
  switch (len) {
  case 1: {
    uint32_t h0 = ((uint32_t)vm_save_get_byte(save)) << 0;
    return h0;
  }
  case 2: {
    uint32_t h1 = ((uint32_t)vm_save_get_byte(save)) << 8;
    uint32_t h0 = ((uint32_t)vm_save_get_byte(save)) << 0;
    return h1 + h0;
  }
  case 3: {
    uint32_t h2 = ((uint32_t)vm_save_get_byte(save)) << 16;
    uint32_t h1 = ((uint32_t)vm_save_get_byte(save)) << 8;
    uint32_t h0 = ((uint32_t)vm_save_get_byte(save)) << 0;
    return h2 + h1 + h0;
  }
  case 4: {
    uint32_t h3 = ((uint32_t)vm_save_get_byte(save)) << 24;
    uint32_t h2 = ((uint32_t)vm_save_get_byte(save)) << 16;
    uint32_t h1 = ((uint32_t)vm_save_get_byte(save)) << 8;
    uint32_t h0 = ((uint32_t)vm_save_get_byte(save)) << 0;
    return h3 + h2 + h1 + h0;
  }
  }
  __builtin_trap();
}

uint32_t vm_save_get_uint(vm_save_t *save) {
  uint8_t tag = vm_save_get_byte(save);
  switch (tag) {
  default: 
    return tag;
  case 252:
    return vm_save_get_uint_len(save, 1) + 252;
  case 253:
    return vm_save_get_uint_len(save, 2) + 252;
  case 254:
    return vm_save_get_uint_len(save, 3) + 252;
  case 255:
    return vm_save_get_uint_len(save, 4) + 252;
  }
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
  size_t max = vm_save_get_uint(save);
  for (size_t i = 0; i < max; i++) {
    state->globals[i] = vm_save_get_obj(save, &state->gc);
  }
  for (size_t i = max; i < VM_LOCALS_UNITS; i++) {
    state->globals[i] = vm_obj_of_none();
  }
}

vm_obj_t vm_save_get_obj(vm_save_t *save, vm_gc_t *gc) {
  uint8_t tag = vm_save_get_byte(save);
  switch (tag) {
  case 255: {
    return vm_obj_of_none();
  }
  case 254: {
    return vm_obj_of_bool(false);
  }
  case 253: {
    return vm_obj_of_bool(true);
  }
  case 252: {
    return vm_obj_of_num(-vm_save_get_uint_len(save, 1));
  }
  case 251: {
    return vm_obj_of_num(-vm_save_get_uint_len(save, 2));
  }
  case 250: {
    return vm_obj_of_num(-vm_save_get_uint_len(save, 3));
  }
  case 249: {
    return vm_obj_of_num(-vm_save_get_uint_len(save, 4));
  }
  case 248: {
    return vm_obj_of_num(243 + vm_save_get_uint_len(save, 1));
  }
  case 247: {
    return vm_obj_of_num(243 + vm_save_get_uint_len(save, 2));
  }
  case 246: {
    return vm_obj_of_num(243 + vm_save_get_uint_len(save, 3));
  }
  case 245: {
    return vm_obj_of_num(243 + vm_save_get_uint_len(save, 4));
  }
  case 244: {
    uint32_t v = vm_save_get_uint(save);
    return vm_obj_of_ptr(gc, gc->mem + v);
  }
  default: {
    return vm_obj_of_num(tag);
  }
  }
  __builtin_trap();
}

void vm_save_get_gc(vm_save_t *save, vm_gc_t *gc) {
  gc->len = vm_save_get_uint(save);
  gc->max = 0;
  size_t pos = 0;
  while (pos < gc->len) {
    vm_gc_entry_t *ent = (vm_gc_entry_t *)&gc->mem[pos];
    ent->data = vm_save_get_uint(save);
    size_t count = sizeof(vm_gc_entry_t) + sizeof(vm_obj_t) * ent->data;
    for (size_t i = 0; i < ent->data; i++) {
      ent->arr[i] = vm_save_get_obj(save, gc);
    }
    pos += count;
  }
}
