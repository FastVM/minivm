#include "./obj.h"

int64_t vm_value_to_i64(vm_obj_t arg) {
    switch (arg.tag) {
        case VM_TAG_I8: {
            return (int64_t)arg.value.i8;
        }
        case VM_TAG_I16: {
            return (int64_t)arg.value.i16;
        }
        case VM_TAG_I32: {
            return (int64_t)arg.value.i32;
        }
        case VM_TAG_I64: {
            return (int64_t)arg.value.i64;
        }
        case VM_TAG_F32: {
            return (int64_t)arg.value.f32;
        }
        case VM_TAG_F64: {
            return (int64_t)arg.value.f64;
        }
        default: {
            return 0;
        }
    }
}

double vm_value_to_f64(vm_obj_t arg) {
    switch (arg.tag) {
        case VM_TAG_I8: {
            return (double)arg.value.i8;
        }
        case VM_TAG_I16: {
            return (double)arg.value.i16;
        }
        case VM_TAG_I32: {
            return (double)arg.value.i32;
        }
        case VM_TAG_I64: {
            return (double)arg.value.i64;
        }
        case VM_TAG_F32: {
            return (double)arg.value.f32;
        }
        case VM_TAG_F64: {
            return (double)arg.value.f64;
        }
        default: {
            return 0;
        }
    }
}

bool vm_value_can_to_n64(vm_obj_t val) {
    return val.tag == VM_TAG_I8 || val.tag == VM_TAG_I16 || val.tag == VM_TAG_I32 || val.tag == VM_TAG_I64 || val.tag == VM_TAG_F32 || val.tag == VM_TAG_F64;
}

