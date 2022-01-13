#pragma once

#include "gc.h"
#include "type.h"

static inline vm_obj_t vm_obj_of_bool(bool log) {
  return (vm_obj_t){.log = log};
}
static inline vm_obj_t vm_obj_of_num(vm_number_t num) {
  return (vm_obj_t){.num = num};
}
static inline vm_obj_t vm_obj_of_ptr(vm_gc_t *gc, void *ptr) {
  return (vm_obj_t){.ptr = (uint8_t*) ptr - gc->base};
}

static inline bool vm_obj_to_bool(vm_obj_t obj) {
  return obj.log;
}
static inline vm_number_t vm_obj_to_num(vm_obj_t obj) {
  return obj.num;
}
static inline void *vm_obj_to_ptr(vm_gc_t *gc, vm_obj_t obj) {
  return gc->base + obj.ptr;
}