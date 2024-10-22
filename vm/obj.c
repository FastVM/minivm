#include "obj.h"
#include "gc.h"
#include "io.h"
#include "math.h"

#include "primes.inc"

vm_obj_t vm_obj_of_string(vm_t *vm, const char *str) {
    vm_obj_t ret = vm_obj_of_buffer(vm_io_buffer_from_str(str));
    vm_gc_add(vm, ret);
    return ret;
}

uint32_t vm_obj_hash(vm_obj_t value) {
    if (vm_obj_is_number(value)) {
        double n = vm_obj_get_number(value);
        if (n == floor(n) && INT32_MIN <= n && n <= INT32_MAX) {
            return (uint32_t) (int32_t) n * 31;
        }
        return (uint32_t) (*(uint64_t *)&n >> 12) * 37;
    }
    if (vm_obj_is_string(value)) {
        vm_io_buffer_t *restrict buf = vm_obj_get_string(value);
        if (buf->hash == 0) {
            uint32_t ret = 0x811c9dc5;
            for (size_t i = 0; i < buf->len; i++) {
                char c = buf->buf[i];
                if (c == '\0') {
                    break;
                }
                ret *= 0x01000193;
                ret ^= c;
            }
            buf->hash = ret;
        }
        return buf->hash;
    }
    if (vm_obj_is_boolean(value)) {
        return UINT32_MAX - (uint32_t)vm_obj_get_boolean(value);
    }
    if (vm_obj_is_ffi(value)) {
        return (uint32_t)(size_t)vm_obj_get_ffi(value) >> 4;
    }
    if (vm_obj_is_closure(value)) {
        return (uint32_t)(size_t)vm_obj_get_closure(value) >> 4;
    }
    if (vm_obj_is_table(value)) {
        return (uint32_t)(size_t)vm_obj_get_table(value) >> 4;
    }
    return 0;
}

static vm_table_pair_t *vm_table_lookup(vm_obj_table_t *table, vm_obj_t key) {
    size_t len = vm_primes_table[table->size];
    size_t init_look = vm_primes_mod(table->size, vm_obj_hash(key));
    size_t look = init_look;
    do {
        vm_table_pair_t *pair = &table->pairs[look];
        if (vm_obj_is_nil(pair->key)) {
            return NULL;
        }
        if (vm_obj_unsafe_eq(key, pair->key)) {
            return pair;
        }
        look += 1;
        if (look == len) {
            look = 0;
        }
    } while (look != init_look);
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
        if (vm_obj_unsafe_eq(key, pair->key)) {
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
    } else if (vm_primes_table[table->size] <= 4 ? table->used == vm_primes_table[table->size] : (table->used) * 100u > vm_primes_table[table->size] * 75u) {
        vm_obj_table_t ret;
        ret.size = table->size + 1;
        uint64_t ret_len = vm_primes_table[ret.size];
#if VM_EMPTY_BYTE == 0
        ret.pairs = vm_calloc(sizeof(vm_table_pair_t) * ret_len);
#else
        ret.pairs = vm_malloc(sizeof(vm_table_pair_t) * ret_len);
        memset(ret.pairs, VM_EMPTY_BYTE, sizeof(vm_table_pair_t) * ret_len);
#endif
        ret.used = 0;
        ret.len = 0;
        ret.mark = table->mark;
        size_t table_len = vm_primes_table[table->size];
        for (size_t i = 0; i < table_len; i++) {
            vm_table_pair_t *in_pair = &table->pairs[i];
            vm_obj_t in_key = in_pair->key;
            if (!vm_obj_is_nil(key)) {
                vm_table_set(&ret, in_key, in_pair->value);
            }
        }
        vm_table_set(&ret, key, value);
        vm_free(table->pairs);
        *table = ret;
    } else {
        table->used += 1;
        table->pairs[look] = (vm_table_pair_t){
            .key = key,
            .value = value,
        };
        vm_obj_t vlen = vm_obj_of_number(table->len + 1);
        if (vm_obj_unsafe_eq(vlen, key)) {
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

vm_obj_t vm_table_get(vm_obj_table_t *table, vm_obj_t key) {
    vm_table_pair_t *pair = vm_table_lookup(table, key);
    if (pair == NULL) {
        return vm_obj_of_nil();
    }
    return pair->value;
}