bool vm_value_is_int(vm_obj_t val) {
    switch (val.tag) {
        case VM_TAG_I8: {
            return true;
        }
        case VM_TAG_I16: {
            return true;
        }
        case VM_TAG_I32: {
            return true;
        }
        case VM_TAG_I64: {
            return true;
        }
        case VM_TAG_F32: {
            float v = val.value.f32;
            if ((float)INT32_MIN <= v && v <= (float)INT32_MAX) {
                return (float)(int32_t)v == v;
            }
            return fmodf(v, 1.0f) == 0.0f;
        }
        case VM_TAG_F64: {
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
        case VM_TAG_I8: {
            switch (rhs.tag) {
                case VM_TAG_I8: {
                    return lhs.value.i8 == rhs.value.i8;
                }
                case VM_TAG_I16: {
                    return lhs.value.i8 == rhs.value.i16;
                }
                case VM_TAG_I32: {
                    return lhs.value.i8 == rhs.value.i32;
                }
                case VM_TAG_I64: {
                    return lhs.value.i8 == rhs.value.i64;
                }
                case VM_TAG_F32: {
                    return lhs.value.i8 == rhs.value.f32;
                }
                case VM_TAG_F64: {
                    return lhs.value.i8 == rhs.value.f64;
                }
                default: {
                    return false;
                }
            }
        }
        case VM_TAG_I16: {
            switch (rhs.tag) {
                case VM_TAG_I8: {
                    return lhs.value.i16 == rhs.value.i8;
                }
                case VM_TAG_I16: {
                    return lhs.value.i16 == rhs.value.i16;
                }
                case VM_TAG_I32: {
                    return lhs.value.i16 == rhs.value.i32;
                }
                case VM_TAG_I64: {
                    return lhs.value.i16 == rhs.value.i64;
                }
                case VM_TAG_F32: {
                    return lhs.value.i16 == rhs.value.f32;
                }
                case VM_TAG_F64: {
                    return lhs.value.i16 == rhs.value.f64;
                }
                default: {
                    return false;
                }
            }
        }
        case VM_TAG_I32: {
            switch (rhs.tag) {
                case VM_TAG_I8: {
                    return lhs.value.i32 == rhs.value.i8;
                }
                case VM_TAG_I16: {
                    return lhs.value.i32 == rhs.value.i16;
                }
                case VM_TAG_I32: {
                    return lhs.value.i32 == rhs.value.i32;
                }
                case VM_TAG_I64: {
                    return lhs.value.i32 == rhs.value.i64;
                }
                case VM_TAG_F32: {
                    return lhs.value.i32 == rhs.value.f32;
                }
                case VM_TAG_F64: {
                    return lhs.value.i32 == rhs.value.f64;
                }
                default: {
                    return false;
                }
            }
        }
        case VM_TAG_I64: {
            switch (rhs.tag) {
                case VM_TAG_I8: {
                    return lhs.value.i64 == rhs.value.i8;
                }
                case VM_TAG_I16: {
                    return lhs.value.i64 == rhs.value.i16;
                }
                case VM_TAG_I32: {
                    return lhs.value.i64 == rhs.value.i32;
                }
                case VM_TAG_I64: {
                    return lhs.value.i64 == rhs.value.i64;
                }
                case VM_TAG_F32: {
                    return lhs.value.i64 == rhs.value.f32;
                }
                case VM_TAG_F64: {
                    return lhs.value.i64 == rhs.value.f64;
                }
                default: {
                    return false;
                }
            }
        }
        case VM_TAG_F32: {
            switch (rhs.tag) {
                case VM_TAG_I8: {
                    return lhs.value.f32 == rhs.value.i8;
                }
                case VM_TAG_I16: {
                    return lhs.value.f32 == rhs.value.i16;
                }
                case VM_TAG_I32: {
                    return lhs.value.f32 == rhs.value.i32;
                }
                case VM_TAG_I64: {
                    return lhs.value.f32 == rhs.value.i64;
                }
                case VM_TAG_F32: {
                    return lhs.value.f32 == rhs.value.f32;
                }
                case VM_TAG_F64: {
                    return lhs.value.f32 == rhs.value.f64;
                }
                default: {
                    return false;
                }
            }
        }
        case VM_TAG_F64: {
            switch (rhs.tag) {
                case VM_TAG_I8: {
                    return lhs.value.f64 == rhs.value.i8;
                }
                case VM_TAG_I16: {
                    return lhs.value.f64 == rhs.value.i16;
                }
                case VM_TAG_I32: {
                    return lhs.value.f64 == rhs.value.i32;
                }
                case VM_TAG_I64: {
                    return lhs.value.f64 == rhs.value.i64;
                }
                case VM_TAG_F32: {
                    return lhs.value.f64 == rhs.value.f32;
                }
                case VM_TAG_F64: {
                    return lhs.value.f64 == rhs.value.f64;
                }
                default: {
                    return false;
                }
            }
        }
        case VM_TAG_STR: {
            return rhs.tag == VM_TAG_STR && !strcmp(lhs.value.str, rhs.value.str);
        }
        case VM_TAG_FUN: {
            return rhs.tag == VM_TAG_FUN && lhs.value.i32 == rhs.value.i32;
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
        case VM_TAG_I8: {
            return (size_t)value.value.i8 * 1610612741;
        }
        case VM_TAG_I16: {
            return (size_t)value.value.i16 * 1610612741;
        }
        case VM_TAG_I32: {
            return (size_t)value.value.i32 * 1610612741;
        }
        case VM_TAG_I64: {
            return (size_t)value.value.i64 * 1610612741;
        }
        case VM_TAG_F32: {
            if (vm_value_is_int(value)) {
                return (size_t)(int32_t)value.value.f32 * 1610612741;
            }
            return (size_t)(uint32_t)value.value.i32;
        }
        case VM_TAG_F64: {
            if (vm_value_is_int(value)) {
                return (size_t)(int64_t)value.value.f64 * 1610612741;
            }
            return (size_t)(uint64_t)value.value.i64;
        }
        case VM_TAG_STR: {
            uint64_t ret = 0xcbf29ce484222325;
            const char *head = value.value.str;
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

void vm_table_init_size(vm_table_t *ret, size_t pow2) {
    ret->pairs = vm_malloc(sizeof(vm_table_pair_t) * (1 << pow2));
    memset(ret->pairs, 0, sizeof(vm_table_pair_t) * (1 << pow2));
    ret->alloc = pow2;
    ret->used = 0;
    ret->len = 0;
}

vm_table_t *vm_table_new_size(size_t pow2) {
    vm_table_t *ret = vm_malloc(sizeof(vm_table_t));
    vm_table_init_size(ret, pow2);
    return ret;
}

void vm_free_table(vm_table_t *table) {
    vm_free(table->pairs);
    vm_free(table);
}

vm_table_t *vm_table_new(void) {
    return vm_table_new_size(2);
}

vm_table_pair_t *vm_table_lookup(vm_table_t *table, vm_value_t key_val, vm_tag_t key_tag) {
    vm_obj_t key = (vm_obj_t){
        .tag = key_tag,
        .value = key_val,
    };
    size_t len = 1 << table->alloc;
    size_t and = len - 1;
    size_t stop = vm_value_hash(key) & and;
    size_t next = stop;
    do {
        vm_table_pair_t *pair = &table->pairs[next];
        vm_obj_t value = (vm_obj_t){
            .tag = pair->key_tag,
            .value = pair->key_val,
        };
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

void vm_table_iset(vm_table_t *table, uint64_t key_ival, uint64_t val_ival, vm_tag_t key_tag, vm_tag_t val_tag) {
    vm_table_set(table, *(vm_value_t *)&key_ival, *(vm_value_t *)&val_ival, key_tag, val_tag);
}

void vm_table_set(vm_table_t *restrict table, vm_value_t key_val, vm_value_t val_val, vm_tag_t key_tag, vm_tag_t val_tag) {
    if (table->alloc == 0) {
        return;
    }
    vm_obj_t key = (vm_obj_t){
        .tag = key_tag,
        .value = key_val,
    };
    size_t len = 1 << table->alloc;
    size_t and = len - 1;
    size_t stop = vm_value_hash(key) & and;
    size_t next = stop & and;
    do {
        vm_table_pair_t *pair = &table->pairs[next];
        if (pair->key_tag == VM_TAG_UNK || pair->key_tag == VM_TAG_NIL) {
            break;
        }
        vm_obj_t check = (vm_obj_t){
            .tag = pair->key_tag,
            .value = pair->key_val,
        };
        if (vm_obj_eq(key, check)) {
            if (val_tag == VM_TAG_NIL) {
                pair->key_tag = VM_TAG_UNK;
                if (vm_value_is_int(key)) {
                    int64_t i64val = vm_value_to_i64(key);
                    if (0 < i64val && i64val <= table->len) {
                        table->len = i64val - 1;
                    }
                }
            } else {
                pair->val_val = val_val;
                pair->val_tag = val_tag;
            }
            return;
        }
        next += 1;
        next &= and;
    } while (next != stop);
    if ((table->used + 1) * 100 >= ((uint32_t)1 << table->alloc) * 76) {
        vm_table_t ret;
        vm_table_init_size(&ret, table->alloc + 1);
        for (size_t i = 0; i < len; i++) {
            vm_table_pair_t *in_pair = &table->pairs[i];
            if (in_pair->key_tag != VM_TAG_UNK && in_pair->key_tag != VM_TAG_NIL) {
                vm_table_set_pair(&ret, in_pair);
            }
        }
        vm_table_set(&ret, key_val, val_val, key_tag, val_tag);
        *table = ret;
    } else if (val_tag == VM_TAG_NIL) {
        table->pairs[next] = (vm_table_pair_t){
            .key_tag = key_tag,
            .key_val = key_val,
            .val_tag = val_tag,
            .val_val = val_val,
        };

        vm_obj_t nv = (vm_obj_t) {
            .tag = key_tag,
            .value = key_val,
        };

        double n = vm_value_to_f64(nv);

        if (1 <= n && n <= table->len && (double) (size_t) n == n) {
            table->len = (size_t) (n - 1);
        }
    } else {
        table->used += 1;
        table->pairs[next] = (vm_table_pair_t){
            .key_tag = key_tag,
            .key_val = key_val,
            .val_tag = val_tag,
            .val_val = val_val,
        };
        vm_obj_t vlen = (vm_obj_t){
            .tag = VM_TAG_I32,
            .value.i32 = table->len + 1,
        };
        if (vm_obj_eq(vlen, key)) {
            while (true) {
                int32_t next = table->len + 1;
                vm_table_pair_t *got = vm_table_lookup(table, (vm_value_t){.i32 = next}, VM_TAG_I32);
                if (got == NULL) {
                    break;
                }
                table->len = next;
            }
        }
    }
}

void vm_table_set_pair(vm_table_t *table, vm_table_pair_t *pair) {
    vm_table_set(table, pair->key_val, pair->val_val, pair->key_tag, pair->val_tag);
}

void vm_table_get_pair(vm_table_t *table, vm_table_pair_t *out) {
    vm_value_t key_val = out->key_val;
    vm_tag_t key_tag = out->key_tag;
    vm_table_pair_t *pair = vm_table_lookup(table, key_val, key_tag);
    if (pair != NULL) {
        out->val_val = pair->val_val;
        out->val_tag = pair->val_tag;
        return;
    }
    out->val_tag = VM_TAG_NIL;
    return;
}
