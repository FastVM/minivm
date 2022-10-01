#include "gc.h"
#include "nanbox.h"

size_t vm_gc_table_size(vm_value_table_t *tab) {
    static const size_t table[] =
        {
            0ul,
            5ul,
            11ul,
            23ul,
            47ul,
            97ul,
            199ul,
            409ul,
            823ul,
            1741ul,
            3469ul,
            6949ul,
            14033ul,
            28411ul,
            57557ul,
            116731ul,
            236897ul,
            480881ul,
            976369ul,
            1982627ul,
            4026031ul,
            8175383ul,
            16601593ul,
            33712729ul,
            68460391ul,
            139022417ul,
            282312799ul,
            573292817ul,
            1164186217ul,
            2364114217ul,
            4294967291ul,
        };
    return table[tab->hash_alloc];
}

static inline size_t vm_gc_table_modsize(uint8_t index, size_t hash) {
    switch (index - 1) {
        case 0: {
            return hash % 5ull;
        }
        case 1: {
            return hash % 11ull;
        }
        case 2: {
            return hash % 23ull;
        }
        case 3: {
            return hash % 47ull;
        }
        case 4: {
            return hash % 97ull;
        }
        case 5: {
            return hash % 199ull;
        }
        case 6: {
            return hash % 409ull;
        }
        case 7: {
            return hash % 823ull;
        }
        case 8: {
            return hash % 1741ull;
        }
        case 9: {
            return hash % 3469ull;
        }
        case 10: {
            return hash % 6949ull;
        }
        case 11: {
            return hash % 14033ull;
        }
        case 12: {
            return hash % 28411ull;
        }
        case 13: {
            return hash % 57557ull;
        }
        case 14: {
            return hash % 116731ull;
        }
        case 15: {
            return hash % 236897ull;
        }
        case 16: {
            return hash % 480881ull;
        }
        case 17: {
            return hash % 976369ull;
        }
        case 18: {
            return hash % 1982627ull;
        }
        case 19: {
            return hash % 4026031ull;
        }
        case 20: {
            return hash % 8175383ull;
        }
        case 21: {
            return hash % 16601593ull;
        }
        case 22: {
            return hash % 33712729ull;
        }
        case 23: {
            return hash % 68460391ull;
        }
        case 24: {
            return hash % 139022417ull;
        }
        case 25: {
            return hash % 282312799ull;
        }
        case 26: {
            return hash % 573292817ull;
        }
        case 27: {
            return hash % 1164186217ull;
        }
        case 28: {
            return hash % 2364114217ull;
        }
        case 29: {
            return hash % 4294967291ull;
        }
        default: {
            __builtin_unreachable();
        }
    }
}

void vm_gc_init(vm_gc_t *gc, size_t nstack, vm_value_t *stack) {
    gc->len = 0;
    gc->alloc = 256;
    gc->vals = vm_malloc(sizeof(vm_value_t) * gc->alloc);
    gc->nstack = nstack;
    gc->stack = stack;
    gc->max = 256;
}

void vm_gc_deinit(vm_gc_t *gc) {
    vm_free(gc->vals);
}

static void vm_gc_mark(vm_value_t value) {
    uint8_t type = vm_typeof(value);
    if (type == VM_TYPE_ARRAY) {
        vm_value_array_t *val = vm_value_to_array(value);
        if (val->mark != 0) {
            return;
        }
        val->mark = 1;
        for (size_t i = 0; i < val->len; i++) {
            vm_gc_mark(val->data[i]);
        }
    } else if (type == VM_TYPE_TABLE) {
        vm_value_table_t *val = vm_value_to_table(value);
        if (val->mark != 0) {
            return;
        }
        val->mark = 1;
        if (val->hash_alloc != 0) {
            size_t len = vm_gc_table_size(val);
            for (size_t i = 0; i < len; i++) {
                vm_gc_mark(val->hash_keys[i]);
                vm_gc_mark(val->hash_values[i]);
            }
        }
#if VM_TABLE_OPT
        for (size_t i = 0; i < val->arr_len; i++) {
            vm_gc_mark(val->arr_data[i]);
        }
#endif
    }
}

