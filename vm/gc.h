#if defined(VM_HEADER_GC)
#define VM_HEADER_GC

// stub

bool vm_gc_mark_get(vm_obj_t obj);
vm_obj_t vm_gc_mark_reset(vm_obj_t obj);
vm_obj_t vm_gc_mark_set(vm_obj_t obj);

void vm_gc_mark(vm_t *vm);
void vm_gc_sweep(vm_t *vm);

void vm_gc_run(vm_t *vm);
void vm_gc_init(vm_t *vm);
void vm_gc_deinit(vm_t *vm);

// todo

#endif
