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

typedef uint8_t vm_tag_t;

struct vm_types_t {
    size_t ntags;
    vm_tag_t *tags;
};

typedef struct vm_types_t vm_types_t;

static uint32_t vm_type_tag(vm_tag_t type) {
    return type;
}

static inline bool vm_type_eq(vm_tag_t a, vm_tag_t b) {
    return a == b;
}

#endif
