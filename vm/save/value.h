#if !defined(VM_HEADER_SAVE_DATA_VALUE)
#define VM_HEADER_SAVE_DATA_VALUE

#include "./io.h"

struct vm_save_loaded_t;
typedef struct vm_save_loaded_t vm_save_loaded_t;

struct vm_save_loaded_t {
    vm_blocks_t *blocks;
    vm_obj_t env;
};


void vm_load_value(vm_t *vm, vm_save_t save);
vm_save_t vm_save_value(vm_t *vm);

#endif
