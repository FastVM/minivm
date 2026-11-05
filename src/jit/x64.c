
#include "x64.h"

void mx_x64_u8(mx_x64_t *a, uint8_t v) {
    a->p[0] = v;
    a->p += 1;
}

void mx_x64_u16(mx_x64_t *a, uint16_t v) {
    mx_x64_u8(a, (uint8_t)v);
    mx_x64_u8(a, (uint8_t)(v >> 8));
}

void mx_x64_u32(mx_x64_t *a, uint32_t v) {
    mx_x64_u16(a, (uint16_t)v);
    mx_x64_u16(a, (uint16_t)(v >> 16));
}

void mx_x64_u64(mx_x64_t *a, uint64_t v) {
    mx_x64_u32(a, (uint32_t)v);
    mx_x64_u32(a, (uint32_t)(v >> 32));
}

void mx_x64_head(mx_x64_t *a, uint32_t enc, uint32_t reg, uint32_t index, uint32_t rm) {
    uint32_t prefix = (enc >> 16) & 0xff;
    if (prefix != 0) {
        mx_x64_u8(a, (uint8_t)prefix);
    }
    uint32_t w = (enc >> 24) & 1;
    uint32_t rex = (w << 3) | (((reg >> 3) & 1) << 2) | (((index >> 3) & 1) << 1) | ((rm >> 3) & 1);
    if (rex != 0) {
        mx_x64_u8(a, (uint8_t)(0x40 | rex));
    }
    if ((enc & MX_X64_0F) != 0) {
        mx_x64_u8(a, 0x0f);
    }
    mx_x64_u8(a, (uint8_t)enc);
}

void mx_x64_rr(mx_x64_t *a, uint32_t enc, uint32_t reg, uint32_t rm) {
    mx_x64_head(a, enc, reg, 0, rm);
    mx_x64_u8(a, (uint8_t)(0xc0 | ((reg & 7) << 3) | (rm & 7)));
}

void mx_x64_rm(mx_x64_t *a, uint32_t enc, uint32_t reg, mx_x64_reg_t base, int32_t disp) {
    mx_x64_head(a, enc, reg, 0, base);
    uint32_t low = base & 7u;
    uint32_t field = (reg & 7) << 3;
    if (disp == 0 && low != 5) {
        mx_x64_u8(a, (uint8_t)(field | low));
    } else if (disp >= -128 && disp <= 127) {
        mx_x64_u8(a, (uint8_t)(0x40 | field | low));
    } else {
        mx_x64_u8(a, (uint8_t)(0x80 | field | low));
    }
    if (low == 4) {
        mx_x64_u8(a, 0x24);
    }
    if (disp == 0 && low != 5) {
        return;
    } else if (disp >= -128 && disp <= 127) {
        mx_x64_u8(a, (uint8_t)disp);
    } else {
        mx_x64_u32(a, (uint32_t)disp);
    }
}

void mx_x64_rx(mx_x64_t *a, uint32_t enc, uint32_t reg, mx_x64_reg_t base, mx_x64_reg_t index, int32_t disp) {
    mx_x64_head(a, enc, reg, index, base);
    uint32_t field = (reg & 7) << 3;
    uint8_t sib = (uint8_t)(0xc0 | ((index & 7u) << 3) | (base & 7u));
    if (disp == 0 && (base & 7) != 5) {
        mx_x64_u8(a, (uint8_t)(field | 4));
        mx_x64_u8(a, sib);
    } else if (disp >= -128 && disp <= 127) {
        mx_x64_u8(a, (uint8_t)(0x40 | field | 4));
        mx_x64_u8(a, sib);
        mx_x64_u8(a, (uint8_t)disp);
    } else {
        mx_x64_u8(a, (uint8_t)(0x80 | field | 4));
        mx_x64_u8(a, sib);
        mx_x64_u32(a, (uint32_t)disp);
    }
}

void mx_x64_rip(mx_x64_t *a, uint32_t enc, uint32_t reg, const void *target) {
    mx_x64_head(a, enc, reg, 0, 0);
    mx_x64_u8(a, (uint8_t)(((reg & 7) << 3) | 5));
    mx_x64_u32(a, (uint32_t)((const uint8_t *)target - (a->p + 4)));
}

