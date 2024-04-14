#if !defined(VM_HEADER_OBJ)
#define VM_HEADER_OBJ

#include "lib.h"

union vm_value_t;
typedef union vm_value_t vm_value_t;

struct vm_pair_t;
typedef struct vm_pair_t vm_pair_t;

struct vm_table_t;
typedef struct vm_table_t vm_table_t;

struct vm_std_value_t;
typedef struct vm_std_value_t vm_std_value_t;

struct vm_std_closure_t;
typedef struct vm_std_closure_t vm_std_closure_t;

#include "ir/tag.h"

union vm_value_t {
    void *all;
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
    void (*ffi)(vm_std_closure_t *closure, vm_std_value_t *args);
};

struct vm_std_value_t {
    vm_value_t value;
    vm_type_t tag;
};

struct vm_pair_t {
    vm_value_t key_val;
    vm_value_t val_val;
    vm_type_t key_tag;
    vm_type_t val_tag;
};

struct vm_table_t {
    vm_pair_t *pairs;
    uint32_t len;
    uint32_t used;
    uint8_t alloc;
};

#include "ir/ir.h"

struct vm_std_closure_t {
    vm_config_t *config;
    vm_blocks_t *blocks;
    vm_std_value_t data[];
};

bool vm_value_eq(vm_std_value_t lhs, vm_std_value_t rhs);
bool vm_value_is_int(vm_std_value_t val);
int64_t vm_value_to_i64(vm_std_value_t arg);
double vm_value_to_f64(vm_std_value_t arg);
bool vm_value_can_to_n64(vm_std_value_t val);

void vm_free_table(vm_table_t *table);
vm_table_t *vm_table_new(void);
vm_pair_t *vm_table_lookup(vm_table_t *table, vm_value_t key_val, vm_type_t key_tag);
void vm_table_iset(vm_table_t *table, uint64_t key_ival, uint64_t val_ival, vm_type_t key_tag, vm_type_t val_tag);
void vm_table_set(vm_table_t *table, vm_value_t key_val, vm_value_t val_val, vm_type_t key_tag, vm_type_t val_tag);
void vm_table_set_pair(vm_table_t *table, vm_pair_t *pair);
void vm_table_get_pair(vm_table_t *table, vm_pair_t *pair);

#define VM_VALUE_LITERAL_TYPE_TO_TAG_b(...) VM_TYPE_BOOL
#define VM_VALUE_LITERAL_TYPE_TO_TAG_i8(...) VM_TYPE_I8
#define VM_VALUE_LITERAL_TYPE_TO_TAG_i16(...) VM_TYPE_I16
#define VM_VALUE_LITERAL_TYPE_TO_TAG_i32(...) VM_TYPE_I32
#define VM_VALUE_LITERAL_TYPE_TO_TAG_i64(...) VM_TYPE_I64
#define VM_VALUE_LITERAL_TYPE_TO_TAG_f32(...) VM_TYPE_F32
#define VM_VALUE_LITERAL_TYPE_TO_TAG_f64(...) VM_TYPE_F64
#define VM_VALUE_LITERAL_TYPE_TO_TAG_str(...) VM_TYPE_STR
#define VM_VALUE_LITERAL_TYPE_TO_TAG_table(...) VM_TYPE_TAB
#define VM_VALUE_LITERAL_TYPE_TO_TAG_ffi(...) VM_TYPE_FFI

#define VM_VALUE_LITERAL_TYPE_TO_TAG_CONCAT2_IMPL(X_, Y_) X_##Y_
#define VM_VALUE_LITERAL_TYPE_TO_TAG_CONCAT2(X_, Y_) VM_VALUE_LITERAL_TYPE_TO_TAG_CONCAT2_IMPL(X_, Y_)

#define VM_STD_VALUE_LITERAL(TYPE_, VALUE_)                                                  \
    (vm_std_value_t) {                                                                       \
        .tag = VM_VALUE_LITERAL_TYPE_TO_TAG_CONCAT2(VM_VALUE_LITERAL_TYPE_TO_TAG_, TYPE_)(), \
        .value = (vm_value_t){                                                               \
            .TYPE_ = (VALUE_),                                                               \
        },                                                                                   \
    }

#define VM_STD_VALUE_NIL ((vm_std_value_t){.tag = (VM_TYPE_NIL)})

#define VM_STD_VALUE_NUMBER(CONFIG_, VALUE_) ({                  \
    vm_config_t *config_ = (CONFIG_);                            \
    vm_std_value_t ret_;                                         \
    switch (config_->use_num) {                                  \
        case VM_USE_NUM_I8: {                                    \
            ret_ = VM_STD_VALUE_LITERAL(i8, (int8_t)(VALUE_));   \
            break;                                               \
        }                                                        \
        case VM_USE_NUM_I16: {                                   \
            ret_ = VM_STD_VALUE_LITERAL(i16, (int16_t)(VALUE_)); \
            break;                                               \
        }                                                        \
        case VM_USE_NUM_I32: {                                   \
            ret_ = VM_STD_VALUE_LITERAL(i32, (int32_t)(VALUE_)); \
            break;                                               \
        }                                                        \
        case VM_USE_NUM_I64: {                                   \
            ret_ = VM_STD_VALUE_LITERAL(i64, (int64_t)(VALUE_)); \
            break;                                               \
        }                                                        \
        case VM_USE_NUM_F32: {                                   \
            ret_ = VM_STD_VALUE_LITERAL(f32, (float)(VALUE_));   \
            break;                                               \
        }                                                        \
        case VM_USE_NUM_F64: {                                   \
            ret_ = VM_STD_VALUE_LITERAL(f64, (double)(VALUE_));  \
            break;                                               \
        }                                                        \
    }                                                            \
    ret_;                                                        \
})

#define VM_TABLE_SET_VALUE(TABLE_, KEY_, VALUE_) ({                       \
    vm_table_t *table_ = (TABLE_);                                        \
    vm_std_value_t key_ = (KEY_);                                         \
    vm_std_value_t value_ = (VALUE_);                                     \
    vm_table_set(table_, key_.value, value_.value, key_.tag, value_.tag); \
})

#define VM_TABLE_SET(TABLE_, KEY_TYPE_, KEY_, VALUE_TYPE_, VALUE_) ({     \
    vm_table_t *table_ = (TABLE_);                                        \
    vm_std_value_t key_ = VM_STD_VALUE_LITERAL(KEY_TYPE_, KEY_);          \
    vm_std_value_t value_ = VM_STD_VALUE_LITERAL(VALUE_TYPE_, VALUE_);    \
    vm_table_set(table_, key_.value, value_.value, key_.tag, value_.tag); \
})

#define VM_TABLE_LOOKUP_STR(TABLE_, STR_) (vm_table_lookup((TABLE_), (vm_value_t){.str = (STR_)}, VM_TYPE_STR))

#endif
