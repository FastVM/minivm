#include "gc.h"

enum
{
  VM_GC_MARK_FREE = 0,
  VM_GC_MARK_TO_FREE = 1,
  VM_GC_MARK_TO_KEEP = 2,
  VM_GC_MARK_ROOT = 3,
};

void vm_gc_init(vm_gc_t *restrict gc)
{
  size_t alloc = 1 << 24;
  gc->used = 0;
  gc->free = NULL;
  gc->low = vm_alloc0(sizeof(vm_pair_t) * alloc);
  gc->high = &gc->low[alloc - 1];
  gc->marks = vm_alloc0(sizeof(uint8_t) * alloc);
  gc->count = 0;
  gc->maxcount = 1 << 12;
}

void vm_gc_deinit(vm_gc_t *restrict gc)
{
  vm_free(gc->low);
  vm_free(gc->marks);
}

void *vm_gc_alloc(vm_gc_t *restrict gc)
{
  if (gc->free != NULL)
  {
    vm_pair_t *ret = gc->free;
    gc->free = (vm_pair_t *)ret->second;
    gc->marks[ret - gc->low] = VM_GC_MARK_TO_FREE;
    return ret;
  }
  size_t nth = gc->used++;
  gc->marks[nth] = VM_GC_MARK_TO_FREE;
  return &gc->low[nth];
}

void *vm_gc_alloc_root(vm_gc_t *restrict gc)
{
  size_t nth = gc->used++;
  gc->marks[nth] = VM_GC_MARK_ROOT;
  return &gc->low[nth];
}

void vm_gc_dealloc(vm_gc_t *restrict gc, vm_pair_t *pair)
{
  gc->marks[pair - gc->low] = VM_GC_MARK_FREE;
  pair->second = (size_t)gc->free;
  gc->free = pair;
}

void vm_gc_mark(vm_gc_t *restrict gc, size_t val)
{
  vm_pair_t *pval = (vm_pair_t *)val;
  if (gc->low <= pval && pval <= gc->high && (val - (size_t)gc->low) % sizeof(vm_pair_t) == 0)
  {
    size_t nth = pval - gc->low;
    if (gc->marks[nth] == VM_GC_MARK_TO_FREE)
    {
      gc->marks[nth] = VM_GC_MARK_TO_KEEP;
      vm_gc_mark(gc, pval->first);
      vm_gc_mark(gc, pval->second);
    }
  }
}

void vm_gc_collect(vm_gc_t *restrict gc, size_t nstack, vm_value_t *stack)
{
  gc->count++;
  if (gc->count >= gc->maxcount)
  {
    size_t count = 0;
    for (size_t i = 0; i < nstack; i++)
    {
      vm_gc_mark(gc, stack[i].u);
    }
    for (size_t i = 0; i < gc->used; i++)
    {
      size_t mark = gc->marks[i];
      if (mark == VM_GC_MARK_TO_FREE)
      {
        vm_gc_dealloc(gc, &gc->low[i]);
      }
      else if (mark == VM_GC_MARK_TO_KEEP)
      {
        gc->marks[i] = VM_GC_MARK_TO_FREE;
        count += 1;
      }
    }
    gc->count = 0;
    gc->maxcount = count;
  }
}
