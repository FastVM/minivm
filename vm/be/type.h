#if !defined(VM_HEADER_BE_TYPE)
#define VM_HEADER_BE_TYPE

#include "../ir.h"
#include "value.h"

struct vm_rblock_t;
typedef struct vm_rblock_t vm_rblock_t;

struct vm_cache_t;
typedef struct vm_cache_t vm_cache_t;

struct vm_rblock_t {
    uint8_t *regs;
    vm_block_t *block;
};

struct vm_cache_t {
    uint8_t **keys;
    vm_opcode_t **values;
    size_t len;
    size_t alloc;
};

vm_opcode_t *vm_run_comp(vm_state_t *state, vm_rblock_t *block);

vm_state_t *vm_state_init(size_t nregs);
void vm_state_deinit(vm_state_t *state);

vm_rblock_t *vm_rblock_new(vm_block_t *block, uint8_t *regs);
vm_cache_t *vm_cache_new(void);
vm_opcode_t *vm_cache_get(vm_cache_t *cache, vm_rblock_t *rblock);
void vm_cache_set(vm_cache_t *cache, vm_rblock_t *rblock, vm_opcode_t *value);
uint8_t *vm_rblock_regs_empty(void);
uint8_t *vm_rblock_regs_dup(uint8_t *regs);
bool vm_rblock_regs_match(uint8_t *a, uint8_t *b);

#endif
