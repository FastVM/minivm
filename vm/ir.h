
#if !defined(VM_HEADER_IR_IR)
#define VM_HEADER_IR_IR

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
    VM_BOP_JUMP,
    VM_BOP_BOOL,
    VM_BOP_LESS,
    VM_BOP_EQUAL,
    VM_BOP_RET,
    VM_BOP_EXIT,
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
    VM_IOP_ARR,
    VM_IOP_TAB,
    VM_IOP_GET,
    VM_IOP_SET,
    VM_IOP_LEN,
    VM_IOP_TYPE,
    VM_IOP_OUT,
    VM_IOP_IN,
    VM_IOP_BOR,
    VM_IOP_BAND,
    VM_IOP_BXOR,
    VM_IOP_BSHL,
    VM_IOP_BSHR,
};

struct vm_arg_t {
#if !defined(__MINIVM__)
    union {
        size_t reg;
        vm_number_t num;
        const char *str;
        vm_block_t *func;
        vm_instr_t *instr;
        bool logic;
    };
#else
    size_t reg;
    vm_number_t num;
    const char *str;
    vm_block_t *func;
    vm_instr_t *instr;
    bool logic;
#endif
    uint8_t type;
};

struct vm_branch_t {
    vm_block_t *targets[2];
    vm_arg_t args[2];
    uint8_t op;
};

struct vm_instr_t {
    vm_arg_t *args;
    vm_arg_t out;
    uint8_t op;
};

struct vm_block_t {
    uint8_t tag;

    ptrdiff_t id;

    vm_instr_t **instrs;
    size_t len;
    size_t alloc;

    vm_branch_t *branch;

    size_t *args;
    size_t nargs;

    size_t nregs;

    void *data;

    bool isfunc : 1;
};

#endif
