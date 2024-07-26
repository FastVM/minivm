
#if !defined(VM_HEADER_BACKEND_INTERP)
#define VM_HEADER_BACKEND_INTERP

#include "../vm.h"
#include "../ir.h"

vm_obj_t vm_run_main(vm_t *vm, vm_ir_block_t *entry);
vm_obj_t vm_run_repl(vm_t *vm, vm_ir_block_t *entry);

#endif