void vm_gc_run(vm_gc_t *restrict gc, vm_value_t *high) {
#if VM_XGC
    return;
#endif
    if (gc->len < gc->max) {
        return;
    }
    vm_value_t *cur = gc->stack;
    while (cur < high) {
        vm_gc_mark(*cur);
        cur += 1;
    }
    size_t head = 0;
    for (size_t i = 0; i < gc->len; i++) {
        vm_value_t value = gc->vals[i];
        uint8_t type = vm_typeof(value);
        if (type == VM_TYPE_ARRAY) {
            vm_value_array_t *val = vm_value_to_array(value);
            if (val->mark != 0) {
                val->mark = 0;
                gc->vals[head++] = value;
            } else {
                // vm_free(val->data);
                vm_free(val);
            }
        } else {
            vm_value_table_t *val = vm_value_to_table(value);
            if (val->mark != 0) {
                val->mark = 0;
                gc->vals[head++] = value;
            } else {
#if VM_TABLE_OPT
                vm_free(val->arr_data);
#endif
                vm_free(val->hash_keys);
                vm_free(val->hash_values);
                vm_free(val);
            }
        }
    }
    gc->len = head;
    gc->max = gc->len * 2;
    size_t min = (size_t)(cur - gc->stack) * 2;
    if (gc->max < min) {
        gc->max = min;
    }
}

vm_value_t vm_gc_arr(vm_gc_t *restrict gc, vm_int_t slots) {
    if (slots == 0) {
        __builtin_trap();
    }
    vm_value_array_t *arr =
        vm_malloc(sizeof(vm_value_array_t) + sizeof(vm_value_t) * (size_t)slots);
    arr->tag = VM_TYPE_ARRAY;
    if (gc->len + 1 >= gc->alloc) {
        gc->alloc = gc->len * 2;
        gc->vals = vm_realloc(gc->vals, sizeof(vm_value_t) * gc->alloc);
    }
    gc->vals[gc->len++] = vm_value_from_array(arr);
    arr->alloc = (uint32_t)slots;
    arr->len = (uint32_t)slots;
    arr->mark = 0;
    arr->data = (vm_value_t *)&arr[1];
    // #if NANBOX_EMPTY_BYTE == 0
    //     arr->data = vm_alloc0(sizeof(vm_value_t) * slots);
    // #else
    //     arr->data = vm_malloc(sizeof(vm_value_t) * slots);
    memset(arr->data, NANBOX_EMPTY_BYTE, sizeof(vm_value_t) * (size_t)slots);
    // #endif
    return vm_value_from_array(arr);
}

vm_value_t vm_gc_get(vm_value_t obj, vm_value_t ind) {
    size_t index = (size_t)vm_value_to_float(ind);
    vm_value_array_t *arr = vm_value_to_array(obj);
    if (index >= arr->len) {
        return vm_value_nil();
    }
    uint8_t type = vm_typeof(arr->data[index]);
    return arr->data[index];
}

void vm_gc_set(vm_value_t obj, vm_value_t ind, vm_value_t value) {
    size_t index = (size_t)vm_value_to_float(ind);
    vm_value_array_t *arr = vm_value_to_array(obj);
    if (index >= arr->alloc) {
        return;
    }
    if (index >= arr->len) {
        arr->len = (uint32_t)(index + 1);
    }
    arr->data[index] = value;
}

vm_int_t vm_gc_len(vm_value_t obj) { return (vm_int_t)vm_value_to_array(obj)->len; }

