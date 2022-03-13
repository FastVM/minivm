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
  size_t shrink;
};

vm_gc_t vm_gc_init(vm_config_t config);
void vm_gc_deinit(vm_gc_t gc);

void vm_gc_collect(vm_gc_t *gc);

void vm_gc_set_locals(vm_gc_t *gc, size_t nlocals, vm_obj_t *locals);
vm_obj_t vm_gc_new(vm_gc_t *gc, size_t count);

#define vm_gc_get(gc_, obj_, index_) ((gc_)->heap[(size_t)(obj_) / 2 + (size_t)(index_)].val)
#define vm_gc_set(gc_, obj_, index_, value_) ((gc_)->heap[(size_t)(obj_) / 2 + (size_t)(index_)].val = (value_))
#define vm_gc_len(gc_, obj_) ((size_t)(gc_)->heap[(size_t)(obj_) / 2 - 1].len / 2)
