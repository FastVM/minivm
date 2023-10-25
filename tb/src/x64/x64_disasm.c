#include <tb_x64.h>
#include <common.h> // __debugbreak

// this is used to parse ModRM and SIB
#define UNPACK_233(a, b, c, src) \
(a = (src >> 6), b = (src >> 3) & 7, c = (src & 7))

#define THROW()  return false
#define ABC(amt) if (current + (amt) > length) THROW()

#define READ32(x)   (memcpy(&(x), &data[current], 4), current += 4, x)
#define READ32LE(x) READ32(x)

static bool x86_parse_memory_op(TB_X86_Inst* restrict inst, size_t length, const uint8_t* data, int reg_slot, uint8_t mod, uint8_t rm, uint8_t rex) {
    if (mod == MOD_DIRECT) {
        inst->regs[reg_slot] = (rex&1 ? 8 : 0) | rm;
        return true;
    } else {
        size_t current = 0;

        inst->disp = 0;
        inst->flags |= TB_X86_INSTR_USE_MEMOP;

        // indirect
        if (rm == TB_X86_RSP) {
            ABC(1);
            uint8_t sib = data[current++];

            uint8_t scale, index, base;
            UNPACK_233(scale, index, base, sib);

            TB_X86_GPR base_gpr  = base != TB_X86_RBP  ? ((rex&1 ? 8 : 0) | base)  : -1;
            TB_X86_GPR index_gpr = index != TB_X86_RSP ? ((rex&2 ? 8 : 0) | index) : -1;

            // odd rule but when mod=00,base=101,index=100
            // and using SIB, enable Disp32. this would technically
            // apply to R13 too which means you can't do
            //   lea rax, [r13 + rcx*2] or lea rax, [rbp + rcx*2]
            // only
            //   lea rax, [r13 + rcx*2 + 0] or lea rax, [rbp + rcx*2 + 0]
            if (mod == 0 && base == TB_X86_RBP) {
                mod = MOD_INDIRECT_DISP32;
            }

            inst->base = base_gpr;
            inst->index = index_gpr;
            inst->scale = scale;
        } else {
            if (mod == MOD_INDIRECT && rm == TB_X86_RBP) {
                // RIP-relative addressing
                ABC(4);
                int32_t disp = READ32LE(disp);

                inst->flags |= TB_X86_INSTR_USE_RIPMEM;
                inst->disp = disp;
            } else {
                inst->base = (rex&1 ? 8 : 0) | rm;
                inst->index = -1;
                inst->scale = 0; // *1
            }
        }

        if (mod == MOD_INDIRECT_DISP8) {
            ABC(1);
            int8_t disp = data[current++];
            inst->disp = disp;
        } else if (mod == MOD_INDIRECT_DISP32) {
            ABC(4);
            int32_t disp = READ32LE(disp);
            inst->disp = disp;
        }

        inst->regs[reg_slot] = -1;
        inst->length += current;
        return true;
    }
}

