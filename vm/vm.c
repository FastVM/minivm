
#include "./lib.h"
#include "./vm.h"

#include "./ir.h"
#include "./std.h"
#include "./backend/backend.h"
#include "./gc.h"

vm_t *vm_state_new(void) {
    vm_obj_t *base = vm_malloc(sizeof(vm_obj_t) * (1 << 20));
    vm_t *vm = vm_malloc(sizeof(vm_t));
    *vm = (vm_t) {
        .base = base,
        .regs = base,
    };
    vm_gc_init(vm);
    vm_std_new(vm);
    vm->blocks = vm_malloc(sizeof(vm_blocks_t));
    *vm->blocks = (vm_blocks_t) {0};
    return vm;
}

void vm_state_delete(vm_t *vm) {
    vm_gc_deinit(vm);

    for (size_t i = 0; i < vm->blocks->len; i++) {
        vm_block_t *block = vm->blocks->blocks[i];
        for (size_t j = 0; j < block->len; j++) {
            vm_instr_t instr = block->instrs[j];
            vm_free(instr.args);
        }
        vm_free(block->branch.args);
        vm_free(block->instrs);
        vm_free(block->code);
        vm_free(block->args);
        vm_free(block);
    }
    {
        vm_blocks_srcs_t *cur = vm->blocks->srcs;
        while (cur != NULL) {
            vm_blocks_srcs_t *last = cur->last;
            vm_free(cur->data);
            vm_free(cur);
            cur = last;
        }
    }
    vm_free(vm->blocks->blocks);
    vm_free(vm->blocks);
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

vm_obj_t vm_str(vm_t *vm, const char *str) {
    vm_obj_t ret = (vm_obj_t) {
        .tag = VM_TAG_STR,
        .value.str = vm_io_buffer_from_str(str),
    };
    vm_gc_add(vm, ret);
    return ret;
}