vm_value_t vm_gc_tab(vm_gc_t *gc) {
    vm_value_table_t *tab = vm_alloc0(sizeof(vm_value_table_t));
    tab->tag = VM_TYPE_TABLE;
    if (gc->len + 1 >= gc->alloc) {
        gc->alloc = gc->len * 2;
        gc->vals = vm_realloc(gc->vals, sizeof(vm_value_t) * gc->alloc);
    }
    gc->vals[gc->len++] = vm_value_from_table(tab);
    return vm_value_from_table(tab);
}

bool vm_gc_eq(vm_value_t v1, vm_value_t v2) {
    uint8_t t1 = vm_typeof(v1);
    uint8_t t2 = vm_typeof(v2);
    if (t2 == VM_TYPE_F64) {
        if (t2 == VM_TYPE_F64) {
            return vm_value_to_float(v1) == vm_value_to_float(v2);
        } else if (t2 == VM_TYPE_I32) {
            return vm_value_to_float(v1) == (double) vm_value_to_int(v2);
        } else {
            return false;
        }
    } else if (t2 == VM_TYPE_I32) {
        if (t2 == VM_TYPE_F64) {
            return (double) vm_value_to_int(v1) == vm_value_to_float(v2);
        } else if (t2 == VM_TYPE_I32) {
            return vm_value_to_int(v1) == vm_value_to_int(v2);
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
                if (!vm_gc_eq(a1->data[i], a2->data[i])) {
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

static inline size_t vm_gc_table_hash(uint8_t nth, vm_value_t val) {
    uint8_t type = vm_typeof(val);
    if (type == VM_TYPE_I32) {
        vm_int_t ival = vm_value_to_int(val);
        if (ival < 0) {
            return vm_gc_table_modsize(nth, (size_t) ((1 << 24) - ival));
        } else {
            return vm_gc_table_modsize(nth, (size_t) ival);
        }
    } else if (type == VM_TYPE_F64) {
        return vm_gc_table_modsize(nth, (2 << 24) + (size_t)vm_value_to_float(val));
    } else if (type == VM_TYPE_BOOL) {
        return (size_t)vm_value_to_bool(val);
    } else if (type == VM_TYPE_FUNC) {
        return vm_gc_table_modsize(nth, (3 << 24));
    } else if (type == VM_TYPE_NIL) {
        return vm_gc_table_modsize(nth, (4 << 24));
    } else if (type == VM_TYPE_ARRAY) {
        vm_value_array_t *arr = vm_value_to_array(val);
        size_t ret = arr->len;
        for (uint32_t i = 0; i < arr->len; i++) {
            ret *= ret << 5;
            vm_value_t nval = arr->data[i];
            if (vm_box_is_double(nval)) {
                ret ^= (size_t)vm_value_to_float(nval);
            }
        }
        return vm_gc_table_modsize(nth, ret);
    } else if (type == VM_TYPE_TABLE) {
        return vm_gc_table_modsize(nth, (size_t)vm_value_to_table(val) >> 4);
    } else {
        return vm_gc_table_modsize(nth, 5 << 24);
    }
}

static inline bool vm_gc_table_isint(double v) {
    return (double)(int)v == v;
}

vm_value_t vm_gc_table_get(vm_value_table_t *tab, vm_value_t key) {
#if VM_TABLE_OPT
    if (vm_box_is_double(key)) {
        double dv = vm_value_to_float(key);
        dv -= 1;
        if (0 <= dv && dv < tab->arr_len) {
            if (vm_gc_table_isint(dv)) {
                return tab->arr_data[(size_t)dv];
            }
        }
    }
#endif
    if (tab->hash_alloc == 0) {
        return vm_value_nil();
    }
    size_t max = vm_gc_table_size(tab);
    size_t start = vm_gc_table_hash(tab->hash_alloc, key);
    size_t look = start;
    for (;;) {
        vm_value_t found = tab->hash_keys[look];
        if (vm_gc_eq(found, key)) {
            return tab->hash_values[look];
        }
        if (vm_box_is_empty(found)) {
            return vm_value_nil();
        }
        look += 1;
        if (look == start) {
            return vm_value_nil();
        }
        if (look == max) {
            look = 0;
        }
    }
}

void vm_gc_table_set(vm_value_table_t *tab, vm_value_t key, vm_value_t val) {
#if VM_TABLE_OPT
    if (vm_box_is_double(key)) {
        double dv = vm_value_to_float(key);
        dv -= 1;
        if (0 <= dv && dv < tab->arr_len && vm_gc_table_isint(dv)) {
            tab->arr_data[(size_t)dv] = val;
            return;
        }
        if (dv == (double)tab->arr_len) {
            if (tab->arr_len + 1 >= tab->arr_alloc) {
                tab->arr_alloc = tab->arr_len * 2 + 1;
                tab->arr_data = vm_realloc(tab->arr_data, sizeof(vm_value_t) * tab->arr_alloc);
            }
            tab->arr_data[tab->arr_len++] = val;
            return;
        }
    }
#endif
    if (tab->hash_alloc == 0) {
        tab->hash_alloc += 1;
        size_t nsize = vm_gc_table_size(tab);
#if NANBOX_EMPTY_BYTE == 0
        tab->hash_keys = vm_alloc0(sizeof(vm_value_t) * nsize);
        tab->hash_values = vm_alloc0(sizeof(vm_value_t) * nsize);
#else
        tab->hash_keys = vm_malloc(sizeof(vm_value_t) * nsize);
        tab->hash_values = vm_malloc(sizeof(vm_value_t) * nsize);
        memset(tab->hash_keys, NANBOX_EMPTY_BYTE, sizeof(vm_value_t) * nsize);
        memset(tab->hash_values, NANBOX_EMPTY_BYTE, sizeof(vm_value_t) * nsize);
#endif
    }
    for (;;) {
        size_t max = vm_gc_table_size(tab);
        size_t start = vm_gc_table_hash(tab->hash_alloc, key);
        size_t look = start;
        size_t count = 7;
        for (;;) {
            vm_value_t found = tab->hash_keys[look];
            if (vm_gc_eq(tab->hash_keys[look], key) || vm_box_is_empty(found)) {
                tab->hash_keys[look] = key;
                tab->hash_values[look] = val;
                return;
            }
            look += 1;
            if (count-- == 0) {
                break;
            }
            if (look == max) {
                look = 0;
            }
        }
        tab->hash_alloc += 1;
        size_t nsize = vm_gc_table_size(tab);
#if NANBOX_EMPTY_BYTE == 0
        vm_value_t *next_keys = vm_alloc0(sizeof(vm_value_t) * nsize);
        vm_value_t *next_values = vm_alloc0(sizeof(vm_value_t) * nsize);
#else
        vm_value_t *next_keys = vm_malloc(sizeof(vm_value_t) * nsize);
        vm_value_t *next_values = vm_malloc(sizeof(vm_value_t) * nsize);
        memset(next_keys, NANBOX_EMPTY_BYTE, sizeof(vm_value_t) * nsize);
        memset(next_values, NANBOX_EMPTY_BYTE, sizeof(vm_value_t) * nsize);
#endif
        for (size_t i = 0; i < max; i++) {
            if (vm_box_is_empty(tab->hash_keys[i])) {
                continue;
            }
            size_t start2 = vm_gc_table_hash(tab->hash_alloc, tab->hash_keys[i]);
            size_t look2 = start2;
            for (;;) {
                vm_value_t found = next_keys[look2];
                if (vm_box_is_empty(found)) {
                    next_keys[look2] = tab->hash_keys[i];
                    next_values[look2] = tab->hash_values[i];
                    return;
                }
                look2 += 1;
                if (look2 == start2) {
                    break;
                }
                if (look2 == nsize) {
                    look2 = 0;
                }
            }
        }
        vm_free(tab->hash_keys);
        vm_free(tab->hash_values);
        tab->hash_keys = next_keys;
        tab->hash_values = next_values;
    }
}
