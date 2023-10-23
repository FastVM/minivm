
#if !defined(VM_HEADER_IR)
#define VM_HEADER_IR

#include "lib.h"
#include "tag.h"
#include "std/std.h"

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
    VM_ARG_FFI,
    VM_ARG_FUNC,
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
    // intro
    VM_IOP_TYPE,
};

struct vm_rblock_t {
    void *cache;
    vm_tags_t *regs;
    vm_block_t *block;
};

struct vm_cache_t {
    vm_rblock_t **keys;
    void **values;
    size_t len;
    size_t alloc;
};

struct vm_arg_t {
    union {
        vm_std_value_t num;
        const char *str;
        void *ffi;
        vm_block_t *func;
        vm_rblock_t *rfunc;
        vm_instr_t *instr;
        bool logic;
        vm_tag_t tag;
        struct {
            uint16_t reg;
            uint8_t reg_tag;
        };
    };
    uint8_t type;
};

struct vm_branch_t {
    union {
        vm_block_t *targets[VM_TAG_MAX];
        vm_rblock_t *rtargets[VM_TAG_MAX];
    };
    int8_t *pass[2];
    vm_arg_t args[16];
    vm_arg_t out;
    uint8_t op;
    vm_tag_t tag;
};

struct vm_instr_t {
    vm_arg_t args[6];
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

    int64_t label : 60;
    bool isfunc : 1;
    bool mark : 1;
    bool checked : 1;
    bool check : 1;
};

void vm_block_realloc(vm_block_t *block, vm_instr_t instr);

void vm_print_arg(FILE *out, vm_arg_t val);
void vm_print_tag(FILE *out, vm_tag_t tag);
void vm_print_branch(FILE *out, vm_branch_t val);
void vm_print_instr(FILE *out, vm_instr_t val);
void vm_print_block(FILE *out, vm_block_t *val);
void vm_print_blocks(FILE *out, size_t nblocks, vm_block_t *val);

void vm_block_info(size_t nblocks, vm_block_t **blocks);

#endif
