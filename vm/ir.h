
#if !defined(VM_HEADER_IR)
#define VM_HEADER_IR

#include "lib.h"
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
    VM_ARG_NIL,
    VM_ARG_BOOL,
    VM_ARG_REG,
    VM_ARG_NUM,
    VM_ARG_STR,
    VM_ARG_TAG,
    VM_ARG_FUNC,
    // for the x64 jit
    VM_ARG_X64,
};

enum {
    VM_BOP_FALL,
    VM_BOP_JUMP,
    VM_BOP_BB,
    VM_BOP_BLT,
    VM_BOP_BEQ,
    VM_BOP_RET,
    VM_BOP_EXIT,
    VM_BOP_BTYPE,
};

enum {
    VM_IOP_NOP,
    VM_IOP_MOVE,
    VM_IOP_CAST,
    VM_IOP_ADD,
    VM_IOP_SUB,
    VM_IOP_MUL,
    VM_IOP_DIV,
    VM_IOP_MOD,
    VM_IOP_CALL,
    VM_IOP_OUT,
    VM_IOP_IN,
    VM_IOP_BNOT,
    VM_IOP_BOR,
    VM_IOP_BAND,
    VM_IOP_BXOR,
    VM_IOP_BSHL,
    VM_IOP_BSHR,
    // for the x64 jit
    VM_IOP_ARGS,
};

struct vm_rblock_t {
    vm_tags_t *regs;
    vm_block_t *block;
    void *cache;
    uint32_t comps: 32;
    uint32_t start: 30;
    bool isfunc: 1;
    bool mark: 1;
};

struct vm_cache_t {
    vm_rblock_t **keys;
    void **values;
    size_t len;
    size_t alloc;
};

struct vm_arg_t {
    union {
        uint64_t reg;
        double num;
        const char *str;
        vm_block_t *func;
        vm_instr_t *instr;
        bool logic;
        vm_tag_t *tag;
        struct {
            uint16_t save;
            uint8_t x64;
            uint8_t x64from;
        };
    };
    uint8_t type;
};

struct vm_branch_t {
    vm_block_t *targets[2];
    vm_arg_t args[2];
    uint8_t op;
    vm_tag_t tag;
};

struct vm_instr_t {
    vm_arg_t args[17];
    vm_arg_t out;
    uint8_t op;
    vm_tag_t tag;
};

struct vm_block_t {
    ptrdiff_t id;

    vm_instr_t *instrs;
    size_t len;
    size_t alloc;

    vm_branch_t branch;

    vm_arg_t *args;
    size_t nargs;

    size_t nregs;

    vm_cache_t cache;
    void *impl;

    bool isfunc : 1;
    bool mark: 1;
};

void vm_instr_free(vm_instr_t *instr);
void vm_block_free(vm_block_t *block);
void vm_blocks_free(size_t nblocks, vm_block_t *blocks);

void vm_block_realloc(vm_block_t *block, vm_instr_t instr);

void vm_print_arg(FILE *out, vm_arg_t val);
void vm_print_tag(FILE *out, vm_tag_t tag);
void vm_print_branch(FILE *out, vm_branch_t val);
void vm_print_instr(FILE *out, vm_instr_t val);
void vm_print_block(FILE *out, vm_block_t *val);
void vm_print_blocks(FILE *out, size_t nblocks, vm_block_t *val);

void vm_info(size_t nops, vm_block_t **blocks);

vm_tag_t vm_instr_get_arg_type(vm_instr_t instr, size_t argno);
uint64_t vm_instr_get_arg_num(vm_instr_t instr, size_t argno);
const char *vm_instr_get_arg_str(vm_instr_t instr, size_t argno);
vm_block_t *vm_instr_get_arg_func(vm_instr_t instr, size_t argno);
size_t vm_instr_get_arg_reg(vm_instr_t instr, size_t argno);

#endif
