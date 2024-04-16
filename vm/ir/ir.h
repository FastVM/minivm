
#if !defined(VM_HEADER_IR)
#define VM_HEADER_IR

struct vm_arg_t;
struct vm_branch_t;
struct vm_instr_t;
struct vm_block_t;
struct vm_rblock_t;
struct vm_cache_t;
struct vm_blocks_t;

typedef struct vm_rblock_t vm_rblock_t;
typedef struct vm_cache_t vm_cache_t;
typedef struct vm_arg_t vm_arg_t;
typedef struct vm_branch_t vm_branch_t;
typedef struct vm_instr_t vm_instr_t;
typedef struct vm_block_t vm_block_t;
typedef struct vm_blocks_t vm_blocks_t;

#include "../lib.h"
#include "../obj.h"
#include "../std/io.h"
#include "../std/std.h"
#include "tag.h"

enum {
    // there are no more args
    VM_ARG_NONE,
    // we dont know
    VM_ARG_UNK,
    // normal args
    VM_ARG_REG,
    VM_ARG_LIT,
    VM_ARG_FUN,
    // for backend use
    VM_ARG_TAG,
    VM_ARG_RFUNC,
};

enum {
    VM_BOP_FALL,
    VM_BOP_JUMP,
    VM_BOP_BB,
    VM_BOP_BLT,
    VM_BOP_BLE,
    VM_BOP_BEQ,
    VM_BOP_RET,
    VM_BOP_BTYPE,
    VM_BOP_LOAD,
    VM_BOP_GET,
    VM_BOP_CALL,
};

enum {
    VM_IOP_NOP,
    // util
    VM_IOP_MOVE,
    // math
    VM_IOP_ADD,
    VM_IOP_SUB,
    VM_IOP_MUL,
    VM_IOP_DIV,
    VM_IOP_IDIV,
    VM_IOP_MOD,
    // tables
    VM_IOP_TABLE_SET,
    VM_IOP_TABLE_NEW,
    VM_IOP_TABLE_LEN,
    // closures
    VM_IOP_CLOSURE_NEW,
    VM_IOP_CLOSURE_SET,
    // objects
    VM_IOP_STD,
};

struct vm_rblock_t {
    void *code;
    void *jit;
    vm_types_t *regs;
    vm_block_t *block;
    vm_block_t *cache;
    void *state;
    void (*del)(vm_rblock_t *self);
};

struct vm_cache_t {
    vm_rblock_t **keys;
    vm_block_t **values;
    size_t len;
    size_t alloc;
};

struct vm_arg_t {
    union {
        vm_std_value_t lit;
        vm_block_t *func;
        vm_rblock_t *rfunc;

        struct {
            uint64_t reg;
            vm_type_t reg_tag;
        };
    };

    uint8_t type;
};

struct vm_branch_t {
    union {
        vm_block_t *targets[VM_TAG_MAX];
        vm_rblock_t *rtargets[VM_TAG_MAX];
    };

    vm_arg_t *args;
    vm_arg_t out;

    struct {
        vm_rblock_t **call_table;
        void **jump_table;
    };

    uint8_t op;
    vm_type_t tag;
};

struct vm_instr_t {
    vm_arg_t *args;
    vm_arg_t out;
    uint8_t op;
    vm_type_t tag;
};

struct vm_block_t {
    ptrdiff_t id;

    size_t alloc;
    vm_instr_t *instrs;
    size_t len;

    vm_branch_t branch;

    size_t nargs;
    vm_arg_t *args;

    size_t nregs;

    vm_cache_t cache;
    void *pass;
    const char *check;

    uint8_t uses[VM_TAG_MAX];

    int64_t label : 61;
    bool isfunc : 1;
    bool mark : 1;
    bool checked : 1;
};

struct vm_blocks_t {
    size_t len;
    vm_block_t **blocks;
    size_t alloc;
    vm_block_t *entry;
};

void vm_block_realloc(vm_block_t *block, vm_instr_t instr);

void vm_io_format_arg(vm_io_buffer_t *out, vm_arg_t val);
void vm_io_format_type(vm_io_buffer_t *out, vm_type_t tag);
void vm_io_format_branch(vm_io_buffer_t *out, vm_branch_t val);
void vm_io_format_instr(vm_io_buffer_t *out, vm_instr_t val);
void vm_io_format_block(vm_io_buffer_t *out, vm_block_t *val);
void vm_io_format_blocks(vm_io_buffer_t *out, vm_blocks_t *val);

void vm_block_info(size_t nblocks, vm_block_t **blocks);
vm_type_t vm_arg_to_tag(vm_arg_t arg);

void vm_free_block_sub(vm_block_t *block);
void vm_free_block(vm_block_t *block);

#define vm_arg_nil() ((vm_arg_t){.type = (VM_ARG_LIT), .lit.tag = (VM_TYPE_NIL)})

#endif
