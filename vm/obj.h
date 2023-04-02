#if !defined(VM_HEADER_TABLE)
#define VM_HEADER_TABLE

#include "lib.h"

union vm_value_t;
typedef union vm_value_t vm_value_t;

struct vm_pair_t;
typedef struct vm_pair_t vm_pair_t;

struct vm_table_t;
typedef struct vm_table_t vm_table_t;

union vm_value_t {
    bool b;
    int64_t i64;
    double f64;
    const char *str;
    vm_func_t *func;
    vm_table_t *table;
};

struct vm_pair_t {
    vm_value_t key_val;
    vm_value_t val_val;
    vm_tag_t key_tag;
    vm_tag_t val_tag;
};

struct vm_table_t {
    vm_pair_t *pairs;
    uint32_t len;
    uint32_t alloc;
};

#endif
