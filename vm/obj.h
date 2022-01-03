#pragma once

#include "gc.h"
#include "type.h"

static inline bool vm_obj_is_none(vm_obj_t obj) { return nanbox_is_empty(obj); }

static inline bool vm_obj_is_bool(vm_obj_t obj) {
  return nanbox_is_boolean(obj);
}

static inline bool vm_obj_is_num(vm_obj_t obj) { return nanbox_is_number(obj); }

static inline bool vm_obj_is_ptr(vm_obj_t obj) {
  return nanbox_is_pointer(obj);
}

// c to obj

static inline vm_obj_t vm_obj_of_none(void) { return nanbox_empty(); }

static inline vm_obj_t vm_obj_of_bool(bool obj) {
  return nanbox_from_boolean(obj);
}

static inline vm_obj_t vm_obj_of_num(vm_number_t obj) {
  return nanbox_from_double(obj);
}

static inline vm_obj_t vm_obj_of_ptr(void *obj) {
  return nanbox_from_pointer(obj);
}

// obj to c

static inline bool vm_obj_to_bool(vm_obj_t obj) {
  return nanbox_to_boolean(obj);
}

static inline vm_number_t vm_obj_to_num(vm_obj_t obj) {
  return nanbox_to_double(obj);
}

static inline void *vm_obj_to_ptr(vm_obj_t obj) {
  return nanbox_to_pointer(obj);
}
