#if !defined(VM_HEADER_TAG)
#define VM_HEADER_TAG

#include "lib.h"

typedef uint8_t vm_tag_t;

enum {
    VM_TAG_UNK,
    VM_TAG_NIL,
    VM_TAG_BOOL,
    VM_TAG_I64,
    VM_TAG_F64,
    VM_TAG_STR,
    VM_TAG_FUN,
    VM_TAG_TAB,
    VM_TAG_FFI,
    VM_TAG_MAX,
};

struct vm_tags_t {
    size_t ntags;
    vm_tag_t *tags;
};

typedef struct vm_tags_t vm_tags_t;

#endif
