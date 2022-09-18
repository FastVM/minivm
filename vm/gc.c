#include "gc.h"

static inline size_t vm_gc_table_size(vm_value_table_t *tab) {
    static const size_t table[] =
    {
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
        8589934583ull,
        17179869143ull,
        34359738337ull,
        68719476731ull,
        137438953447ull,
        274877906899ull,
        549755813881ull,
        1099511627689ull,
        2199023255531ull,
        4398046511093ull,
        8796093022151ull,
        17592186044399ull,
        35184372088777ull,
        70368744177643ull,
        140737488355213ull,
        281474976710597ull,
        562949953421231ull, 
        1125899906842597ull,
        2251799813685119ull, 
        4503599627370449ull,
        9007199254740881ull, 
        18014398509481951ull,
        36028797018963913ull, 
        72057594037927931ull,
        144115188075855859ull,
        288230376151711717ull,
        576460752303423433ull,
        1152921504606846883ull,
        2305843009213693951ull,
        4611686018427387847ull,
        9223372036854775783ull,
        18446744073709551557ull,
    };
    return table[tab->hash_alloc - 1];
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
    case 30: {
        return hash % 8589934583ull; 
    }
    case 31: {
        return hash % 17179869143ull; 
    }
    case 32: {
        return hash % 34359738337ull; 
    }
    case 33: {
        return hash % 68719476731ull; 
    }
    case 34: {
        return hash % 137438953447ull; 
    }
    case 35: {
        return hash % 274877906899ull; 
    }
    case 36: {
        return hash % 549755813881ull; 
    }
    case 37: {
        return hash % 1099511627689ull; 
    }
    case 38: {
        return hash % 2199023255531ull; 
    }
    case 39: {
        return hash % 4398046511093ull; 
    }
    case 40: {
        return hash % 8796093022151ull; 
    }
    case 41: {
        return hash % 17592186044399ull; 
    }
    case 42: {
        return hash % 35184372088777ull; 
    }
    case 43: {
        return hash % 70368744177643ull; 
    }
    case 44: {
        return hash % 140737488355213ull;
    }
    case 45: {
        return hash % 281474976710597ull; 
    }
    case 46: {
        return hash % 562949953421231ull; 
    }
    case 47: {
        return hash % 1125899906842597ull; 
    }
    case 48: {
        return hash % 2251799813685119ull; 
    }
    case 49: {
        return hash % 4503599627370449ull; 
    }
    case 50: {
        return hash % 9007199254740881ull; 
    }
    case 51: {
        return hash % 18014398509481951ull; 
    }
    case 52: {
        return hash % 36028797018963913ull; 
    }
    case 53: {
        return hash % 72057594037927931ull; 
    }
    case 54: {
        return hash % 144115188075855859ull; 
    }
    case 55: {
        return hash % 288230376151711717ull; 
    }
    case 56: {
        return hash % 576460752303423433ull; 
    }
    case 57: {
        return hash % 1152921504606846883ull; 
    }
    case 58: {
        return hash % 2305843009213693951ull; 
    }
    case 59: {
        return hash % 4611686018427387847ull; 
    }
    case 60: {
        return hash % 9223372036854775783ull; 
    }
    case 61: {
        return hash % 18446744073709551557ull; 
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
    // fprintf(stderr, "%zu : %zu\n", gc->len, gc->alloc);
    gc->max = gc->len * 2;
    if (gc->max < cur - gc->stack) {
        gc->max = cur - gc->stack;
    }
}

vm_value_t vm_gc_arr(vm_gc_t *restrict gc, vm_int_t slots) {
    if (slots == 0) {
        __builtin_trap();
    }
    vm_value_array_t *arr =
        vm_malloc(sizeof(vm_value_array_t) + sizeof(vm_value_t) * slots);
    arr->tag = VM_TYPE_ARRAY;
    if (gc->len + 1 >= gc->alloc) {
        gc->alloc = gc->len * 2;
        gc->vals = vm_realloc(gc->vals, sizeof(vm_value_t) * gc->alloc);
    }
    gc->vals[gc->len++] = vm_value_from_array(arr);
    arr->alloc = slots;
    arr->len = slots;
    arr->mark = 0;
    arr->data = (vm_value_t *) &arr[1];
// #if NANBOX_EMPTY_BYTE == 0
//     arr->data = vm_alloc0(sizeof(vm_value_t) * slots);
// #else
//     arr->data = vm_malloc(sizeof(vm_value_t) * slots);
    memset(arr->data, NANBOX_EMPTY_BYTE, sizeof(vm_value_t) * slots);
// #endif
    return vm_value_from_array(arr);
}

vm_value_t vm_gc_get(vm_value_t obj, vm_value_t ind) {
    size_t index = (size_t)vm_value_to_float(ind);
    vm_value_array_t *arr = vm_value_to_array(obj);
    if (index >= arr->len) {
        return vm_value_nil();
    }
    return arr->data[index];
}

void vm_gc_set(vm_value_t obj, vm_value_t ind, vm_value_t value) {
    size_t index = (size_t)vm_value_to_float(ind);
    vm_value_array_t *arr = vm_value_to_array(obj);
    if (index >= arr->alloc) {
        fprintf(stderr, "(alloc: %zu) (index: %zu)\n", arr->alloc, index);
        // size_t next = index * 2 + 1;
        // arr->data = vm_realloc(arr->data, sizeof(vm_value_t) * next);
        // memset(&arr->data[arr->alloc], NANBOX_EMPTY_BYTE, next - arr->alloc);
        // arr->alloc = next;
        return;
    }
    if (index >= arr->len) {
        arr->len = index + 1;
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

static inline size_t vm_gc_table_hash(uint8_t nth, vm_value_t val) {
    uint8_t type = vm_typeof(val);
    if (type == VM_TYPE_FLOAT) {
        return vm_gc_table_modsize(nth, (1 << 24) + (size_t) vm_value_to_float(val));
    } else if (type == VM_TYPE_BOOL) {
        return (size_t) vm_value_to_bool(val);
    } else if (type == VM_TYPE_FUNC) {
        return vm_gc_table_modsize(nth, (2 << 24));
    } else if (type == VM_TYPE_NIL) {
        return vm_gc_table_modsize(nth, (3 << 24));
    } else if (type == VM_TYPE_ARRAY) {
        vm_value_array_t *arr = vm_value_to_array(val);
        size_t ret = arr->len;
        for (uint32_t i = 0; i < arr->len; i++) {
            ret *= ret << 5;
            vm_value_t val = arr->data[i];
            if (vm_box_is_double(val)) {
                ret ^= (size_t) vm_value_to_float(val);
            }
        }
        return vm_gc_table_modsize(nth, ret);
    } else if (type == VM_TYPE_TABLE) {
        return vm_gc_table_modsize(nth, (size_t) vm_value_to_table(val) >> 4);
    } else {
        return vm_gc_table_modsize(nth, 4 << 24);
    }
}

static inline bool vm_gc_table_isint(double v) {
    return (double) (int) v == v; 
}

vm_value_t vm_gc_table_get(vm_value_table_t *tab, vm_value_t key) {
#if VM_TABLE_OPT
    if (vm_box_is_double(key)) {
        double dv = vm_value_to_float(key);
        dv -= 1;
        if (0 <= dv && dv < tab->arr_len) {
            if (vm_gc_table_isint(dv)) {
                return tab->arr_data[(size_t) dv];
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
        if (vm_gc_table_eq(found, key)) {
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
            tab->arr_data[(size_t) dv] = val;
            return;
        }
        if (dv == (double) tab->arr_len) {
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
            if (vm_box_is_empty(found)) {
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
            size_t start = vm_gc_table_hash(tab->hash_alloc, tab->hash_keys[i]);
            size_t look = start;
            for (;;) {
                vm_value_t found = next_keys[look];
                if (vm_box_is_empty(found)) {
                    next_keys[look] = tab->hash_keys[i];
                    next_values[look] = tab->hash_values[i];
                    return;
                }
                look += 1;
                if (look == start) {
                    break;
                }
                if (look == nsize) {
                    look = 0;
                }
            }
        }
        vm_free(tab->hash_keys);
        vm_free(tab->hash_values);
        tab->hash_keys = next_keys;
        tab->hash_values = next_values;
    }
}
