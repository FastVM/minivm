#include "gc.h"

void vm_gc_init(vm_gc_t *gc, size_t nstack, vm_value_t *stack) {
    gc->len = 0;
    gc->alloc = 256;
    gc->vals = vm_malloc(sizeof(vm_value_array_t *) * gc->alloc);
    gc->nstack = nstack;
    gc->stack = stack;
}

void vm_gc_deinit(vm_gc_t *gc) {
    vm_free(gc->vals);
}

static void vm_gc_mark(vm_value_array_t *val) {
    if (val->mark != 0) {
        return;
    }
    val->mark = 1;
    for (size_t i = 0; i < val->len; i++) {
        vm_value_t el = val->data[i];
        if (vm_typeof(el) == VM_TYPE_ARRAY) {
            vm_gc_mark(vm_value_to_array(el));
        }
    }
}

void vm_gc_run(vm_gc_t *restrict gc) {
    if (gc->len < gc->max) {
        return;
    }
    for (size_t i = 0; i < gc->nstack; i++) {
        vm_value_t el = gc->stack[i];
        if (vm_typeof(el) == VM_TYPE_ARRAY) {
            vm_gc_mark(vm_value_to_array(el));
        }
    }
    size_t head = 0;
    for (size_t i = 0; i < gc->len; i++) {
        vm_value_array_t *val = gc->vals[i];
        if (val->mark != 0) {
            val->mark = 0;
            gc->vals[head++] = val;
        } else {
            vm_free(gc->vals[i]->data);
            vm_free(gc->vals[i]);
        }
    }
    gc->len = head;
    gc->max = head * 2;
}

vm_value_t vm_gc_arr(vm_gc_t *gc, vm_int_t slots) {
    vm_value_array_t *arr =
        vm_malloc(sizeof(vm_value_array_t));
    if (gc->len + 1 >= gc->alloc) {
        gc->alloc = gc->len * 2;
        gc->vals = vm_realloc(gc->vals, sizeof(vm_value_array_t *) * gc->alloc);
    }
    gc->vals[gc->len++] = arr;
    arr->tag = VM_TYPE_ARRAY;
    arr->alloc = slots;
    arr->len = slots;
    arr->mark = 0;
#if NANBOX_EMPTY_BYTE == 0
    arr->data = vm_alloc0(sizeof(vm_value_t) * slots);
#else
    arr->data = vm_malloc(sizeof(vm_value_t) * slots);
    memset(arr->data, NANBOX_EMPTY_BYTE, sizeof(vm_value_t) * slots);
#endif
    return vm_value_from_array(arr);
}

vm_value_t vm_gc_get(vm_gc_t *gc, vm_value_t obj, vm_value_t ind) {
    size_t index = (size_t)vm_value_to_float(ind);
    vm_value_array_t *arr = vm_value_to_array(obj);
    if (index >= arr->len) {
        return vm_value_nil();
    }
    return arr->data[index];
}

void vm_gc_set(vm_gc_t *gc, vm_value_t obj, vm_value_t ind, vm_value_t value) {
    size_t index = (size_t)vm_value_to_float(ind);
    vm_value_array_t *arr = vm_value_to_array(obj);
    if (index >= arr->alloc) {
        size_t next = index * 2 + 1;
        arr->data = vm_realloc(arr->data, sizeof(vm_value_t) * next);
        memset(&arr->data[arr->alloc], NANBOX_EMPTY_BYTE, next - arr->alloc);
        arr->alloc = next;
    }
    if (index >= arr->len) {
        arr->len = index + 1;
    }
    arr->data[index] = value;
}

vm_int_t vm_gc_len(vm_gc_t *gc, vm_value_t obj) { return (vm_int_t)vm_value_to_array(obj)->len; }

vm_value_t vm_gc_tab(vm_gc_t *gc) {
    vm_value_table_t *tab = vm_alloc0(sizeof(vm_value_table_t));
    tab->tag = VM_TYPE_TABLE;
    return vm_value_from_table(tab);
}

static inline bool vm_gc_table_eq(vm_value_t v1, vm_value_t v2) {
    uint8_t t1 = vm_typeof(v1);
    uint8_t t2 = vm_typeof(v2);
    if (t2 == VM_TYPE_FLOAT) {
        if (t2 == VM_TYPE_FLOAT) {
            return vm_value_to_float(v1) == vm_value_to_float(v2);
        } else {
            return false;
        }
    } else if (t1 == VM_TYPE_BOOL) {
        if (t1 == VM_TYPE_BOOL) {
            return vm_value_to_bool(v1) == vm_value_to_bool(v2);
        } else {
            return false;
        }
    } else if (t1 == VM_TYPE_NIL) {
        return t2 == VM_TYPE_NIL;
    } else if (t1 == VM_TYPE_ARRAY) {
        if (t2 == VM_TYPE_ARRAY) {
            vm_value_array_t *a1 = vm_value_to_array(v1);
            vm_value_array_t *a2 = vm_value_to_array(v2);
            if (a1->len != a2->len) {
                return false;
            }
            for (size_t i = 0; i < a1->len; i++) {
                if (!vm_gc_table_eq(a1->data[i], a2->data[i])) {
                    return false;
                }
            }
            return true;
        } else {
            return false;
        }
    } else {
        return vm_box_to_pointer(v1) == vm_box_to_pointer(v2);
    }
}

vm_value_t vm_gc_table_get(vm_value_table_t *tab, vm_value_t key) {
    for (size_t i = 0; i < tab->len; i++) {
        if (vm_gc_table_eq(tab->keys[i], key)) {
            return tab->values[i];
        }
    }
    return vm_value_nil();
}

void vm_gc_table_set(vm_value_table_t *tab, vm_value_t key, vm_value_t val) {
    for (size_t i = 0; i < tab->len; i++) {
        if (vm_gc_table_eq(tab->keys[i], key)) {
            tab->values[i] = val;
        }
    }
    if (tab->len + 1 >= tab->alloc) {
        tab->alloc = tab->len * 2 + 1;
        tab->keys = vm_realloc(tab->keys, sizeof(vm_value_t) * tab->alloc);
        tab->values = vm_realloc(tab->values, sizeof(vm_value_t) * tab->alloc);
    }
    tab->keys[tab->len] = key;
    tab->values[tab->len] = val;
    tab->len += 1;
}
