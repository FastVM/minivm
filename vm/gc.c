
#include "./gc.h"
#include "./ir.h"

struct vm_gc_objs_t;
struct vm_gc_t;

typedef struct vm_gc_t vm_gc_t;
typedef struct vm_gc_objs_t vm_gc_objs_t;

struct vm_gc_objs_t {
    size_t alloc;
    size_t len;
    vm_obj_t *objs;
};

struct vm_gc_t {
    vm_gc_objs_t objs;
};

static void vm_gc_mark_obj(vm_obj_t obj) {
    switch (obj.tag) {
        case VM_TAG_NIL: {
            break;
        }
        case VM_TAG_BOOL: {
            break;
        }
        case VM_TAG_I8: {
            break;
        }
        case VM_TAG_I16: {
            break;
        }
        case VM_TAG_I32: {
            break;
        }
        case VM_TAG_I64: {
            break;
        }
        case VM_TAG_F32: {
            break;
        }
        case VM_TAG_F64: {
            break;
        }
        case VM_TAG_STR: {
            break;
        }
        case VM_TAG_CLOSURE: {
            vm_closure_t *closure = obj.value.closure;
            closure->mark = true;
            for (size_t i = 0; i < closure->len; i++) {
                vm_gc_mark_obj(closure->values[i]);
            }
            break;
        }
        case VM_TAG_FUN: {
            break;
        }
        case VM_TAG_TAB: {
            vm_table_t *table = obj.value.table;
            table->mark = true;
            for (size_t i = 0; i < table->len; i++) {
                vm_gc_mark_obj((vm_obj_t) {
                    .tag = table->pairs[i].key_tag,
                    .value = table->pairs[i].key_val,
                });
                vm_gc_mark_obj((vm_obj_t) {
                    .tag = table->pairs[i].val_tag,
                    .value = table->pairs[i].val_val,
                });
            }
            break;
        }
        case VM_TAG_FFI: {
            break;
        }
        case VM_TAG_ERROR: {
            break;
        }
    }
}

static void vm_gc_mark_arg(vm_arg_t arg) {
    if (arg.type == VM_ARG_LIT) {
        vm_gc_mark_obj(arg.lit);
    }
}

void vm_gc_mark(vm_t *vm) {
    vm_gc_mark_obj(vm->std);
    for (vm_obj_t *head = vm->base; head != vm->regs; head++) {
        vm_gc_mark_obj(*head);
    }
    for (size_t i = 0; i < vm->blocks->len; i++) {
        vm_block_t *block = vm->blocks->blocks[i];
        for (size_t j = 0; j < block->len; j++) {
            vm_instr_t *instr = &block->instrs[j];
            for (size_t k = 0; instr->args[k].type != VM_ARG_NONE; k++) {
                vm_gc_mark_arg(instr->args[k]);
            }
        }
        for (size_t j = 0; block->branch.args[j].type != VM_ARG_NONE; j++) {
            vm_gc_mark_arg(block->branch.args[j]);
        }
        vm_gc_mark_arg(block->branch.out);
    }
}

void vm_gc_sweep(vm_t *vm) {

}

void vm_gc_run(vm_t *vm) {
    vm_gc_mark(vm);
    vm_gc_sweep(vm);
}

void vm_gc_init(vm_t *vm) {
    vm->gc = vm_malloc(sizeof(vm_gc_t));
}

void vm_gc_deinit(vm_t *vm) {
    vm_gc_sweep(vm);
    vm_free(vm->gc);
}

