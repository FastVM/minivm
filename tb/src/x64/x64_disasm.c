#include <tb_x64.h>
#include <common.h> // __debugbreak

// this is used to parse ModRM and SIB
#define UNPACK_233(a, b, c, src) \
(a = (src >> 6), b = (src >> 3) & 7, c = (src & 7))

#define THROW()  return false
#define ABC(amt) if (current + (amt) >= length) THROW()

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
            ABC(4);
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

    uint8_t op;
    while (true) {
        ABC(1);
        op = data[current++];

        if ((op & 0xF0) == 0x40) rex = op;
        else if (op == 0xF0) inst->flags |= TB_X86_INSTR_LOCK;
        else if (op == 0x66) addr16 = true;
        else if (op == 0x67) addr32 = true;
        else if (op == 0xF3) rep = true;
        else if (op == 0xF2) repne = true;
        else if (op == 0x2E) inst->segment = TB_X86_SEGMENT_CS;
        else if (op == 0x36) inst->segment = TB_X86_SEGMENT_SS;
        else if (op == 0x3E) inst->segment = TB_X86_SEGMENT_DS;
        else if (op == 0x26) inst->segment = TB_X86_SEGMENT_ES;
        else if (op == 0x64) inst->segment = TB_X86_SEGMENT_FS;
        else if (op == 0x65) inst->segment = TB_X86_SEGMENT_GS;
        else break;
    }

    ////////////////////////////////
    // Parse opcode
    ////////////////////////////////
    enum {
        // size flags
        OP_8BIT = 4,

        // encoding
        OP_MR = 64,
        OP_RM,
        OP_MI,
    };

    static const uint8_t first_table[256] = {
        // mov r/m, reg
        [0x88] = OP_MR | OP_8BIT,
        [0x89] = OP_MR,
        // mov reg, r/m
        [0x8A] = OP_RM | OP_8BIT,
        [0x8B] = OP_RM,
        // mov r/m, imm
        [0xC6] = OP_MI | OP_8BIT,
        [0xC7] = OP_MI,
    };

    inst->opcode = op;

    uint8_t first = first_table[op];
    uint8_t flags = first & 63;
    assert(first != 0 && "unknown op");

    // info from table
    uint8_t enc = first >> 6;
    bool uses_imm = enc == OP_MI;

    // in the "default" "type" "system", REX.W is 64bit, certain ops
    // will mark they're 8bit and most will just be 32bit (with 16bit on ADDR16)
    inst->data_type = TB_X86_TYPE_DWORD;
    if (rex & 0x8) inst->data_type = TB_X86_TYPE_QWORD;
    else if (flags & OP_8BIT) inst->data_type = TB_X86_TYPE_BYTE;
    else if (addr16) inst->data_type = TB_X86_TYPE_WORD;

    ////////////////////////////////
    // Parse ModRM
    ////////////////////////////////
    if (1) {
        bool rm_slot = enc == OP_RM;
        bool rx_slot = !rm_slot;

        ABC(1);
        uint8_t mod, rx, rm, modrm = data[current++];
        UNPACK_233(mod, rx, rm, modrm);

        // immediates might use RX for an extended opcode
        if (uses_imm) {
            tb_todo();
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
    } else {
        inst->length = current;
    }

    return current;
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

