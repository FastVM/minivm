#if !defined(VM_HEADER_TAG)
#define VM_HEADER_TAG

#include "../lib.h"

enum vm_tag_t {
    VM_TAG_UNK,
    VM_TAG_NIL,
    VM_TAG_BOOL,
    VM_TAG_I8,
    VM_TAG_I16,
    VM_TAG_I32,
    VM_TAG_I64,
    VM_TAG_F32,
    VM_TAG_F64,
    VM_TAG_STR,
    VM_TAG_CLOSURE,
    VM_TAG_FUN,
    VM_TAG_TAB,
    VM_TAG_FFI,
    VM_TAG_ERROR,
    VM_TAG_MAX,
};

typedef enum vm_tag_t vm_tag_t;

struct vm_type_value_t;
typedef struct vm_type_value_t vm_type_value_t;

typedef const vm_type_value_t *vm_type_t;

struct vm_type_value_t {
    uint32_t tag;
};

extern const vm_type_value_t vm_type_base[VM_TAG_MAX];

#define VM_TYPE_UNK (NULL)
#define VM_TYPE_NIL (&vm_type_base[VM_TAG_NIL])
#define VM_TYPE_BOOL (&vm_type_base[VM_TAG_BOOL])
#define VM_TYPE_I8 (&vm_type_base[VM_TAG_I8])
#define VM_TYPE_I16 (&vm_type_base[VM_TAG_I16])
#define VM_TYPE_I32 (&vm_type_base[VM_TAG_I32])
#define VM_TYPE_I64 (&vm_type_base[VM_TAG_I64])
#define VM_TYPE_F32 (&vm_type_base[VM_TAG_F32])
#define VM_TYPE_F64 (&vm_type_base[VM_TAG_F64])
#define VM_TYPE_STR (&vm_type_base[VM_TAG_STR])
#define VM_TYPE_CLOSURE (&vm_type_base[VM_TAG_CLOSURE])
#define VM_TYPE_FUN (&vm_type_base[VM_TAG_FUN])
#define VM_TYPE_TAB (&vm_type_base[VM_TAG_TAB])
#define VM_TYPE_FFI (&vm_type_base[VM_TAG_FFI])
#define VM_TYPE_ERROR (&vm_type_base[VM_TAG_ERROR])

struct vm_types_t {
    size_t ntags;
    vm_type_t *tags;
};

typedef struct vm_types_t vm_types_t;

static uint32_t vm_type_tag(vm_type_t type) {
    if (type == NULL) {
        return VM_TAG_UNK;
    }
    return type->tag;
}

static inline bool vm_type_eq(vm_type_t a, vm_type_t b) {
    if (a == NULL || b == NULL) {
        return a == NULL && b == NULL;
    }
    return vm_type_tag(a) == vm_type_tag(b);
}

#endif
