#if !defined(VM_HEADER_TABLE)
#define VM_HEADER_TABLE

#include "lib.h"
#include "tag.h"

union vm_value_t;
typedef union vm_value_t vm_value_t;

struct vm_pair_t;
typedef struct vm_pair_t vm_pair_t;

struct vm_table_t;
typedef struct vm_table_t vm_table_t;

union vm_value_t {
    bool b;
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
    const char *str;
    vm_table_t *table;
    void *all;
};

struct vm_pair_t {
    // uint8_t buf[256-24];
    vm_value_t key_val;
    vm_value_t val_val;
    uint32_t key_tag;
    uint32_t val_tag;
};

struct vm_table_t {
    vm_pair_t *pairs;
    uint32_t nbytes;
    uint8_t alloc;
};

vm_table_t *vm_table_new(void);
void vm_table_set(vm_table_t *table, vm_value_t key_val, vm_value_t val_val,
                  uint32_t key_tag, uint32_t val_tag);
void vm_table_set_pair(vm_table_t *table, vm_pair_t *pair);
vm_pair_t *vm_table_get_pair(vm_table_t *table, vm_pair_t *pair);
double vm_table_len(vm_table_t *table);

#endif
