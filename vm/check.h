
#if !defined(VM_HEADER_CHECK)
#define VM_HEADER_CHECK
#include "ir.h"

bool vm_check_branch(vm_branch_t block);
bool vm_check_instr(vm_instr_t block);
bool vm_check_block(vm_block_t *block);

#endif
