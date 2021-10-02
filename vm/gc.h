#pragma once
#include <vm/libc.h>
#include <vm/obj.h>
#include <vm/vm.h>
#include <pthread.h>

struct vm_gc_t;
typedef struct vm_gc_t vm_gc_t;
typedef struct vm_gc_entry_t vm_gc_entry_t;

void vm_gc_start(vm_gc_t *out, vm_obj_t *base, vm_obj_t *end);

vm_obj_t vm_gc_new(vm_gc_t *gc, size_t len, vm_obj_t *values);
size_t vm_gc_sizeof(vm_gc_t *gc, uint64_t ptr);
vm_gc_entry_t vm_gc_get(vm_gc_t *gc, uint64_t ptr);

#define VM_GC_ENTRY_TYPE_PTR 0
#define VM_GC_ENTRY_TYPE_OBJ 1

struct vm_gc_entry_t
{
    int tag : 1;
    uint64_t ptr : 63;
    union
    {
        size_t len;
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
    vm_obj_t *base;
    vm_obj_t *end;
    size_t max1;
    uint64_t last;
    bool die;
    bool not_collecting;
    pthread_t thread;
    pthread_mutex_t lock;
    pthread_cond_t cond;
};
