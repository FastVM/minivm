
#if !defined(VM_HEADER_VM)
#define VM_HEADER_VM

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../vendor/nanbox/nanbox.h"

#define VM_VERSION "0.0.5"

#define VM_GC_MIN 16
#define VM_GC_FACTOR 1.7
#define VM_GC_STATS 0

#define VM_DEBUG_BACKEND_BLOCKS 0
#define VM_DEBUG_BACKEND_OPCODES 0

#define VM_FORMAT_FLOAT "%.14g"

#define VM_USE_SPALL 0
#define VM_USE_SPALL_INSTR 0

#define VM_EMPTY_BYTE NANBOX_EMPTY_BYTE

struct vm_t;
struct vm_error_t;
struct vm_ir_block_t;
struct vm_ir_blocks_t;
struct vm_obj_closure_t;
struct vm_obj_gc_header_t;
struct vm_obj_table_t;
struct vm_table_pair_t;
struct vm_table_pair_t;
struct vm_io_buffer_t;

typedef struct vm_io_buffer_t vm_io_buffer_t;
typedef struct vm_t vm_t;
typedef struct vm_error_t vm_error_t;
typedef struct vm_ir_block_t vm_ir_block_t;
typedef struct vm_ir_blocks_t vm_ir_blocks_t;
typedef struct vm_obj_closure_t vm_obj_closure_t;
typedef struct vm_obj_table_t vm_obj_table_t;
typedef struct vm_table_pair_t vm_table_pair_t;

typedef nanbox_t vm_obj_t;
typedef void vm_ffi_t(vm_t *vm, size_t nargs, vm_obj_t *args);

struct vm_table_pair_t {
    vm_obj_t key;
    vm_obj_t value;
};

struct vm_obj_table_t {
    vm_table_pair_t *restrict pairs;
    uint32_t used: 28;
    uint32_t len: 28;
    uint8_t size: 5;
    uint8_t mark: 1;
};

struct vm_obj_closure_t {
    struct vm_ir_block_t *block;
    uint32_t len: 31;
    uint8_t mark: 1;
    vm_obj_t values[];
};

struct vm_io_buffer_t {
    uint32_t alloc;
    uint32_t len;
    char *buf;
    uint32_t hash;
    uint8_t mark: 1;
};

struct vm_t {
    vm_ir_blocks_t *blocks;

    vm_obj_t std;

    void *restrict gc;

    vm_obj_t *base;
    vm_obj_t *regs;

    uint32_t nblocks;
};

vm_t *vm_state_new(void);
void vm_state_delete(vm_t *vm);

#include "obj.inc"

#endif
