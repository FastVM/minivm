#if !defined(VM_HEADER_SAVE_SAVE)
#define VM_HEADER_SAVE_SAVE

#include "../lib.h"
#include "../std/io.h"

struct vm_save_t;
typedef struct vm_save_t vm_save_t;

struct vm_save_t {
    size_t len;
    uint8_t *buf;
};

vm_save_t vm_save_load(FILE *file);
vm_std_value_t vm_load_value(vm_save_t arg);
vm_save_t vm_save_value(vm_std_value_t arg);

#endif
