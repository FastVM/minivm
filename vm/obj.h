#pragma once

#include "gc.h"
#include "type.h"

static inline bool vm_obj_is_none(vm_obj_t obj) {
  return obj.type == VM_TYPE_NONE;
}

static inline bool vm_obj_is_bool(vm_obj_t obj) {
  return obj.type == VM_TYPE_BOOL;
}

static inline bool vm_obj_is_num(vm_obj_t obj) {
  return obj.type == VM_TYPE_NUMBER;
}

static inline bool vm_obj_is_ptr(vm_obj_t obj) {
  return obj.type == VM_TYPE_ARRAY;
}

// c to obj

static inline vm_obj_t vm_obj_of_none(void) {
  return (vm_obj_t){
      .type = VM_TYPE_NONE,
  };
}

static inline vm_obj_t vm_obj_of_bool(bool obj) {
  return (vm_obj_t){
      .type = VM_TYPE_BOOL,
      .value = obj,
  };
}

static inline vm_obj_t vm_obj_of_int(int obj) {
  return (vm_obj_t){
      .type = VM_TYPE_NUMBER,
      .value = obj,
  };
}

static inline vm_obj_t vm_obj_of_num(vm_number_t obj) {
  return (vm_obj_t){
      .type = VM_TYPE_NUMBER,
      .value = obj,
  };
}

static inline vm_obj_t vm_obj_of_ptr(vm_gc_t *gc, void *obj) {
  return (vm_obj_t){
      .type = VM_TYPE_ARRAY,
      .value = (uint8_t *)obj - gc->base,
  };
}

static inline vm_obj_t vm_obj_of_xptr(vm_gc_t *gc, void *obj) {
  return (vm_obj_t){
      .type = VM_TYPE_ARRAY,
      .value = (uint8_t *)obj - gc->base,
  };
}

// obj to c

static inline bool vm_obj_to_bool(vm_obj_t obj) { return obj.value; }

static inline vm_int_t vm_obj_to_int(vm_obj_t obj) { return obj.value; }

static inline vm_number_t vm_obj_to_num(vm_obj_t obj) { return obj.value; }

static inline void *vm_obj_to_ptr(vm_gc_t *gc, vm_obj_t obj) {
  return gc->base + obj.value;
}
