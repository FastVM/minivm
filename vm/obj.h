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

struct vm_std_value_t;
typedef struct vm_std_value_t vm_std_value_t;

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
    vm_std_value_t *closure;
    void (*ffi)(vm_std_value_t *args);
    void *all;
};

struct vm_std_value_t {
    vm_value_t value;
    uint32_t tag;
};

struct vm_pair_t {
    vm_value_t key_val;
    vm_value_t val_val;
    uint32_t key_tag;
    uint32_t val_tag;
};

struct vm_table_t {
    vm_pair_t *pairs;
    uint32_t len;
    uint32_t used;
    uint8_t alloc;
};

bool vm_value_eq(vm_std_value_t lhs, vm_std_value_t rhs);
bool vm_value_is_int(vm_std_value_t val);
int64_t vm_value_to_i64(vm_std_value_t arg);

vm_table_t *vm_table_new(void);
vm_pair_t *vm_table_lookup(vm_table_t *table, vm_value_t key_val, uint32_t key_tag);
void vm_table_set(vm_table_t *table, vm_value_t key_val, vm_value_t val_val, uint32_t key_tag, uint32_t val_tag);
void vm_table_set_pair(vm_table_t *table, vm_pair_t *pair);
void vm_table_get_pair(vm_table_t *table, vm_pair_t *pair);

#define vm_table_lookup_str(TABLE_, STR_) (vm_table_lookup((TABLE_), (vm_value_t) {.str = (STR_)}, VM_TAG_STR))

#endif
