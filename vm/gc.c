#include "gc.h"

static inline void vm_gc_set_mark(vm_gc_t *gc, vm_obj_t obj) {
  gc->heap[obj / 2 - 1].len += 1;
}

static inline void vm_gc_unset_mark(vm_gc_t *gc, vm_obj_t obj) {
  gc->heap[obj / 2 - 1].len -= 1;
}

static inline int vm_gc_get_mark(vm_gc_t *gc, vm_obj_t obj) {
  return gc->heap[obj / 2 - 1].len % 2;
}

static inline void vm_gc_mark(vm_gc_t *gc, vm_obj_t obj) {
  if (obj % 2 == 0) {
    return;
  }
  if (vm_gc_get_mark(gc, obj) != 0) {
    return;
  }
  vm_gc_set_mark(gc, obj);
  size_t len = vm_gc_len(gc, obj);
  for (size_t i = 0; i < len; i++) {
    vm_obj_t next = vm_gc_get(gc, obj, i);
    vm_gc_mark(gc, next);
  }
}

static inline vm_obj_t vm_gc_update(vm_gc_t *gc, vm_obj_t obj) {
  if (obj % 2 == 0) {
    return obj;
  }
  size_t where = gc->off_heap[obj / 2 - 1].newpos;
  size_t retobj = where * 2 + 1;
  size_t len = gc->heap[where - 1].len / 2;
  for (size_t i = 0; i < len; i++) {
    gc->heap[retobj / 2 + i].val =
        vm_gc_update(gc, gc->off_heap[obj / 2 + i].val);
  }
  return retobj;
}

static inline void vm_gc_collect_once(vm_gc_t *gc) {
  for (vm_obj_t *cur = gc->start; cur < gc->end; cur++) {
    vm_gc_mark(gc, *cur);
  }
  size_t heap_used = 0;
  size_t head = 0;
  size_t max = gc->heap_used;
  while (head < max) {
    size_t len = gc->heap[head].len / 2;
    head += 1;
    if (vm_gc_get_mark(gc, head * 2 + 1) != 0) {
      heap_used += 1;
      gc->heap[head - 1].newpos = heap_used;
      gc->off_heap[heap_used - 1].len = len * 2;
      heap_used += len;
    }
    head += len;
  }
  vm_gc_slot_t *old_heap = gc->heap;
  vm_gc_slot_t *old_off_heap = gc->off_heap;
  gc->heap = old_off_heap;
  gc->off_heap = old_heap;
  gc->heap_used = heap_used;
  for (vm_obj_t *cur = gc->start; cur < gc->end; cur++) {
    *cur = vm_gc_update(gc, *cur);
  }
  if (gc->shrink || heap_used * gc->grow / 100 > gc->max) {
    gc->max = heap_used * gc->grow / 100;
  }
}

void vm_gc_collect(vm_gc_t *gc) {
  if (gc->heap_used <= gc->max) {
    return;
  }
  vm_gc_collect_once(gc);
}

void vm_gc_grow(vm_gc_t *gc, size_t count) {
  if (gc->heap_used + count >= gc->heap_alloc) {
    gc->heap_alloc = (gc->heap_used + count) * 2;
    gc->heap = vm_realloc(gc->heap, sizeof(vm_gc_slot_t) * gc->heap_alloc);
    gc->off_heap = vm_realloc(gc->off_heap, sizeof(vm_gc_slot_t) * gc->heap_alloc);
  }
}

vm_gc_t vm_gc_init(vm_config_t config) {
  vm_gc_t ret;
  ret.heap_used = 0;
  ret.grow = config.gc_ents;
  ret.max = config.gc_init;
  ret.heap_alloc = config.gc_ents;
  ret.heap = vm_malloc(sizeof(vm_gc_slot_t) * ret.heap_alloc);
  ret.off_heap = vm_malloc(sizeof(vm_gc_slot_t) * ret.heap_alloc);
  ret.start = NULL;
  return ret;
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
  vm_gc_grow(gc, count + 1);
  gc->heap_used += count + 1;
  gc->heap[start] = (vm_gc_slot_t){
      .len = count * 2,
  };
  return (start + 1) * 2 + 1;
}

vm_obj_t vm_gc_get(vm_gc_t *gc, vm_obj_t obj, size_t index) {
  return gc->heap[obj / 2 + index].val;
}

void vm_gc_set(vm_gc_t *gc, vm_obj_t obj, size_t index, vm_obj_t value) {
  gc->heap[obj / 2 + index].val = value;
}

size_t vm_gc_len(vm_gc_t *gc, vm_obj_t obj) {
  return gc->heap[obj / 2 - 1].len / 2;
}
