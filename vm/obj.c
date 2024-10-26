
#include <math.h>

#include "obj.h"
#include "gc.h"
#include "io.h"
#include "math.h"
#include "lib.h"

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

vm_obj_t vm_table_get(vm_obj_table_t *table, vm_obj_t key) {
    size_t len = vm_primes_table[table->size];
    size_t init_look = vm_primes_mod(table->size, vm_obj_hash(key));
    size_t look = init_look;
    do {
        vm_obj_t found_key = table->entries[look];
        if (vm_obj_is_nil(found_key)) {
            return vm_obj_of_nil();
        }
        if (vm_obj_unsafe_eq(key, found_key)) {
            return table->entries[vm_primes_table[table->size] + look];
        }
        look += 1;
        if (look == len) {
            look = 0;
        }
    } while (look != init_look);

    return vm_obj_of_nil();
}

void vm_table_set(vm_obj_table_t *restrict table, vm_obj_t key, vm_obj_t value) {
    size_t len = vm_primes_table[table->size];
    size_t look = vm_primes_mod(table->size, vm_obj_hash(key));
    for (size_t i = 0; i < len; i++) {
        vm_obj_t table_key = table->entries[look];
        if (vm_obj_is_nil(table_key)) {
            break;
        }
        if (vm_obj_unsafe_eq(key, table_key)) {
            if (vm_obj_is_nil(value)) {
                if (vm_obj_is_number(key)) {
                    double f64val = vm_obj_get_number(key);
                    if (1 <= f64val && f64val <= table->len) {
                        int32_t i64val = (int32_t)f64val;
                        if ((double)i64val == f64val) {
                            table->len = i64val - 1;
                        }
                    }
                }
                table->entries[look] = value;
            }
            table->entries[vm_primes_table[table->size] + look] = value;
            return;
        }
        look += 1;
        if (look == len) {
            look = 0;
        }
    }
    if ((table->used) * 100u > vm_primes_table[table->size] * 75u) {
        vm_obj_table_t ret;
        ret.size = table->size + 1;
        ret.used = 0;
        ret.len = 0;
        ret.mark = table->mark;
#if VM_EMPTY_BYTE == 0
        ret.entries = vm_calloc(sizeof(vm_obj_t) * vm_primes_table[ret.size] * 2);
#else
        ret.entries = vm_malloc(sizeof(vm_obj_t) * vm_primes_table[ret.size] * 2);
        memset(ret.entries, VM_EMPTY_BYTE, sizeof(vm_obj_t) * vm_primes_table[ret.size] * 2);
#endif
        for (size_t i = 0; i < vm_primes_table[table->size]; i++) {
            vm_obj_t in_key = table->entries[i];
            if (!vm_obj_is_nil(in_key)) {
                vm_table_set(&ret, in_key, table->entries[vm_primes_table[table->size] + i]);
            }
        }
        vm_table_set(&ret, key, value);
        vm_free(table->entries);
        *table = ret;
    } else {
        table->used += 1;
        table->entries[look] = key;
        table->entries[vm_primes_table[table->size] + look] = value;
        int32_t next = table->len + 1;
        if (vm_obj_is_number(key) && vm_obj_get_number(key) == next) {
            while (true) {
                vm_obj_t got = vm_table_get(table, vm_obj_of_number(next + 1));
                if (vm_obj_is_nil(got)) {
                    break;
                }
                next += 1;
            }
            table->len = next;
        }
    }
}
