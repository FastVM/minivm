
#include "rv64.h"

void mx_rv64_emit32(mx_rv64_t *a, uint32_t inst) {
    a->p[0] = (uint8_t)(inst & 0xff);
    a->p[1] = (uint8_t)((inst >> 8) & 0xff);
    a->p[2] = (uint8_t)((inst >> 16) & 0xff);
    a->p[3] = (uint8_t)((inst >> 24) & 0xff);
    a->p += 4;
}

static inline uint32_t mx_rv64_r(uint32_t funct7, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t rd, uint32_t op) {
    return (funct7 << 25) | ((rs2 & 0x1f) << 20) | ((rs1 & 0x1f) << 15) | ((funct3 & 7) << 12) | ((rd & 0x1f) << 7) | (op & 0x7f);
}

static inline uint32_t mx_rv64_i(int32_t imm, uint32_t rs1, uint32_t funct3, uint32_t rd, uint32_t op) {
    return (((uint32_t)imm & 0xfff) << 20) | ((rs1 & 0x1f) << 15) | ((funct3 & 7) << 12) | ((rd & 0x1f) << 7) | (op & 0x7f);
}

static inline uint32_t mx_rv64_s(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t op) {
    uint32_t u = (uint32_t)imm;
    return (((u >> 5) & 0x7f) << 25) | ((rs2 & 0x1f) << 20) | ((rs1 & 0x1f) << 15) | ((funct3 & 7) << 12) | ((u & 0x1f) << 7) | (op & 0x7f);
}

static inline uint32_t mx_rv64_b(int32_t imm, uint32_t rs2, uint32_t rs1, uint32_t funct3, uint32_t op) {
    uint32_t u = (uint32_t)imm;
    uint32_t b12 = (u >> 12) & 1;
    uint32_t b10_5 = (u >> 5) & 0x3f;
    uint32_t b4_1 = (u >> 1) & 0x0f;
    uint32_t b11 = (u >> 11) & 1;
    return (b12 << 31) | (b10_5 << 25) | ((rs2 & 0x1f) << 20) | ((rs1 & 0x1f) << 15) | ((funct3 & 7) << 12) | (b4_1 << 8) | (b11 << 7) | (op & 0x7f);
}

static inline uint32_t mx_rv64_j(int32_t imm, uint32_t rd, uint32_t op) {
    uint32_t u = (uint32_t)imm;
    uint32_t j20 = (u >> 20) & 1;
    uint32_t j10_1 = (u >> 1) & 0x3ff;
    uint32_t j11 = (u >> 11) & 1;
    uint32_t j19_12 = (u >> 12) & 0xff;
    return (j20 << 31) | (j10_1 << 21) | (j11 << 20) | (j19_12 << 12) | ((rd & 0x1f) << 7) | (op & 0x7f);
}

void mx_rv64_addi(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, int32_t imm) {
    mx_rv64_emit32(a, mx_rv64_i(imm, rs1, 0, rd, 0x13));
}

void mx_rv64_add(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2) {
    mx_rv64_emit32(a, mx_rv64_r(0x00, rs2, rs1, 0, rd, 0x33));
}

void mx_rv64_sub(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2) {
    mx_rv64_emit32(a, mx_rv64_r(0x20, rs2, rs1, 0, rd, 0x33));
}

void mx_rv64_mul(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2) {
    mx_rv64_emit32(a, mx_rv64_r(0x01, rs2, rs1, 0, rd, 0x33));
}

void mx_rv64_div(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2) {
    mx_rv64_emit32(a, mx_rv64_r(0x01, rs2, rs1, 4, rd, 0x33));
}

void mx_rv64_rem(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2) {
    mx_rv64_emit32(a, mx_rv64_r(0x01, rs2, rs1, 6, rd, 0x33));
}

void mx_rv64_and(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2) {
    mx_rv64_emit32(a, mx_rv64_r(0x00, rs2, rs1, 7, rd, 0x33));
}

void mx_rv64_or(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2) {
    mx_rv64_emit32(a, mx_rv64_r(0x00, rs2, rs1, 6, rd, 0x33));
}

void mx_rv64_xor(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2) {
    mx_rv64_emit32(a, mx_rv64_r(0x00, rs2, rs1, 4, rd, 0x33));
}

void mx_rv64_sll(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2) {
    mx_rv64_emit32(a, mx_rv64_r(0x00, rs2, rs1, 1, rd, 0x33));
}

void mx_rv64_srl(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2) {
    mx_rv64_emit32(a, mx_rv64_r(0x00, rs2, rs1, 5, rd, 0x33));
}

void mx_rv64_sra(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2) {
    mx_rv64_emit32(a, mx_rv64_r(0x20, rs2, rs1, 5, rd, 0x33));
}

void mx_rv64_ld(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, int32_t offset) {
    mx_rv64_emit32(a, mx_rv64_i(offset, rs1, 3, rd, 0x03));
}

void mx_rv64_sd(mx_rv64_t *a, mx_rv64_reg_t rs2, mx_rv64_reg_t rs1, int32_t offset) {
    mx_rv64_emit32(a, mx_rv64_s(offset, rs2, rs1, 3, 0x23));
}

void mx_rv64_beq(mx_rv64_t *a, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2, int32_t offset) {
    mx_rv64_emit32(a, mx_rv64_b(offset, rs2, rs1, 0, 0x63));
}

void mx_rv64_bne(mx_rv64_t *a, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2, int32_t offset) {
    mx_rv64_emit32(a, mx_rv64_b(offset, rs2, rs1, 1, 0x63));
}

void mx_rv64_blt(mx_rv64_t *a, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2, int32_t offset) {
    mx_rv64_emit32(a, mx_rv64_b(offset, rs2, rs1, 4, 0x63));
}

void mx_rv64_bge(mx_rv64_t *a, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2, int32_t offset) {
    mx_rv64_emit32(a, mx_rv64_b(offset, rs2, rs1, 5, 0x63));
}

void mx_rv64_jal(mx_rv64_t *a, mx_rv64_reg_t rd, int32_t offset) {
    mx_rv64_emit32(a, mx_rv64_j(offset, rd, 0x6f));
}

void mx_rv64_jalr(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, int32_t offset) {
    mx_rv64_emit32(a, mx_rv64_i(offset, rs1, 0, rd, 0x67));
}

void mx_rv64_ret(mx_rv64_t *a) {
    mx_rv64_jalr(a, MX_RV64_ZERO, MX_RV64_RA, 0);
}

void mx_rv64_patch_b(uint8_t *site, const uint8_t *target) {
    int32_t diff = (int32_t)(target - site);
    uint32_t inst = (uint32_t)site[0] | ((uint32_t)site[1] << 8) | ((uint32_t)site[2] << 16) | ((uint32_t)site[3] << 24);
    uint32_t rs1 = (inst >> 15) & 0x1f;
    uint32_t rs2 = (inst >> 20) & 0x1f;
    uint32_t funct3 = (inst >> 12) & 7;
    uint32_t op = inst & 0x7f;
    uint32_t patched = mx_rv64_b(diff, rs2, rs1, funct3, op);
    site[0] = (uint8_t)(patched & 0xff);
    site[1] = (uint8_t)((patched >> 8) & 0xff);
    site[2] = (uint8_t)((patched >> 16) & 0xff);
    site[3] = (uint8_t)((patched >> 24) & 0xff);
}