// INSTRUCTION ::= PREFIX* OPCODE[1-4] (MODRM SIB?)? IMMEDIATE? OFFSET?
bool tb_x86_disasm(TB_X86_Inst* restrict inst, size_t length, const uint8_t* data) {
    *inst = (TB_X86_Inst){ 0 };
    for (int i = 0; i < 4; i++) inst->regs[i] = -1;

    size_t current = 0;

    ////////////////////////////////
    // Parse prefixes
    ////////////////////////////////
    uint8_t rex = 0;     // 0x4X
    bool addr32 = false; // 0x67
    bool addr16 = false; // 0x66
    bool rep    = false; // 0xF3 these are both used
    bool repne  = false; // 0xF2 to define SSE types
    bool ext    = false; // 0x0F

    uint8_t op;
    while (true) {
        ABC(1);
        op = data[current++];

        switch (op) {
            case 0x40 ... 0x4F: rex = op; break;
            case 0xF0: inst->flags |= TB_X86_INSTR_LOCK; break;
            case 0x0F: ext = true; break;
            case 0x66: addr16 = true; break;
            case 0x67: addr32 = true; break;
            case 0xF3: rep = true; break;
            case 0xF2: repne = true; break;
            case 0x2E: inst->segment = TB_X86_SEGMENT_CS; break;
            case 0x36: inst->segment = TB_X86_SEGMENT_SS; break;
            case 0x3E: inst->segment = TB_X86_SEGMENT_DS; break;
            case 0x26: inst->segment = TB_X86_SEGMENT_ES; break;
            case 0x64: inst->segment = TB_X86_SEGMENT_FS; break;
            case 0x65: inst->segment = TB_X86_SEGMENT_GS; break;
            default: goto done_prefixing;
        }
    }

    done_prefixing:;
    ////////////////////////////////
    // Parse opcode
    ////////////////////////////////
    enum {
        OP_8BIT   = 1,
        OP_64BIT  = 2,
        OP_FAKERX = 4,
        OP_2DT    = 8,
    };

    enum {
        // encoding
        OP_BAD   = 0x00,
        OP_MR    = 0x10,
        OP_RM    = 0x20,
        OP_MI    = 0x30,
        OP_MI8   = 0x40,
        OP_PLUSR = 0x50,
        OP_0ARY  = 0x60,
    };

    #define NORMIE_BINOP(op) [op+0] = OP_MR | OP_8BIT, [op+1] = OP_MR, [op+2] = OP_RM | OP_8BIT, [op+3] = OP_RM
    static const uint8_t first_table[256] = {
        NORMIE_BINOP(0x00), // add
        NORMIE_BINOP(0x08), // or
        NORMIE_BINOP(0x20), // and
        NORMIE_BINOP(0x28), // sub
        NORMIE_BINOP(0x30), // xor
        NORMIE_BINOP(0x38), // cmp

        // OP r/m, imm32
        [0x81] = OP_MI | OP_FAKERX,
        // OP r/m, imm8
        [0x83] = OP_MI8 | OP_FAKERX,
        // push reg
        // pop reg
        [0x50 ... 0x5F] = OP_PLUSR | OP_64BIT,
        // movsxd reg, r/m
        [0x63] = OP_RM | OP_2DT,
        // mov r/m, reg
        [0x88] = OP_MR | OP_8BIT, [0x89] = OP_MR, [0x8A] = OP_RM | OP_8BIT, [0x8B] = OP_RM,
        // nop
        [0x90] = OP_0ARY,
        // ret
        [0xC3] = OP_0ARY,
        // mov r/m, imm
        [0xC6] = OP_MI | OP_FAKERX | OP_8BIT,
        [0xC7] = OP_MI | OP_FAKERX,
    };
    #undef NORMIE_BINOP

    static const uint8_t ext_table[256] = {
        // nop r/m
        [0x1F] = OP_RM,
        // imul reg, r/m
        [0xAF] = OP_RM,
    };

    inst->opcode = (ext ? 0x0F00 : 0) | op;

    uint8_t first = ext ? ext_table[op] : first_table[op];
    uint8_t flags = first & 0xF;
    assert(first != 0 && "unknown op");

    // info from table
    uint8_t enc = first & 0xF0;
    bool uses_imm = enc == OP_MI || enc == OP_MI8;

    // in the "default" "type" "system", REX.W is 64bit, certain ops
    // will mark they're 8bit and most will just be 32bit (with 16bit on ADDR16)
    inst->data_type = TB_X86_TYPE_DWORD;
    if (flags & OP_64BIT) {
        // basically forced 64bit
        inst->data_type = TB_X86_TYPE_QWORD;
    } else {
        if (rex & 0x8) inst->data_type = TB_X86_TYPE_QWORD;
        else if (flags & OP_8BIT) inst->data_type = TB_X86_TYPE_BYTE;
        else if (addr16) inst->data_type = TB_X86_TYPE_WORD;
    }

    assert(enc != OP_BAD);
    if (enc == OP_0ARY) {
        inst->length = current;
    } else if (enc == OP_PLUSR) {
        // bottom 8bits of the opcode are the base reg
        inst->regs[0] = (rex & 1 ? 1 : 0) | (inst->opcode & 7);
        inst->opcode &= ~7;
        inst->length = current;
    } else {
        ////////////////////////////////
        // Parse ModRM
        ////////////////////////////////
        bool rm_slot = enc == OP_RM;
        bool rx_slot = !rm_slot;

        ABC(1);
        uint8_t mod, rx, rm, modrm = data[current++];
        UNPACK_233(mod, rx, rm, modrm);

        if (flags & OP_2DT) {
            inst->flags |= TB_X86_INSTR_TWO_DATA_TYPES;
            inst->data_type2 = TB_X86_TYPE_DWORD;
        }

        // immediates might use RX for an extended opcode
        if (uses_imm) {
            if (flags & OP_FAKERX) {
                inst->opcode <<= 4;
                inst->opcode |= rx;
            }

            if (enc == OP_MI8) {
                ABC(1);
                int8_t imm = data[current++];
                inst->flags |= TB_X86_INSTR_IMMEDIATE;
                inst->imm = imm;
            } else if (enc == OP_MI) {
                ABC(4);
                int32_t imm = READ32LE(imm);
                inst->flags |= TB_X86_INSTR_IMMEDIATE;
                inst->imm = imm;
            } else {
                return false;
            }
        } else {
            int8_t real_rx = ((rex & 4 ? 8 : 0) | rx);
            if (rex == 0 && inst->data_type == TB_X86_TYPE_BYTE && real_rx >= 4) {
                // use high registers
                real_rx += 16;
            }
            inst->regs[rx_slot] = real_rx;
        }

        // writes out RM reg (or base and index)
        inst->length = current;
        x86_parse_memory_op(inst, length - current, &data[current], rm_slot, mod, rm, rex);
    }

    return current > 0;
}

