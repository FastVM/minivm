#include "gc.h"

vm_gc_t vm_gc_init(void) {
  vm_gc_t ret;
  ret.used = 0;
  ret.alloc = 256;
  ret.heap = vm_malloc(sizeof(vm_gc_slot_t) * ret.alloc);
  ret.start = (void *)0;
  return ret;
}

void vm_gc_grow(vm_gc_t *gc, size_t count) {
  if (gc->used + count >= gc->alloc) {
    gc->alloc = (gc->used + count) * 4;
    gc->heap = vm_realloc(gc->heap, sizeof(vm_gc_slot_t) * gc->alloc);
  }
}

void vm_gc_deinit(vm_gc_t gc) { vm_free(gc.heap); }

void vm_gc_set_locals(vm_gc_t *gc, size_t nlocals, vm_obj_t *locals) {
  gc->start = locals;
  gc->end = locals + nlocals;
}

vm_obj_t vm_gc_new(vm_gc_t *gc, size_t count) {
  size_t start = gc->used;
  vm_gc_grow(gc, count + 1);
  gc->used += count + 1;
  gc->heap[start] = (vm_gc_slot_t){
      .len = count * 2,
  };
  return (vm_obj_t){
      .ptr = (start + 1) * 2 + 1,
  };
}

vm_obj_t vm_gc_get(vm_gc_t *gc, vm_obj_t obj, size_t index) {
  return gc->heap[obj.ptr / 2 + index].val;
}

void vm_gc_set(vm_gc_t *gc, vm_obj_t obj, size_t index, vm_obj_t value) {
  gc->heap[obj.ptr / 2 + index].val = value;
}

size_t vm_gc_len(vm_gc_t *gc, vm_obj_t obj) {
  return gc->heap[obj.ptr / 2 - 1].len / 2;
}
