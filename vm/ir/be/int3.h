#if !defined(VM_HEADER_IR_BE_INT3)
#define VM_HEADER_IR_BE_INT3

#include "../../gc.h"
#include "../../opcode.h"
#include "../ir.h"
#include "spall.h"

/// I = Int Value
/// R = Reg Local
/// T = Tmp Block
/// L = Ptr Block
/// C = Check Block

enum {
    VM_INT_OP_EXIT,

    VM_INT_OP_MOV_V,
    VM_INT_OP_MOV_B,
    VM_INT_OP_MOV_I,
    VM_INT_OP_MOV_F,
    VM_INT_OP_MOV_R,
    VM_INT_OP_MOV_T,
    VM_INT_OP_FMOV_R,

    VM_INT_OP_DYNBEQ_RRLL,
    VM_INT_OP_DYNBEQ_RRTT,

    VM_INT_OP_I32ADD_RR,
    VM_INT_OP_I32ADD_RI,
    VM_INT_OP_I32SUB_RR,
    VM_INT_OP_I32SUB_RI,
    VM_INT_OP_I32SUB_IR,
    VM_INT_OP_I32MUL_RR,
    VM_INT_OP_I32MUL_RI,
    VM_INT_OP_I32DIV_RR,
    VM_INT_OP_I32DIV_RI,
    VM_INT_OP_I32DIV_IR,
    VM_INT_OP_I32MOD_RR,
    VM_INT_OP_I32MOD_RI,
    VM_INT_OP_I32MOD_IR,
    VM_INT_OP_I32BLT_RRLL,
    VM_INT_OP_I32BLT_RILL,
    VM_INT_OP_I32BLT_IRLL,
    VM_INT_OP_I32BEQ_RRLL,
    VM_INT_OP_I32BEQ_RILL,
    VM_INT_OP_I32BEQ_IRLL,
    VM_INT_OP_I32BLT_RRTT,
    VM_INT_OP_I32BLT_RITT,
    VM_INT_OP_I32BLT_IRTT,
    VM_INT_OP_I32BEQ_RRTT,
    VM_INT_OP_I32BEQ_RITT,
    VM_INT_OP_I32BEQ_IRTT,

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
    VM_INT_OP_FBLT_RRLL,
    VM_INT_OP_FBLT_RFLL,
    VM_INT_OP_FBLT_FRLL,
    VM_INT_OP_FBEQ_RRLL,
    VM_INT_OP_FBEQ_RFLL,
    VM_INT_OP_FBEQ_FRLL,
    VM_INT_OP_FBLT_RRTT,
    VM_INT_OP_FBLT_RFTT,
    VM_INT_OP_FBLT_FRTT,
    VM_INT_OP_FBEQ_RRTT,
    VM_INT_OP_FBEQ_RFTT,
    VM_INT_OP_FBEQ_FRTT,

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

    VM_INT_OP_ARR_F,
    VM_INT_OP_ARR_R,
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

    VM_INT_OP_RET_I,
    VM_INT_OP_RET_RV,
    VM_INT_OP_RET_RB,
    VM_INT_OP_RET_RI,
    VM_INT_OP_RET_RIF,
    VM_INT_OP_RET_RF,
    VM_INT_OP_RET_RA,
    VM_INT_OP_RET_RT,

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

    VM_INT_OP_TAB,
    VM_INT_OP_TSET_RRR,
    VM_INT_OP_TSET_RRF,
    VM_INT_OP_TSET_RFR,
    VM_INT_OP_TSET_RFF,
    VM_INT_OP_TGET_RR,
    VM_INT_OP_TGET_RF,

    VM_INT_OP_DEBUG_PRINT_INSTRS,

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
    vm_gc_t gc;
    FILE *debug_print_instrs;
    vm_trace_profile_t spall_ctx;
    bool use_spall;
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
        int32_t ival;
        vm_number_t fval;
        bool bval;
    };
};

vm_value_t vm_int_run(vm_int_state_t *state, vm_ir_block_t *block);
vm_value_t vm_ir_be_int3(size_t nblocks, vm_ir_block_t *blocks, vm_int_func_t *funcs);
vm_value_t vm_run_arch_int(size_t nops, vm_opcode_t *opcodes, vm_int_func_t *funcs);

#endif
