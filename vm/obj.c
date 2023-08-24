#include "obj.h"

vm_table_t *vm_table_new(void) {
    vm_table_t *ret = vm_malloc(sizeof(vm_table_t));
    *ret = (vm_table_t){0};
    return ret;
}

vm_pair_t *vm_table_lookup(vm_table_t *table, vm_value_t key_val, uint32_t key_tag) {
    uint32_t head = 0;
    while (head * sizeof(vm_pair_t) < table->nbytes) {
        vm_pair_t *pair = &table->pairs[head];
        if (pair->key_tag == key_tag) {
            switch (key_tag) {
                case VM_TAG_NIL: {
                    return pair;
                }
                case VM_TAG_BOOL: {
                    if (pair->key_val.b == key_val.b) {
                        return pair;
                    }
                    break;
                }
                case VM_TAG_F64: {
                    if (pair->key_val.f64 == key_val.f64) {
                        return pair;
                    }
                    break;
                }
                case VM_TAG_STR: {
                    if (!strcmp(pair->key_val.str, key_val.str)) {
                        return pair;
                    }
                    break;
                }
                default: {
                    if (pair->key_val.all == key_val.all) {
                        return pair;
                    }
                    break;
                }
            }
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

void vm_table_get_pair(vm_table_t *table, vm_pair_t *out) {
    vm_value_t key_val = out->key_val;
    vm_tag_t key_tag = out->key_tag;
    uint32_t head = 0;
    vm_pair_t *pair = vm_table_lookup(table, key_val, key_tag);
    if (pair != NULL) {
        out->val_val = pair->val_val;
        out->val_tag = pair->val_tag;
        return;
    }
    out->val_tag = VM_TAG_NIL;
    return;
}

double vm_table_len(vm_table_t *table) {
    int64_t check = 1;
    uint32_t head = 0;
    while (head * sizeof(vm_pair_t) < table->nbytes) {
        vm_pair_t *pair = &table->pairs[head];
        switch (pair->key_tag) {
            case VM_TAG_I64: {
                if (pair->key_val.i64 == check) {
                    check += 1;
                    head = 0;
                    goto next;
                }
                break;
            }
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