void mx_x64_movsd_rip(mx_x64_t *a, uint32_t x, const void *target) {
    mx_x64_rip(a, MX_X64_PF2 | MX_X64_0F | 0x10, x, target);
}

void mx_x64_sse_rip(mx_x64_t *a, mx_x64_sse_t op, uint32_t d, const void *target) {
    mx_x64_rip(a, MX_X64_PF2 | MX_X64_0F | op, d, target);
}

void mx_x64_mov_rr(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t s) {
    mx_x64_rr(a, MX_X64_REX_W | 0x89, s, d);
}

void mx_x64_mov32_rr(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t s) {
    mx_x64_rr(a, 0x89, s, d);
}

void mx_x64_mov_ri(mx_x64_t *a, mx_x64_reg_t d, uint64_t imm) {
    if (imm <= UINT32_MAX) {
        mx_x64_head(a, 0xb8 | (d & 7u), 0, 0, d);
        mx_x64_u32(a, (uint32_t)imm);
    } else if ((uint64_t)(int64_t)(int32_t)imm == imm) {
        mx_x64_rr(a, MX_X64_REX_W | 0xc7, 0, d);
        mx_x64_u32(a, (uint32_t)imm);
    } else {
        mx_x64_head(a, MX_X64_REX_W | 0xb8 | (d & 7u), 0, 0, d);
        mx_x64_u64(a, imm);
    }
}

void mx_x64_load(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, int32_t disp) {
    mx_x64_rm(a, MX_X64_REX_W | 0x8b, d, base, disp);
}

void mx_x64_store(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, mx_x64_reg_t s) {
    mx_x64_rm(a, MX_X64_REX_W | 0x89, s, base, disp);
}

void mx_x64_load32(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, int32_t disp) {
    mx_x64_rm(a, 0x8b, d, base, disp);
}

void mx_x64_store32(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, mx_x64_reg_t s) {
    mx_x64_rm(a, 0x89, s, base, disp);
}

void mx_x64_store_i32(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, int32_t imm) {
    mx_x64_rm(a, MX_X64_REX_W | 0xc7, 0, base, disp);
    mx_x64_u32(a, (uint32_t)imm);
}

void mx_x64_store32_i(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, int32_t imm) {
    mx_x64_rm(a, 0xc7, 0, base, disp);
    mx_x64_u32(a, (uint32_t)imm);
}

void mx_x64_store16_i(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, uint16_t imm) {
    mx_x64_rm(a, MX_X64_P66 | 0xc7, 0, base, disp);
    mx_x64_u16(a, imm);
}

void mx_x64_movzx16(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, int32_t disp) {
    mx_x64_rm(a, MX_X64_0F | 0xb7, d, base, disp);
}

void mx_x64_movzx8(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, int32_t disp) {
    mx_x64_rm(a, MX_X64_0F | 0xb6, d, base, disp);
}

void mx_x64_load_x(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, mx_x64_reg_t index, int32_t disp) {
    mx_x64_rx(a, MX_X64_REX_W | 0x8b, d, base, index, disp);
}

void mx_x64_store_x(mx_x64_t *a, mx_x64_reg_t base, mx_x64_reg_t index, int32_t disp, mx_x64_reg_t s) {
    mx_x64_rx(a, MX_X64_REX_W | 0x89, s, base, index, disp);
}

void mx_x64_movzx16_x(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, mx_x64_reg_t index, int32_t disp) {
    mx_x64_rx(a, MX_X64_0F | 0xb7, d, base, index, disp);
}

void mx_x64_lea(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, int32_t disp) {
    mx_x64_rm(a, MX_X64_REX_W | 0x8d, d, base, disp);
}

void mx_x64_lea_x(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, mx_x64_reg_t index, int32_t disp) {
    mx_x64_rx(a, MX_X64_REX_W | 0x8d, d, base, index, disp);
}

void mx_x64_lea32(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, int32_t disp) {
    mx_x64_rm(a, 0x8d, d, base, disp);
}

void mx_x64_alu_rr(mx_x64_t *a, mx_x64_alu_t op, mx_x64_reg_t d, mx_x64_reg_t s) {
    mx_x64_rr(a, MX_X64_REX_W | ((uint32_t)op << 3) | 1, s, d);
}

