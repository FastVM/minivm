#include "gc.h"
#include "config.h"
#include "obj.h"

#define VM_GC_DATA_TYPE uint32_t
#define VM_GC_HIGHEST_BIT (sizeof(VM_GC_DATA_TYPE) * 8 - 1)
#define VM_GC_MASK (1L << VM_GC_HIGHEST_BIT)

#define gc_mem(gc) (gc->mem)
#define gc_xmem(gc) (gc->xmem)

void vm_gc_start(vm_gc_t *gc) {
  gc->alloc = VM_MEM_MAX;
  gc->max = VM_MEM_MIN;
  gc->len = 0;
  gc->up = 0;
  gc->base = vm_malloc(gc->alloc * 2);
  gc->mem = (&gc->base[gc->up * gc->alloc]);
  gc->xmem = (&gc->base[!gc->up * gc->alloc]);
}

void vm_gc_stop(vm_gc_t *gc) { vm_free(gc->base); }

vm_gc_entry_t *vm_gc_static_array_new(vm_gc_t *gc, size_t size) {
  vm_gc_entry_t *entry = (vm_gc_entry_t *)&gc_mem(gc)[gc->len];
  *entry = (vm_gc_entry_t){
      .data = size,
  };
  gc->len += sizeof(vm_gc_entry_t) + sizeof(vm_obj_t) * size;
  return entry;
}

vm_obj_t vm_gc_get_index(vm_gc_t *gc, vm_gc_entry_t *ptr, vm_int_t index) {
  return ptr->arr[index];
}

void vm_gc_set_index(vm_gc_t *gc, vm_gc_entry_t *ptr, vm_int_t index,
                     vm_obj_t value) {
  ptr->arr[index] = value;
}

vm_int_t vm_gc_sizeof(vm_gc_t *gc, vm_gc_entry_t *ptr) { return ptr->data; }

vm_obj_t vm_gc_static_concat(vm_gc_t *gc, vm_obj_t lhs, vm_obj_t rhs) {
  vm_gc_entry_t *left = vm_obj_to_ptr(gc, lhs);
  vm_gc_entry_t *right = vm_obj_to_ptr(gc, rhs);
  vm_int_t llen = left->data;
  vm_int_t rlen = right->data;
  vm_gc_entry_t *ent = vm_gc_static_array_new(gc, llen + rlen);
  for (vm_int_t i = 0; i < llen; i++) {
    ent->arr[i] = left->arr[i];
  }
  for (vm_int_t i = 0; i < rlen; i++) {
    ent->arr[llen + i] = right->arr[i];
  }
  return vm_obj_of_ptr(gc, ent);
}

vm_obj_t vm_gc_dup(vm_gc_t *out, vm_gc_t *in, vm_obj_t obj) {
  if (!vm_obj_is_ptr(obj)) {
    return obj;
  }
  vm_gc_entry_t *ent = vm_obj_to_ptr(in, obj);
  vm_gc_entry_t *ret = vm_gc_static_array_new(out, ent->data);
  for (size_t i = 0; i < ent->data; i++) {
    ret->arr[i] = vm_gc_dup(out, in, ent->arr[i]);
  }
  return vm_obj_of_ptr(out, ret);
}
