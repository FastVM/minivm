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
    VM_INT_OP_MOV_F,
    VM_INT_OP_MOV_R,
    VM_INT_OP_MOV_T,

    VM_INT_OP_FLOAT,

    VM_INT_OP_IADD_RR,
    VM_INT_OP_IADD_RI,
    VM_INT_OP_ISUB_RR,
    VM_INT_OP_ISUB_RI,
    VM_INT_OP_ISUB_IR,
    VM_INT_OP_IMUL_RR,
    VM_INT_OP_IMUL_RI,
    VM_INT_OP_IMOD_RR,
    VM_INT_OP_IMOD_RI,
    VM_INT_OP_IMOD_IR,

    VM_INT_OP_FADD_RR,
    VM_INT_OP_FADD_RF,
    VM_INT_OP_FSUB_RR,
    VM_INT_OP_FSUB_RF,
    VM_INT_OP_FSUB_FR,
    VM_INT_OP_FMUL_RR,
    VM_INT_OP_FMUL_RF,
    VM_INT_OP_FDIV_RR,
    VM_INT_OP_FDIV_RF,
    VM_INT_OP_FDIV_FR,
    VM_INT_OP_FMOD_RR,
    VM_INT_OP_FMOD_RF,
    VM_INT_OP_FMOD_FR,

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
    VM_INT_OP_CALL_C0,
    VM_INT_OP_CALL_C1,
    VM_INT_OP_CALL_C2,
    VM_INT_OP_CALL_C3,
    VM_INT_OP_CALL_C4,
    VM_INT_OP_CALL_C5,
    VM_INT_OP_CALL_C6,
    VM_INT_OP_CALL_C7,

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
    VM_INT_OP_IBB_RLL,
    VM_INT_OP_IBLT_RRLL,
    VM_INT_OP_IBLT_RILL,
    VM_INT_OP_IBLT_IRLL,
    VM_INT_OP_IBEQ_RRLL,
    VM_INT_OP_IBEQ_RILL,
    VM_INT_OP_IBEQ_IRLL,
    VM_INT_OP_FBB_RLL,
    VM_INT_OP_FBLT_RRLL,
    VM_INT_OP_FBLT_RILL,
    VM_INT_OP_FBLT_IRLL,
    VM_INT_OP_FBEQ_RRLL,
    VM_INT_OP_FBEQ_RILL,
    VM_INT_OP_FBEQ_IRLL,

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
    VM_INT_OP_IBB_RTT,
    VM_INT_OP_IBLT_RRTT,
    VM_INT_OP_IBLT_RITT,
    VM_INT_OP_IBLT_IRTT,
    VM_INT_OP_IBEQ_RRTT,
    VM_INT_OP_IBEQ_RITT,
    VM_INT_OP_IBEQ_IRTT,
    VM_INT_OP_FBB_RTT,
    VM_INT_OP_FBLT_RRTT,
    VM_INT_OP_FBLT_RITT,
    VM_INT_OP_FBLT_IRTT,
    VM_INT_OP_FBEQ_RRTT,
    VM_INT_OP_FBEQ_RITT,
    VM_INT_OP_FBEQ_IRTT,

    VM_INT_OP_BTY_T,
    VM_INT_OP_BTY_L,

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
        vm_number_t fval;
    };
};

vm_value_t vm_int_run(vm_int_state_t *state, vm_ir_block_t *block);
vm_value_t vm_ir_be_int3(size_t nblocks, vm_ir_block_t *blocks, vm_int_func_t *funcs);
vm_value_t vm_run_arch_int(size_t nops, vm_opcode_t *opcodes, vm_int_func_t *funcs);

#endif
