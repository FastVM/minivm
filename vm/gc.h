#pragma once


union vm_gc_slot_t;
struct vm_gc_t;

typedef union vm_gc_slot_t vm_gc_slot_t;
typedef struct vm_gc_t vm_gc_t;

#include "obj.h"
#include "config.h"

union vm_gc_slot_t {
  vm_obj_t len;
  vm_obj_t newpos;
  vm_obj_t val;
};

struct vm_gc_t {
  size_t heap_used;
  size_t heap_alloc;
  vm_gc_slot_t *heap;
  vm_gc_slot_t *off_heap;
  vm_obj_t *start;
  vm_obj_t *end;
  size_t max;
  size_t grow;
  size_t shrink: 1;
};

vm_gc_t vm_gc_init(vm_config_t config);
void vm_gc_deinit(vm_gc_t gc);

void vm_gc_collect(vm_gc_t *gc);

void vm_gc_set_locals(vm_gc_t *gc, size_t nlocals, vm_obj_t *locals);
vm_obj_t vm_gc_new(vm_gc_t *gc, size_t count);

size_t vm_gc_len(vm_gc_t *gc, vm_obj_t obj);
vm_obj_t vm_gc_get(vm_gc_t *gc, vm_obj_t obj, size_t index);
void vm_gc_set(vm_gc_t *gc, vm_obj_t obj, size_t index, vm_obj_t value);
