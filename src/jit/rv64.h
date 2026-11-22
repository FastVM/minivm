
#pragma once

#if !defined(MX_HEADER_JIT_RV64)
#define MX_HEADER_JIT_RV64

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t mx_rv64_reg_t;
enum {
    MX_RV64_ZERO = 0,
    MX_RV64_RA = 1,
    MX_RV64_SP = 2,
    MX_RV64_GP = 3,
    MX_RV64_TP = 4,
    MX_RV64_T0 = 5,
    MX_RV64_T1 = 6,
    MX_RV64_T2 = 7,
    MX_RV64_S0 = 8,
    MX_RV64_FP = 8,
    MX_RV64_S1 = 9,
    MX_RV64_A0 = 10,
    MX_RV64_A1 = 11,
    MX_RV64_A2 = 12,
    MX_RV64_A3 = 13,
    MX_RV64_A4 = 14,
    MX_RV64_A5 = 15,
    MX_RV64_A6 = 16,
    MX_RV64_A7 = 17,
    MX_RV64_S2 = 18,
    MX_RV64_S3 = 19,
    MX_RV64_S4 = 20,
    MX_RV64_S5 = 21,
    MX_RV64_S6 = 22,
    MX_RV64_S7 = 23,
    MX_RV64_S8 = 24,
    MX_RV64_S9 = 25,
    MX_RV64_S10 = 26,
    MX_RV64_S11 = 27,
    MX_RV64_T3 = 28,
    MX_RV64_T4 = 29,
    MX_RV64_T5 = 30,
    MX_RV64_T6 = 31,
};

typedef struct mx_rv64_t mx_rv64_t;

struct mx_rv64_t {
    uint8_t *p;
};

void mx_rv64_emit32(mx_rv64_t *a, uint32_t inst);

void mx_rv64_addi(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, int32_t imm);
void mx_rv64_add(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2);
void mx_rv64_sub(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2);
void mx_rv64_mul(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2);
void mx_rv64_div(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2);
void mx_rv64_rem(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2);
void mx_rv64_and(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2);
void mx_rv64_or(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2);
void mx_rv64_xor(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2);
void mx_rv64_sll(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2);
void mx_rv64_srl(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2);
void mx_rv64_sra(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2);

void mx_rv64_ld(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, int32_t offset);
void mx_rv64_sd(mx_rv64_t *a, mx_rv64_reg_t rs2, mx_rv64_reg_t rs1, int32_t offset);

void mx_rv64_beq(mx_rv64_t *a, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2, int32_t offset);
void mx_rv64_bne(mx_rv64_t *a, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2, int32_t offset);
void mx_rv64_blt(mx_rv64_t *a, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2, int32_t offset);
void mx_rv64_bge(mx_rv64_t *a, mx_rv64_reg_t rs1, mx_rv64_reg_t rs2, int32_t offset);
void mx_rv64_jal(mx_rv64_t *a, mx_rv64_reg_t rd, int32_t offset);
void mx_rv64_jalr(mx_rv64_t *a, mx_rv64_reg_t rd, mx_rv64_reg_t rs1, int32_t offset);
void mx_rv64_ret(mx_rv64_t *a);

void mx_rv64_patch_b(uint8_t *site, const uint8_t *target);

#endif
