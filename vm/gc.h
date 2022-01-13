#pragma once

#include "libc.h"

struct vm_gc_entry_t;
typedef struct vm_gc_entry_t vm_gc_entry_t;

#include "type.h"

struct vm_gc_entry_t {
  size_t len;
  vm_obj_t arr[0];
};

#include "vm.h"

void vm_gc_start(void);

vm_gc_entry_t *vm_gc_static_array_new(size_t len);

vm_obj_t vm_gc_static_concat(vm_obj_t lhs, vm_obj_t rhs);
