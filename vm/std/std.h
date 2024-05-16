#if !defined(VM_HEADER_STD_STD)
#define VM_HEADER_STD_STD

#include "../obj.h"
#include "./io.h"

#define VM_STD_REF(cfg, x) (vm_config_add_extern(cfg, &x), &x)

void vm_value_buffer_tostring(vm_io_buffer_t *buf, vm_std_value_t value);
void vm_std_set_arg(vm_config_t *config, vm_table_t *std, const char *prog, const char *file, int argc, char **argv);
vm_table_t *vm_std_new(vm_config_t *config);

#endif
