#pragma once
#include <vm/obj.h>
#include <vm/vector.h>

struct vm_gc_t;
typedef struct vm_gc_t vm_gc_t;

vm_gc_t *vm_gc_start(void);
void vm_gc_stop(vm_gc_t *gc);
void vm_gc_run(vm_gc_t *gc, vm_obj_t *base, vm_obj_t *useful, vm_obj_t *end);

vm_obj_t vm_gc_new(vm_gc_t *gc, int size);
int vm_gc_sizeof(vm_gc_t *gc, vm_obj_t ptr);
vm_obj_t vm_gc_get(vm_gc_t *gc, vm_obj_t ptr, int index);
void vm_gc_set(vm_gc_t *gc, vm_obj_t ptr, int index, vm_obj_t value);

struct vm_gc_t
{
    uint8_t **ptrs;
    int length;
    int alloc;
    int maxlen;
    int state;
};

// for internal use:

// *_even runs when state starts even
void vm_gc_mark_even(vm_gc_t *gc, int len, vm_obj_t *ptr);
void vm_gc_mark_stack_even(vm_gc_t *gc, vm_obj_t *base, vm_obj_t *useful, vm_obj_t *end);
void vm_gc_sweep_even(vm_gc_t *gc);

// *_odd runs when state start odd
void vm_gc_mark_odd(vm_gc_t *gc, int len, vm_obj_t *ptr);
void vm_gc_mark_stack_odd(vm_gc_t *gc, vm_obj_t *base, vm_obj_t *useful, vm_obj_t *end);
void vm_gc_sweep_odd(vm_gc_t *gc);
