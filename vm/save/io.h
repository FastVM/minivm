#if !defined(VM_HEADER_SAVE_IO)
#define VM_HEADER_SAVE_IO

#include "../lib.h"

struct vm_save_t;
typedef struct vm_save_t vm_save_t;

struct vm_save_t {
    size_t len;
    uint8_t *buf;
};

vm_save_t vm_save_load(FILE *file);

#endif
