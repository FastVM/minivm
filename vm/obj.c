#include "./obj.h"
#include "./gc.h"
#include "./io.h"

int64_t vm_value_to_i64(vm_obj_t arg) {
    switch (arg.tag) {
        case VM_TAG_NUMBER: {
            return (int64_t)arg.value.f64;
        }
        default: {
            return 0;
        }
    }
}

double vm_value_to_f64(vm_obj_t arg) {
    switch (arg.tag) {
        case VM_TAG_NUMBER: {
            return (double)arg.value.f64;
        }
        default: {
            return 0;
        }
    }
}

bool vm_value_can_to_n64(vm_obj_t val) {
    return val.tag == VM_TAG_NUMBER;
}

bool vm_value_is_int(vm_obj_t val) {
    switch (val.tag) {
        case VM_TAG_NUMBER: {
            double v = val.value.f64;
            if ((double)INT64_MIN <= v && v <= (double)INT64_MAX) {
                return (double)(int64_t)v == v;
            }
            return fmod(v, 1.0) == 0.0;
        }
        default: {
            return false;
        }
    }
}

bool vm_obj_eq(vm_obj_t lhs, vm_obj_t rhs) {
    switch (lhs.tag) {
        case VM_TAG_NIL: {
            return rhs.tag == VM_TAG_NIL;
        }
        case VM_TAG_BOOL: {
            return rhs.tag == VM_TAG_BOOL && lhs.value.b == rhs.value.b;
        }
        case VM_TAG_NUMBER: {
            switch (rhs.tag) {
                case VM_TAG_NUMBER: {
                    return lhs.value.f64 == rhs.value.f64;
                }
                default: {
                    return false;
                }
            }
        }
        case VM_TAG_STR: {
            return rhs.tag == VM_TAG_STR && !strcmp(lhs.value.str->buf, rhs.value.str->buf);
        }
        case VM_TAG_FUN: {
            return rhs.tag == VM_TAG_FUN && lhs.value.fun == rhs.value.fun;
        }
        default: {
            return lhs.tag == rhs.tag && lhs.value.all == rhs.value.all;
        }
    }
}

size_t vm_value_hash(vm_obj_t value) {
    switch (value.tag) {
        case VM_TAG_NIL: {
            return SIZE_MAX - 2;
        }
        case VM_TAG_BOOL: {
            return SIZE_MAX - (size_t)value.value.b;
        }
        case VM_TAG_NUMBER: {
            if (vm_value_is_int(value)) {
                return (size_t)(int64_t)value.value.f64 * 1610612741;
            }
            return (size_t)*(uint64_t *)&value.value.f64;
        }
        case VM_TAG_STR: {
            uint64_t ret = 0xcbf29ce484222325;
            const char *head = value.value.str->buf;
            while (true) {
                char c = *head++;
                if (c == '\0') {
                    break;
                }
                ret *= 0x00000100000001B3;
                ret ^= c;
            }
            return (size_t) ret;
        }
        case VM_TAG_FFI:
        case VM_TAG_CLOSURE:
        case VM_TAG_TAB:
        case VM_TAG_FUN: {
            return (size_t)value.value.all >> 4;
        }
        default: {
            return SIZE_MAX - 3;
        }
    }
}

vm_table_pair_t *vm_table_lookup(vm_table_t *table, vm_obj_t key) {
    size_t len = 1 << table->alloc;
    size_t and = len - 1;
    size_t stop = vm_value_hash(key) & and;
    size_t next = stop;
    do {
        vm_table_pair_t *pair = &table->pairs[next];
        vm_obj_t value = pair->key;
        if (value.tag == VM_TAG_UNK || value.tag == VM_TAG_NIL) {
            return NULL;
        }
        if (vm_obj_eq(key, value)) {
            return pair;
        }
        next += 1;
        next &= and;
    } while (next != stop);
    return NULL;
}

void vm_table_init_size(vm_table_t *ret, size_t pow2) {
    ret->pairs = vm_malloc(sizeof(vm_table_pair_t) * (1 << pow2));
    memset(ret->pairs, 0, sizeof(vm_table_pair_t) * (1 << pow2));
    ret->alloc = pow2;
    ret->used = 0;
    ret->len = 0;
    ret->mark = false;
    ret->pairs_auto = false;
}

void vm_table_set(vm_table_t *restrict table, vm_obj_t key, vm_obj_t value) {
    if (table->alloc == 0) {
        return;
    }
    size_t len = 1 << table->alloc;
    size_t and = len - 1;
    size_t stop = vm_value_hash(key) & and;
    size_t next = stop & and;
    do {
        vm_table_pair_t *pair = &table->pairs[next];
        if (pair->key.tag == VM_TAG_UNK || pair->key.tag == VM_TAG_NIL) {
            break;
        }
        vm_obj_t check = pair->key;
        if (vm_obj_eq(key, check)) {
            if (value.tag == VM_TAG_NIL) {
                pair->key.tag = VM_TAG_UNK;
                if (vm_value_is_int(key)) {
                    int64_t i64val = vm_value_to_i64(key);
                    if (0 < i64val && i64val <= table->len) {
                        table->len = i64val - 1;
                    }
                }
            } else {
                pair->key = key;
                pair->value = value;
            }
            return;
        }
        next += 1;
        next &= and;
    } while (next != stop);
    if (value.tag == VM_TAG_NIL) {
        table->pairs[next] = (vm_table_pair_t){
            .key = key,
            .value = value,
        };

        vm_obj_t nv = value;

        double n = vm_value_to_f64(nv);

        if (1 <= n && n <= table->len && (double) (size_t) n == n) {
            table->len = (size_t) (n - 1);
        }
    } else if ((table->used + 1) * 100 >= ((uint32_t)1 << table->alloc) * 76) {
        vm_table_t ret;
        vm_table_init_size(&ret, table->alloc + 1);
        for (size_t i = 0; i < len; i++) {
            vm_table_pair_t *in_pair = &table->pairs[i];
            if (in_pair->key.tag != VM_TAG_UNK && in_pair->key.tag != VM_TAG_NIL) {
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
        table->pairs[next] = (vm_table_pair_t){
            .key = key,
            .value = value,
        };
        vm_obj_t vlen = (vm_obj_t){
            .tag = VM_TAG_NUMBER,
            .value.f64 = table->len + 1,
        };
        if (vm_obj_eq(vlen, key)) {
            while (true) {
                int32_t next = table->len + 1;
                vm_table_pair_t *got = vm_table_lookup(table, (vm_obj_t){.tag = VM_TAG_NUMBER, .value.f64 = next});
                if (got == NULL) {
                    break;
                }
                table->len = next;
            }
        }
    }
}

void vm_table_set_pair(vm_table_t *table, vm_table_pair_t *pair) {
    vm_table_set(table, pair->key, pair->value);
}

void vm_table_get_pair(vm_table_t *table, vm_table_pair_t *out) {
    vm_table_pair_t *pair = vm_table_lookup(table, out->key);
    if (pair != NULL) {
        out->value = pair->value;
        return;
    }
    out->value = vm_obj_of_nil();
    return;
}
