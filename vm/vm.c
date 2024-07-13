
#include "./lib.h"
#include "./vm.h"

#include "./ir.h"
#include "./std.h"
#include "./gc.h"
#include "./backend/backend.h"

vm_t *vm_state_new(void) {
    vm_t *vm = vm_malloc(sizeof(vm_t));
    *vm = (vm_t) {
        .use_num = VM_USE_NUM_F64,
        .regs = vm_malloc(sizeof(vm_obj_t) * 65536),
    };
    vm_std_new(vm);
    vm->blocks = vm_malloc(sizeof(vm_blocks_t));
    *vm->blocks = (vm_blocks_t) {0};
    return vm;
}

void vm_state_delete(vm_t *vm) {
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

vm_obj_t vm_state_load(vm_t *vm, const char *str, const char *filename) {
    vm_block_t *entry = vm_compile(vm, str, filename ? filename : "__no_name__");

    vm_closure_t *closure = vm_malloc(sizeof(vm_closure_t) + sizeof(vm_obj_t) * 1);
    closure->len = 1;
    closure->values[0] = (vm_obj_t){
        .tag = VM_TAG_FUN,
        .value.i32 = (int32_t)entry->id,
    };

    return (vm_obj_t){
        .tag = VM_TAG_CLOSURE,
        .value.closure = closure,
    };
}

vm_obj_t vm_state_invoke(vm_t *vm, vm_obj_t obj, size_t nargs, vm_obj_t *args) {
    vm_obj_t ret;
    switch (obj.tag) {
        case VM_TAG_CLOSURE: {
            vm->regs[0] = obj;
            for (size_t i = 0; i < nargs; i++) {
                vm->regs[i + 1] = args[i];
            }
            ret = vm_run_repl(vm, vm->blocks->blocks[obj.value.closure->values[0].value.i32]);
            break;
        }
        case VM_TAG_FFI: {
            for (size_t i = 0; i < nargs; i++) {
                vm->regs[i] = args[i];
            }
            vm->regs[nargs] = (vm_obj_t) {
                .tag = VM_TAG_UNK,
            };
            vm_obj_t *regs_init = vm->regs;
            vm->regs += nargs + 1;
            obj.value.ffi(vm, regs_init);
            vm->regs = regs_init;
            break;
        }
    }
    return ret;
}
