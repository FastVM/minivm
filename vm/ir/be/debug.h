
#if !defined(VM_HEADER_IR_BE_DEBUG)
#define VM_HEADER_IR_BE_DEBUG

#include "int3.h"

static inline const char *vm_int_debug_instr_name(size_t op) {
    switch (op) {
        case VM_INT_OP_EXIT:
            return "exit";
        case VM_INT_OP_MOV_V:
            return "mov";
        case VM_INT_OP_MOV_B:
            return "mov";
        case VM_INT_OP_MOV_I:
            return "mov";
        case VM_INT_OP_MOV_F:
            return "mov";
        case VM_INT_OP_MOV_R:
            return "mov";
        case VM_INT_OP_MOV_T:
            return "mov";
        case VM_INT_OP_FMOV_R:
            return "fmov";
        case VM_INT_OP_I32ADD_RR:
            return "i32add";
        case VM_INT_OP_I32ADD_RI:
            return "i32add";
        case VM_INT_OP_I32SUB_RR:
            return "i32sub";
        case VM_INT_OP_I32SUB_RI:
            return "i32sub";
        case VM_INT_OP_I32SUB_IR:
            return "i32sub";
        case VM_INT_OP_I32MUL_RR:
            return "i32mul";
        case VM_INT_OP_I32MUL_RI:
            return "i32mul";
        case VM_INT_OP_I32DIV_RR:
            return "i32div";
        case VM_INT_OP_I32DIV_RI:
            return "i32div";
        case VM_INT_OP_I32DIV_IR:
            return "i32div";
        case VM_INT_OP_I32MOD_RR:
            return "i32mod";
        case VM_INT_OP_I32MOD_RI:
            return "i32mod";
        case VM_INT_OP_I32MOD_IR:
            return "i32mod";
        case VM_INT_OP_I32BLT_RRLL:
            return "i32blt";
        case VM_INT_OP_I32BLT_RILL:
            return "i32blt";
        case VM_INT_OP_I32BLT_IRLL:
            return "i32blt";
        case VM_INT_OP_I32BEQ_RRLL:
            return "i32beq";
        case VM_INT_OP_I32BEQ_RILL:
            return "i32beq";
        case VM_INT_OP_I32BEQ_IRLL:
            return "i32beq";
        case VM_INT_OP_I32BLT_RRTT:
            return "i32blt";
        case VM_INT_OP_I32BLT_RITT:
            return "i32blt";
        case VM_INT_OP_I32BLT_IRTT:
            return "i32blt";
        case VM_INT_OP_I32BEQ_RRTT:
            return "i32beq";
        case VM_INT_OP_I32BEQ_RITT:
            return "i32beq";
        case VM_INT_OP_I32BEQ_IRTT:
            return "i32beq";
        case VM_INT_OP_FADD_RR:
            return "fadd";
        case VM_INT_OP_FADD_RF:
            return "fadd";
        case VM_INT_OP_FSUB_RR:
            return "fsub";
        case VM_INT_OP_FSUB_RF:
            return "fsub";
        case VM_INT_OP_FSUB_FR:
            return "fsub";
        case VM_INT_OP_FMUL_RR:
            return "fmul";
        case VM_INT_OP_FMUL_RF:
            return "fmul";
        case VM_INT_OP_FDIV_RR:
            return "fdiv";
        case VM_INT_OP_FDIV_RF:
            return "fdiv";
        case VM_INT_OP_FDIV_FR:
            return "fdiv";
        case VM_INT_OP_FMOD_RR:
            return "fmod";
        case VM_INT_OP_FMOD_RF:
            return "fmod";
        case VM_INT_OP_FMOD_FR:
            return "fmod";
        case VM_INT_OP_FBLT_RRLL:
            return "fblt";
        case VM_INT_OP_FBLT_RFLL:
            return "fblt";
        case VM_INT_OP_FBLT_FRLL:
            return "fblt";
        case VM_INT_OP_FBEQ_RRLL:
            return "fbeq";
        case VM_INT_OP_FBEQ_RFLL:
            return "fbeq";
        case VM_INT_OP_FBEQ_FRLL:
            return "fbeq";
        case VM_INT_OP_FBLT_RRTT:
            return "fblt";
        case VM_INT_OP_FBLT_RFTT:
            return "fblt";
        case VM_INT_OP_FBLT_FRTT:
            return "fblt";
        case VM_INT_OP_FBEQ_RRTT:
            return "fbeq";
        case VM_INT_OP_FBEQ_RFTT:
            return "fbeq";
        case VM_INT_OP_FBEQ_FRTT:
            return "fbeq";
        case VM_INT_OP_CALL_L0:
            return "call";
        case VM_INT_OP_CALL_L1:
            return "call";
        case VM_INT_OP_CALL_L2:
            return "call";
        case VM_INT_OP_CALL_L3:
            return "call";
        case VM_INT_OP_CALL_L4:
            return "call";
        case VM_INT_OP_CALL_L5:
            return "call";
        case VM_INT_OP_CALL_L6:
            return "call";
        case VM_INT_OP_CALL_L7:
            return "call";
        case VM_INT_OP_CALL_L8:
            return "call";
        case VM_INT_OP_CALL_R0:
            return "call";
        case VM_INT_OP_CALL_R1:
            return "call";
        case VM_INT_OP_CALL_R2:
            return "call";
        case VM_INT_OP_CALL_R3:
            return "call";
        case VM_INT_OP_CALL_R4:
            return "call";
        case VM_INT_OP_CALL_R5:
            return "call";
        case VM_INT_OP_CALL_R6:
            return "call";
        case VM_INT_OP_CALL_R7:
            return "call";
        case VM_INT_OP_CALL_R8:
            return "call";
        case VM_INT_OP_CALL_X0:
            return "call";
        case VM_INT_OP_CALL_X1:
            return "call";
        case VM_INT_OP_CALL_X2:
            return "call";
        case VM_INT_OP_CALL_X3:
            return "call";
        case VM_INT_OP_CALL_X4:
            return "call";
        case VM_INT_OP_CALL_X5:
            return "call";
        case VM_INT_OP_CALL_X6:
            return "call";
        case VM_INT_OP_CALL_X7:
            return "call";
        case VM_INT_OP_CALL_X8:
            return "call";
        case VM_INT_OP_CALL_C0:
            return "call";
        case VM_INT_OP_CALL_C1:
            return "call";
        case VM_INT_OP_CALL_C2:
            return "call";
        case VM_INT_OP_CALL_C3:
            return "call";
        case VM_INT_OP_CALL_C4:
            return "call";
        case VM_INT_OP_CALL_C5:
            return "call";
        case VM_INT_OP_CALL_C6:
            return "call";
        case VM_INT_OP_CALL_C7:
            return "call";
        case VM_INT_OP_ARR_F:
            return "arr";
        case VM_INT_OP_ARR_R:
            return "arr";
        case VM_INT_OP_SET_RRR:
            return "set";
        case VM_INT_OP_SET_RRI:
            return "set";
        case VM_INT_OP_SET_RIR:
            return "set";
        case VM_INT_OP_SET_RII:
            return "set";
        case VM_INT_OP_GET_RR:
            return "get";
        case VM_INT_OP_GET_RI:
            return "get";
        case VM_INT_OP_LEN_R:
            return "len";
        case VM_INT_OP_OUT_I:
            return "out";
        case VM_INT_OP_OUT_R:
            return "out";
        case VM_INT_OP_JUMP_L:
            return "jump";
        case VM_INT_OP_BB_RLL:
            return "bb";
        case VM_INT_OP_RET_I:
            return "ret";
        case VM_INT_OP_RET_RV:
            return "ret";
        case VM_INT_OP_RET_RB:
            return "ret";
        case VM_INT_OP_RET_RI:
            return "ret";
        case VM_INT_OP_RET_RIF:
            return "ret";
        case VM_INT_OP_RET_RF:
            return "ret";
        case VM_INT_OP_RET_RA:
            return "ret";
        case VM_INT_OP_RET_RT:
            return "ret";
        case VM_INT_OP_CALL_T0:
            return "call";
        case VM_INT_OP_CALL_T1:
            return "call";
        case VM_INT_OP_CALL_T2:
            return "call";
        case VM_INT_OP_CALL_T3:
            return "call";
        case VM_INT_OP_CALL_T4:
            return "call";
        case VM_INT_OP_CALL_T5:
            return "call";
        case VM_INT_OP_CALL_T6:
            return "call";
        case VM_INT_OP_CALL_T7:
            return "call";
        case VM_INT_OP_CALL_T8:
            return "call";
        case VM_INT_OP_JUMP_T:
            return "jump";
        case VM_INT_OP_BB_RTT:
            return "bb";
        case VM_INT_OP_TAB:
            return "tab";
        case VM_INT_OP_TSET_RRR:
            return "tset";
        case VM_INT_OP_TSET_RRF:
            return "tset";
        case VM_INT_OP_TSET_RFR:
            return "tset";
        case VM_INT_OP_TSET_RFF:
            return "tset";
        case VM_INT_OP_TGET_RR:
            return "tget";
        case VM_INT_OP_TGET_RF:
            return "tget";
        default:
            return "<invalid>";
    }
}

