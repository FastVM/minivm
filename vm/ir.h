
#if !defined(VM_HEADER_IR)
#define VM_HEADER_IR


struct vm_arg_t;
struct vm_block_t;
struct vm_blocks_srcs_t;
struct vm_branch_t;
struct vm_instr_t;

typedef struct vm_arg_t vm_arg_t;
typedef struct vm_block_t vm_block_t;
typedef struct vm_blocks_srcs_t vm_blocks_srcs_t;
typedef struct vm_branch_t vm_branch_t;
typedef struct vm_instr_t vm_instr_t;

#include "./lib.h"
#include "./io.h"

enum {
    // there are no more args
    VM_ARG_NONE,
    // we dont know
    VM_ARG_UNK,
    // normal args
    VM_ARG_REG,
    VM_ARG_LIT,
    VM_ARG_FUN,
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

        struct {
            uint64_t reg;
            vm_tag_t reg_tag;
        };
    };

    uint8_t type;
};

struct vm_branch_t {
    vm_block_t *targets[VM_TAG_MAX];

    vm_arg_t *args;
    vm_arg_t out;

    uint8_t op;
    vm_tag_t tag;
};

struct vm_instr_t {
    vm_tag_t tag;
    vm_arg_t *args;
    uint8_t op;
    vm_arg_t out;
};

struct vm_block_t {
    size_t id;
    size_t nregs;

    size_t alloc;
    size_t len;
    vm_instr_t *instrs;

    vm_branch_t branch;

    size_t nargs;
    vm_arg_t *args;

    void *code;

    bool isfunc : 1;
};

struct vm_blocks_srcs_t {
    vm_blocks_srcs_t *last;
    const char *src;
    void *data;
};

struct vm_blocks_t {
    size_t len;
    vm_block_t **blocks;
    size_t alloc;
    vm_blocks_srcs_t *srcs;
};

void vm_block_realloc(vm_block_t *block, vm_instr_t instr);

void vm_io_format_arg(vm_io_buffer_t *out, vm_arg_t val);
void vm_io_format_type(vm_io_buffer_t *out, vm_tag_t tag);
void vm_io_format_branch(vm_io_buffer_t *out, vm_branch_t val);
void vm_io_format_instr(vm_io_buffer_t *out, vm_instr_t val);
void vm_io_format_block(vm_io_buffer_t *out, vm_block_t *val);
void vm_io_format_blocks(vm_io_buffer_t *out, vm_blocks_t *val);

void vm_block_info(size_t nblocks, vm_block_t **blocks);
vm_tag_t vm_arg_to_tag(vm_arg_t arg);

void vm_free_block_sub(vm_block_t *block);
void vm_free_block(vm_block_t *block);

#define vm_arg_nil() ((vm_arg_t){.type = (VM_ARG_LIT), .lit.tag = (VM_TAG_NIL)})

vm_block_t *vm_compile(vm_t *vm, const char *src);

#endif