void mx_x64_alu_rm(mx_x64_t *a, mx_x64_alu_t op, mx_x64_reg_t d, mx_x64_reg_t base, int32_t disp) {
    mx_x64_rm(a, MX_X64_REX_W | ((uint32_t)op << 3) | 3, d, base, disp);
}

void mx_x64_alu_ri(mx_x64_t *a, mx_x64_alu_t op, mx_x64_reg_t d, int32_t imm) {
    if (imm >= -128 && imm <= 127) {
        mx_x64_rr(a, MX_X64_REX_W | 0x83, op, d);
        mx_x64_u8(a, (uint8_t)imm);
    } else {
        mx_x64_rr(a, MX_X64_REX_W | 0x81, op, d);
        mx_x64_u32(a, (uint32_t)imm);
    }
}

void mx_x64_cmp_mr(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, mx_x64_reg_t r) {
    mx_x64_rm(a, MX_X64_REX_W | 0x39, r, base, disp);
}

void mx_x64_cmp_mi(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, int32_t imm) {
    if (imm >= -128 && imm <= 127) {
        mx_x64_rm(a, MX_X64_REX_W | 0x83, MX_X64_ALU_CMP, base, disp);
        mx_x64_u8(a, (uint8_t)imm);
    } else {
        mx_x64_rm(a, MX_X64_REX_W | 0x81, MX_X64_ALU_CMP, base, disp);
        mx_x64_u32(a, (uint32_t)imm);
    }
}

void mx_x64_cmp32_rm(mx_x64_t *a, mx_x64_reg_t r, mx_x64_reg_t base, int32_t disp) {
    mx_x64_rm(a, 0x3b, r, base, disp);
}

void mx_x64_cmp32_ri(mx_x64_t *a, mx_x64_reg_t r, int32_t imm) {
    if (imm >= -128 && imm <= 127) {
        mx_x64_rr(a, 0x83, MX_X64_ALU_CMP, r);
        mx_x64_u8(a, (uint8_t)imm);
    } else {
        mx_x64_rr(a, 0x81, MX_X64_ALU_CMP, r);
        mx_x64_u32(a, (uint32_t)imm);
    }
}

void mx_x64_cmp32_mi(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, int32_t imm) {
    if (imm >= -128 && imm <= 127) {
        mx_x64_rm(a, 0x83, MX_X64_ALU_CMP, base, disp);
        mx_x64_u8(a, (uint8_t)imm);
    } else {
        mx_x64_rm(a, 0x81, MX_X64_ALU_CMP, base, disp);
        mx_x64_u32(a, (uint32_t)imm);
    }
}

void mx_x64_cmp16_mi(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, uint16_t imm) {
    if (imm <= 127) {
        mx_x64_rm(a, MX_X64_P66 | 0x83, MX_X64_ALU_CMP, base, disp);
        mx_x64_u8(a, (uint8_t)imm);
    } else {
        mx_x64_rm(a, MX_X64_P66 | 0x81, MX_X64_ALU_CMP, base, disp);
        mx_x64_u16(a, imm);
    }
}

void mx_x64_cmp8_mi(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, uint8_t imm) {
    mx_x64_rm(a, 0x80, MX_X64_ALU_CMP, base, disp);
    mx_x64_u8(a, imm);
}

void mx_x64_alu8_mi(mx_x64_t *a, mx_x64_alu_t op, mx_x64_reg_t base, int32_t disp, uint8_t imm) {
    mx_x64_rm(a, 0x80, op, base, disp);
    mx_x64_u8(a, imm);
}

void mx_x64_alu32_mi8(mx_x64_t *a, mx_x64_alu_t op, mx_x64_reg_t base, int32_t disp, int8_t imm) {
    mx_x64_rm(a, 0x83, op, base, disp);
    mx_x64_u8(a, (uint8_t)imm);
}

void mx_x64_test8_mi(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, uint8_t imm) {
    mx_x64_rm(a, 0xf6, 0, base, disp);
    mx_x64_u8(a, imm);
}

void mx_x64_test_rr(mx_x64_t *a, mx_x64_reg_t r, mx_x64_reg_t s) {
    mx_x64_rr(a, MX_X64_REX_W | 0x85, s, r);
}

