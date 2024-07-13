
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
    size_t last;
    vm_gc_objs_t objs;
};

static void vm_gc_mark_obj(vm_obj_t obj) {
    switch (obj.tag) {
        default: {
            // not allocated
            break;
        }
        case VM_TAG_STR: {
            vm_io_buffer_t *buffer = obj.value.str;
            if (buffer->mark) {
                break;
            }
            buffer->mark = true;
            break;
        }
        case VM_TAG_CLOSURE: {
            vm_closure_t *closure = obj.value.closure;
            if (closure->mark) {
                break;
            }
            closure->mark = true;
            for (size_t i = 0; i < closure->len; i++) {
                vm_gc_mark_obj(closure->values[i]);
            }
            break;
        }
        case VM_TAG_TAB: {
            vm_table_t *table = obj.value.table;
            if (table->mark) {
                break;
            }
            table->mark = true;
            for (size_t i = 0; i < (1 << table->alloc); i++) {
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
        case VM_TAG_ERROR: {
            // todo
            break;
        }
    }
}

static void vm_gc_mark_arg(vm_arg_t arg) {
    if (arg.type == VM_ARG_LIT) {
        vm_gc_mark_obj(arg.lit);
    }
}

void vm_gc_mark(vm_t *vm, vm_obj_t *top) {
    vm_gc_mark_obj(vm->std);
    for (vm_obj_t *head = vm->base; head < top; head++) {
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
    vm_gc_t *gc = vm->gc;
    size_t write = 0;
    for (size_t i = 0; i < gc->objs.len; i++) {
        vm_obj_t obj = gc->objs.objs[i];
        bool keep = true;
        switch (obj.tag) {
            default: {
                // not allocated
                break;
            }
            case VM_TAG_STR: {
                vm_io_buffer_t *buffer = obj.value.str;
                if (!buffer->mark) {
                    vm_free(buffer->buf);
                    vm_free(buffer);
                    keep = false;
                } else {
                    buffer->mark = false;
                }
                break;
            }
            case VM_TAG_CLOSURE: {
                vm_closure_t *closure = obj.value.closure;
                if (!closure->mark) {
                    vm_free(closure);
                    keep = false;
                } else {
                    closure->mark = false;
                }
                break;
            }
            case VM_TAG_TAB: {
                vm_table_t *table = obj.value.table;
                if (!table->mark) {
                    vm_free_table(table);
                    keep = false;
                } else {
                    table->mark = false;
                }
                break;
            }
            case VM_TAG_ERROR: {
                // todo
                break;
            }
        }
        if (keep) {
            gc->objs.objs[write++] = obj;
        }
    }
    // printf("%zu -> %zu\n", gc->objs.len, write);
    gc->objs.len = write;
    gc->last = write;
}

void vm_gc_run(vm_t *vm, vm_obj_t *top) {
    vm_gc_t *gc = vm->gc;
    if (gc->last * VM_GC_FACTOR >= gc->objs.len) {
        return;
    }
    vm_gc_mark(vm, top);
    vm_gc_sweep(vm);
}

void vm_gc_init(vm_t *vm) {
    vm->gc = vm_malloc(sizeof(vm_gc_t));
    memset(vm->gc, 0, sizeof(vm_gc_t));
}

void vm_gc_deinit(vm_t *vm) {
    vm_gc_sweep(vm);
    vm_gc_t *gc = vm->gc;
    vm_free(gc->objs.objs);
    vm_free(vm->gc);
}

void vm_gc_add(vm_t *vm, vm_obj_t obj) {
    vm_gc_t *gc = vm->gc;
    if (gc->objs.len + 1 >= gc->objs.alloc) {
        gc->objs.alloc = (gc->objs.len + 1) * 2;
        gc->objs.objs = vm_realloc(gc->objs.objs, sizeof(vm_obj_t) * gc->objs.alloc);
    }
    gc->objs.objs[gc->objs.len++] = obj;
}