const char* tb_x86_mnemonic(TB_X86_Inst* inst) {
    switch (inst->opcode) {
        case 0x00 ... 0x03: return "add";
        case 0x08 ... 0x0B: return "or";
        case 0x20 ... 0x23: return "and";
        case 0x28 ... 0x2B: return "sub";
        case 0x30 ... 0x33: return "xor";
        case 0x38 ... 0x3B: return "cmp";

        case 0x830: return "add";
        case 0x831: return "or";
        case 0x834: return "and";
        case 0x835: return "sub";
        case 0x836: return "xor";
        case 0x837: return "cmp";
        case 0xC70: return "mov";

        case 0x88: case 0x89: case 0x8A: case 0x8B: return "mov";
        case 0x90: return "nop";
        case 0xC3: return "ret";
        case 0x63: return "movsxd";
        case 0x50: return "push";
        case 0x58: return "pop";

        case 0x0F1F: return "nop";
        case 0x0FAF: return "imul";

        default:   return "???";
    }
}

const char* tb_x86_reg_name(int8_t reg, TB_X86_DataType dt) {
    static const char* X86__GPR_NAMES[4][16] = {
        { "al",  "cl",  "dl",  "bl",  "spl", "bpl", "sil", "dil", "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b" },
        { "ax",  "cx",  "dx",  "bx",  "sp",  "bp",  "si",  "di",  "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w" },
        { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d" },
        { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8",  "r9",  "r10",  "r11",  "r12",  "r13",  "r14",  "r15" }
    };

    static const char* X86__HIGH_NAMES[] = {
        "ah", "ch", "dh", "bh"
    };

    assert(dt >= TB_X86_TYPE_BYTE && dt <= TB_X86_TYPE_QWORD);
    return X86__GPR_NAMES[dt - TB_X86_TYPE_BYTE][reg];
}

const char* tb_x86_type_name(TB_X86_DataType dt) {
    switch (dt) {
        case TB_X86_TYPE_BYTE:  return "byte";
        case TB_X86_TYPE_WORD:  return "word";
        case TB_X86_TYPE_DWORD: return "dword";
        case TB_X86_TYPE_QWORD: return "qword";

        default: return "???";
    }
}
