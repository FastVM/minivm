#include "./obj.h"

#include "./ir.h"
#include "./std/libs/io.h"


bool vm_value_eq(vm_std_value_t lhs, vm_std_value_t rhs) {
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
        default: {
            return lhs.tag == rhs.tag && lhs.value.all == rhs.value.all;
        }
    }
}

vm_table_t *vm_table_new(void) {
    vm_table_t *ret = vm_malloc(sizeof(vm_table_t));
    *ret = (vm_table_t){0};
    return ret;
}

static vm_pair_t *vm_table_lookup(vm_table_t *table, vm_value_t key_val, uint32_t key_tag) {
    uint32_t head = 0;
    vm_std_value_t lhs = (vm_std_value_t) {
        .tag = key_tag,
        .value = key_val,
    };
    while (head * sizeof(vm_pair_t) < table->nbytes) {
        vm_pair_t *pair = &table->pairs[head];
        vm_std_value_t rhs = (vm_std_value_t){.tag = pair->key_tag, .value = pair->key_val};
        if (vm_value_eq(lhs, rhs)) {
            return pair;
        }
        head += 1;
    }
    return NULL;
}

void vm_table_set(vm_table_t *table, vm_value_t key_val, vm_value_t val_val,
                  uint32_t key_tag, uint32_t val_tag) {
    vm_pair_t *pair = vm_table_lookup(table, key_val, key_tag);
    if (pair != NULL) {
        pair->val_val = pair->val_val;
        pair->val_tag = pair->val_tag;
        return;
    }
    size_t head = table->nbytes / sizeof(vm_pair_t);
    table->nbytes += sizeof(vm_pair_t);
    if (table->nbytes >= (UINT32_C(1) << table->alloc)) {
        while (table->nbytes >= (UINT32_C(1) << table->alloc)) {
            table->alloc += 1;
        }
        table->pairs = vm_realloc(table->pairs, UINT32_C(1) << table->alloc);
    }
    table->pairs[head] = (vm_pair_t){
        .key_val = key_val,
        .val_val = val_val,
        .key_tag = key_tag,
        .val_tag = val_tag,
    };
}

void vm_table_set_pair(vm_table_t *table, vm_pair_t *pair) {
    vm_table_set(table, pair->key_val, pair->val_val, pair->key_tag, pair->val_tag);
}

vm_pair_t *vm_table_get_pair(vm_table_t *table, vm_pair_t *out) {
    vm_value_t key_val = out->key_val;
    vm_tag_t key_tag = (vm_tag_t)out->key_tag;
    vm_pair_t *pair = vm_table_lookup(table, key_val, key_tag);
    if (pair != NULL) {
        out->val_val = pair->val_val;
        out->val_tag = pair->val_tag;
        return out;
    }
    out->val_tag = VM_TAG_NIL;
    return out;
}

double vm_table_len(vm_table_t *table) {
    int64_t check = 1;
    uint32_t head = 0;
    while (head * sizeof(vm_pair_t) < table->nbytes) {
        vm_pair_t *pair = &table->pairs[head];
        switch (pair->key_tag) {
            // case VM_TAG_I64: {
            //     if (pair->key_val.i64 == check) {
            //         check += 1;
            //         head = 0;
            //         goto next;
            //     }
            //     break;
            // }
            case VM_TAG_F64: {
                if (pair->key_val.f64 == (double)check) {
                    check += 1;
                    head = 0;
                    goto next;
                }
                break;
            }
            default: {
                break;
            }
        }
        head += 1;
    next:;
    }
    return (double)(check - 1);
}
