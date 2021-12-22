
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
  uint8_t len = 0;
  if (val == 0) {
    vm_save_byte(save, 0);
  } else if (val + 4 < 256) {
    vm_save_byte(save, val + 4);
  } else {
    if (val < (1 << 8)) {
      len = 1;
    } else if (val < (1 << 16)) {
      len = 2;
    } else if (val < (1 << 24)) {
      len = 3;
    } else {
      len = 4;
    }
    vm_save_byte(save, len);
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
  for (size_t i = 0; i < max; i++) {
    vm_save_obj(save, &state->gc, state->globals[i]);
  }
}

void vm_save_obj(vm_save_t *save, vm_gc_t *gc, vm_obj_t obj) {
  if (vm_obj_is_none(obj)) {
    vm_save_byte(save, 0);
  } else if (vm_obj_is_bool(obj)) {
    if (vm_obj_to_bool(obj) == false) {
      vm_save_byte(save, 1);
    } else {
      vm_save_byte(save, 2);
    }
  } else if (vm_obj_is_num(obj)) {
    vm_number_t num = vm_obj_to_num(obj);
    if (num < 0) {
      num = -num;
      uint8_t len = 0;
      if (num < (1 << 8)) {
        len = 1;
        vm_save_byte(save, 4);
      } else if (num < (1 << 16)) {
        len = 2;
        vm_save_byte(save, 5);
      } else if (num < (1 << 24)) {
        len = 3;
        vm_save_byte(save, 6);
      } else {
        len = 4;
        vm_save_byte(save, 7);
      }
      vm_save_uint_len(save, len, num);
    } else {
      uint8_t len = 0;
      if (num < (1 << 8)) {
        len = 1;
        vm_save_byte(save, 8);
      } else if (num < (1 << 16)) {
        len = 2;
        vm_save_byte(save, 9);
      } else if (num < (1 << 24)) {
        len = 3;
        vm_save_byte(save, 10);
      } else {
        len = 4;
        vm_save_byte(save, 11);
      }
      vm_save_uint_len(save, len, num);
    }
  } else if (vm_obj_is_ptr(obj)) {
    vm_gc_entry_t *ptr = vm_obj_to_ptr(gc, obj);
    vm_save_byte(save, 3);
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
  if (tag == 0) {
    return 0;
  } else if (1 <= tag && tag <= 4) {
    return vm_save_get_uint_len(save, tag);
  } else {
    return tag - 4;
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
  size_t max = state->nlocals + state->frames[state->framenum].nlocals;
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
  case 0: {
    return vm_obj_of_none();
  }
  case 1: {
    return vm_obj_of_bool(false);
  }
  case 2: {
    return vm_obj_of_bool(true);
  }
  case 3: {
    uint32_t v = vm_save_get_uint(save);
    return vm_obj_of_ptr(gc, gc->mem + v);
  }
  case 4: {
    return vm_obj_of_num(-vm_save_get_uint_len(save, 1));
  }
  case 5: {
    return vm_obj_of_num(-vm_save_get_uint_len(save, 2));
  }
  case 6: {
    return vm_obj_of_num(-vm_save_get_uint_len(save, 3));
  }
  case 7: {
    return vm_obj_of_num(-vm_save_get_uint_len(save, 4));
  }
  case 8: {
    return vm_obj_of_num(vm_save_get_uint_len(save, 1));
  }
  case 9: {
    return vm_obj_of_num(vm_save_get_uint_len(save, 2));
  }
  case 10: {
    return vm_obj_of_num(vm_save_get_uint_len(save, 3));
  }
  case 11: {
    return vm_obj_of_num(vm_save_get_uint_len(save, 4));
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
