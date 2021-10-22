#pragma once
struct vm_gc_t;
struct vm_gc_entry_t;
typedef struct vm_gc_t vm_gc_t;
typedef struct vm_gc_entry_t vm_gc_entry_t;

#define VM_GC_ADD (1000)
#define VM_GC_MUL (1.1)

#include "libc.h"
#include "obj.h"
#include "vm.h"

void vm_gc_start(vm_gc_t *out);
void vm_gc_stop(vm_gc_t *gc);

void vm_gc_run1(vm_gc_t *gc, vm_obj_t *low, vm_obj_t *high);

int vm_gc_type(vm_gc_entry_t *ent);

vm_gc_entry_t *vm_gc_array_new(vm_gc_t *gc, size_t len);
vm_gc_entry_t *vm_gc_string_new(vm_gc_t *gc, size_t len);

size_t vm_gc_sizeof(vm_gc_entry_t *ptr);
vm_obj_t vm_gc_get_index(vm_gc_entry_t *ptr, vm_obj_t index);
void vm_gc_set_index(vm_gc_entry_t *ptr, vm_obj_t index, vm_obj_t value);
vm_obj_t vm_gc_concat(vm_gc_t *gc, vm_obj_t lhs, vm_obj_t rhs);

struct vm_gc_entry_t
{
    bool keep;
    uint16_t type;
    uint32_t len;
    uint8_t obj[0];
};

struct vm_gc_t
{
    vm_gc_entry_t **objs;
    size_t len;
    size_t alloc;
    size_t max;
};