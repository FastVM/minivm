#include "gc.h"
#include "config.h"
#include "obj.h"

#define VM_GC_DATA_TYPE uint32_t
#define VM_GC_HIGHEST_BIT (sizeof(VM_GC_DATA_TYPE) * 8 - 1)
#define VM_GC_MASK (1L << VM_GC_HIGHEST_BIT)

#define gc_mem(gc) (gc->mem)
#define gc_xmem(gc) (gc->xmem)

bool vm_gc_owns(vm_gc_t *gc, vm_gc_entry_t *ent) {
  return (void *)&gc_mem(gc)[0] <= (void *)ent &&
         (void *)ent < (void *)&gc_mem(gc)[gc->alloc];
}

bool vm_gc_xowns(vm_gc_t *gc, vm_gc_entry_t *ent) {
  return (void *)&gc_xmem(gc)[0] <= (void *)ent &&
         (void *)ent < (void *)&gc_xmem(gc)[gc->alloc];
}

void vm_gc_mark_ptr(vm_gc_t *gc, vm_gc_entry_t *ent) {
  if (ent->data >= (VM_GC_MASK)) {
    return;
  }
  size_t data = ent->data;
  ent->data |= (VM_GC_MASK);
  for (size_t cur = 0; cur < data; cur++) {
    vm_obj_t obj = ent->arr[cur];
    if (vm_obj_is_ptr(obj)) {
      vm_gc_mark_ptr(gc, vm_obj_to_ptr(gc, obj));
    }
  }
}

void vm_gc_run1_mark(vm_gc_t *gc, vm_obj_t *low) {
  for (vm_obj_t *base = low; base < low + VM_LOCALS_UNITS; base++) {
    vm_obj_t obj = *base;
    if (vm_obj_is_ptr(obj)) {
      vm_gc_entry_t *ptr = vm_obj_to_ptr(gc, obj);
      vm_gc_mark_ptr(gc, ptr);
    }
  }
}

void vm_gc_update_any(vm_gc_t *gc, vm_obj_t *obj) {
  if (!vm_obj_is_ptr(*obj)) {
    return;
  }
  vm_gc_entry_t *ent = vm_obj_to_ptr(gc, *obj);
  if (vm_gc_xowns(gc, ent)) {
    return;
  }
  vm_gc_entry_t *put = (void *)&gc_xmem(gc)[ent->data];
  *obj = vm_obj_of_ptr(gc, put);
  for (size_t cur = 0; cur < put->data; cur++) {
    vm_gc_update_any(gc, &put->arr[cur]);
  }
}

void vm_gc_run1_update(vm_gc_t *gc, vm_obj_t *low) {
  for (vm_obj_t *base = low; base < low + VM_LOCALS_UNITS; base++) {
    vm_gc_update_any(gc, base);
  }
}

size_t vm_gc_run1_move(vm_gc_t *gc) {
  size_t max = gc->len;
  size_t pos = 0;
  size_t out = 0;
  size_t yes = 0;
  size_t no = 0;
  while (pos < max) {
    vm_gc_entry_t *ent = (void *)&gc_mem(gc)[pos];
    size_t count =
        sizeof(vm_gc_entry_t) + sizeof(vm_obj_t) * (ent->data & ~(VM_GC_MASK));
    if (ent->data >= (VM_GC_MASK)) {
      vm_gc_entry_t *put = (void *)&gc_xmem(gc)[out];
      size_t len = ent->data & ~(VM_GC_MASK);
      put->data = len;
      for (size_t i = 0; i < len; i++) {
        put->arr[i] = ent->arr[i];
      }
      ent->data = out;
      yes += 1;
      out += count;
    } else {
      no += 1;
    }
    pos += count;
  }
  return out;
}

void vm_gc_run1_impl(vm_gc_t *gc, vm_obj_t *low) {
  vm_gc_run1_mark(gc, low);
  size_t len = vm_gc_run1_move(gc);
  vm_gc_run1_update(gc, low);
  gc->up = !gc->up;
  gc->len = len;

#if VM_SHRINK_GC_CAP
  gc->max = gc->len * (VM_MEM_GROWTH);
#else
  if (gc->len * 2 > gc->max) {
    gc->max = gc->len * (VM_MEM_GROWTH);
  }
#endif

  gc->xmem = (&gc->base[!gc->up * gc->alloc]);
  gc->mem = (&gc->base[gc->up * gc->alloc]);
}

#if defined(VM_TIME_GC)
#include <sys/time.h>
int printf(const char *fmt, ...);
#endif

void vm_gc_run1(vm_gc_t *gc, vm_obj_t *low) {
  if (gc->max > gc->len) {
    return;
  }
#if defined(VM_TIME_GC)
  double size = (double)gc->len / 1000 / 1000;

  struct timeval start;
  gettimeofday(&start, NULL);

  vm_gc_run1_impl(gc, low);

  struct timeval end;
  gettimeofday(&end, NULL);

  double diff =
      (double)((1000000 + end.tv_usec - start.tv_usec) % 1000000) / 1000;
  static double total = 0;
  total += diff;

  double endsize = (double)gc->len / 1000 / 1000;

  printf("%.0lfms (+ %.3lfms) (%.3lfMiB -> %.3lfMiB)\n", total, diff, size,
         endsize);
#else
  vm_gc_run1_impl(gc, low);
#endif
}

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