void mx_x64_test8_al(mx_x64_t *a) {
    mx_x64_rr(a, 0x84, MX_X64_RAX, MX_X64_RAX);
}

void mx_x64_and32_ri(mx_x64_t *a, mx_x64_reg_t d, int32_t imm) {
    if (imm >= -128 && imm <= 127) {
        mx_x64_rr(a, 0x83, MX_X64_ALU_AND, d);
        mx_x64_u8(a, (uint8_t)imm);
    } else {
        mx_x64_rr(a, 0x81, MX_X64_ALU_AND, d);
        mx_x64_u32(a, (uint32_t)imm);
    }
}

void mx_x64_imul_rr(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t s) {
    mx_x64_rr(a, MX_X64_REX_W | MX_X64_0F | 0xaf, d, s);
}

void mx_x64_imul_rm(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t base, int32_t disp) {
    mx_x64_rm(a, MX_X64_REX_W | MX_X64_0F | 0xaf, d, base, disp);
}

void mx_x64_imul_rri(mx_x64_t *a, mx_x64_reg_t d, mx_x64_reg_t s, int32_t imm) {
    if (imm >= -128 && imm <= 127) {
        mx_x64_rr(a, MX_X64_REX_W | 0x6b, d, s);
        mx_x64_u8(a, (uint8_t)imm);
    } else {
        mx_x64_rr(a, MX_X64_REX_W | 0x69, d, s);
        mx_x64_u32(a, (uint32_t)imm);
    }
}

void mx_x64_neg(mx_x64_t *a, mx_x64_reg_t r) {
    mx_x64_rr(a, MX_X64_REX_W | 0xf7, 3, r);
}

void mx_x64_not(mx_x64_t *a, mx_x64_reg_t r) {
    mx_x64_rr(a, MX_X64_REX_W | 0xf7, 2, r);
}

void mx_x64_shift_i(mx_x64_t *a, mx_x64_shift_t op, mx_x64_reg_t r, uint8_t count) {
    mx_x64_rr(a, MX_X64_REX_W | 0xc1, op, r);
    mx_x64_u8(a, count);
}

void mx_x64_shl32_cl(mx_x64_t *a, mx_x64_reg_t r) {
    mx_x64_rr(a, 0xd3, MX_X64_SHIFT_SHL, r);
}

void mx_x64_btc_i(mx_x64_t *a, mx_x64_reg_t r, uint8_t bit) {
    mx_x64_rr(a, MX_X64_REX_W | MX_X64_0F | 0xba, 7, r);
    mx_x64_u8(a, bit);
}

void mx_x64_cqo(mx_x64_t *a) {
    mx_x64_u8(a, 0x48);
    mx_x64_u8(a, 0x99);
}

void mx_x64_idiv(mx_x64_t *a, mx_x64_reg_t r) {
    mx_x64_rr(a, MX_X64_REX_W | 0xf7, 7, r);
}

void mx_x64_push(mx_x64_t *a, mx_x64_reg_t r) {
    mx_x64_head(a, 0x50 | (r & 7u), 0, 0, r);
}

void mx_x64_pop(mx_x64_t *a, mx_x64_reg_t r) {
    mx_x64_head(a, 0x58 | (r & 7u), 0, 0, r);
}

void mx_x64_ret(mx_x64_t *a) {
    mx_x64_u8(a, 0xc3);
}

void mx_x64_call_rip(mx_x64_t *a, const void *slot) {
    mx_x64_u8(a, 0xff);
    mx_x64_u8(a, 0x15);
    mx_x64_u32(a, (uint32_t)((const uint8_t *)slot - (a->p + 4)));
}

void mx_x64_call_r(mx_x64_t *a, mx_x64_reg_t r) {
    mx_x64_rr(a, 0xff, 2, r);
}

void mx_x64_jmp_r(mx_x64_t *a, mx_x64_reg_t r) {
    mx_x64_rr(a, 0xff, 4, r);
}

void mx_x64_jmp_m(mx_x64_t *a, mx_x64_reg_t base, int32_t disp) {
    mx_x64_rm(a, 0xff, 4, base, disp);
}

void mx_x64_jmp(mx_x64_t *a, const uint8_t *target) {
    mx_x64_u8(a, 0xe9);
    mx_x64_u32(a, (uint32_t)(target - (a->p + 4)));
}

