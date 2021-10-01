#pragma once
#include <vm/libc.h>
#include <vm/obj.h>

struct vm_gc_t;
typedef struct vm_gc_t vm_gc_t;
typedef struct vm_gc_entry_t vm_gc_entry_t;

void vm_gc_start(vm_gc_t *out);
void vm_gc_run(vm_gc_t *gc, vm_obj_t *base, vm_obj_t *stop);

vm_obj_t vm_gc_new(vm_gc_t *gc, int len, vm_obj_t *values);
int vm_gc_sizeof(vm_gc_t *gc, uint64_t ptr);
vm_gc_entry_t vm_gc_get(vm_gc_t *gc, uint64_t ptr);

#define VM_GC_ENTRY_TYPE_PTR 0
#define VM_GC_ENTRY_TYPE_OBJ 1

struct vm_gc_entry_t
{
    uint64_t ptr: 48;
    int tag : 8;
    int type : 8;
    union
    {
        uint32_t len;
        vm_obj_t obj;
    };
};

_Static_assert(sizeof(vm_gc_entry_t) == 16, "bad size");

struct vm_gc_t
{
    vm_gc_entry_t *objs1;
    vm_gc_entry_t *objs2;
    vm_gc_entry_t *objs3;
    vm_gc_entry_t *swap;
    uint64_t last;
    size_t nelems;
    size_t len1;
    size_t len2;
    size_t len3;
    size_t big;
};
