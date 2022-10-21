#if !defined(VM_HEADER_BE_TYPE)
#define VM_HEADER_BE_TYPE

#include "../ir.h"
#include "../tag.h"
#include "./value.h"

vm_opcode_t *vm_run_comp(vm_state_t *state, vm_rblock_t *block);

vm_state_t *vm_state_init(size_t nregs);
void vm_state_deinit(vm_state_t *state);

vm_rblock_t *vm_rblock_new(vm_block_t *block, uint8_t *regs);
void vm_cache_new(vm_cache_t *cache);
vm_opcode_t *vm_cache_get(vm_cache_t *cache, vm_rblock_t *rblock);
void vm_cache_set(vm_cache_t *cache, vm_rblock_t *rblock, vm_opcode_t *value);
uint8_t *vm_rblock_regs_empty(void);
uint8_t *vm_rblock_regs_dup(uint8_t *regs);
bool vm_rblock_regs_match(uint8_t *a, uint8_t *b);
vm_instr_t vm_rblock_type_specialize_instr(uint8_t *a, vm_instr_t instr);
vm_branch_t vm_rblock_type_specialize_branch(uint8_t *a, vm_branch_t block);

#endif
