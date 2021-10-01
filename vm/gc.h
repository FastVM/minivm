#pragma once
#include <vm/libc.h>
#include <vm/obj.h>

struct vm_gc_t;
typedef struct vm_gc_t vm_gc_t;
typedef struct vm_gc_entry_t vm_gc_entry_t;

vm_gc_t *vm_gc_start(void);
void vm_gc_stop(vm_gc_t *gc);
void vm_gc_run(vm_gc_t *gc, vm_obj_t *base, vm_obj_t *useful, vm_obj_t *end);

vm_obj_t vm_gc_new(vm_gc_t *gc, int size, vm_obj_t *values);
int vm_gc_sizeof(vm_gc_t *gc, uint64_t ptr);
vm_gc_entry_t vm_gc_get(vm_gc_t *gc, uint64_t ptr);

struct vm_gc_entry_t
{
    uint64_t ptr;
    union
    {
        uint64_t u64;
        struct
        {
            uint32_t tag;
            uint32_t len;
        };
        vm_obj_t obj;
    };
};

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
    int mark_no;
    int mark_yes;
};
