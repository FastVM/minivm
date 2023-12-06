
#if !defined(VM_HEADER_SERIALIZE_BINARY)
#define VM_HEADER_SERIALIZE_BINARY

#include "../obj.h"

struct vm_serialize_binary_buf_t;
typedef struct vm_serialize_binary_buf_t vm_serialize_binary_buf_t;

struct vm_serialize_binary_buf_t {
    uint8_t *mem;
    uint32_t len;
    uint32_t alloc;
};

#endif
