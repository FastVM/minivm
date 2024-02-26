#if !defined(VM_HEADER_TAG)
#define VM_HEADER_TAG

#include "../lib.h"

typedef struct {
    uint32_t tag;
} vm_type_t;

enum {
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

#define VM_TYPE_UNK ((vm_type_t) { VM_TAG_UNK })
#define VM_TYPE_NIL ((vm_type_t) { VM_TAG_NIL })
#define VM_TYPE_BOOL ((vm_type_t) { VM_TAG_BOOL })
#define VM_TYPE_I8 ((vm_type_t) { VM_TAG_I8 })
#define VM_TYPE_I16 ((vm_type_t) { VM_TAG_I16 })
#define VM_TYPE_I32 ((vm_type_t) { VM_TAG_I32 })
#define VM_TYPE_I64 ((vm_type_t) { VM_TAG_I64 })
#define VM_TYPE_F32 ((vm_type_t) { VM_TAG_F32 })
#define VM_TYPE_F64 ((vm_type_t) { VM_TAG_F64 })
#define VM_TYPE_STR ((vm_type_t) { VM_TAG_STR })
#define VM_TYPE_CLOSURE ((vm_type_t) { VM_TAG_CLOSURE })
#define VM_TYPE_FUN ((vm_type_t) { VM_TAG_FUN })
#define VM_TYPE_TAB ((vm_type_t) { VM_TAG_TAB })
#define VM_TYPE_FFI ((vm_type_t) { VM_TAG_FFI })
#define VM_TYPE_ERROR ((vm_type_t) { VM_TAG_ERROR })

struct vm_types_t {
    size_t ntags;
    vm_type_t *tags;
};

typedef struct vm_types_t vm_types_t;

static bool vm_type_eq(vm_type_t a, vm_type_t b) {
    return a.tag == b.tag;
}

#endif
