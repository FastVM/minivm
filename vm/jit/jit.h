#if !defined(VM_HEADER_BE_JIT)
#define VM_HEADER_BE_JIT

#include "../ir.h"
#include "../type.h"

typedef void func(void);

typedef struct {
    void **funcs;
    size_t alloc;
    size_t len;
} vm_jit_func_buf_t;

typedef struct {
    void *mem;
    size_t alloc;
    size_t used;
} vm_jit_mmap_t;

typedef struct {
    vm_jit_mmap_t *mmaps;
    size_t alloc;
    size_t len;
} vm_jit_map_buf_t;

typedef struct {
    void *exitptr;
    size_t count;
    vm_jit_func_buf_t funcbuf;
    vm_jit_map_buf_t mapbuf;
} vm_jit_state_t;


vm_jit_state_t *vm_jit_state_new(void);
void vm_jit_state_free(vm_jit_state_t *state);
void vm_jit_run(void *state_ptr, vm_block_t *block);

#endif
