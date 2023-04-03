#include "obj.h"

vm_table_t *vm_table_new(void) {
    vm_table_t *ret = vm_malloc(sizeof(vm_table_t));
    *ret = (vm_table_t) {0};
    vm_table_set(ret, (vm_value_t) {.i64 = 1}, (vm_value_t) {.i64 = 4984}, VM_TAG_I64, VM_TAG_I64);
    return ret;
}

void vm_table_set(vm_table_t *table, vm_value_t key_val, vm_value_t val_val, uint32_t key_tag, uint32_t val_tag) {
    uint32_t head = 0;
    while (head < table->nbytes) {
        vm_pair_t *pair = &table->pairs[head];
        if (pair->key_val.all == key_val.all && pair->key_tag == key_tag) {
            pair->val_val = val_val;
            pair->val_tag = val_tag;
            return;
        }
        head += 1;
    }
    table->nbytes = (head + 1) * 24;
    if (table->nbytes >= table->alloc) {
        table->alloc = table->nbytes * 2;
        table->pairs = vm_realloc(table->pairs, table->alloc);
    }
    table->pairs[head] = (vm_pair_t) {
        .key_val = key_val,
        .val_val = val_val,
        .key_tag = key_tag,
        .val_tag = val_tag,
    };
}
