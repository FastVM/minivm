
#pragma once

#if !defined(MX_HEADER_JIT_X64)
#define MX_HEADER_JIT_X64

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MX_X64_REX_W (UINT32_C(1) << 24)
#define MX_X64_P66 (UINT32_C(0x66) << 16)
#define MX_X64_PF2 (UINT32_C(0xf2) << 16)
#define MX_X64_0F UINT32_C(0x0f00)

typedef uint8_t mx_x64_reg_t;
enum {
    MX_X64_RAX,
    MX_X64_RCX,
    MX_X64_RDX,
    MX_X64_RBX,
    MX_X64_RSP,
    MX_X64_RBP,
    MX_X64_RSI,
    MX_X64_RDI,
    MX_X64_R8,
    MX_X64_R9,
    MX_X64_R10,
    MX_X64_R11,
    MX_X64_R12,
    MX_X64_R13,
    MX_X64_R14,
    MX_X64_R15,
};

typedef uint8_t mx_x64_alu_t;
enum {
    MX_X64_ALU_ADD = 0,
    MX_X64_ALU_OR = 1,
    MX_X64_ALU_AND = 4,
    MX_X64_ALU_SUB = 5,
    MX_X64_ALU_XOR = 6,
    MX_X64_ALU_CMP = 7,
};

typedef uint8_t mx_x64_cc_t;
enum {
    MX_X64_CC_B = 0x2,
    MX_X64_CC_AE = 0x3,
    MX_X64_CC_E = 0x4,
    MX_X64_CC_NE = 0x5,
    MX_X64_CC_BE = 0x6,
    MX_X64_CC_A = 0x7,
    MX_X64_CC_P = 0xa,
    MX_X64_CC_NP = 0xb,
    MX_X64_CC_L = 0xc,
    MX_X64_CC_GE = 0xd,
    MX_X64_CC_LE = 0xe,
    MX_X64_CC_G = 0xf,
};

typedef uint8_t mx_x64_sse_t;
enum {
    MX_X64_SSE_ADD = 0x58,
    MX_X64_SSE_MUL = 0x59,
    MX_X64_SSE_SUB = 0x5c,
    MX_X64_SSE_DIV = 0x5e,
};

typedef uint8_t mx_x64_shift_t;
enum {
    MX_X64_SHIFT_SHL = 4,
    MX_X64_SHIFT_SHR = 5,
    MX_X64_SHIFT_SAR = 7,
};

typedef struct mx_x64_t mx_x64_t;

struct mx_x64_t {
    uint8_t *p;
};

void mx_x64_u8(mx_x64_t *a, uint8_t v);
void mx_x64_u16(mx_x64_t *a, uint16_t v);
void mx_x64_u32(mx_x64_t *a, uint32_t v);
void mx_x64_u64(mx_x64_t *a, uint64_t v);
void mx_x64_head(mx_x64_t *a, uint32_t enc, uint32_t reg, uint32_t index, uint32_t rm);
void mx_x64_rr(mx_x64_t *a, uint32_t enc, uint32_t reg, uint32_t rm);
void mx_x64_rm(mx_x64_t *a, uint32_t enc, uint32_t reg, mx_x64_reg_t base, int32_t disp);
void mx_x64_rx(mx_x64_t *a, uint32_t enc, uint32_t reg, mx_x64_reg_t base, mx_x64_reg_t index, int32_t disp);

void mx_x64_rip(mx_x64_t *a, uint32_t enc, uint32_t reg, const void *target);
void mx_x64_movsd_rip(mx_x64_t *a, uint32_t x, const void *target);
void mx_x64_sse_rip(mx_x64_t *a, mx_x64_sse_t op, uint32_t d, const void *target);
void mx_x64_mov_rr(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t s);
void mx_x64_mov32_rr(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t s);
void mx_x64_mov_ri(mx_x64_t *a, mx_x64_reg_t d, uint64_t imm);
void mx_x64_load(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, int32_t disp);
void mx_x64_store(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, mx_x64_reg_t s);
void mx_x64_load32(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, int32_t disp);
void mx_x64_store32(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, mx_x64_reg_t s);
void mx_x64_store_i32(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, int32_t imm);
void mx_x64_store32_i(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, int32_t imm);
void mx_x64_store16_i(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, uint16_t imm);
void mx_x64_movzx16(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, int32_t disp);
void mx_x64_movzx8(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, int32_t disp);
void mx_x64_load_x(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, mx_x64_reg_t index, int32_t disp);
void mx_x64_store_x(mx_x64_t *a, mx_x64_reg_t base, mx_x64_reg_t index, int32_t disp, mx_x64_reg_t s);
void mx_x64_movzx16_x(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, mx_x64_reg_t index, int32_t disp);
void mx_x64_lea(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, int32_t disp);
void mx_x64_lea_x(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, mx_x64_reg_t index, int32_t disp);
void mx_x64_lea32(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, int32_t disp);

