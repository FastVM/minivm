#if !defined(VM_HEADER_GC)
#define VM_HEADER_GC

#include "lib.h"

void vm_gc_mark(vm_t *vm, vm_obj_t *top);
void vm_gc_sweep(vm_t *vm);

void vm_gc_run(vm_t *vm, vm_obj_t *top);
void vm_gc_init(vm_t *vm);
void vm_gc_deinit(vm_t *vm);

void vm_gc_add(vm_t *vm, vm_obj_t obj);

vm_obj_table_t *vm_table_new_size(vm_t *vm, size_t pow2);

#endif