static inline const char *vm_int_debug_instr_format(size_t opcode) {
    static const char *table[] = {
        [VM_INT_OP_EXIT] = "?",
        [VM_INT_OP_MOV_V] = ":N",
        [VM_INT_OP_MOV_B] = ":B",
        [VM_INT_OP_MOV_I] = ":I",
        [VM_INT_OP_MOV_F] = ":F",
        [VM_INT_OP_MOV_R] = ":t",
        [VM_INT_OP_MOV_T] = ":T",
        [VM_INT_OP_FMOV_R] = ":F",
        [VM_INT_OP_I32ADD_RR] = ":ii",
        [VM_INT_OP_I32ADD_RI] = ":iI",
        [VM_INT_OP_I32SUB_RR] = ":ii",
        [VM_INT_OP_I32SUB_RI] = ":iI",
        [VM_INT_OP_I32SUB_IR] = ":Ii",
        [VM_INT_OP_I32MUL_RR] = ":ii",
        [VM_INT_OP_I32MUL_RI] = ":iI",
        [VM_INT_OP_I32DIV_RR] = ":ii",
        [VM_INT_OP_I32DIV_RI] = ":iI",
        [VM_INT_OP_I32DIV_IR] = ":Ii",
        [VM_INT_OP_I32MOD_RR] = ":ii",
        [VM_INT_OP_I32MOD_RI] = ":iI",
        [VM_INT_OP_I32MOD_IR] = ":Ii",
        [VM_INT_OP_I32BLT_RRLL] = "?iiLL",
        [VM_INT_OP_I32BLT_RILL] = "?iILL",
        [VM_INT_OP_I32BLT_IRLL] = "?IiLL",
        [VM_INT_OP_I32BEQ_RRLL] = "?iiLL",
        [VM_INT_OP_I32BEQ_RILL] = "?iILL",
        [VM_INT_OP_I32BEQ_IRLL] = "?IiLL",
        [VM_INT_OP_I32BLT_RRTT] = "?iiLL",
        [VM_INT_OP_I32BLT_RITT] = "?iILL",
        [VM_INT_OP_I32BLT_IRTT] = "?IiLL",
        [VM_INT_OP_I32BEQ_RRTT] = "?iiLL",
        [VM_INT_OP_I32BEQ_RITT] = "?iILL",
        [VM_INT_OP_I32BEQ_IRTT] = "?IiLL",
        [VM_INT_OP_FADD_RR] = ":ff",
        [VM_INT_OP_FADD_RF] = ":fF",
        [VM_INT_OP_FSUB_RR] = ":ff",
        [VM_INT_OP_FSUB_RF] = ":fF",
        [VM_INT_OP_FSUB_FR] = ":Ff",
        [VM_INT_OP_FMUL_RR] = ":ff",
        [VM_INT_OP_FMUL_RF] = ":fF",
        [VM_INT_OP_FDIV_RR] = ":ff",
        [VM_INT_OP_FDIV_RF] = ":fF",
        [VM_INT_OP_FDIV_FR] = ":Ff",
        [VM_INT_OP_FMOD_RR] = ":ff",
        [VM_INT_OP_FMOD_RF] = ":fF",
        [VM_INT_OP_FMOD_FR] = ":Ff",
        [VM_INT_OP_FBLT_RRLL] = "?ffLL",
        [VM_INT_OP_FBLT_RFLL] = "?fFLL",
        [VM_INT_OP_FBLT_FRLL] = "?FfLL",
        [VM_INT_OP_FBEQ_RRLL] = "?ffLL",
        [VM_INT_OP_FBEQ_RFLL] = "?fFLL",
        [VM_INT_OP_FBEQ_FRLL] = "?FfLL",
        [VM_INT_OP_FBLT_RRTT] = "?ffLL",
        [VM_INT_OP_FBLT_RFTT] = "?fFLL",
        [VM_INT_OP_FBLT_FRTT] = "?FfLL",
        [VM_INT_OP_FBEQ_RRTT] = "?ffLL",
        [VM_INT_OP_FBEQ_RFTT] = "?fIFLL",
        [VM_INT_OP_FBEQ_FRTT] = "?FfLL",
        [VM_INT_OP_CALL_L0] = ":L",
        [VM_INT_OP_CALL_L1] = ":Ld",
        [VM_INT_OP_CALL_L2] = ":Ldd",
        [VM_INT_OP_CALL_L3] = ":Lddd",
        [VM_INT_OP_CALL_L4] = ":Ldddd",
        [VM_INT_OP_CALL_L5] = ":Lddddd",
        [VM_INT_OP_CALL_L6] = ":Ldddddd",
        [VM_INT_OP_CALL_L7] = ":Lddddddd",
        [VM_INT_OP_CALL_L8] = ":Ldddddddd",
        [VM_INT_OP_CALL_R0] = ":t",
        [VM_INT_OP_CALL_R1] = ":td",
        [VM_INT_OP_CALL_R2] = ":tdd",
        [VM_INT_OP_CALL_R3] = ":tddd",
        [VM_INT_OP_CALL_R4] = ":tdddd",
        [VM_INT_OP_CALL_R5] = ":tddddd",
        [VM_INT_OP_CALL_R6] = ":tdddddd",
        [VM_INT_OP_CALL_R7] = ":tddddddd",
        [VM_INT_OP_CALL_R8] = ":tdddddddd",
        [VM_INT_OP_CALL_X0] = ":X",
        [VM_INT_OP_CALL_X1] = ":Xd",
        [VM_INT_OP_CALL_X2] = ":Xdd",
        [VM_INT_OP_CALL_X3] = ":Xddd",
        [VM_INT_OP_CALL_X4] = ":Xdddd",
        [VM_INT_OP_CALL_X5] = ":Xddddd",
        [VM_INT_OP_CALL_X6] = ":Xdddddd",
        [VM_INT_OP_CALL_X7] = ":Xddddddd",
        [VM_INT_OP_CALL_X8] = ":Xdddddddd",
        [VM_INT_OP_CALL_C0] = ":c",
        [VM_INT_OP_CALL_C1] = ":cd",
        [VM_INT_OP_CALL_C2] = ":cdd",
        [VM_INT_OP_CALL_C3] = ":cddd",
        [VM_INT_OP_CALL_C4] = ":cdddd",
        [VM_INT_OP_CALL_C5] = ":cddddd",
        [VM_INT_OP_CALL_C6] = ":cdddddd",
        [VM_INT_OP_CALL_C7] = ":cddddddd",
        [VM_INT_OP_ARR_F] = ":F",
        [VM_INT_OP_ARR_R] = ":f",
        [VM_INT_OP_SET_RRR] = "aii",
        [VM_INT_OP_SET_RRI] = "aiI",
        [VM_INT_OP_SET_RIR] = "aIi",
        [VM_INT_OP_SET_RII] = "aII",
        [VM_INT_OP_GET_RR] = ":ai",
        [VM_INT_OP_GET_RI] = ":aI",
        [VM_INT_OP_LEN_R] = ":a",
        [VM_INT_OP_OUT_I] = ".I",
        [VM_INT_OP_OUT_R] = ".i",
        [VM_INT_OP_JUMP_L] = "?L",
        [VM_INT_OP_BB_RLL] = "?bLL",
        [VM_INT_OP_RET_I] = "?I",
        [VM_INT_OP_RET_RV] = "?N",
        [VM_INT_OP_RET_RB] = "?b",
        [VM_INT_OP_RET_RI] = "?i",
        [VM_INT_OP_RET_RIF] = "?f",
        [VM_INT_OP_RET_RF] = "?l",
        [VM_INT_OP_RET_RA] = "?a",
        [VM_INT_OP_RET_RT] = "?t",
        [VM_INT_OP_CALL_T0] = ":T",
        [VM_INT_OP_CALL_T1] = ":Td",
        [VM_INT_OP_CALL_T2] = ":Tdd",
        [VM_INT_OP_CALL_T3] = ":Tddd",
        [VM_INT_OP_CALL_T4] = ":Tdddd",
        [VM_INT_OP_CALL_T5] = ":Tddddd",
        [VM_INT_OP_CALL_T6] = ":Tdddddd",
        [VM_INT_OP_CALL_T7] = ":Tddddddd",
        [VM_INT_OP_CALL_T8] = ":Tdddddddd",
        [VM_INT_OP_JUMP_T] = "?T",
        [VM_INT_OP_BB_RTT] = "?bTT",
        [VM_INT_OP_TAB] = ":",
        [VM_INT_OP_TSET_RRR] = "odd",
        [VM_INT_OP_TSET_RRF] = "odF",
        [VM_INT_OP_TSET_RFR] = "oFd",
        [VM_INT_OP_TSET_RFF] = "oFF",
        [VM_INT_OP_TGET_RR] = ":od",
        [VM_INT_OP_TGET_RF] = ":oF"};
    return table[opcode];
}

#endif
