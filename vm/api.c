#include "config.h"
#include "gc.h"
#include "libc.h"
#include "type.h"

VM_API void vm_api_reset(vm_state_t *state) {
  state->tmphead = 0;
}

static vm_int_t vm_objdup(vm_state_t *state, vm_obj_t obj) {
  if (state->tmphead + 4 > state->tmpsize) {
    state->tmpsize = state->tmpsize * 4 + 4;
    state->tmpbuf = vm_realloc(state->tmpbuf, sizeof(vm_obj_t) * state->tmpsize);
  }
  const vm_int_t ret = state->tmphead;
  state->tmpbuf[ret] = obj;
  state->tmphead += 1;
  return ret;
}

VM_API bool vm_api_is_none(vm_state_t *state, vm_int_t obj) {
  return vm_obj_is_none(state->tmpbuf[obj]);
}
VM_API bool vm_api_is_bool(vm_state_t *state, vm_int_t obj) {
  return vm_obj_is_bool(state->tmpbuf[obj]);
}
VM_API bool vm_api_is_num(vm_state_t *state, vm_int_t obj) {
  return vm_obj_is_num(state->tmpbuf[obj]);
}

VM_API vm_int_t vm_api_of_none(vm_state_t *state) {
  return vm_objdup(state, vm_obj_of_none());
}
VM_API vm_int_t vm_api_of_bool(vm_state_t *state, bool obj) {
  return vm_objdup(state, vm_obj_of_bool(obj));
}
VM_API vm_int_t vm_api_of_num(vm_state_t *state, vm_number_t obj) {
  return vm_objdup(state, vm_obj_of_num(obj));
}

VM_API bool vm_api_to_bool(vm_state_t *state, vm_int_t obj) {
  return vm_obj_to_bool(state->tmpbuf[obj]);
}
VM_API vm_number_t vm_api_to_num(vm_state_t *state, vm_int_t obj) {
  return vm_obj_to_num(state->tmpbuf[obj]);
}

VM_API vm_int_t vm_api_new(vm_state_t *state, size_t size) {
  vm_gc_entry_t *ret = vm_gc_static_array_new(&state->gc, size);
  for (size_t i = 0; i < size; i++) {
    vm_gc_set_index(&state->gc, ret, i, vm_obj_of_none());
  }
  return vm_objdup(state, vm_obj_of_ptr(&state->gc, ret));
}

VM_API void vm_api_set(vm_state_t *state, vm_int_t obj, size_t index,
                       vm_int_t value) {
  vm_gc_set_index(&state->gc, vm_obj_to_ptr(&state->gc, state->tmpbuf[obj]), index, state->tmpbuf[value]);
}

VM_API vm_int_t vm_api_get(vm_state_t *state, vm_int_t obj, size_t index) {
  return vm_objdup(state, vm_gc_get_index(&state->gc, vm_obj_to_ptr(&state->gc, state->tmpbuf[obj]), index));
}

VM_API size_t vm_api_len(vm_state_t *state, vm_int_t obj) {
  return vm_gc_sizeof(&state->gc, vm_obj_to_ptr(&state->gc, state->tmpbuf[obj]));
}

VM_API vm_int_t vm_api_concat(vm_state_t *state, vm_int_t lhs,
                               vm_int_t rhs) {
  return vm_objdup(state, vm_gc_static_concat(&state->gc, state->tmpbuf[lhs], state->tmpbuf[rhs]));
}

VM_API vm_int_t vm_api_str(vm_state_t *state, size_t len, const char *str) {
  vm_gc_entry_t *ret = vm_gc_static_array_new(&state->gc, len);
  for (size_t i = 0; i < len; i++) {
    vm_gc_set_index(&state->gc, ret, i, vm_obj_of_num(str[i]));
  }
  return vm_objdup(state, vm_obj_of_ptr(&state->gc, ret));
}

VM_API void vm_api_stack_set(vm_state_t *state, size_t n, vm_int_t obj) {
  if (n < 0) {
    n += VM_LOCALS_UNITS;
  }
  state->globals[n] = state->tmpbuf[obj];
}

VM_API vm_int_t vm_api_stack_get(vm_state_t *state, size_t n) {
  if (n < 0) {
    n += VM_LOCALS_UNITS;
  }
  return vm_objdup(state, state->globals[n]);
}

#if defined(VM_EMCC)

#include "vm.h"

EM_JS(void, vm_do_emcc_save, (size_t val, uint8_t *ptr), {
  let ret = [];
  for (let i = 0; i < val; i++) {
    ret.push(Module.HEAP8[ptr + i]);
  }
  Module.vm_do_saved(ret);
});

VM_API void vm_api_save(vm_state_t *state) {
  vm_save_t save;
  vm_save_init(&save);
  vm_save_state(&save, state);
  vm_do_emcc_save(save.len, save.str);
  vm_save_deinit(&save);
}

VM_API vm_state_t *vm_api_load_save(size_t len, uint8_t *str) {
  vm_save_t save = (vm_save_t) {
    .len = 0,
    .str = str,
  };
  vm_state_t *state = vm_state_new(0, NULL);
  vm_save_get_state(&save, state);
  return state;
}

#endif
