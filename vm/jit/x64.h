#if !defined(VM_HEADER_BE_X64)
#define VM_HEADER_BE_X64

#include "../ir.h"
#include "../lib.h"
#include "../obj.h"

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

struct vm_x64_regs_t;
typedef struct vm_x64_regs_t vm_x64_regs_t;

struct vm_x64_reg_save_t;
typedef struct vm_x64_reg_save_t vm_x64_reg_save_t;

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
    vm_x64_func_buf_t funcbuf;
    vm_x64_map_buf_t mapbuf;
    vm_x64_link_t *links;
    vm_table_t *std;
    uint32_t pc_alloc;
    uint32_t count;
    uint32_t push;
    uint32_t epoch;
};

struct vm_x64_link_t {
    void **out;
    vm_x64_link_t *next;
    vm_rblock_t *block;
    vm_x64_reg_save_t save;
    uint32_t label : 32;
    uint32_t epoch : 31;
    bool iscall : 1;
};

struct vm_x64_cache_t {
    vm_block_t *block;
};

struct vm_x64_regs_t {
    int8_t r64[16];
    int8_t xmm[8];
    int16_t vm[256];
};

vm_x64_cache_t *vm_x64_cache_new(void);
vm_block_t *vm_x64_rblock_version(vm_rblock_t *rblock);
void *vm_x64_full_comp(vm_x64_state_t *state, vm_block_t *block);
vm_std_value_t vm_x64_run(vm_block_t *block, vm_table_t *std, vm_std_value_t *args);

#endif