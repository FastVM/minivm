#pragma once
struct vm_gc_t;
struct vm_gc_entry_t;
struct vm_gc_obj_t;
typedef struct vm_gc_t vm_gc_t;
typedef struct vm_gc_entry_t vm_gc_entry_t;
typedef struct vm_gc_obj_t vm_gc_obj_t;

#include "libc.h"
#include "obj.h"
#include "vm.h"

void vm_gc_start(vm_gc_t *out);
void vm_gc_stop(vm_gc_t *gc);
void vm_gc_run1(vm_gc_t *gc, vm_obj_t *low, vm_obj_t *high);

int vm_gc_type(vm_gc_entry_t *ent);

vm_gc_entry_t *vm_gc_array_new(vm_gc_t *gc, size_t len);
vm_gc_entry_t *vm_gc_static_array_new(vm_gc_t *gc, size_t len);

void vm_gc_push(vm_gc_entry_t *to, vm_obj_t from);
void vm_gc_extend(vm_gc_entry_t *to, vm_gc_entry_t *from);
int vm_gc_sizeof(vm_gc_entry_t *ptr);
vm_obj_t vm_gc_get_index(vm_gc_entry_t *ptr, int index);
void vm_gc_set_index(vm_gc_entry_t *ptr, int index, vm_obj_t value);
vm_obj_t vm_gc_concat(vm_gc_t *gc, vm_obj_t lhs, vm_obj_t rhs);
vm_obj_t vm_gc_static_concat(vm_gc_t *gc, vm_obj_t lhs, vm_obj_t rhs);

struct vm_gc_t
{
    vm_gc_entry_t *first;
    int64_t remain;
};

struct vm_gc_entry_t
{
    vm_gc_entry_t *next;
    uint32_t keep: 1;
    uint32_t alloc: 31;
    uint32_t stat: 1;
    uint32_t len: 31;
    union
    {
        vm_obj_t *ptr;
        vm_obj_t arr[0];
    };
};
