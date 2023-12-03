
#if !defined(VM_HEADER_CHECK)
#define VM_HEADER_CHECK
#include "ir.h"

const char *vm_check_branch(vm_branch_t block);
const char *vm_check_instr(vm_instr_t block);
const char *vm_check_block(vm_block_t *block);

#endif
