
#if !defined(VM_HEADER_OPT_OPT)
#define VM_HEADER_OPT_OPT

#include "../ir.h"

struct vm_opt_pass_t;
typedef struct vm_opt_pass_t vm_opt_pass_t;

struct vm_opt_jump_done_t;
typedef struct vm_opt_jump_done_t vm_opt_jump_done_t;

struct vm_opt_pass_t {
    void (*pre)(vm_block_t *);
    void (*post)(vm_block_t *);
    void (*final)(size_t, vm_block_t **);
};

struct vm_opt_jump_done_t {
    size_t nblocks;
    vm_block_t **blocks;
    size_t blocks_alloc;
    vm_opt_pass_t pass;
};

void vm_opt_pass_by(vm_block_t *block, vm_opt_pass_t pass);
void vm_opt_do_pass(const char *pass, vm_block_t *block);
void vm_opt_do_passes(const char *passes, vm_block_t *block);

#define vm_opt_pass(block, ...) (vm_opt_pass_by((block), (vm_opt_pass_t) { __VA_ARGS__ }))

void vm_opt_jump(vm_block_t *block);
void vm_opt_inline(vm_block_t *block);
void vm_opt_unused(vm_block_t *block);
void vm_opt_info(vm_block_t *block);

#endif
