
#include "gc.h"
#include "ir.h"
#include "primes.inc"

#define vm_get_gc(vm) ((vm_gc_t *) (vm)->gc)

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
    vm_obj_table_t **tables;
};

struct vm_gc_t {
    size_t last;
    vm_gc_objs_t keep;
    vm_gc_objs_t todo;
    vm_gc_objs_t objs;
};

static inline void vm_gc_objs_push(vm_gc_objs_t *restrict objs, vm_obj_t obj) {
    if (objs->len + 1 >= objs->alloc) {
        objs->alloc = (objs->len) * VM_GC_FACTOR + 1;
        objs->objs = vm_realloc(objs->objs, sizeof(vm_obj_t) * objs->alloc);
    }
    objs->objs[objs->len++] = obj;
}

static inline vm_obj_t vm_gc_objs_pop(vm_gc_objs_t *restrict objs) {
    return objs->objs[--objs->len];
}

static inline bool vm_gc_objs_is_empty(vm_gc_objs_t *restrict objs) {
    return objs->len == 0;
}

static inline void vm_gc_objs_clear(vm_gc_objs_t *restrict objs) {
    objs->len = 0;
}

static inline void vm_gc_objs_free(vm_gc_objs_t *restrict objs) {
    vm_free(objs->objs);
    *objs = (vm_gc_objs_t) {
        .objs = NULL,
    };
}

static inline void vm_gc_mark_obj(vm_t *vm, vm_obj_t obj);

static inline void vm_gc_mark_arg(vm_t *vm, vm_ir_arg_t arg) {
    if (arg.type == VM_IR_ARG_TYPE_LIT) {
        vm_gc_mark_obj(vm, arg.lit);
    }
}

static inline void vm_gc_mark_block(vm_t *vm, vm_ir_block_t *block) {
    if (!block->header.mark) {
        block->header.mark = true;
        for (size_t j = 0; j < block->len; j++) {
            vm_ir_instr_t *instr = &block->instrs[j];
            for (size_t k = 0; instr->args[k].type != VM_IR_ARG_TYPE_NONE; k++) {
                vm_gc_mark_arg(vm, instr->args[k]);
            }
        }
        for (size_t j = 0; j < 2 && block->branch.targets[j] != NULL; j++) {
            vm_gc_mark_block(vm, block->branch.targets[j]);
        }
        for (size_t k = 0; block->branch.args[k].type != VM_IR_ARG_TYPE_NONE; k++) {
            vm_gc_mark_arg(vm, block->branch.args[k]);
        }
        vm_gc_mark_arg(vm, block->branch.out);
    }
}

static inline void vm_gc_mark_run(vm_t *vm) {
    while (!vm_gc_objs_is_empty(&vm_get_gc(vm)->todo)) {
        vm_obj_t obj = vm_gc_objs_pop(&vm_get_gc(vm)->todo);
        if (vm_obj_is_string(obj)) {
            vm_io_buffer_t *buffer = vm_obj_get_string(obj);
            if (!buffer->header.mark) {
                buffer->header.mark = true;
                vm_gc_objs_push(&vm_get_gc(vm)->keep, obj);
            }
        } else if (vm_obj_is_closure(obj)) {
            vm_obj_closure_t *closure = vm_obj_get_closure(obj);
            if (!closure->header.mark) {
                closure->header.mark = true;
                vm_gc_objs_push(&vm_get_gc(vm)->keep, obj);
                for (size_t i = 0; i < closure->len; i++) {
                    vm_gc_mark_obj(vm, closure->values[i]);
                }
            }
            vm_gc_mark_block(vm, closure->block);
        } else if (vm_obj_is_table(obj)) {
            vm_obj_table_t *table = vm_obj_get_table(obj);
            if (!table->header.mark) {
                table->header.mark = true;
                vm_gc_objs_push(&vm_get_gc(vm)->keep, obj);
                uint64_t len = vm_primes_table[table->size];
                vm_table_pair_t *pairs = table->pairs;
                for (size_t i = 0; i < len; i++) {
                    vm_gc_mark_obj(vm, pairs[i].key);
                    vm_gc_mark_obj(vm, pairs[i].value);
                }
            }
        } else if (vm_obj_is_block(obj)) {
            vm_gc_mark_block(vm, vm_obj_get_block(obj));
        }
    }
}

