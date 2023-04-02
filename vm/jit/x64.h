#if !defined(VM_HEADER_BE_X64)
#define VM_HEADER_BE_X64

#include "../ir.h"
#include "../lib.h"
#include "../type.h"

struct vm_x64_func_buf_t;
typedef struct vm_x64_func_buf_t vm_x64_func_buf_t;

struct vm_x64_mmap_t;
typedef struct vm_x64_mmap_t vm_x64_mmap_t;

struct vm_x64_map_buf_t;
typedef struct vm_x64_map_buf_t vm_x64_map_buf_t;

struct vm_x64_state_t;
typedef struct vm_x64_state_t vm_x64_state_t;

struct vm_x64_link_t;
typedef struct vm_x64_link_t vm_x64_link_t;

struct vm_x64_cache_t;
typedef struct vm_x64_cache_t vm_x64_cache_t;

struct vm_x64_func_buf_t {
    void **funcs;
    size_t alloc;
    size_t len;
};

struct vm_x64_mmap_t {
    void *mem;
    size_t alloc;
    size_t used;
};

struct vm_x64_map_buf_t {
    vm_x64_mmap_t *mmaps;
    size_t alloc;
    size_t len;
};

struct vm_x64_state_t {
    void *exitptr;
    size_t count;
    vm_x64_func_buf_t funcbuf;
    vm_x64_map_buf_t mapbuf;
    size_t push;
    vm_x64_link_t *links;
};

struct vm_x64_link_t {
    size_t label;
    void **out;
    vm_x64_link_t *next;
    vm_block_t *block;
    size_t nregs;
};

struct vm_x64_cache_t {
    vm_block_t *block;
};

vm_x64_cache_t *vm_x64_cache_new(void);
vm_block_t *vm_x64_rblock_version(vm_rblock_t *rblock);
void *vm_x64_func_comp(vm_x64_state_t *state, vm_block_t *block);
void *vm_x64_full_comp(vm_x64_state_t *state, vm_block_t *block);
void vm_x64_run(vm_block_t *block);

#endif