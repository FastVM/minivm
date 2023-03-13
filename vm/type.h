#if !defined(VM_HEADER_BE_TYPE)
#define VM_HEADER_BE_TYPE

#include "./ir.h"
#include "./tag.h"

vm_rblock_t *vm_rblock_new(vm_block_t *block, vm_tags_t *regs);
void vm_cache_new(vm_cache_t *cache);
void *vm_cache_get(vm_cache_t *cache, vm_rblock_t *rblock);
void vm_cache_set(vm_cache_t *cache, vm_rblock_t *rblock, void *value);
vm_tags_t *vm_rblock_regs_empty(size_t nregs);
vm_tags_t *vm_rblock_regs_dup(vm_tags_t *regs, size_t nregs);
bool vm_rblock_regs_match(vm_tags_t *a, vm_tags_t *b);
bool vm_rblock_type_check_instr(vm_tags_t *a, vm_instr_t instr);
bool vm_rblock_type_check_branch(vm_tags_t *a, vm_branch_t block);
vm_instr_t vm_rblock_type_specialize_instr(vm_tags_t *a, vm_instr_t instr);
vm_branch_t vm_rblock_type_specialize_branch(vm_tags_t *a, vm_branch_t block);

#endif
