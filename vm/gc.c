
#include "gc.h"
#include "ir.h"

struct vm_gc_objs_t;
struct vm_gc_table_cache_t;
struct vm_gc_t;

typedef struct vm_gc_t vm_gc_t;
typedef struct vm_gc_objs_t vm_gc_objs_t;
typedef struct vm_gc_table_cache_t vm_gc_table_cache_t;

struct vm_gc_objs_t {
    size_t alloc;
    size_t len;
    vm_obj_t *objs;
};

struct vm_gc_table_cache_t {
    size_t alloc;
    size_t len;
    vm_table_t **tables;
};

struct vm_gc_t {
    size_t last;
    vm_gc_objs_t objs;
};

static void vm_gc_mark_obj(vm_obj_t obj);

static void vm_gc_mark_arg(vm_ir_arg_t arg) {
    if (arg.type == VM_IR_ARG_TYPE_LIT) {
        vm_gc_mark_obj(arg.lit);
    }
}

static void vm_gc_mark_block(vm_ir_block_t *block) {
    if (!block->mark) {
        block->mark = true;
        for (size_t j = 0; j < block->len; j++) {
            vm_ir_instr_t *instr = &block->instrs[j];
            for (size_t k = 0; instr->args[k].type != VM_IR_ARG_TYPE_NONE; k++) {
                vm_gc_mark_arg(instr->args[k]);
            }
        }
        for (size_t j = 0; j < 2 && block->branch.targets[j] != NULL; j++) {
            vm_gc_mark_block(block->branch.targets[j]);
        }
        for (size_t k = 0; block->branch.args[k].type != VM_IR_ARG_TYPE_NONE; k++) {
            vm_gc_mark_arg(block->branch.args[k]);
        }
        vm_gc_mark_arg(block->branch.out);
    }
}

static void vm_gc_mark_obj(vm_obj_t obj) {
    if (vm_obj_is_string(obj)) {
        vm_io_buffer_t *buffer = vm_obj_get_string(obj);
        buffer->mark = true;
    } else if (vm_obj_is_closure(obj)) {
        vm_closure_t *closure = vm_obj_get_closure(obj);
        if (!closure->mark) {
            closure->mark = true;
            for (size_t i = 0; i < closure->len; i++) {
                vm_gc_mark_obj(closure->values[i]);
            }
        }
        vm_gc_mark_block(closure->block);
    } else if (vm_obj_is_table(obj)) {
        vm_table_t *table = vm_obj_get_table(obj);
        if (!table->mark) {
            table->mark = true;
            for (size_t i = 0; i < (1 << table->alloc); i++) {
                vm_gc_mark_obj(table->pairs[i].key);
                vm_gc_mark_obj(table->pairs[i].value);
            }
        }
    } else if (vm_obj_is_block(obj)) {
        vm_gc_mark_block(vm_obj_get_block(obj));
    }
}

void vm_gc_mark(vm_t *vm, vm_obj_t *top) {
    for (vm_ir_blocks_t *blocks = vm->blocks; blocks; blocks = blocks->next) {
        vm_gc_mark_block(blocks->block);
    }
    vm_gc_mark_obj(vm->std);
    for (vm_obj_t *head = vm->base; head < top; head++) {
        vm_gc_mark_obj(*head);
    }
}

void vm_gc_sweep(vm_t *vm) {
    vm_gc_t *gc = vm->gc;
    size_t write = 0;
    for (size_t i = 0; i < gc->objs.len; i++) {
        vm_obj_t obj = gc->objs.objs[i];
        if (vm_obj_is_string(obj)) {
            vm_io_buffer_t *buffer = vm_obj_get_string(obj);
            if (!buffer->mark) {
                vm_free(buffer->buf);
                vm_free(buffer);
            } else {
                buffer->mark = false;
                gc->objs.objs[write++] = obj;
            }
        } else if (vm_obj_is_closure(obj)) {
            vm_closure_t *closure = vm_obj_get_closure(obj);
            if (!closure->mark) {
                vm_free(closure);
            } else {
                closure->mark = false;
                gc->objs.objs[write++] = obj;
            }
        } else if (vm_obj_is_table(obj)) {
            vm_table_t *table = vm_obj_get_table(obj);
            if (!table->mark) {
                if (!table->pairs_auto) {
                    vm_free(table->pairs);
                }
                vm_free(table);
            } else {
                table->mark = false;
                gc->objs.objs[write++] = obj;
            }
        } else if (vm_obj_is_block(obj)) {
            vm_ir_block_t *block = vm_obj_get_block(obj);
            if (!block->mark) {
                for (size_t j = 0; j < block->len; j++) {
                    vm_ir_instr_t instr = block->instrs[j];
                    vm_free(instr.args);
                }
                vm_free(block->instrs);
                vm_free(block->branch.args);
                vm_free(block->code);
                vm_free(block);
            } else {
                block->mark = false;
                gc->objs.objs[write++] = obj;
            }
        }
    }
    gc->objs.len = write;
    size_t next = write * VM_GC_FACTOR;
    if (next >= gc->last) {
        gc->last = next;
    }
}

void vm_table_init_size(vm_table_t *ret, size_t pow2);

vm_table_t *vm_table_new_size(vm_t *vm, size_t pow2) {
    vm_gc_t *gc = vm->gc;
    vm_table_t *ret = vm_malloc(sizeof(vm_table_t) + sizeof(vm_table_pair_t) * (1 << pow2));
    ret->pairs = (vm_table_pair_t *)&ret[1];
    memset(ret->pairs, VM_EMPTY_BYTE, sizeof(vm_table_pair_t) * (1 << pow2));
    ret->alloc = pow2;
    ret->used = 0;
    ret->len = 0;
    ret->mark = false;
    ret->pairs_auto = true;
    vm_gc_add(vm, vm_obj_of_table(ret));
    return ret;
}

vm_table_t *vm_table_new(vm_t *vm) {
    return vm_table_new_size(vm, 2);
}

void vm_gc_run(vm_t *vm, vm_obj_t *top) {
    vm_gc_t *gc = vm->gc;
    if (gc->last >= gc->objs.len) {
        return;
    }
    vm_gc_mark(vm, top);
    vm_gc_sweep(vm);
}

void vm_gc_init(vm_t *vm) {
    vm->gc = vm_malloc(sizeof(vm_gc_t));
    vm_gc_t *gc = vm->gc;
    *gc = (vm_gc_t){
        .last = VM_GC_MIN,
    };
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
        gc->objs.alloc = (gc->objs.len) * VM_GC_FACTOR + 1;
        gc->objs.objs = vm_realloc(gc->objs.objs, sizeof(vm_obj_t) * gc->objs.alloc);
    }
    gc->objs.objs[gc->objs.len++] = obj;
}
