#include "gc.h"

static inline void vm_gc_mark(vm_gc_t *gc, vm_obj_t obj) {
  if (obj % 2 == 0) {
    return;
  }
  if (gc->heap[obj / 2 - 1].len % 2 != 0) {
    return;
  }
  gc->heap[obj / 2 - 1].len += 1;
  size_t len = vm_gc_len(gc, obj);
  for (size_t i = 0; i < len; i++) {
    vm_gc_mark(gc, vm_gc_get(gc, obj, i));
  }
}

static inline vm_obj_t vm_gc_update(vm_gc_t *gc, vm_obj_t obj) {
  if (obj % 2 == 0) {
    return obj;
  }
  vm_number_t where = gc->off_heap[obj / 2 - 1].newpos;
  vm_number_t retobj = where * 2 + 1;
  vm_number_t len = gc->heap[where - 1].len / 2;
  for (vm_number_t i = 0; i < len; i++) {
    gc->heap[retobj / 2 + i].val =
        vm_gc_update(gc, gc->off_heap[obj / 2 + i].val);
  }
  return retobj;
}

void vm_gc_collect(vm_gc_t *gc) {
  if (gc->heap_used <= gc->max) {
    return;
  }
  for (vm_obj_t *cur = gc->start; cur < gc->end; cur++) {
    vm_gc_mark(gc, *cur);
  }
  size_t heap_used = 0;
  size_t head = 0;
  size_t max = gc->heap_used;
  while (head < max) {
    vm_number_t len = gc->heap[head].len / 2;
    if (gc->heap[head].len % 2 != 0) {
      gc->off_heap[heap_used].len = len * 2;
      heap_used += 1;
      gc->heap[head].newpos = (vm_number_t) heap_used;
      heap_used += (size_t) len;
    }
    head += (size_t) (len + 1);
  }
  vm_gc_slot_t *old_heap = gc->heap;
  vm_gc_slot_t *old_off_heap = gc->off_heap;
  gc->heap = old_off_heap;
  gc->off_heap = old_heap;
  gc->heap_used = heap_used;
  for (vm_obj_t *cur = gc->start; cur < gc->end; cur++) {
    *cur = vm_gc_update(gc, *cur);
  }
  if (gc->shrink || gc->heap_used * gc->grow / 100 > gc->max) {
    gc->max = gc->heap_used * gc->grow / 100;
  }
}

vm_gc_t vm_gc_init(vm_config_t config) {
  return (vm_gc_t) {
    .heap_used = 0,
    .grow = config.gc_ents,
    .max = config.gc_init,
    .heap_alloc = config.gc_ents,
    .heap = vm_malloc(sizeof(vm_gc_slot_t) * config.gc_ents),
    .off_heap = vm_malloc(sizeof(vm_gc_slot_t) * config.gc_ents),
  };
}

void vm_gc_deinit(vm_gc_t gc) {
  vm_free(gc.heap);
  vm_free(gc.off_heap);
}

void vm_gc_set_locals(vm_gc_t *gc, size_t nlocals, vm_obj_t *locals) {
  gc->start = locals;
  gc->end = locals + nlocals;
}

vm_obj_t vm_gc_new(vm_gc_t *gc, size_t count) {
  size_t start = gc->heap_used;
  if (gc->heap_used + count >= gc->heap_alloc) {
    gc->heap_alloc = (gc->heap_used + count) * 2;
    gc->heap = vm_realloc(gc->heap, sizeof(vm_gc_slot_t) * gc->heap_alloc);
    gc->off_heap =
        vm_realloc(gc->off_heap, sizeof(vm_gc_slot_t) * gc->heap_alloc);
  }
  gc->heap_used += count + 1;
  gc->heap[start] = (vm_gc_slot_t){
      .len = (vm_number_t) (count * 2),
  };
  return (vm_obj_t) ((start + 1) * 2 + 1);
}
