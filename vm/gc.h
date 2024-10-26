#if !defined(VM_HEADER_GC)
#define VM_HEADER_GC

#include "vm.h"

void vm_gc_run(vm_t *vm, vm_obj_t *top);
void vm_gc_init(vm_t *vm);
void vm_gc_deinit(vm_t *vm);

void vm_gc_add(vm_t *vm, vm_obj_t obj);

#endif
