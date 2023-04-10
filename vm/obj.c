#include "obj.h"

vm_table_t *vm_table_new(void) {
    vm_table_t *ret = vm_malloc(sizeof(vm_table_t));
    *ret = (vm_table_t){0};
    return ret;
}

void vm_table_set(vm_table_t *table, vm_value_t key_val, vm_value_t val_val,
                  uint32_t key_tag, uint32_t val_tag) {
    uint32_t head = 0;
    while (head * sizeof(vm_pair_t) < table->nbytes) {
        vm_pair_t *pair = &table->pairs[head];
        if (pair->key_val.all == key_val.all && pair->key_tag == key_tag) {
            pair->val_val = val_val;
            pair->val_tag = val_tag;
            return;
        }
        head += 1;
    }
    table->nbytes = (head + 1) * sizeof(vm_pair_t);
    if (table->nbytes >= (UINT32_C(1) << table->alloc)) {
        while (table->nbytes >= (UINT32_C(1) << table->alloc)) {
            table->alloc += 1;
        }
        if (table->pairs == NULL) {
            table->pairs = vm_malloc(UINT32_C(1) << table->alloc);
        } else {
            table->pairs = vm_realloc(table->pairs, UINT32_C(1) << table->alloc);
        }
    }
    table->pairs[head] = (vm_pair_t){
        .key_val = key_val,
        .val_val = val_val,
        .key_tag = key_tag,
        .val_tag = val_tag,
    };
}

int64_t vm_table_len(vm_table_t *table) {
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
    return check - 1;
}
