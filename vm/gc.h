#pragma once
#include <vm/nanbox.h>
#include <vm/vector.h>

struct vm_gc_t;
typedef struct vm_gc_t vm_gc_t;

struct vm_gc_t
{
    vec_t ptrs;
};

vm_gc_t vm_gc_start(void);
void vm_gc_stop(vm_gc_t *gc);

void vm_gc_run(vm_gc_t *gc, int len, nanbox_t *ptr);

void vm_gc_mark(vm_gc_t *gc, int len, nanbox_t *ptr);
void vm_gc_sweep(vm_gc_t *gc);

nanbox_t vm_gc_new(vm_gc_t *gc, int size);
int vm_gc_sizeof(vm_gc_t *gc, nanbox_t ptr);
nanbox_t vm_gc_get(vm_gc_t *gc, nanbox_t ptr, int index);
void vm_gc_set(vm_gc_t *gc, nanbox_t ptr, int index, nanbox_t value);
