#if !defined(VM_HEADER_TAG)
#define VM_HEADER_TAG

#include "lib.h"

typedef uint8_t vm_tag_t;

enum {
    VM_TAG_INIT,
    VM_TAG_UNK,
    VM_TAG_NIL,
    VM_TAG_BOOL,
    VM_TAG_I8,
    VM_TAG_I16,
    VM_TAG_I32,
    VM_TAG_I64,
    VM_TAG_U8,
    VM_TAG_U16,
    VM_TAG_U32,
    VM_TAG_U64,
    VM_TAG_F32,
    VM_TAG_F64,
    VM_TAG_PTR,
    VM_TAG_MAX,
};

struct vm_tags_t {
    size_t ntags;
    vm_tag_t tags[0];
};

typedef struct vm_tags_t vm_tags_t;

#endif
