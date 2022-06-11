
#if !defined(VM_HEADER_VM)
#define VM_HEADER_VM

#include "opcode.h"

int vm_run_arch_int(size_t nops, const vm_opcode_t *ops);

#endif
