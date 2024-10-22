
#include "lib.h"
#include "vm.h"

#include "ir.h"
#include "std.h"
#include "backend/backend.h"
#include "gc.h"

vm_t *vm_state_new(void) {
#if VM_EMPTY_BYTE == 0
    vm_obj_t *base = vm_calloc(sizeof(vm_obj_t) * (1 << 20));
#else
    vm_obj_t *base = vm_malloc(sizeof(vm_obj_t) * (1 << 20));
    memset(base, VM_EMPTY_BYTE, sizeof(vm_obj_t) * (1 << 20));
#endif
    
    vm_t *vm = vm_malloc(sizeof(vm_t));
    *vm = (vm_t) {
        .base = base,
        .regs = base,
        .nblocks = 0,
    };
    vm_gc_init(vm);
    vm_std_new(vm);
    return vm;
}

void vm_state_delete(vm_t *vm) {
    vm_gc_deinit(vm);

    vm_free(vm->regs);
    vm_free(vm);
}
