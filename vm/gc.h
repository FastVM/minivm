#pragma once
struct vm_gc_t;
struct vm_gc_entry_t;
struct vm_gc_obj_t;
typedef struct vm_gc_t vm_gc_t;
typedef struct vm_gc_entry_t vm_gc_entry_t;
typedef struct vm_gc_obj_t vm_gc_obj_t;

#include "obj.h"
#include "libc.h"

struct vm_gc_t
{
    vm_gc_entry_t *first;
    int64_t remain;
};

enum {
    VM_GC_ENTRY_TYPE_ARRAY,
    VM_GC_ENTRY_TYPE_STATIC_ARRAY,
};

struct vm_gc_entry_t
{
    vm_gc_entry_t *next;
    uint32_t keep: 2;
    uint32_t alloc: 30;
    uint32_t type: 2;
    uint32_t len: 30;
    union
    {
        vm_obj_t *ptr;
        vm_obj_t arr[0];
    };
};

#include "vm.h"

void vm_gc_start(vm_gc_t *out);
void vm_gc_stop(vm_gc_t *gc);
void vm_gc_run1(vm_gc_t *gc, vm_obj_t *low, vm_obj_t *high);

vm_int_t vm_gc_type(vm_gc_entry_t *ent);

vm_obj_t vm_gc_dup(vm_gc_t *gc, vm_obj_t obj);

vm_gc_entry_t *vm_gc_array_new(vm_gc_t *gc, size_t len);
vm_gc_entry_t *vm_gc_static_array_new(vm_gc_t *gc, size_t len);

void vm_gc_push(vm_gc_entry_t *to, vm_obj_t from);
void vm_gc_extend(vm_gc_entry_t *to, vm_gc_entry_t *from);
vm_int_t vm_gc_sizeof(vm_gc_entry_t *ptr);
vm_obj_t vm_gc_get_index(vm_gc_entry_t *ptr, vm_int_t index);
void vm_gc_set_index(vm_gc_entry_t *ptr, vm_int_t index, vm_obj_t value);
vm_obj_t vm_gc_concat(vm_gc_t *gc, vm_obj_t lhs, vm_obj_t rhs);
vm_obj_t vm_gc_static_concat(vm_gc_t *gc, vm_obj_t lhs, vm_obj_t rhs);
