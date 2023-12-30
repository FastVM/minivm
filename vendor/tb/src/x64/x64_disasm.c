#include <tb_x64.h>
#include <common.h> // __debugbreak

// this is used to parse ModRM and SIB
#define UNPACK_233(a, b, c, src) \
(a = (src >> 6), b = (src >> 3) & 7, c = (src & 7))

#define READ32(x)   (memcpy(&(x), &data[current], 4), current += 4, x)
#define READ64(x)   (memcpy(&(x), &data[current], 8), current += 8, x)

#define READ32LE(x) READ32(x)
#define READ64LE(x) READ64(x)

#define ABC(amt) if (current + (amt) > length) return -1
static ptrdiff_t x86_parse_memory_op(TB_X86_Inst* restrict inst, size_t length, const uint8_t* data, int reg_slot, uint8_t mod, uint8_t rm, uint8_t rex) {
    if (mod == MOD_DIRECT) {
        inst->regs[reg_slot] = (rex&1 ? 8 : 0) | rm;
        return 0;
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

            TB_X86_GPR base_gpr  = mod != MOD_INDIRECT || base != TB_X86_RBP  ? ((rex&1 ? 8 : 0) | base)  : -1;
            TB_X86_GPR index_gpr = mod != MOD_INDIRECT || index != TB_X86_RSP ? ((rex&2 ? 8 : 0) | index) : -1;

            // odd rule but when mod=00,base=101,index=100
            // and using SIB, enable Disp32. this would technically
            // apply to R13 too which means you can't do
            //   lea rax, [r13 + rcx*2] or lea rax, [rbp + rcx*2]
            // only
            //   lea rax, [r13 + rcx*2 + 0] or lea rax, [rbp + rcx*2 + 0]
            if (base == TB_X86_RSP && index == TB_X86_RSP) {
                index_gpr = -1;
            } else if (mod == 0 && base == TB_X86_RBP) {
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
                inst->base = -1;
                inst->index = -1;
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
        return current;
    }
}
#undef ABC

// INSTRUCTION ::= PREFIX* OPCODE[1-4] (MODRM SIB?)? IMMEDIATE? OFFSET?
#define ABC(amt) if (current + (amt) > length) return false
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
            case 0x66: addr16 = true; break;
            case 0x67: addr32 = true; break;
            case 0xF3: inst->flags |= TB_X86_INSTR_REP; break;
            case 0xF2: inst->flags |= TB_X86_INSTR_REPNE; break;
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
    if (op == 0x0F) {
        ext = true;
        op = data[current++];
    }

    ////////////////////////////////
    // Parse opcode
    ////////////////////////////////
    enum {
        OP_8BIT   = 1,
        OP_16BIT  = 2,
        OP_64BIT  = 4,
        OP_FAKERX = 8,
        OP_2DT    = 16,
        OP_SSE    = 32,
    };

    enum {
        // encoding
        OP_BAD   = 0x0000,
        OP_MR    = 0x1000,
        OP_RM    = 0x2000,
        OP_M     = 0x3000,
        OP_MI    = 0x4000,
        OP_MI8   = 0x5000,
        OP_MC    = 0x6000,
        OP_PLUSR = 0x7000,
        OP_0ARY  = 0x8000,
        OP_REL8  = 0x9000,
        OP_REL32 = 0xA000,
        OP_IMM   = 0xB000,
    };

    #define NORMIE_BINOP(op) [op+0] = OP_MR | OP_8BIT, [op+1] = OP_MR, [op+2] = OP_RM | OP_8BIT, [op+3] = OP_RM
    static const uint16_t first_table[256] = {
        NORMIE_BINOP(0x00), // add
        NORMIE_BINOP(0x08), // or
        NORMIE_BINOP(0x20), // and
        NORMIE_BINOP(0x28), // sub
        NORMIE_BINOP(0x30), // xor
        NORMIE_BINOP(0x38), // cmp
        NORMIE_BINOP(0x88), // mov

        // OP r/m8, imm8
        [0x80] = OP_MI8 | OP_8BIT | OP_FAKERX,
        // OP r/m, imm32
        [0x81] = OP_MI | OP_FAKERX,
        // OP r/m, imm8
        [0x83] = OP_MI8 | OP_FAKERX,
        // TEST r/m, reg
        [0x84] = OP_MR | OP_8BIT,
        [0x85] = OP_MR,
        // push reg
        // pop reg
        [0x50 ... 0x5F] = OP_PLUSR | OP_64BIT,
        // lea reg, r/m
        [0x8D] = OP_RM,
        // movsxd reg, r/m
        [0x63] = OP_RM | OP_2DT,
        // push imm32
        [0x68] = OP_IMM,
        // imul reg, r/m, imm32
        [0x69] = OP_RM,
        // jcc rel8
        [0x70 ... 0x7F] = OP_REL8,
        // nop
        [0x90] = OP_0ARY,
        // cwd/cdq/cqo
        [0x99] = OP_0ARY,
        // ret
        [0xC3] = OP_0ARY,
        // int3
        [0xCC] = OP_0ARY,
        // movabs
        [0xB8 ... 0xBF] = OP_PLUSR | OP_64BIT,
        // mov r/m, imm
        [0xC6] = OP_MI | OP_FAKERX | OP_8BIT,
        [0xC7] = OP_MI | OP_FAKERX,
        // movs
        [0xA4 ... 0xA5] = OP_0ARY,
        // stos
        [0xAA ... 0xAB] = OP_0ARY,
        // scas
        [0xAE ... 0xAF] = OP_0ARY,
        // sar r/m, imm8
        // shl r/m, imm8
        // shr r/m, imm8
        [0xC0] = OP_MI8 | OP_FAKERX | OP_8BIT,
        [0xC1] = OP_MI8 | OP_FAKERX,
        // sar r/m, CL
        // shl r/m, CL
        // shr r/m, CL
        // rol r/m, CL
        // ror r/m, CL
        [0xD2] = OP_MC | OP_FAKERX | OP_8BIT,
        [0xD3] = OP_MC | OP_FAKERX,
        // call rel32
        [0xE8] = OP_REL32,
        // jmp rel32
        [0xE9] = OP_REL32,
        // jmp rel8
        [0xEB] = OP_REL8,
        // idiv r/m8
        [0xF6] = OP_M | OP_FAKERX | OP_8BIT,
        [0xF7] = OP_M | OP_FAKERX,
        // jmp r/m
        [0xFF] = OP_M | OP_FAKERX | OP_64BIT,
    };
    #undef NORMIE_BINOP

    static const uint16_t ext_table[256] = {
        // ud2
        [0x0B] = OP_0ARY,
        // prefetch r/m
        [0x18] = OP_FAKERX,
        // SSE: movu
        [0x10] = OP_RM | OP_SSE, [0x11] = OP_MR | OP_SSE,
        // SSE: add, mul, sub, min, div, max
        [0x51 ... 0x5F] = OP_RM | OP_SSE,
        // cmovcc
        [0x40 ... 0x4F] = OP_RM,
        // SSE: cvtsi2sd
        [0x2A] = OP_RM | OP_SSE,
        [0x2C] = OP_RM | OP_SSE,
        // SSE: ucomi
        [0x2E] = OP_RM | OP_SSE,
        // rdtsc
        [0x31] = OP_0ARY,
        // nop r/m
        [0x1F] = OP_RM,
        // imul reg, r/m
        [0xAF] = OP_RM,
        // movzx
        [0xB6 ... 0xB7] = OP_RM | OP_2DT,
        // movsx reg, r/m
        [0xBE] = OP_RM | OP_2DT,
        // movsx reg, r/m
        [0xBF] = OP_RM | OP_2DT,
        // jcc rel32
        [0x80 ... 0x8F] = OP_REL32,
        // setcc r/m
        [0x90 ... 0x9F] = OP_M,
    };

    inst->opcode = (ext ? 0x0F00 : 0) | op;

    uint16_t first = ext ? ext_table[op] : first_table[op];
    uint16_t flags = first & 0xFFF;

    #if 1
    if (first == 0) return false;
    #else
    assert(first != 0 && "unknown op");
    #endif

    // info from table
    uint16_t enc = first & 0xF000;
    bool uses_imm = enc == OP_MI || enc == OP_MI8;

    // in the "default" "type" "system", REX.W is 64bit, certain ops
    // will mark they're 8bit and most will just be 32bit (with 16bit on ADDR16)
    inst->data_type = TB_X86_TYPE_DWORD;
    if (flags & OP_64BIT) {
        // basically forced 64bit
        inst->data_type = TB_X86_TYPE_QWORD;
    } else if (flags & OP_SSE) {
        // ss     REP    OPCODE
        // sd     REPNE  OPCODE
        // ps     __     OPCODE
        // pd     DATA16 OPCODE
        if (inst->flags & TB_X86_INSTR_REPNE) {
            inst->data_type = TB_X86_TYPE_SSE_SD;
        } else if (inst->flags & TB_X86_INSTR_REP) {
            inst->data_type = TB_X86_TYPE_SSE_SS;
        } else if (addr16) {
            inst->data_type = TB_X86_TYPE_SSE_SS;
        } else {
            inst->data_type = TB_X86_TYPE_SSE_PS;
        }

        inst->flags &= ~(TB_X86_INSTR_REP | TB_X86_INSTR_REPNE);
    } else {
        if (rex & 0x8) inst->data_type = TB_X86_TYPE_QWORD;
        else if (flags & OP_8BIT) inst->data_type = TB_X86_TYPE_BYTE;
        else if (addr16) inst->data_type = TB_X86_TYPE_WORD;
    }

    assert(enc != OP_BAD);
    if (enc == OP_IMM) {
        ABC(4);
        int32_t imm = READ32LE(imm);
        inst->flags |= TB_X86_INSTR_IMMEDIATE;
        inst->imm = imm;
        inst->length = current;
        return true;
    } else if (enc == OP_REL32) {
        inst->flags |= TB_X86_INSTR_USE_RIPMEM;
        inst->flags |= TB_X86_INSTR_USE_MEMOP;
        inst->base = -1;
        inst->index = -1;

        ABC(4);
        int32_t imm = READ32LE(imm);
        inst->disp = imm;
        inst->length = current;
        return true;
    } else if (enc == OP_0ARY) {
        inst->length = current;
        return true;
    } else if (enc == OP_PLUSR) {
        // bottom 8bits of the opcode are the base reg
        inst->regs[0] = (rex & 1 ? 1 : 0) | (inst->opcode & 7);
        inst->opcode &= ~7;

        if (op >= 0xB8 && op <= 0xBF) {
            // movabs (pre-APX) is the only instruction with a 64bit immediate so i'm finna
            // special case it.
            ABC(8);
            uint64_t imm = READ64LE(imm);
            inst->flags |= TB_X86_INSTR_ABSOLUTE;
            inst->abs = imm;
    	}
        inst->length = current;
        return true;
    } else {
        ////////////////////////////////
        // Parse ModRM
        ////////////////////////////////
        bool rm_slot = enc == OP_RM;
        bool rx_slot = !rm_slot;

        ABC(1);
        uint8_t mod, rx, rm, modrm = data[current++];
        UNPACK_233(mod, rx, rm, modrm);

        if (flags & OP_FAKERX) {
            inst->opcode <<= 4;
            inst->opcode |= rx;

            if (inst->opcode == 0xF60) {
                flags = OP_MI8;
            } else if (inst->opcode == 0xF70) {
                flags = OP_MI;
            }
        }

        if (flags & OP_2DT) {
            if (inst->opcode == 0x0FB6 || inst->opcode == 0x0FB7 || inst->opcode == 0x0FBE || inst->opcode == 0x0FBF) {
                inst->flags |= TB_X86_INSTR_TWO_DATA_TYPES;
                inst->data_type2 = (inst->opcode == 0x0FB6 || inst->opcode == 0x0FBE) ? TB_X86_TYPE_BYTE : TB_X86_TYPE_WORD;
            } else {
                inst->flags |= TB_X86_INSTR_TWO_DATA_TYPES;
                inst->data_type2 = TB_X86_TYPE_DWORD;
            }
        }

        if (enc != OP_M && !uses_imm) {
            if (enc == OP_MC) {
                inst->flags |= TB_X86_INSTR_TWO_DATA_TYPES;
                inst->data_type2 = TB_X86_TYPE_BYTE;
                inst->regs[rx_slot] = RCX;
            } else {
                int8_t real_rx = ((rex & 4 ? 8 : 0) | rx);
                if (rex == 0 && inst->data_type == TB_X86_TYPE_BYTE && real_rx >= 4) {
                    // use high registers
                    real_rx += 16;
                }
                inst->regs[rx_slot] = real_rx;
            }
        }

        // writes out RM reg (or base and index)
        ptrdiff_t delta = x86_parse_memory_op(inst, length - current, &data[current], rm_slot, mod, rm, rex);
        if (delta < 0) {
            return false;
        }
        current += delta;

        // immediates might use RX for an extended opcode
        // IMUL's ternary is a special case
        if (uses_imm || op == 0x68 || op == 0x69) {
            if ((enc == OP_MI && inst->data_type == TB_X86_TYPE_BYTE) || enc == OP_MI8 || op == 0x68) {
                ABC(1);
                int8_t imm = data[current++];
                inst->flags |= TB_X86_INSTR_IMMEDIATE;
                inst->imm = imm;
            } else if (enc == OP_MI || op == 0x69) {
                ABC(4);
                int32_t imm = READ32LE(imm);
                inst->flags |= TB_X86_INSTR_IMMEDIATE;
                inst->imm = imm;
            } else {
                return false;
            }
        }

        inst->length = current;
        return true;
    }
}
#undef ABC

