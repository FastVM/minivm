
#if !defined(VM_HEADER_IR_IR)
#define VM_HEADER_IR_IR

#include "../lib.h"

struct vm_ir_arg_t;
struct vm_ir_branch_t;
struct vm_ir_instr_t;
struct vm_ir_block_t;
struct vm_ir_func_t;

typedef struct vm_ir_arg_t vm_ir_arg_t;
typedef struct vm_ir_branch_t vm_ir_branch_t;
typedef struct vm_ir_instr_t vm_ir_instr_t;
typedef struct vm_ir_block_t vm_ir_block_t;
typedef struct vm_ir_func_t vm_ir_func_t;

enum
{
    VM_IR_ARG_REG,
    VM_IR_ARG_NUM,
    VM_IR_ARG_FUNC,
};

enum
{
    VM_IR_BOP_GOTO,
    VM_IR_BOP_BOOL,
    VM_IR_BOP_LESS,
    VM_IR_BOP_EQUAL,
    VM_IR_BOP_RET,
    VM_IR_BOP_EXIT,
};

enum
{
    VM_IR_IOP_MOVE,
    VM_IR_IOP_ADD,
    VM_IR_IOP_SUB,
    VM_IR_IOP_MUL,
    VM_IR_IOP_DIV,
    VM_IR_IOP_MOD,
    VM_IR_IOP_ADDR,
    VM_IR_IOP_CALL,
    VM_IR_IOP_ARR,
    VM_IR_IOP_GET,
    VM_IR_IOP_SET,
    VM_IR_IOP_LEN,
    VM_IR_IOP_TYPE,
};

struct vm_ir_arg_t
{
    union
    {
        size_t reg;
        size_t num;
        vm_ir_func_t *func;
    };
    uint8_t type;
};

struct vm_ir_branch_t
{
    int32_t targets[2];
    vm_ir_arg_t *args[2];
    uint8_t op;
};

struct vm_ir_instr_t
{
    vm_ir_arg_t *out;
    vm_ir_arg_t *args[16];
    uint8_t op;
};

struct vm_ir_block_t
{
    vm_ir_instr_t **instrs;
    size_t len;
    size_t alloc;

    vm_ir_branch_t *branch;
  
    vm_ir_func_t *func;
};

struct vm_ir_func_t
{
    vm_ir_block_t *blocks;
    size_t len;
    size_t alloc;
};

#endif
