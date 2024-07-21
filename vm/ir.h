
#if !defined(VM_HEADER_IR)
#define VM_HEADER_IR

struct vm_arg_t;
struct vm_block_t;
struct vm_branch_t;
struct vm_instr_t;

typedef struct vm_arg_t vm_arg_t;
typedef struct vm_block_t vm_block_t;
typedef struct vm_branch_t vm_branch_t;
typedef struct vm_instr_t vm_instr_t;

#include "lib.h"
#include "io.h"
#include "errors.h"

enum {
    // there are no more args
    VM_ARG_NONE,
    // there was an error generating this arg
    VM_ARG_ERROR,
    // normal args
    VM_ARG_REG,
    VM_ARG_LIT,
};

enum {
    VM_BOP_FALL,
    VM_BOP_JUMP,
    VM_BOP_BB,
    VM_BOP_BLT,
    VM_BOP_BLE,
    VM_BOP_BEQ,
    VM_BOP_RET,
    VM_BOP_LOAD,
    VM_BOP_GET,
    VM_BOP_CALL,
    VM_MAX_BOP,
};

enum {
    VM_IOP_NOP,
    VM_IOP_MOVE,
    VM_IOP_ADD,
    VM_IOP_SUB,
    VM_IOP_MUL,
    VM_IOP_DIV,
    VM_IOP_IDIV,
    VM_IOP_MOD,
    VM_IOP_TABLE_SET,
    VM_IOP_TABLE_NEW,
    VM_IOP_TABLE_LEN,
    VM_IOP_STD,
    VM_MAX_IOP,
};

struct vm_arg_t {
    union {
        vm_obj_t lit;
        vm_block_t *func;
        vm_error_t *error;

        uint64_t reg;
    };

    uint8_t type;
};

struct vm_branch_t {
    vm_location_range_t range;

    vm_block_t *targets[2];

    vm_arg_t *args;
    vm_arg_t out;

    uint8_t op;
};

struct vm_instr_t {
    vm_location_range_t range;
    
    vm_arg_t *args;
    vm_arg_t out;

    uint8_t op;
};

struct vm_block_t {
    vm_location_range_t range;
    
    uint32_t id;

    uint32_t alloc;
    uint32_t len;
    vm_instr_t *instrs;

    vm_branch_t branch;

    void *code;

    bool isfunc : 1;
    bool mark: 1;
};


struct vm_blocks_t {
    vm_block_t *block;
    vm_blocks_t *next;
};

void vm_block_realloc(vm_block_t *block, vm_instr_t instr);

void vm_io_format_arg(vm_io_buffer_t *out, vm_arg_t val);
void vm_io_format_branch(vm_io_buffer_t *out, vm_branch_t val);
void vm_io_format_instr(vm_io_buffer_t *out, vm_instr_t val);
void vm_io_format_block(vm_io_buffer_t *out, vm_block_t *val);

void vm_free_block_sub(vm_block_t *block);
void vm_free_block(vm_block_t *block);

#define vm_arg_nil() ((vm_arg_t){.type = (VM_ARG_LIT), .lit = vm_obj_of_nil()})

vm_block_t *vm_compile(vm_t *vm, const char *src, const char *file);

#endif
