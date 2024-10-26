
#include "gc.h"
#include "ir.h"
#include "lib.h"
#include <stddef.h>

struct vm_gc_objs_t;
struct vm_gc_t;

typedef struct vm_gc_t vm_gc_t;
typedef struct vm_gc_objs_t vm_gc_objs_t;

struct vm_gc_objs_t {
    size_t alloc;
    size_t len;
    vm_obj_t *restrict objs;
};

struct vm_gc_t {
    size_t runs;
    size_t last;
    vm_gc_objs_t objs;

#if VM_GC_STATS
    struct {
        struct {
            struct {
                size_t count;
            } string;

            struct {
                size_t count;
            } table;

            struct {
                size_t count;
            } closure;

            struct {
                size_t count;
            } block;
        } by_type;
    } stats;
#endif
};

static inline void vm_gc_objs_add(vm_gc_objs_t *restrict objs, vm_obj_t obj) {
    if (objs->len + 1 >= objs->alloc) {
        objs->alloc = (objs->len + 1) * VM_GC_FACTOR;
        objs->objs = vm_realloc(objs->objs, sizeof(vm_obj_t) * objs->alloc);
    }
    objs->objs[objs->len++] = obj;
}

static inline void vm_gc_mark_obj(vm_obj_t obj);

static inline void vm_gc_mark_arg(vm_ir_arg_t arg) {
    if (arg.type == VM_IR_ARG_TYPE_LIT) {
        vm_gc_mark_obj(arg.lit);
    }
}

static inline void vm_gc_mark_block(vm_ir_block_t *restrict block) {
    if (!block->mark) {
        block->mark = true;
        for (size_t j = 0; j < block->len; j++) {
            vm_ir_instr_t *instr = &block->instrs[j];
            for (size_t k = 0; instr->args[k].type != VM_IR_ARG_TYPE_NONE; k++) {
                vm_gc_mark_arg(instr->args[k]);
            }
        }
        for (size_t k = 0; block->branch.args[k].type != VM_IR_ARG_TYPE_NONE; k++) {
            vm_gc_mark_arg(block->branch.args[k]);
        }
        vm_gc_mark_arg(block->branch.out);
        for (size_t j = 0; j < 2 && block->branch.targets[j] != NULL; j++) {
            vm_gc_mark_block(block->branch.targets[j]);
        }
    }
}


static inline void vm_gc_mark_obj(vm_obj_t obj) {
    if (vm_obj_is_string(obj)) {
        vm_io_buffer_t *buffer = vm_obj_get_string(obj);
        buffer->mark = true;
    } else if (vm_obj_is_table(obj)) {
        vm_obj_table_t *restrict table = vm_obj_get_table(obj);
        if (!table->mark) {
            table->mark = true;
            uint32_t len = vm_primes_table[table->size];
            for (uint32_t i = 0; i < len * 2; i++) {
                vm_gc_mark_obj(table->entries[i]);
            }
        }
    } else if (vm_obj_is_closure(obj)) {
        vm_obj_closure_t *restrict closure = vm_obj_get_closure(obj);
        if (!closure->mark) {
            closure->mark = true;
            for (size_t i = 0; i < closure->len; i++) {
                vm_gc_mark_obj(closure->values[i]);
            }
        }
        vm_gc_mark_block(closure->block);
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
    vm_gc_t *restrict gc = vm->gc;
    size_t write = 0;
#if VM_GC_STATS
    gc->stats.by_type.string.count = 0;
    gc->stats.by_type.table.count = 0;
    gc->stats.by_type.closure.count = 0;
    gc->stats.by_type.block.count = 0;
#endif
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
#if VM_GC_STATS
                gc->stats.by_type.string.count += 1;
#endif
            }
        } else if (vm_obj_is_table(obj)) {
            vm_obj_table_t *table = vm_obj_get_table(obj);
            if (!table->mark) {
                vm_free(table->entries);
                vm_free(table);
            } else {
                table->mark = false;
                gc->objs.objs[write++] = obj;
#if VM_GC_STATS
                gc->stats.by_type.table.count += 1;
#endif
            }
        } else if (vm_obj_is_closure(obj)) {
            vm_obj_closure_t *closure = vm_obj_get_closure(obj);
            if (!closure->mark) {
                // vm_free(closure);
            } else {
                closure->mark = false;
                gc->objs.objs[write++] = obj;
#if VM_GC_STATS
                gc->stats.by_type.closure.count += 1;
#endif
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
                if (* (const int64_t *) block->code != 0) {
                    vm_free(block->code);
                }
                vm_free(block);
            } else {
                block->mark = false;
                gc->objs.objs[write++] = obj;
#if VM_GC_STATS
                gc->stats.by_type.block.count += 1;
#endif
            }
        }
    }
#if VM_GC_STATS && VM_GC_STATS_DEBUG
    printf("--- gc#%zu: %f%% keep ---\n", gc->runs, (double) write / gc->objs.len * 100.0);
    printf("strings: %zu\n", gc->stats.by_type.string.count);
    printf("tables: %zu\n", gc->stats.by_type.table.count);
    printf("closures: %zu\n", gc->stats.by_type.closure.count);
    printf("blocks: %zu\n", gc->stats.by_type.block.count);
    printf("\n");
#endif
    gc->objs.len = write;
    size_t next = write * VM_GC_FACTOR;
    if (next >= gc->last) {
        gc->last = next;
    }
}

vm_obj_table_t *vm_table_new(vm_t *vm) {
    vm_obj_table_t *restrict ret = vm_malloc(sizeof(vm_obj_table_t));
    ret->size = 0;
    ret->used = 0;
    ret->len = 0;
    ret->mark = false;
#if VM_EMPTY_BYTE == 0
    ret->entries = vm_calloc(sizeof(vm_obj_t) * vm_primes_table[ret->size] * 2);
#else
    ret->entries = vm_malloc(sizeof(vm_obj_t) * vm_primes_table[ret->size] * 2);
    memset(ret->entries, VM_EMPTY_BYTE, sizeof(vm_obj_t) * vm_primes_table[ret->size] * 2);
#endif
    vm_gc_add(vm, vm_obj_of_table(ret));
    return ret;
}

void vm_gc_run(vm_t *vm, vm_obj_t *top) {
    vm_gc_t *restrict gc = vm->gc;
    if (gc->last >= gc->objs.len || VM_GC_DISABLED) {
        return;
    }
    gc->runs += 1;
    vm_gc_mark(vm, top);
    vm_gc_sweep(vm);
}

void vm_gc_init(vm_t *vm) {
    vm_gc_t *gc = vm_malloc(sizeof(vm_gc_t));
    *gc = (vm_gc_t){
        .last = VM_GC_MIN,
    };
    vm->gc = gc;
}

void vm_gc_deinit(vm_t *vm) {
    vm_gc_sweep(vm);
    vm_gc_t *restrict gc = vm->gc;
    // printf("%zu\n", gc->runs);
    vm_free(gc->objs.objs);
    vm_free(gc);
}

void vm_gc_add(vm_t *vm, vm_obj_t obj) {
    vm_gc_t *restrict gc = vm->gc;
    vm_gc_objs_add(&gc->objs, obj);
#if VM_GC_STATS
    if (vm_obj_is_string(obj)) {
        gc->stats.by_type.string.count += 1;
    } else if (vm_obj_is_table(obj)) {
        gc->stats.by_type.table.count += 1;
    } else if (vm_obj_is_closure(obj)) {
        gc->stats.by_type.closure.count += 1;
    } else if (vm_obj_is_block(obj)) {
        gc->stats.by_type.block.count += 1;
    }
#endif
}
