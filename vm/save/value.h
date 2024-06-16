#if !defined(VM_HEADER_SAVE_DATA_VALUE)
#define VM_HEADER_SAVE_DATA_VALUE

#include "../obj.h"
#include "./io.h"

struct vm_save_loaded_t;
typedef struct vm_save_loaded_t vm_save_loaded_t;

struct vm_save_loaded_t {
    vm_blocks_t *blocks;
    vm_std_value_t env;
};


vm_save_loaded_t vm_load_value(vm_config_t *config, vm_save_t arg);
vm_save_t vm_save_value(vm_config_t *config, vm_blocks_t *blocks, vm_std_value_t arg);

#endif
