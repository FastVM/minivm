#include "obj.h"
#include "gc.h"
#include "io.h"

#include "primes.inc"

bool vm_obj_eq(vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_nil(v1) && vm_obj_is_nil(v2)) {
        return true;
    } else if (vm_obj_is_boolean(v1) && vm_obj_is_boolean(v2)) {
        return vm_obj_get_boolean(v1) == vm_obj_get_boolean(v2);
    } else if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_get_number(v1) == vm_obj_get_number(v2);
    } else if (vm_obj_is_string(v1) && vm_obj_is_string(v2)) {
        return strcmp(vm_obj_get_string(v1)->buf, vm_obj_get_string(v2)->buf) == 0;
    } else if (vm_obj_is_table(v1) && vm_obj_is_table(v2)) {
        return vm_obj_get_table(v1) == vm_obj_get_table(v2);
    } else if (vm_obj_is_closure(v1) && vm_obj_is_closure(v2)) {
        return vm_obj_get_closure(v1) == vm_obj_get_closure(v2);
    } else if (vm_obj_is_ffi(v1) && vm_obj_is_ffi(v2)) {
        return vm_obj_get_ffi(v1) == vm_obj_get_ffi(v2);
    } else {
        return false;
    }
}

uint64_t vm_obj_hash(vm_obj_t value) {
    if (vm_obj_is_number(value)) {
        double n = vm_obj_get_number(value);
        return *(uint64_t *)&n;
    }
    if (vm_obj_is_string(value)) {
        uint64_t ret = 0xcbf29ce484222325;
        vm_io_buffer_t *restrict buf = vm_obj_get_string(value);
        for (size_t i = 0; i < buf->len; i++) {
            char c = buf->buf[i];
            if (c == '\0') {
                break;
            }
            ret *= 0x00000100000001B3;
            ret ^= c;
        }
        return ret;
    }
    if (vm_obj_is_boolean(value)) {
        return UINT64_MAX - (uint64_t)vm_obj_get_boolean(value);
    }
    if (vm_obj_is_ffi(value)) {
        return (uint64_t)(size_t)vm_obj_get_ffi(value) >> 4;
    }
    if (vm_obj_is_closure(value)) {
        return (uint64_t)(size_t)vm_obj_get_closure(value) >> 4;
    }
    if (vm_obj_is_table(value)) {
        return (uint64_t)(size_t)vm_obj_get_table(value) >> 4;
    }
    if (vm_obj_is_nil(value)) {
        return 0;
    }
    __builtin_trap();
    // return 0;
}

vm_table_pair_t *vm_table_lookup(vm_obj_table_t *table, vm_obj_t key) {
    size_t len = 1 << table->alloc;
    size_t and = len - 1;
    size_t stop = vm_obj_hash(key) & and;
    size_t next = stop;
    do {
        vm_table_pair_t *pair = &table->pairs[next];
        vm_obj_t value = pair->key;
        if (vm_obj_is_empty(value) || vm_obj_is_nil(value)) {
            return NULL;
        }
        if (vm_obj_eq(key, pair->key)) {
            return pair;
        }
        look += 1;
        if (look == len) {
            look = 0;
        }
    }
    return NULL;
}

void vm_table_set(vm_obj_table_t *restrict table, vm_obj_t key, vm_obj_t value) {
    size_t len = vm_primes_table[table->size];
    size_t look = vm_primes_mod(table->size, vm_obj_hash(key));
    for (size_t i = 0; i < len; i++) {
        vm_table_pair_t *pair = &table->pairs[look];
        if (vm_obj_is_nil(pair->key)) {
            break;
        }
        if (vm_obj_eq(key, pair->key)) {
            if (vm_obj_is_nil(key)) {
                pair->key = vm_obj_of_nil();
                if (vm_obj_is_number(key)) {
                    double f64val = vm_obj_get_number(key);
                    if ((double)INT64_MIN <= f64val && f64val <= (double)INT64_MAX) {
                        int64_t i64val = (int64_t)f64val;
                        if ((double)i64val == f64val) {
                            if (0 < i64val && i64val <= table->len) {
                                table->len = i64val - 1;
                            }
                        }
                    }
                }
            } else {
                pair->key = key;
                pair->value = value;
            }
            return;
        }
        look += 1;
        if (look == len) {
            look = 0;
        }
    }
    if (vm_obj_is_nil(value)) {
        table->pairs[look] = (vm_table_pair_t){
            .key = vm_obj_of_nil(),
            .value = value,
        };

        if (vm_obj_is_number(value)) {
            double n = vm_obj_get_number(value);

            if (1 <= n && n <= table->len && (double)(size_t)n == n) {
                table->len = (size_t)(n - 1);
            }
        }
    } else if ((table->used + 1) * 100 > vm_primes_table[table->size] * 75) {
        vm_obj_table_t ret;
        ret.size = table->size + 1;
        uint64_t ret_len = vm_primes_table[ret.size];
        ret.pairs = vm_malloc(sizeof(vm_table_pair_t) * ret_len);
        memset(ret.pairs, VM_EMPTY_BYTE, sizeof(vm_table_pair_t) * ret_len);
        ret.used = 0;
        ret.len = 0;
        ret.mark = false;
        ret.pairs_auto = false;
        size_t table_len = vm_primes_table[table->size];
        for (size_t i = 0; i < table_len; i++) {
            vm_table_pair_t *in_pair = &table->pairs[i];
            if (!vm_obj_is_nil(in_pair->key)) {
                vm_table_set_pair(&ret, in_pair);
            }
        }
        vm_table_set(&ret, key, value);
        if (!table->pairs_auto) {
            vm_free(table->pairs);
        }
        *table = ret;
    } else {
        table->used += 1;
        table->pairs[look] = (vm_table_pair_t){
            .key = key,
            .value = value,
        };
        vm_obj_t vlen = vm_obj_of_number(table->len + 1);
        if (vm_obj_eq(vlen, key)) {
            while (true) {
                int32_t next = table->len + 1;
                vm_table_pair_t *got = vm_table_lookup(table, vm_obj_of_number(next));
                if (got == NULL) {
                    break;
                }
                table->len = next;
            }
        }
    }
}

void vm_table_set_pair(vm_obj_table_t *table, vm_table_pair_t *pair) {
    vm_table_set(table, pair->key, pair->value);
}

void vm_table_get_pair(vm_obj_table_t *table, vm_table_pair_t *out) {
    vm_table_pair_t *pair = vm_table_lookup(table, out->key);
    if (pair != NULL) {
        out->value = pair->value;
        return;
    }
    out->value = vm_obj_of_nil();
    return;
}
