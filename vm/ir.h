
#if !defined(VM_HEADER_IR)
#define VM_HEADER_IR

#include "lib.h"

struct vm_arg_t;
struct vm_branch_t;
struct vm_instr_t;
struct vm_block_t;

typedef struct vm_arg_t vm_arg_t;
typedef struct vm_branch_t vm_branch_t;
typedef struct vm_instr_t vm_instr_t;
typedef struct vm_block_t vm_block_t;

enum {
    // internal error if found
    VM_ARG_INIT,
    // we dont know
    VM_ARG_UNK,
    // there are no more args
    VM_ARG_NONE,
    VM_ARG_NIL,
    VM_ARG_BOOL,
    VM_ARG_REG,
    VM_ARG_NUM,
    VM_ARG_STR,
    VM_ARG_EXTERN,
    VM_ARG_FUNC,
};

enum {
    VM_BOP_FALL,
    VM_BOP_JUMP,
    VM_BOP_BB,
    VM_BOP_BLT,
    VM_BOP_BEQ,
    VM_BOP_RET,
    VM_BOP_EXIT,
    VM_BOP_MAX,
};

enum {
    VM_IOP_NOP,
    VM_IOP_MOVE,
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
    VM_IOP_MAX,
};

struct vm_arg_t {
    union {
        size_t reg;
        double num;
        const char *str;
        vm_block_t *func;
        vm_instr_t *instr;
        bool logic;
    };
    uint8_t type;
};

struct vm_branch_t {
    vm_block_t *targets[2];
    vm_arg_t args[2];
    uint8_t op;
    uint8_t tag;
};

struct vm_instr_t {
    vm_arg_t args[11];
    vm_arg_t out;
    uint8_t op;
    uint8_t tag;
};

struct vm_block_t {
    ptrdiff_t id;

    vm_instr_t *instrs;
    size_t len;
    size_t alloc;

    vm_branch_t branch;

    size_t *args;
    size_t nargs;

    size_t nregs;

    void *cache;

    bool isfunc : 1;
};

void vm_instr_free(vm_instr_t *instr);
void vm_block_free(vm_block_t *block);
void vm_blocks_free(size_t nblocks, vm_block_t *blocks);

void vm_block_realloc(vm_block_t *block, vm_instr_t instr);

void vm_print_arg(FILE *out, vm_arg_t val);
void vm_print_branch(FILE *out, vm_branch_t val);
void vm_print_instr(FILE *out, vm_instr_t val);
void vm_print_block(FILE *out, vm_block_t *val);
void vm_print_blocks(FILE *out, size_t nblocks, vm_block_t *val);

void vm_info(size_t nops, vm_block_t **blocks);

#endif
