#pragma once
struct vm_gc_t;
struct vm_gc_entry_stats_t;
struct vm_gc_entry_t;
typedef struct vm_gc_t vm_gc_t;
typedef struct vm_gc_entry_stats_t vm_gc_entry_stats_t;
typedef struct vm_gc_entry_t vm_gc_entry_t;

#include <vm/libc.h>
#include <vm/obj.h>
#include <vm/vm.h>
#if defined(VM_GC_THREADS)
#include <signal.h>
#include <pthread.h>
#endif

void vm_gc_start(vm_gc_t *out, vm_obj_t *base, size_t nlocals);
void vm_gc_stop(vm_gc_t *gc);

vm_obj_t vm_gc_new(vm_gc_t *gc, size_t len, vm_obj_t *values);
size_t vm_gc_sizeof(vm_gc_t *gc, uint64_t ptr);
vm_obj_t vm_gc_index(vm_gc_t *gc, uint64_t ptr, size_t index);

#define VM_GC_ENTRY_TYPE_PTR 0
#define VM_GC_ENTRY_TYPE_OBJ 1

struct vm_gc_entry_stats_t
{
    size_t count;
    size_t findall;
    size_t maxfind;
};

struct vm_gc_entry_t
{
    union
    {
        struct
        {
            bool keep : 1;
            size_t len : 15;
            uint64_t ptr : 48;
        };
        size_t xlen;
    };
    vm_obj_t obj;
};

_Static_assert(sizeof(vm_gc_entry_t) == 16, "bad size");


struct vm_gc_t
{
#if defined(VM_GC_THREADS)
    pthread_t thread;
#endif
    vm_gc_entry_t *objs[3];
    vm_obj_t *base;
    size_t nlocals;
    uint64_t last;
    bool die;
    double usage;
};
