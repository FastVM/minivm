#include "config.h"
#include "gc.h"
#include "libc.h"
#include "type.h"

static vm_obj_t *vm_objdup(vm_obj_t obj) {
  vm_obj_t *ret = vm_malloc(sizeof(vm_obj_t));
  *ret = obj;
  return ret;
}

VM_API bool vm_api_is_none(vm_state_t *state, vm_obj_t *obj) {
  return vm_obj_is_none(*obj);
}
VM_API bool vm_api_is_bool(vm_state_t *state, vm_obj_t *obj) {
  return vm_obj_is_bool(*obj);
}
VM_API bool vm_api_is_num(vm_state_t *state, vm_obj_t *obj) {
  return vm_obj_is_num(*obj);
}

VM_API vm_obj_t *vm_api_of_none(vm_state_t *state) {
  return vm_objdup(vm_obj_of_none());
}
VM_API vm_obj_t *vm_api_of_bool(vm_state_t *state, bool obj) {
  return vm_objdup(vm_obj_of_bool(obj));
}
VM_API vm_obj_t *vm_api_of_num(vm_state_t *state, vm_number_t obj) {
  return vm_objdup(vm_obj_of_num(obj));
}

VM_API bool vm_api_to_bool(vm_state_t *state, vm_obj_t *obj) {
  return vm_obj_to_bool(*obj);
}
VM_API vm_number_t vm_api_to_num(vm_state_t *state, vm_obj_t *obj) {
  return vm_obj_to_num(*obj);
}

VM_API vm_obj_t *vm_api_new(vm_state_t *state, size_t size) {
  vm_gc_entry_t *ret = vm_gc_static_array_new(&state->gc, size);
  for (size_t i = 0; i < size; i++) {
    vm_gc_set_index(&state->gc, ret, i, vm_obj_of_none());
  }
  return vm_objdup(vm_obj_of_ptr(&state->gc, ret));
}

VM_API void vm_api_set(vm_state_t *state, vm_obj_t *obj, size_t index,
                       vm_obj_t *value) {
  vm_gc_set_index(&state->gc, vm_obj_to_ptr(&state->gc, *obj), index, *value);
}

VM_API vm_obj_t *vm_api_get(vm_state_t *state, vm_obj_t *obj, size_t index) {
  return vm_objdup(vm_gc_get_index(&state->gc, vm_obj_to_ptr(&state->gc, *obj), index));
}

VM_API size_t vm_api_len(vm_state_t *state, vm_obj_t *obj) {
  return vm_gc_sizeof(&state->gc, vm_obj_to_ptr(&state->gc, *obj));
}

VM_API vm_obj_t *vm_api_concat(vm_state_t *state, vm_obj_t *lhs,
                               vm_obj_t *rhs) {
  return vm_objdup(vm_gc_static_concat(&state->gc, *lhs, *rhs));
}

VM_API vm_obj_t *vm_api_str(vm_state_t *state, size_t len, const char *str) {
  vm_gc_entry_t *ret = vm_gc_static_array_new(&state->gc, len);
  for (size_t i = 0; i < len; i++) {
    vm_gc_set_index(&state->gc, ret, i, vm_obj_of_num(str[i]));
  }
  return vm_objdup(vm_obj_of_ptr(&state->gc, ret));
}

VM_API void vm_api_stack_set(vm_state_t *state, size_t n, vm_obj_t *obj) {
  if (n < 0) {
    n += VM_LOCALS_UNITS;
  }
  state->globals[n] = *obj;
}

VM_API vm_obj_t *vm_api_stack_get(vm_state_t *state, size_t n) {
  if (n < 0) {
    n += VM_LOCALS_UNITS;
  }
  return vm_objdup(state->globals[n]);
}