void mx_x64_alu_rr(mx_x64_t *a, mx_x64_alu_t op, mx_x64_reg_t d, mx_x64_reg_t s);
void mx_x64_alu_rm(mx_x64_t *a, mx_x64_alu_t op, mx_x64_reg_t d, mx_x64_reg_t base, int32_t disp);
void mx_x64_alu_ri(mx_x64_t *a, mx_x64_alu_t op, mx_x64_reg_t d, int32_t imm);
void mx_x64_cmp_mr(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, mx_x64_reg_t r);
void mx_x64_cmp_mi(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, int32_t imm);
void mx_x64_cmp32_rm(mx_x64_t *a, mx_x64_reg_t r, mx_x64_reg_t base, int32_t disp);
void mx_x64_cmp32_ri(mx_x64_t *a, mx_x64_reg_t r, int32_t imm);
void mx_x64_cmp32_mi(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, int32_t imm);
void mx_x64_cmp16_mi(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, uint16_t imm);
void mx_x64_cmp8_mi(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, uint8_t imm);
void mx_x64_alu8_mi(mx_x64_t *a, mx_x64_alu_t op, mx_x64_reg_t base, int32_t disp, uint8_t imm);
void mx_x64_alu32_mi8(mx_x64_t *a, mx_x64_alu_t op, mx_x64_reg_t base, int32_t disp, int8_t imm);
void mx_x64_test8_mi(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, uint8_t imm);
void mx_x64_test_rr(mx_x64_t *a, mx_x64_reg_t r, mx_x64_reg_t s);
void mx_x64_test8_al(mx_x64_t *a);
void mx_x64_and32_ri(mx_x64_t *a, mx_x64_reg_t d, int32_t imm);

void mx_x64_imul_rr(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t s);
void mx_x64_imul_rm(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, int32_t disp);
void mx_x64_imul_rri(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t s, int32_t imm);
void mx_x64_neg(mx_x64_t *a, mx_x64_reg_t r);
void mx_x64_not(mx_x64_t *a, mx_x64_reg_t r);
void mx_x64_shift_i(mx_x64_t *a, mx_x64_shift_t op, mx_x64_reg_t r, uint8_t count);
void mx_x64_shl32_cl(mx_x64_t *a, mx_x64_reg_t r);
void mx_x64_btc_i(mx_x64_t *a, mx_x64_reg_t r, uint8_t bit);
void mx_x64_cqo(mx_x64_t *a);
void mx_x64_idiv(mx_x64_t *a, mx_x64_reg_t r);

void mx_x64_push(mx_x64_t *a, mx_x64_reg_t r);
void mx_x64_pop(mx_x64_t *a, mx_x64_reg_t r);
void mx_x64_ret(mx_x64_t *a);
void mx_x64_call_rip(mx_x64_t *a, const void *slot);
void mx_x64_call_r(mx_x64_t *a, mx_x64_reg_t r);
void mx_x64_jmp_r(mx_x64_t *a, mx_x64_reg_t r);
void mx_x64_jmp_m(mx_x64_t *a, mx_x64_reg_t base, int32_t disp);
void mx_x64_jmp(mx_x64_t *a, const uint8_t *target);
void mx_x64_jcc(mx_x64_t *a, mx_x64_cc_t cc, const uint8_t *target);
void mx_x64_patch(uint8_t *site, const uint8_t *target);

void mx_x64_movq_xr(mx_x64_t *a, uint32_t x, mx_x64_reg_t r);
void mx_x64_movq_rx(mx_x64_t *a, mx_x64_reg_t r, uint32_t x);
void mx_x64_movsd_load(mx_x64_t *a, uint32_t x, mx_x64_reg_t base, int32_t disp);
void mx_x64_movsd_store(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, uint32_t x);
void mx_x64_movaps(mx_x64_t *a, uint32_t d, uint32_t s);
void mx_x64_sse_rr(mx_x64_t *a, mx_x64_sse_t op, uint32_t d, uint32_t s);
void mx_x64_sse_rm(mx_x64_t *a, mx_x64_sse_t op, uint32_t d, mx_x64_reg_t base, int32_t disp);
void mx_x64_ucomisd_rr(mx_x64_t *a, uint32_t l, uint32_t r);
void mx_x64_ucomisd_rm(mx_x64_t *a, uint32_t l, mx_x64_reg_t base, int32_t disp);
void mx_x64_cvtsi2sd_rr(mx_x64_t *a, uint32_t x, mx_x64_reg_t r);
void mx_x64_cvtsi2sd_rm(mx_x64_t *a, uint32_t x, mx_x64_reg_t base, int32_t disp);
void mx_x64_sqrtsd_rr(mx_x64_t *a, uint32_t d, uint32_t s);
void mx_x64_cvttsd2si(mx_x64_t *a, mx_x64_reg_t r, uint32_t x);
void mx_x64_btr_i(mx_x64_t *a, mx_x64_reg_t r, uint8_t bit);
void mx_x64_xorpd(mx_x64_t *a, uint32_t d, uint32_t s);

#endif