static inline void vm_gc_mark_obj(vm_t *vm, vm_obj_t obj) {
    vm_gc_objs_push(&vm_get_gc(vm)->todo, obj);
}

static inline void vm_gc_maybe_reset_obj(vm_t *vm, vm_obj_t obj) {
    if (vm_obj_is_string(obj)) {
        vm_io_buffer_t *buffer = vm_obj_get_string(obj);
        if (!buffer->header.mark) {
            vm_free(buffer->buf);
            vm_free(buffer);
        } else {
            buffer->header.mark = false;
        }
    } else if (vm_obj_is_closure(obj)) {
        vm_obj_closure_t *closure = vm_obj_get_closure(obj);
        if (!closure->header.mark) {
            vm_free(closure);
        } else {
            closure->header.mark = false;
        }
    } else if (vm_obj_is_table(obj)) {
        vm_obj_table_t *table = vm_obj_get_table(obj);
        if (!table->header.mark) {
            vm_free(table->pairs);
            vm_free(table);
        } else {
            table->header.mark = false;
        }
    } else if (vm_obj_is_block(obj)) {
        vm_ir_block_t *block = vm_obj_get_block(obj);
        if (!block->header.mark) {
            for (size_t j = 0; j < block->len; j++) {
                vm_ir_instr_t instr = block->instrs[j];
                vm_free(instr.args);
            }
            vm_free(block->instrs);
            vm_free(block->branch.args);
            vm_free(block->code);
            vm_free(block);
        } else {
            block->header.mark = false;
        }
    }
}

void vm_gc_mark(vm_t *vm, vm_obj_t *top) {
    vm_gc_objs_clear(&vm_get_gc(vm)->keep);

    for (vm_ir_blocks_t *blocks = vm->blocks; blocks; blocks = blocks->next) {
        vm_gc_mark_block(vm, blocks->block);
    }
    vm_gc_mark_obj(vm, vm->std);
    for (vm_obj_t *head = vm->base; head < top; head++) {
        vm_gc_mark_obj(vm, *head);
    }
    vm_gc_mark_run(vm);

    printf("%zu -> %zu\n", vm_get_gc(vm)->objs.len, vm_get_gc(vm)->keep.len);

    for (size_t i = 0; i < vm_get_gc(vm)->objs.len; i++) {
        vm_gc_maybe_reset_obj(vm, vm_get_gc(vm)->objs.objs[i]);    
    }

    vm_gc_objs_t old_keep = vm_get_gc(vm)->keep;
    vm_gc_objs_t old_objs = vm_get_gc(vm)->objs;
    vm_get_gc(vm)->objs = old_keep;
    vm_get_gc(vm)->keep = old_objs;

    vm_get_gc(vm)->last = vm_get_gc(vm)->objs.len * VM_GC_FACTOR;
}

vm_obj_table_t *vm_table_new(vm_t *vm) {
    vm_obj_table_t *ret = vm_malloc(sizeof(vm_obj_table_t) + sizeof(vm_table_pair_t) * vm_primes_table[0]);
    ret->pairs = vm_malloc(sizeof(vm_table_pair_t) * vm_primes_table[0]);
    memset(ret->pairs, VM_EMPTY_BYTE, sizeof(vm_table_pair_t) * vm_primes_table[0]);
    ret->size = 0;
    ret->used = 0;
    ret->len = 0;
    ret->header.mark = false;
    vm_gc_add(vm, vm_obj_of_table(ret));
    return ret;
}

void vm_gc_run(vm_t *vm, vm_obj_t *top) {
    if (vm_get_gc(vm)->last >= vm_get_gc(vm)->objs.len) {
        return;
    }
    vm_gc_mark(vm, top);
}

void vm_gc_init(vm_t *vm) {
    vm->gc = vm_malloc(sizeof(vm_gc_t));
    *vm_get_gc(vm) = (vm_gc_t){
        .last = VM_GC_MIN,
    };
}

void vm_gc_deinit(vm_t *vm) {
    vm_gc_objs_free(&vm_get_gc(vm)->objs);
    // vm_gc_objs_free(&gc->black);
    vm_free(vm_get_gc(vm));
}

void vm_gc_add(vm_t *vm, vm_obj_t obj) {
    vm_gc_objs_push(&vm_get_gc(vm)->objs, obj);
}
