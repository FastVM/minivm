#if !defined(VM_HEADER_IR_BE_INT3)
#define VM_HEADER_IR_BE_INT3

#include "../../gc.h"
#include "../../opcode.h"
#include "../ir.h"

/// I = Int Value
/// R = Reg Local
/// T = Tmp Block
/// L = Ptr Block
/// C = Check Block

enum {
    VM_INT_OP_EXIT,

    VM_INT_OP_MOV_I,
    VM_INT_OP_MOV_R,
    VM_INT_OP_MOV_T,

    VM_INT_OP_ADD_RR,
    VM_INT_OP_ADD_RI,
    VM_INT_OP_SUB_RR,
    VM_INT_OP_SUB_RI,
    VM_INT_OP_SUB_IR,
    VM_INT_OP_MUL_RR,
    VM_INT_OP_MUL_RI,
    VM_INT_OP_DIV_RR,
    VM_INT_OP_DIV_RI,
    VM_INT_OP_DIV_IR,
    VM_INT_OP_MOD_RR,
    VM_INT_OP_MOD_RI,
    VM_INT_OP_MOD_IR,

    VM_INT_OP_CALL_L0,
    VM_INT_OP_CALL_L1,
    VM_INT_OP_CALL_L2,
    VM_INT_OP_CALL_L3,
    VM_INT_OP_CALL_L4,
    VM_INT_OP_CALL_L5,
    VM_INT_OP_CALL_L6,
    VM_INT_OP_CALL_L7,
    VM_INT_OP_CALL_L8,
    VM_INT_OP_CALL_R0,
    VM_INT_OP_CALL_R1,
    VM_INT_OP_CALL_R2,
    VM_INT_OP_CALL_R3,
    VM_INT_OP_CALL_R4,
    VM_INT_OP_CALL_R5,
    VM_INT_OP_CALL_R6,
    VM_INT_OP_CALL_R7,
    VM_INT_OP_CALL_R8,
    VM_INT_OP_CALL_X0,
    VM_INT_OP_CALL_X1,
    VM_INT_OP_CALL_X2,
    VM_INT_OP_CALL_X3,
    VM_INT_OP_CALL_X4,
    VM_INT_OP_CALL_X5,
    VM_INT_OP_CALL_X6,
    VM_INT_OP_CALL_X7,
    VM_INT_OP_CALL_X8,

    VM_INT_OP_NEW_I,
    VM_INT_OP_NEW_R,
    VM_INT_OP_SET_RRR,
    VM_INT_OP_SET_RRI,
    VM_INT_OP_SET_RIR,
    VM_INT_OP_SET_RII,
    VM_INT_OP_GET_RR,
    VM_INT_OP_GET_RI,
    VM_INT_OP_LEN_R,

    VM_INT_OP_OUT_I,
    VM_INT_OP_OUT_R,

    VM_INT_OP_JUMP_L,
    VM_INT_OP_BB_RLL,
    VM_INT_OP_BLT_RRLL,
    VM_INT_OP_BLT_RILL,
    VM_INT_OP_BLT_IRLL,
    VM_INT_OP_BEQ_RRLL,
    VM_INT_OP_BEQ_RILL,
    VM_INT_OP_BEQ_IRLL,

    VM_INT_OP_RET_I,
    VM_INT_OP_RET_R,

    VM_INT_OP_CALL_T0,
    VM_INT_OP_CALL_T1,
    VM_INT_OP_CALL_T2,
    VM_INT_OP_CALL_T3,
    VM_INT_OP_CALL_T4,
    VM_INT_OP_CALL_T5,
    VM_INT_OP_CALL_T6,
    VM_INT_OP_CALL_T7,
    VM_INT_OP_CALL_T8,

    VM_INT_OP_JUMP_T,
    VM_INT_OP_BB_RTT,
    VM_INT_OP_BLT_RRTT,
    VM_INT_OP_BLT_RITT,
    VM_INT_OP_BLT_IRTT,
    VM_INT_OP_BEQ_RRTT,
    VM_INT_OP_BEQ_RITT,
    VM_INT_OP_BEQ_IRTT,

    VM_INT_OP_JUMP_C,

    VM_INT_MAX_OP,
};

struct vm_int_state_t;
typedef struct vm_int_state_t vm_int_state_t;

struct vm_int_buf_t;
typedef struct vm_int_buf_t vm_int_buf_t;

struct vm_int_opcode_t;
typedef struct vm_int_opcode_t vm_int_opcode_t;

typedef vm_value_t (*vm_int_func_ptr_t)(void *ptr, vm_int_state_t *state, size_t nargs, vm_value_t *args);

typedef struct {
    void *data;
    vm_int_func_ptr_t func;
} vm_int_func_t;

struct vm_int_state_t {
    vm_int_opcode_t **heads;
    size_t framesize;
    vm_int_func_t *funcs;
    vm_value_t *locals;
};

struct vm_int_buf_t {
    size_t len;
    size_t alloc;
    vm_int_opcode_t *ops;
};

struct vm_int_opcode_t {
    union {
        void *ptr;
        vm_ir_block_t *block;
        size_t reg;
        ptrdiff_t ival;
        double fval;
    };
};

vm_value_t vm_int_run(vm_int_state_t *state, vm_ir_block_t *block);
vm_value_t vm_ir_be_int3(size_t nblocks, vm_ir_block_t *blocks, vm_int_func_t *funcs);
vm_value_t vm_run_arch_int(size_t nops, vm_opcode_t *opcodes, vm_int_func_t *funcs);

#endif
