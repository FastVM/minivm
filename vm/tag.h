#if !defined(VM_HEADER_TAG)
#define VM_HEADER_TAG

#include "lib.h"

struct vm_tag_t;
typedef struct vm_tag_t vm_tag_t;

enum {
    VM_TAG_TYPE_INIT,
    VM_TAG_TYPE_UNK,
    VM_TAG_TYPE_NIL,
    VM_TAG_TYPE_BOOL,
    VM_TAG_TYPE_SINT,
    VM_TAG_TYPE_UINT,
    VM_TAG_TYPE_FLOAT,
    VM_TAG_TYPE_LIB,
    VM_TAG_TYPE_SYM,
    VM_TAG_TYPE_FN,
};

struct vm_tag_t {
    uint8_t type;
    size_t size;
    vm_tag_t *data;
    size_t ndata;
};

#define VM_TAG_UNK ((vm_tag_t){.type = VM_TAG_TYPE_UNK})
#define VM_TAG_NIL ((vm_tag_t){.type = VM_TAG_TYPE_NIL, .size = 0})
#define VM_TAG_BOOL ((vm_tag_t){.type = VM_TAG_TYPE_BOOL, .size = sizeof(bool)})
#define VM_TAG_LIB ((vm_tag_t){.type = VM_TAG_TYPE_LIB, .size = sizeof(void *)})
#define VM_TAG_SYM ((vm_tag_t){.type = VM_TAG_TYPE_SYM, .size = sizeof(void *)})
#define VM_TAG_FN ((vm_tag_t){.type = VM_TAG_TYPE_FN, .size = sizeof(void*)})

#define VM_TAG_I8 ((vm_tag_t){.type = VM_TAG_TYPE_SINT, .size = sizeof(int8_t)})
#define VM_TAG_I16 ((vm_tag_t){.type = VM_TAG_TYPE_SINT, .size = sizeof(int16_t)})
#define VM_TAG_I32 ((vm_tag_t){.type = VM_TAG_TYPE_SINT, .size = sizeof(int32_t)})
#define VM_TAG_I64 ((vm_tag_t){.type = VM_TAG_TYPE_SINT, .size = sizeof(int64_t)})

#define VM_TAG_U8 ((vm_tag_t){.type = VM_TAG_TYPE_UINT, .size = sizeof(uint8_t)})
#define VM_TAG_U16 ((vm_tag_t){.type = VM_TAG_TYPE_UINT, .size = sizeof(uint16_t)})
#define VM_TAG_U32 ((vm_tag_t){.type = VM_TAG_TYPE_UINT, .size = sizeof(uint32_t)})
#define VM_TAG_U64 ((vm_tag_t){.type = VM_TAG_TYPE_UINT, .size = sizeof(uint64_t)})

#define VM_TAG_F32 ((vm_tag_t){.type = VM_TAG_TYPE_UINT, .size = sizeof(float)})
#define VM_TAG_F64 ((vm_tag_t){.type = VM_TAG_TYPE_UINT, .size = sizeof(double)})

static vm_tag_t vm_tag_ptr(void) {
    if (sizeof(size_t) == 4) {
        return VM_TAG_U32;
    } else {
        return VM_TAG_U64;
    }
}

#define VM_TAG_PTR (vm_tag_ptr())

static bool vm_tag_eq(vm_tag_t a, vm_tag_t b) {
    if (a.type != b.type || a.size != b.size || a.ndata != b.ndata) {
        return false;
    }
    for (size_t index = 0; index < a.ndata; index++) {
        if (!vm_tag_eq(a.data[index], b.data[index])) {
            return false;
        }
    }
    return true;
}

#endif
