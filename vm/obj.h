#pragma once

#include "gc.h"
#include "type.h"

#if defined(VM_USE_FLOAT)
static inline bool vm_obj_is_none(vm_obj_t obj) { return vm_nanbox_is_empty(obj); }
static inline bool vm_obj_is_bool(vm_obj_t obj) {
  return vm_nanbox_is_boolean(obj);
}
static inline bool vm_obj_is_num(vm_obj_t obj) { return vm_nanbox_is_double(obj); }
static inline bool vm_obj_is_ptr(vm_obj_t obj) {
  return vm_nanbox_is_pointer(obj);
}

static inline vm_obj_t vm_obj_of_none(void) { return vm_nanbox_empty(); }
static inline vm_obj_t vm_obj_of_bool(bool obj) {
  return vm_nanbox_from_boolean(obj);
}
static inline vm_obj_t vm_obj_of_num(vm_number_t obj) {
  return vm_nanbox_from_double(obj);
}
static inline vm_obj_t vm_obj_of_ptr(vm_gc_t *gc, void *obj) {
  return vm_nanbox_from_pointer(obj);
}

static inline bool vm_obj_to_bool(vm_obj_t obj) {
  return vm_nanbox_to_boolean(obj);
}
static inline vm_number_t vm_obj_to_num(vm_obj_t obj) {
  return vm_nanbox_to_double(obj);
}
static inline void *vm_obj_to_ptr(vm_gc_t *gc, vm_obj_t obj) {
  return vm_nanbox_to_pointer(obj);
}
#else
static inline bool vm_obj_is_none(vm_obj_t obj) {
  return obj.itype == VM_TYPE_NONE;
}
static inline bool vm_obj_is_bool(vm_obj_t obj) {
  return obj.itype == VM_TYPE_BOOL;
}
static inline bool vm_obj_is_num(vm_obj_t obj) {
  return obj.itype == VM_TYPE_NUM;
}
static inline bool vm_obj_is_ptr(vm_obj_t obj) {
  return obj.ptype == VM_TYPE_PTR;
}

static inline vm_obj_t vm_obj_of_none(void) {
  return (vm_obj_t){.itype = VM_TYPE_NONE};
}
static inline vm_obj_t vm_obj_of_bool(bool log) {
  return (vm_obj_t){.itype = VM_TYPE_BOOL, .ival = log};
}
static inline vm_obj_t vm_obj_of_num(vm_number_t num) {
  return (vm_obj_t){.itype = VM_TYPE_NUM, .ival = num};
}
static inline vm_obj_t vm_obj_of_ptr(vm_gc_t *gc, void *ptr) {
  return (vm_obj_t){.ptype = VM_TYPE_PTR, .pval = (uint8_t *)ptr - gc->base};
}

static inline bool vm_obj_to_bool(vm_obj_t obj) {
  return obj.ival;
}
static inline vm_number_t vm_obj_to_num(vm_obj_t obj) {
  return obj.ival;
}
static inline void *vm_obj_to_ptr(vm_gc_t *gc, vm_obj_t obj) {
  return gc->base + obj.pval;
}

#endif