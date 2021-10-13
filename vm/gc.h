#pragma once
struct vm_gc_t;
struct vm_gc_entry_t;
typedef struct vm_gc_t vm_gc_t;
typedef struct vm_gc_entry_t vm_gc_entry_t;

#include "libc.h"
#include "obj.h"
#include "vm.h"

    void
    vm_gc_start(vm_gc_t *out, vm_obj_t *base, size_t nlocals);
void vm_gc_stop(vm_gc_t *gc);

vm_obj_t vm_gc_new(vm_gc_t *gc, size_t len, vm_obj_t *values);
size_t vm_gc_sizeof(vm_gc_t *gc, vm_gc_entry_t *ptr);
vm_obj_t vm_gc_get_index(vm_gc_t *gc, vm_gc_entry_t *ptr, size_t index);

#define VM_GC_ENTRY_TYPE_PTR 0
#define VM_GC_ENTRY_TYPE_OBJ 1

struct vm_gc_entry_t
{
    uint32_t len;
    uint32_t keep;
    vm_obj_t obj[];
};

struct vm_gc_t
{
    // vm_gc_entry_array_t objs1;
    size_t len;
    size_t max;
    vm_gc_entry_t **objs2;
    vm_obj_t *base;
    size_t nlocals;
    size_t calls;
};
