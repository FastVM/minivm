
#if !defined(VM_HEADER_IR)
#define VM_HEADER_IR

#include "../lib.h"
#include "../std/std.h"
#include "tag.h"

struct vm_arg_t;
struct vm_branch_t;
struct vm_instr_t;
struct vm_block_t;
struct vm_rblock_t;
struct vm_cache_t;

typedef struct vm_rblock_t vm_rblock_t;
typedef struct vm_cache_t vm_cache_t;
typedef struct vm_arg_t vm_arg_t;
typedef struct vm_branch_t vm_branch_t;
typedef struct vm_instr_t vm_instr_t;
typedef struct vm_block_t vm_block_t;

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
    VM_BOP_BEQ,
    VM_BOP_RET,
    VM_BOP_BTYPE,
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
    VM_IOP_MOD,
    // tables
    VM_IOP_SET,
    VM_IOP_NEW,
    VM_IOP_LEN,
    // objects
    VM_IOP_STD,
};

struct vm_rblock_t {
    void *jit;
    vm_tags_t *regs;
    vm_block_t *block;
    vm_block_t *cache;
    void *state;
    size_t count;
    vm_block_t *versioned;
    size_t least_faults;
    size_t base_redo;
    size_t redo;
};

struct vm_cache_t {
    vm_rblock_t **keys;
    void **values;
    size_t len;
    size_t alloc;
};

struct vm_arg_t {
    union {
        vm_std_value_t lit;
        vm_block_t *func;
        vm_rblock_t *rfunc;
        struct {
            uint64_t reg : 56;
            uint8_t reg_tag : 8;
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
    vm_tag_t tag;
};

struct vm_instr_t {
    vm_arg_t *args;
    vm_arg_t out;
    uint8_t op;
    vm_tag_t tag;
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

    int64_t label : 61;
    bool isfunc : 1;
    bool mark : 1;
    bool checked : 1;
};

void vm_block_realloc(vm_block_t *block, vm_instr_t instr);

void vm_print_arg(FILE *out, vm_arg_t val);
void vm_print_tag(FILE *out, vm_tag_t tag);
void vm_print_branch(FILE *out, vm_branch_t val);
void vm_print_instr(FILE *out, vm_instr_t val);
void vm_print_block(FILE *out, vm_block_t *val);
void vm_print_blocks(FILE *out, size_t nblocks, vm_block_t **val);

void vm_block_info(size_t nblocks, vm_block_t **blocks);
vm_tag_t vm_arg_to_tag(vm_arg_t arg);

#define vm_arg_nil() ((vm_arg_t) { .type = (VM_ARG_LIT), .lit.tag = (VM_TAG_NIL) })

#endif
