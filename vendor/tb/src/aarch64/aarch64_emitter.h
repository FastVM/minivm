
// Xn refers to the 64bit variants of the registers,
// usually the 32bit aliases are Wn (we don't have enums
// for them because it's not that important, they're equal)
typedef enum {
    X0,  X1,   X2,  X3,  X4,  X5,  X6,  X7,
    X8,  X9,  X10, X11, X12, X13, X14, X15,
    X16, X17, X18, X19, X20, X21, X22, X23,
    X24, X25, X26, X27, X28, X29, X30,

    // frame pointer
    FP = 29,
    // link register is basically just the RPC
    LR = 30,
    // It's context specific because ARM lmao
    ZR = 31, SP = 31,

    // not a real gpr
    GPR_NONE = -1,
} GPR;

// refers to the data processing immediate operand.
// Aarch64 has a bunch of weird immediate fields so
// we might wanna rename this later.
typedef struct {
    uint16_t imm;
    uint8_t shift;
} Immediate;

typedef enum {
    ADD,
    SUB,
    EOR,
    UDIV,
    SDIV,
} DPOpcode;

enum {
    SHIFT_LSL,
    SHIFT_LSR,
    SHIFT_ASR,
    SHIFT_RESERVED,
};

typedef struct {
    uint32_t r, i;
} DPInst;

// op0  30-29
// op1  28
// 101  27-25
// op2  24-21
// op3  15-10
#define DPR(op0, op1, op2, op3) ((op0 << 29u) | (op1 << 28) | (0b101 << 25) | (op2 << 21) | (op3 << 10))
// op   30
// 100  28-26
// op0  25-23
#define DPI(op, op0) ((op << 30u) | (0b100 << 26u) | (op0 << 23u))

#define DP3(op0, op1, op2) ((op0 << 30u) | (op1 << 28u) | (0b101 << 25u) | (op2 << 21u))

static const DPInst inst_table[] = {
    //         register                     immediate
    [ADD]  = { DPR(0, 0, 0b1000, 0),        DPI(0, 0b010) },
    [SUB]  = { DPR(2, 0, 0b1000, 0),        DPI(1, 0b010) },
    [EOR]  = { 0b01001010000 << 21u,         0 },

    [UDIV] = { DPR(0, 1, 0b0110, 0b000010) },
    [SDIV] = { DPR(0, 1, 0b0110, 0b000011) },
};

enum {
    UBFM = DPI(0, 0b110) | (0b10 << 29u),

    //                       op0
    //                       V
    MADD = 0b00011011000000000000000000000000,
    MSUB = 0b00011011000000001000000000000000,
};

static void emit_ret(TB_CGEmitter* restrict e, GPR rn) {
    // 1101 0110 0101 1111 0000 00NN NNN0 0000
    //
    // 'ret rn' just does 'mov pc, rn', although in practice
    // we only pass the link reg to it.
    uint32_t inst = 0b11010110010111110000000000000000;
    inst |= (rn & 0b11111) << 5u;
    EMIT4(e, inst);
}

// OP Rd, Rn, Rm, Ra
static void emit_dp3(TB_CGEmitter* restrict e, uint32_t inst, GPR d, GPR n, GPR m, GPR a, bool _64bit) {
    inst |= (_64bit ? (1u << 31u) : 0);
    inst |= (m & 0x1F) << 16u;
    inst |= (a & 0x1F) << 10u;
    inst |= (n & 0x1F) << 5u;
    inst |= (d & 0x1F) << 0u;
    EMIT4(e, inst);
}

// data processing instruction
//   OP dst, src, imm
//
// 0000 0000 0000 0000 0000 0000 0000 0000
// AOOO OOOO SSII IIII IIII IINN NNND DDDD
//
// A - set when we're doing the 64bit variant of the instruction
// O - is the opcode
// S - shift
// I - immediate
// N - source
// D - destination
static void emit_dp_imm(TB_CGEmitter* restrict e, DPOpcode op, GPR dst, GPR src, uint16_t imm, uint8_t shift, bool _64bit) {
    uint32_t inst = inst_table[op].i | (_64bit ? (1u << 31u) : 0);

    if (op == ADD || op == SUB) {
        assert(shift == 0 || shift == 12);
        inst |= (1 << 22u);
    } else if (op == UBFM) {
        inst |= (1 << 22u);
    }

    inst |= (imm & 0xFFF) << 10u;
    inst |= (src & 0x1F) << 5u;
    inst |= (dst & 0x1F) << 0u;
    EMIT4(e, inst);
}

// bitfield
static void emit_bitfield(TB_CGEmitter* restrict e, uint32_t op, GPR dst, GPR src, uint8_t immr, uint8_t imms, bool _64bit) {
    uint32_t inst = op | (_64bit ? (1u << 31u) : 0);
    inst |= (immr & 0b111111) << 16u;
    inst |= (imms & 0b111111) << 10u;
    inst |= (src  & 0b11111) << 5u;
    inst |= (dst  & 0b11111) << 0u;
    EMIT4(e, inst);
}

// data processing instruction
//   OP dst, a, b
//
// 0000 0000 0000 0000 0000 0000 0000 0000
// AOOO OOOO SS_M MMMM IIII IINN NNND DDDD
// x101 1110 xx1m mmmm x000 01nn nnnd dddd  -  add Sd Sn Sm
static void emit_dp_r(TB_CGEmitter* restrict e, DPOpcode op, GPR dst, GPR a, GPR b, uint16_t imm, uint8_t shift, bool _64bit) {
    uint32_t inst = inst_table[op].r | (_64bit ? (1u << 31u) : 0);

    if (op == ADD || op == SUB) {
        assert(shift == 0 || shift == 12);
        inst |= (1 << 22u);
    } else if (op == UBFM) {
        if (shift) inst |= (1 << 22u);
    }

    inst |= (b & 0b11111) << 16u;
    inst |= (imm & 0xFFF) << 10u;
    inst |= (a & 0b11111) << 5u;
    inst |= (dst & 0b11111) << 0u;
    EMIT4(e, inst);
}

static void emit_mov(TB_CGEmitter* restrict e, uint8_t dst, uint8_t src, bool _64bit) {
    uint32_t inst = (_64bit ? (1 << 31u) : 0);

    inst |= (0b00101010 << 24u);
    inst |= (src & 0b11111) << 16u;
    inst |= (0b11111) << 5u;
    inst |= (dst & 0b11111) << 0u;
    EMIT4(e, inst);
}

// clear means movz, else movk
static void emit_movimm(TB_CGEmitter* restrict e, uint8_t dst, uint16_t imm, uint8_t shift, bool _64bit, bool clear) {
    uint32_t inst = (_64bit ? (1 << 31u) : 0);

    if (clear) {
        inst |= (0b010100101 << 23u);
    } else {
        inst |= (0b011100101 << 23u);
    }
    inst |= shift << 21u;
    inst |= imm << 5u;
    inst |= (dst & 0b11111) << 0u;
    EMIT4(e, inst);
}