const char* tb_x86_mnemonic(TB_X86_Inst* inst) {
    // cwd/cdq/cqo
    if (inst->opcode == 0x99) {
        if (inst->data_type == TB_X86_TYPE_WORD)  return "cwd";
        if (inst->data_type == TB_X86_TYPE_DWORD) return "cdq";
        if (inst->data_type == TB_X86_TYPE_QWORD) return "cqo";
        return "??";
    }

    switch (inst->opcode) {
        case 0x0F0B: return "ud2";
        case 0x0F31: return "rdtsc";
        case 0xCC: return "int3";

        case 0x0F180: return "prefetchnta";
        case 0x0F181: return "prefetch0";
        case 0x0F182: return "prefetch1";
        case 0x0F183: return "prefetch2";

        case 0x00 ... 0x03: return "add";
        case 0x08 ... 0x0B: return "or";
        case 0x20 ... 0x23: return "and";
        case 0x28 ... 0x2B: return "sub";
        case 0x30 ... 0x33: return "xor";
        case 0x38 ... 0x3B: return "cmp";
        case 0x88 ... 0x8B: return "mov";

        case 0xA4: case 0xA5: return "movs";
        case 0xAA: case 0xAB: return "stos";
        case 0xAE: case 0xAF: return "scas";

        case 0xC00: case 0xC10: case 0xD20: case 0xD30: return "rol";
        case 0xC01: case 0xC11: case 0xD21: case 0xD31: return "ror";
        case 0xC04: case 0xC14: case 0xD24: case 0xD34: return "shl";
        case 0xC05: case 0xC15: case 0xD25: case 0xD35: return "shr";
        case 0xC07: case 0xC17: case 0xD27: case 0xD37: return "sar";

        case 0xF60: case 0xF70: return "test";
        case 0xF62: case 0xF72: return "not";
        case 0xF66: case 0xF76: return "div";
        case 0xF67: case 0xF77: return "idiv";

        case 0x810: case 0x800: case 0x830: return "add";
        case 0x811: case 0x801: case 0x831: return "or";
        case 0x814: case 0x804: case 0x834: return "and";
        case 0x815: case 0x805: case 0x835: return "sub";
        case 0x816: case 0x806: case 0x836: return "xor";
        case 0x817: case 0x807: case 0x837: return "cmp";
        case 0xC60: case 0xC70: return "mov";
        case 0x84: case 0x85: return "test";

        case 0x0F10: case 0x0F11: return "mov";
        case 0x0F58: return "add";
        case 0x0F59: return "mul";
        case 0x0F5C: return "sub";
        case 0x0F5D: return "min";
        case 0x0F5E: return "div";
        case 0x0F5F: return "max";
        case 0x0FC2: return "cmp";
        case 0x0F2E: return "ucomi";
        case 0x0F51: return "sqrt";
        case 0x0F52: return "rsqrt";
        case 0x0F54: return "and";
        case 0x0F56: return "or";
        case 0x0F57: return "xor";

        case 0xB8 ... 0xBF: return "mov";
        case 0x0FB6: case 0x0FB7: return "movzx";
        case 0x0FBE: case 0x0FBF: return "movsx";

        case 0x8D: return "lea";
        case 0x90: return "nop";
        case 0xC3: return "ret";
        case 0x63: return "movsxd";
        case 0x50: return "push";
        case 0x58: return "pop";

        case 0xE8: case 0xFF2: return "call";
        case 0xEB: case 0xE9: case 0xFF4: return "jmp";

        case 0x0F1F: return "nop";
        case 0x68: return "push";
        case 0x0FAF: case 0x69: case 0x6B: return "imul";

        case 0x0F40: return "cmovo";
        case 0x0F41: return "cmovno";
        case 0x0F42: return "cmovb";
        case 0x0F43: return "cmovnb";
        case 0x0F44: return "cmove";
        case 0x0F45: return "cmovne";
        case 0x0F46: return "cmovbe";
        case 0x0F47: return "cmova";
        case 0x0F48: return "cmovs";
        case 0x0F49: return "cmovns";
        case 0x0F4A: return "cmovp";
        case 0x0F4B: return "cmovnp";
        case 0x0F4C: return "cmovl";
        case 0x0F4D: return "cmovge";
        case 0x0F4E: return "cmovle";
        case 0x0F4F: return "cmovg";

        case 0x0F90: return "seto";
        case 0x0F91: return "setno";
        case 0x0F92: return "setb";
        case 0x0F93: return "setnb";
        case 0x0F94: return "sete";
        case 0x0F95: return "setne";
        case 0x0F96: return "setbe";
        case 0x0F97: return "seta";
        case 0x0F98: return "sets";
        case 0x0F99: return "setns";
        case 0x0F9A: return "setp";
        case 0x0F9B: return "setnp";
        case 0x0F9C: return "setl";
        case 0x0F9D: return "setge";
        case 0x0F9E: return "setle";
        case 0x0F9F: return "setg";

        case 0x0F80: case 0x70: return "jo";
        case 0x0F81: case 0x71: return "jno";
        case 0x0F82: case 0x72: return "jb";
        case 0x0F83: case 0x73: return "jnb";
        case 0x0F84: case 0x74: return "je";
        case 0x0F85: case 0x75: return "jne";
        case 0x0F86: case 0x76: return "jbe";
        case 0x0F87: case 0x77: return "ja";
        case 0x0F88: case 0x78: return "js";
        case 0x0F89: case 0x79: return "jns";
        case 0x0F8A: case 0x7A: return "jp";
        case 0x0F8B: case 0x7B: return "jnp";
        case 0x0F8C: case 0x7C: return "jl";
        case 0x0F8D: case 0x7D: return "jge";
        case 0x0F8E: case 0x7E: return "jle";
        case 0x0F8F: case 0x7F: return "jg";

        default: return "??";
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

    if (dt >= TB_X86_TYPE_BYTE && dt <= TB_X86_TYPE_QWORD) {
        return X86__GPR_NAMES[dt - TB_X86_TYPE_BYTE][reg];
    } else if (dt >= TB_X86_TYPE_SSE_SS && dt <= TB_X86_TYPE_SSE_PD) {
        static const char* X86__XMM_NAMES[] = {
            "xmm0", "xmm1", "xmm2",  "xmm3",  "xmm4",  "xmm5",  "xmm6",  "xmm7",
            "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15",
        };

        return X86__XMM_NAMES[reg];
    } else {
        return "??";
    }
}

const char* tb_x86_type_name(TB_X86_DataType dt) {
    switch (dt) {
        case TB_X86_TYPE_BYTE:  return "byte";
        case TB_X86_TYPE_WORD:  return "word";
        case TB_X86_TYPE_DWORD: return "dword";
        case TB_X86_TYPE_QWORD: return "qword";
        case TB_X86_TYPE_SSE_SS:return "dword";
        case TB_X86_TYPE_SSE_SD:return "qword";
        case TB_X86_TYPE_SSE_PS:return "xmmword";
        case TB_X86_TYPE_SSE_PD:return "xmmword";

        default: return "??";
    }
}
