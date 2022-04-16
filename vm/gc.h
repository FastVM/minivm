#pragma once

#include "lib.h"

struct vm_pair_t;
typedef struct vm_pair_t vm_pair_t;

struct vm_gc_t;
typedef struct vm_gc_t vm_gc_t;

struct vm_pair_t
{
  size_t first;
  size_t second;
};

struct vm_gc_t
{
  size_t used;
  vm_pair_t *free;
  vm_pair_t *low;
  vm_pair_t *high;
  uint8_t *marks;
  size_t count;
  size_t maxcount;
};

void vm_gc_init(vm_gc_t *gc);
void vm_gc_deinit(vm_gc_t *gc);
void *vm_gc_alloc(vm_gc_t *gc);
void *vm_gc_alloc_root(vm_gc_t *gc);
void vm_gc_dealloc(vm_gc_t *gc, vm_pair_t *pair);
void vm_gc_collect(vm_gc_t *gc, size_t nstack, size_t *stack);