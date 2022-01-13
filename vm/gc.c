#include "gc.h"
#include "config.h"

void GC_init(void);
void *GC_malloc(size_t size);

void vm_gc_start(void) {
  GC_init();
}

vm_gc_entry_t *vm_gc_static_array_new(size_t size) {
  vm_gc_entry_t *ent = GC_malloc(sizeof(vm_gc_entry_t) + sizeof(vm_obj_t) * size);
  ent->len = size;
  return ent;
}

vm_obj_t vm_gc_static_concat(vm_obj_t lhs, vm_obj_t rhs) {
  vm_gc_entry_t *left = lhs.ptr;
  vm_gc_entry_t *right = rhs.ptr;
  vm_int_t llen = left->len;
  vm_int_t rlen = right->len;
  vm_gc_entry_t *ent = vm_gc_static_array_new(llen + rlen);
  for (vm_int_t i = 0; i < llen; i++) {
    ent->arr[i] = left->arr[i];
  }
  for (vm_int_t i = 0; i < rlen; i++) {
    ent->arr[llen + i] = right->arr[i];
  }
  return (vm_obj_t) {.ptr = ent};
}