void mx_x64_jcc(mx_x64_t *a, mx_x64_cc_t cc, const uint8_t *target) {
    mx_x64_u8(a, 0x0f);
    mx_x64_u8(a, (uint8_t)(0x80 | cc));
    mx_x64_u32(a, (uint32_t)(target - (a->p + 4)));
}

void mx_x64_patch(uint8_t *site, const uint8_t *target) {
    mx_x64_t a = { .p = site - 4 };
    mx_x64_u32(&a, (uint32_t)(target - site));
}

void mx_x64_movq_xr(mx_x64_t *a, uint32_t x, mx_x64_reg_t r) {
    mx_x64_rr(a, MX_X64_P66 | MX_X64_REX_W | MX_X64_0F | 0x6e, x, r);
}

void mx_x64_movq_rx(mx_x64_t *a, mx_x64_reg_t r, uint32_t x) {
    mx_x64_rr(a, MX_X64_P66 | MX_X64_REX_W | MX_X64_0F | 0x7e, x, r);
}

void mx_x64_movsd_load(mx_x64_t *a, uint32_t x, mx_x64_reg_t base, int32_t disp) {
    mx_x64_rm(a, MX_X64_PF2 | MX_X64_0F | 0x10, x, base, disp);
}

void mx_x64_movsd_store(mx_x64_t *a, mx_x64_reg_t base, int32_t disp, uint32_t x) {
    mx_x64_rm(a, MX_X64_PF2 | MX_X64_0F | 0x11, x, base, disp);
}

void mx_x64_movaps(mx_x64_t *a, uint32_t d, uint32_t s) {
    mx_x64_rr(a, MX_X64_0F | 0x28, d, s);
}

void mx_x64_sse_rr(mx_x64_t *a, mx_x64_sse_t op, uint32_t d, uint32_t s) {
    mx_x64_rr(a, MX_X64_PF2 | MX_X64_0F | op, d, s);
}

void mx_x64_sse_rm(mx_x64_t *a, mx_x64_sse_t op, uint32_t d, mx_x64_reg_t base, int32_t disp) {
    mx_x64_rm(a, MX_X64_PF2 | MX_X64_0F | op, d, base, disp);
}

void mx_x64_ucomisd_rr(mx_x64_t *a, uint32_t l, uint32_t r) {
    mx_x64_rr(a, MX_X64_P66 | MX_X64_0F | 0x2e, l, r);
}

void mx_x64_ucomisd_rm(mx_x64_t *a, uint32_t l, mx_x64_reg_t base, int32_t disp) {
    mx_x64_rm(a, MX_X64_P66 | MX_X64_0F | 0x2e, l, base, disp);
}

void mx_x64_cvtsi2sd_rr(mx_x64_t *a, uint32_t x, mx_x64_reg_t r) {
    mx_x64_rr(a, MX_X64_PF2 | MX_X64_REX_W | MX_X64_0F | 0x2a, x, r);
}

void mx_x64_cvtsi2sd_rm(mx_x64_t *a, uint32_t x, mx_x64_reg_t base, int32_t disp) {
    mx_x64_rm(a, MX_X64_PF2 | MX_X64_REX_W | MX_X64_0F | 0x2a, x, base, disp);
}

void mx_x64_sqrtsd_rr(mx_x64_t *a, uint32_t d, uint32_t s) {
    mx_x64_rr(a, MX_X64_PF2 | MX_X64_0F | 0x51, d, s);
}

void mx_x64_cvttsd2si(mx_x64_t *a, mx_x64_reg_t r, uint32_t x) {
    mx_x64_rr(a, MX_X64_PF2 | MX_X64_REX_W | MX_X64_0F | 0x2c, r, x);
}

void mx_x64_btr_i(mx_x64_t *a, mx_x64_reg_t r, uint8_t bit) {
    mx_x64_rr(a, MX_X64_REX_W | MX_X64_0F | 0xba, 6, r);
    mx_x64_u8(a, bit);
}

void mx_x64_xorpd(mx_x64_t *a, uint32_t d, uint32_t s) {
    mx_x64_rr(a, MX_X64_P66 | MX_X64_0F | 0x57, d, s);
}
