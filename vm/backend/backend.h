
#if !defined(VM_HEADER_BACKEND_INTERP)
#define VM_HEADER_BACKEND_INTERP

#include "../vm.h"
#include "../ir.h"

vm_std_value_t vm_run_main(vm_t *config, vm_block_t *entry);
vm_std_value_t vm_run_repl(vm_t *config, vm_block_t *entry);

#endif
