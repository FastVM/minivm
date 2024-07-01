#if !defined(VM_HEADER_OBJ)
#define VM_HEADER_OBJ

#include "lib.h"

struct vm_table_pair_t;
typedef struct vm_table_pair_t vm_table_pair_t;

bool vm_obj_eq(vm_obj_t lhs, vm_obj_t rhs);
bool vm_value_is_int(vm_obj_t val);
int64_t vm_value_to_i64(vm_obj_t arg);
double vm_value_to_f64(vm_obj_t arg);
bool vm_value_can_to_n64(vm_obj_t val);

void vm_free_table(vm_table_t *table);
vm_table_t *vm_table_new(void);
vm_table_pair_t *vm_table_lookup(vm_table_t *table, vm_value_t key_val, vm_tag_t key_tag);
void vm_table_iset(vm_table_t *table, uint64_t key_ival, uint64_t val_ival, vm_tag_t key_tag, vm_tag_t val_tag);
void vm_table_set(vm_table_t *table, vm_value_t key_val, vm_value_t val_val, vm_tag_t key_tag, vm_tag_t val_tag);
void vm_table_set_pair(vm_table_t *table, vm_table_pair_t *pair);
void vm_table_get_pair(vm_table_t *table, vm_table_pair_t *pair);

#define VM_VALUE_LITERAL_TYPE_TO_TAG_b(...) VM_TAG_BOOL
#define VM_VALUE_LITERAL_TYPE_TO_TAG_i8(...) VM_TAG_I8
#define VM_VALUE_LITERAL_TYPE_TO_TAG_i16(...) VM_TAG_I16
#define VM_VALUE_LITERAL_TYPE_TO_TAG_i32(...) VM_TAG_I32
#define VM_VALUE_LITERAL_TYPE_TO_TAG_i64(...) VM_TAG_I64
#define VM_VALUE_LITERAL_TYPE_TO_TAG_f32(...) VM_TAG_F32
#define VM_VALUE_LITERAL_TYPE_TO_TAG_f64(...) VM_TAG_F64
#define VM_VALUE_LITERAL_TYPE_TO_TAG_str(...) VM_TAG_STR
#define VM_VALUE_LITERAL_TYPE_TO_TAG_table(...) VM_TAG_TAB
#define VM_VALUE_LITERAL_TYPE_TO_TAG_ffi(...) VM_TAG_FFI
#define VM_VALUE_LITERAL_TYPE_TO_TAG_error(...) VM_TAG_ERROR

#define VM_VALUE_LITERAL_TYPE_TO_TAG_CONCAT2_IMPL(X_, Y_) X_##Y_
#define VM_VALUE_LITERAL_TYPE_TO_TAG_CONCAT2(X_, Y_) VM_VALUE_LITERAL_TYPE_TO_TAG_CONCAT2_IMPL(X_, Y_)

#define VM_OBJ_LITERAL(TYPE_, VALUE_)                                                  \
    (vm_obj_t) {                                                                       \
        .tag = VM_VALUE_LITERAL_TYPE_TO_TAG_CONCAT2(VM_VALUE_LITERAL_TYPE_TO_TAG_, TYPE_)(), \
        .value = (vm_value_t){                                                               \
            .TYPE_ = (VALUE_),                                                               \
        },                                                                                   \
    }

#define VM_OBJ_NIL ((vm_obj_t){.tag = (VM_TAG_NIL)})

#define VM_OBJ_NUMBER(CONFIG_, VALUE_) ({                  \
    vm_t *config_ = (CONFIG_);                            \
    vm_obj_t ret_;                                         \
    switch (config_->use_num) {                                  \
        case VM_USE_NUM_I8: {                                    \
            ret_ = VM_OBJ_LITERAL(i8, (int8_t)(VALUE_));   \
            break;                                               \
        }                                                        \
        case VM_USE_NUM_I16: {                                   \
            ret_ = VM_OBJ_LITERAL(i16, (int16_t)(VALUE_)); \
            break;                                               \
        }                                                        \
        case VM_USE_NUM_I32: {                                   \
            ret_ = VM_OBJ_LITERAL(i32, (int32_t)(VALUE_)); \
            break;                                               \
        }                                                        \
        case VM_USE_NUM_I64: {                                   \
            ret_ = VM_OBJ_LITERAL(i64, (int64_t)(VALUE_)); \
            break;                                               \
        }                                                        \
        case VM_USE_NUM_F32: {                                   \
            ret_ = VM_OBJ_LITERAL(f32, (float)(VALUE_));   \
            break;                                               \
        }                                                        \
        case VM_USE_NUM_F64: {                                   \
            ret_ = VM_OBJ_LITERAL(f64, (double)(VALUE_));  \
            break;                                               \
        }                                                        \
    }                                                            \
    ret_;                                                        \
})

#define VM_TABLE_SET_VALUE(TABLE_, KEY_, VALUE_) ({                       \
    vm_table_t *table_ = (TABLE_);                                        \
    vm_obj_t key_ = (KEY_);                                         \
    vm_obj_t value_ = (VALUE_);                                     \
    vm_table_set(table_, key_.value, value_.value, key_.tag, value_.tag); \
})

#define VM_TABLE_SET(TABLE_, KEY_TYPE_, KEY_, VALUE_TYPE_, VALUE_) ({     \
    vm_table_t *table_ = (TABLE_);                                        \
    vm_obj_t key_ = VM_OBJ_LITERAL(KEY_TYPE_, KEY_);          \
    vm_obj_t value_ = VM_OBJ_LITERAL(VALUE_TYPE_, VALUE_);    \
    vm_table_set(table_, key_.value, value_.value, key_.tag, value_.tag); \
})

#define VM_TABLE_LOOKUP_STR(TABLE_, STR_) (vm_table_lookup((TABLE_), (vm_value_t){.str = (STR_)}, VM_TAG_STR))

#endif
