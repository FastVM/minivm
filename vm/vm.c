
#include "lib.h"
#include "vm.h"

#include "ir.h"
#include "std.h"
#include "backend/backend.h"
#include "gc.h"

vm_t *vm_state_new(void) {
    vm_obj_t *base = vm_malloc(sizeof(vm_obj_t) * (1 << 20));
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

    {
        vm_externs_t *cur = vm->externs;
        while (cur != NULL) {
            vm_externs_t *last = cur->last;
            vm_free(cur);
            cur = last;
        }
    }
    vm_free(vm->regs);
    vm_free(vm);
}

vm_obj_t vm_obj_of_string(vm_t *vm, const char *str) {
    vm_obj_t ret = vm_obj_of_buffer(vm_io_buffer_from_str(str));
    vm_gc_add(vm, ret);
    return ret;
}
