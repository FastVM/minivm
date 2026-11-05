
#include <stdio.h>
#include <string.h>

#include "../../src/jit/x64.h"

#define FORM(text, ...) \
    do { \
        uint8_t buf[32]; \
        mx_x64_t a_ = { .p = buf }; \
        mx_x64_t *a = &a_; \
        __VA_ARGS__; \
        printf("%s\t", text); \
        for (uint8_t *p = buf; p < a_.p; p++) { \
            printf("%02x", *p); \
        } \
        printf("\n"); \
    } while (0)

int main(void) {
    FORM("mov rax,rcx", mx_x64_mov_rr(a, MX_X64_RAX, MX_X64_RCX));
    FORM("mov r13,rax", mx_x64_mov_rr(a, MX_X64_R13, MX_X64_RAX));
    FORM("mov rax,r13", mx_x64_mov_rr(a, MX_X64_RAX, MX_X64_R13));
    FORM("mov r15,r14", mx_x64_mov_rr(a, MX_X64_R15, MX_X64_R14));
    FORM("mov ecx,ecx", mx_x64_mov32_rr(a, MX_X64_RCX, MX_X64_RCX));
    FORM("mov r9d,r9d", mx_x64_mov32_rr(a, MX_X64_R9, MX_X64_R9));
    FORM("mov eax,0x0", mx_x64_mov_ri(a, MX_X64_RAX, 0));
    FORM("mov eax,0x12345678", mx_x64_mov_ri(a, MX_X64_RAX, 0x12345678));
    FORM("mov r13d,0x12345678", mx_x64_mov_ri(a, MX_X64_R13, 0x12345678));
    FORM("mov eax,0xffffffff", mx_x64_mov_ri(a, MX_X64_RAX, 0xffffffff));
    FORM("mov rax,0xfffffffffffffffe", mx_x64_mov_ri(a, MX_X64_RAX, (uint64_t)-2));
    FORM("mov r13,0xfffffffffffffffe", mx_x64_mov_ri(a, MX_X64_R13, (uint64_t)-2));
    FORM("movabs rax,0x1122334455667788", mx_x64_mov_ri(a, MX_X64_RAX, 0x1122334455667788));
    FORM("movabs r13,0x1122334455667788", mx_x64_mov_ri(a, MX_X64_R13, 0x1122334455667788));
    FORM("mov rax,QWORD PTR [rbx]", mx_x64_load(a, MX_X64_RAX, MX_X64_RBX, 0));
    FORM("mov rax,QWORD PTR [rbx+0x8]", mx_x64_load(a, MX_X64_RAX, MX_X64_RBX, 8));
    FORM("mov rax,QWORD PTR [rbx-0x8]", mx_x64_load(a, MX_X64_RAX, MX_X64_RBX, -8));
    FORM("mov rax,QWORD PTR [rbx+0x80]", mx_x64_load(a, MX_X64_RAX, MX_X64_RBX, 128));
    FORM("mov rax,QWORD PTR [rbx+0x1000]", mx_x64_load(a, MX_X64_RAX, MX_X64_RBX, 0x1000));
    FORM("mov rax,QWORD PTR [r12]", mx_x64_load(a, MX_X64_RAX, MX_X64_R12, 0));
    FORM("mov rax,QWORD PTR [r12+0x8]", mx_x64_load(a, MX_X64_RAX, MX_X64_R12, 8));
    FORM("mov rax,QWORD PTR [r13+0x0]", mx_x64_load(a, MX_X64_RAX, MX_X64_R13, 0));
    FORM("mov rax,QWORD PTR [r13+0x8]", mx_x64_load(a, MX_X64_RAX, MX_X64_R13, 8));
    FORM("mov rax,QWORD PTR [rbp+0x0]", mx_x64_load(a, MX_X64_RAX, MX_X64_RBP, 0));
    FORM("mov rax,QWORD PTR [rsp+0x8]", mx_x64_load(a, MX_X64_RAX, MX_X64_RSP, 8));
    FORM("mov r9,QWORD PTR [r10+0x1000]", mx_x64_load(a, MX_X64_R9, MX_X64_R10, 0x1000));
    FORM("mov QWORD PTR [rbx+0x8],rax", mx_x64_store(a, MX_X64_RBX, 8, MX_X64_RAX));
    FORM("mov QWORD PTR [r12+0x1000],r13", mx_x64_store(a, MX_X64_R12, 0x1000, MX_X64_R13));
    FORM("mov eax,DWORD PTR [rbx+0x8]", mx_x64_load32(a, MX_X64_RAX, MX_X64_RBX, 8));
    FORM("mov r9d,DWORD PTR [r12+0x8]", mx_x64_load32(a, MX_X64_R9, MX_X64_R12, 8));
    FORM("mov DWORD PTR [rbx+0x8],eax", mx_x64_store32(a, MX_X64_RBX, 8, MX_X64_RAX));
    FORM("mov DWORD PTR [r13+0x8],r9d", mx_x64_store32(a, MX_X64_R13, 8, MX_X64_R9));
    FORM("mov QWORD PTR [rbx+0x8],0x7", mx_x64_store_i32(a, MX_X64_RBX, 8, 7));
    FORM("mov QWORD PTR [rsp+0x18],0x7", mx_x64_store_i32(a, MX_X64_RSP, 24, 7));
    FORM("mov DWORD PTR [r12+0x1000],0x1", mx_x64_store32_i(a, MX_X64_R12, 0x1000, 1));
    FORM("mov DWORD PTR [rbx+0x8],0x0", mx_x64_store32_i(a, MX_X64_RBX, 8, 0));
    FORM("mov WORD PTR [rbx+0x8],0x7", mx_x64_store16_i(a, MX_X64_RBX, 8, 7));
    FORM("mov WORD PTR [rsp+0x10],0x7", mx_x64_store16_i(a, MX_X64_RSP, 16, 7));
    FORM("mov WORD PTR [r13+0x1000],0x1234", mx_x64_store16_i(a, MX_X64_R13, 0x1000, 0x1234));
    FORM("movzx eax,WORD PTR [rbx+0x8]", mx_x64_movzx16(a, MX_X64_RAX, MX_X64_RBX, 8));
    FORM("movzx r9d,WORD PTR [r12+0x8]", mx_x64_movzx16(a, MX_X64_R9, MX_X64_R12, 8));
    FORM("movzx eax,BYTE PTR [rbx+0x8]", mx_x64_movzx8(a, MX_X64_RAX, MX_X64_RBX, 8));
    FORM("movzx r9d,BYTE PTR [r13+0x8]", mx_x64_movzx8(a, MX_X64_R9, MX_X64_R13, 8));
    FORM("mov rax,QWORD PTR [rbx+rcx*8]", mx_x64_load_x(a, MX_X64_RAX, MX_X64_RBX, MX_X64_RCX, 0));
    FORM("mov rax,QWORD PTR [rbx+rcx*8+0x10]", mx_x64_load_x(a, MX_X64_RAX, MX_X64_RBX, MX_X64_RCX, 16));
    FORM("mov rax,QWORD PTR [r13+r9*8+0x1000]", mx_x64_load_x(a, MX_X64_RAX, MX_X64_R13, MX_X64_R9, 0x1000));
    FORM("mov rax,QWORD PTR [r13+rcx*8+0x0]", mx_x64_load_x(a, MX_X64_RAX, MX_X64_R13, MX_X64_RCX, 0));
    FORM("mov QWORD PTR [rbx+rcx*8],rax", mx_x64_store_x(a, MX_X64_RBX, MX_X64_RCX, 0, MX_X64_RAX));
    FORM("movzx eax,WORD PTR [rbx+rcx*8]", mx_x64_movzx16_x(a, MX_X64_RAX, MX_X64_RBX, MX_X64_RCX, 0));
    FORM("lea rax,[rbx+0x8]", mx_x64_lea(a, MX_X64_RAX, MX_X64_RBX, 8));
    FORM("lea r9,[r12+0x1000]", mx_x64_lea(a, MX_X64_R9, MX_X64_R12, 0x1000));
    FORM("lea rax,[rbx+rcx*8+0x10]", mx_x64_lea_x(a, MX_X64_RAX, MX_X64_RBX, MX_X64_RCX, 16));
    FORM("lea rbx,[rsi+rcx*8+0x10]", mx_x64_lea_x(a, MX_X64_RBX, MX_X64_RSI, MX_X64_RCX, 16));
    FORM("lea edx,[rcx+0x1000]", mx_x64_lea32(a, MX_X64_RDX, MX_X64_RCX, 0x1000));
    FORM("lea eax,[rcx+0x1]", mx_x64_lea32(a, MX_X64_RAX, MX_X64_RCX, 1));

    static const struct {
        mx_x64_alu_t op;
        const char *name;
    } alus[] = {
        {MX_X64_ALU_ADD, "add"},
        {MX_X64_ALU_OR, "or"},
        {MX_X64_ALU_AND, "and"},
        {MX_X64_ALU_SUB, "sub"},
        {MX_X64_ALU_XOR, "xor"},
        {MX_X64_ALU_CMP, "cmp"},
    };
    for (size_t i = 0; i < sizeof(alus) / sizeof(alus[0]); i++) {
        char text[64];
        snprintf(text, sizeof(text), "%s rax,rcx", alus[i].name);
        FORM(text, mx_x64_alu_rr(a, alus[i].op, MX_X64_RAX, MX_X64_RCX));
        snprintf(text, sizeof(text), "%s r13,r14", alus[i].name);
        FORM(text, mx_x64_alu_rr(a, alus[i].op, MX_X64_R13, MX_X64_R14));
        snprintf(text, sizeof(text), "%s rax,QWORD PTR [rbx+0x8]", alus[i].name);
        FORM(text, mx_x64_alu_rm(a, alus[i].op, MX_X64_RAX, MX_X64_RBX, 8));
        snprintf(text, sizeof(text), "%s r13,QWORD PTR [r12+0x8]", alus[i].name);
        FORM(text, mx_x64_alu_rm(a, alus[i].op, MX_X64_R13, MX_X64_R12, 8));
        snprintf(text, sizeof(text), "%s rax,0x7", alus[i].name);
        FORM(text, mx_x64_alu_ri(a, alus[i].op, MX_X64_RAX, 7));
        snprintf(text, sizeof(text), "%s rcx,0x1000", alus[i].name);
        FORM(text, mx_x64_alu_ri(a, alus[i].op, MX_X64_RCX, 0x1000));
        snprintf(text, sizeof(text), "%s r13,0x1000", alus[i].name);
        FORM(text, mx_x64_alu_ri(a, alus[i].op, MX_X64_R13, 0x1000));
    }
    FORM("add rsp,0x108", mx_x64_alu_ri(a, MX_X64_ALU_ADD, MX_X64_RSP, 0x108));
    FORM("sub rsp,0x108", mx_x64_alu_ri(a, MX_X64_ALU_SUB, MX_X64_RSP, 0x108));
    FORM("cmp rcx,0xffffffffffffffff", mx_x64_alu_ri(a, MX_X64_ALU_CMP, MX_X64_RCX, -1));
    FORM("cmp QWORD PTR [rbx+0x8],rax", mx_x64_cmp_mr(a, MX_X64_RBX, 8, MX_X64_RAX));
    FORM("cmp QWORD PTR [r13+0x8],r14", mx_x64_cmp_mr(a, MX_X64_R13, 8, MX_X64_R14));
    FORM("cmp QWORD PTR [rbx+0x8],0x0", mx_x64_cmp_mi(a, MX_X64_RBX, 8, 0));
    FORM("cmp QWORD PTR [r12+0x1000],0x0", mx_x64_cmp_mi(a, MX_X64_R12, 0x1000, 0));
    FORM("cmp QWORD PTR [rbx+0x8],0x1000", mx_x64_cmp_mi(a, MX_X64_RBX, 8, 0x1000));
    FORM("cmp edx,DWORD PTR [rax+0x8]", mx_x64_cmp32_rm(a, MX_X64_RDX, MX_X64_RAX, 8));
    FORM("cmp r9d,DWORD PTR [r12+0x8]", mx_x64_cmp32_rm(a, MX_X64_R9, MX_X64_R12, 8));
    FORM("cmp eax,0x7", mx_x64_cmp32_ri(a, MX_X64_RAX, 7));
    FORM("cmp r9d,0x1000", mx_x64_cmp32_ri(a, MX_X64_R9, 0x1000));
    FORM("cmp DWORD PTR [rax+0x8],0x7", mx_x64_cmp32_mi(a, MX_X64_RAX, 8, 7));
    FORM("cmp DWORD PTR [r13+0x1000],0x1000", mx_x64_cmp32_mi(a, MX_X64_R13, 0x1000, 0x1000));
    FORM("cmp WORD PTR [rbx+0x8],0x7", mx_x64_cmp16_mi(a, MX_X64_RBX, 8, 7));
    FORM("cmp WORD PTR [r13+0x1000],0x1234", mx_x64_cmp16_mi(a, MX_X64_R13, 0x1000, 0x1234));
    FORM("cmp BYTE PTR [rbx+0x8],0x0", mx_x64_cmp8_mi(a, MX_X64_RBX, 8, 0));
    FORM("cmp BYTE PTR [rbx-0xc],0x0", mx_x64_cmp8_mi(a, MX_X64_RBX, -12, 0));
    FORM("cmp BYTE PTR [r12+0x1000],0x0", mx_x64_cmp8_mi(a, MX_X64_R12, 0x1000, 0));
    FORM("test BYTE PTR [rbx+0x8],0x7", mx_x64_test8_mi(a, MX_X64_RBX, 8, 7));
    FORM("test BYTE PTR [r13+0x1000],0x40", mx_x64_test8_mi(a, MX_X64_R13, 0x1000, 0x40));
    FORM("test rax,rax", mx_x64_test_rr(a, MX_X64_RAX, MX_X64_RAX));
    FORM("test r13,r13", mx_x64_test_rr(a, MX_X64_R13, MX_X64_R13));
    FORM("test al,al", mx_x64_test8_al(a));
    FORM("and eax,0x7f", mx_x64_and32_ri(a, MX_X64_RAX, 0x7f));
    FORM("and ecx,0x1000", mx_x64_and32_ri(a, MX_X64_RCX, 0x1000));
    FORM("imul rax,rcx", mx_x64_imul_rr(a, MX_X64_RAX, MX_X64_RCX));
    FORM("imul r13,r14", mx_x64_imul_rr(a, MX_X64_R13, MX_X64_R14));
    FORM("imul rax,QWORD PTR [rbx+0x8]", mx_x64_imul_rm(a, MX_X64_RAX, MX_X64_RBX, 8));
    FORM("imul rax,rcx,0x7", mx_x64_imul_rri(a, MX_X64_RAX, MX_X64_RCX, 7));
    FORM("imul r13,r14,0x1000", mx_x64_imul_rri(a, MX_X64_R13, MX_X64_R14, 0x1000));
    FORM("neg rax", mx_x64_neg(a, MX_X64_RAX));
    FORM("neg r13", mx_x64_neg(a, MX_X64_R13));
    FORM("not rax", mx_x64_not(a, MX_X64_RAX));
    FORM("not r13", mx_x64_not(a, MX_X64_R13));
    FORM("sar rax,0x3", mx_x64_shift_i(a, MX_X64_SHIFT_SAR, MX_X64_RAX, 3));
    FORM("sar r13,0x3", mx_x64_shift_i(a, MX_X64_SHIFT_SAR, MX_X64_R13, 3));
    FORM("shl rax,0x3", mx_x64_shift_i(a, MX_X64_SHIFT_SHL, MX_X64_RAX, 3));
    FORM("shl r13,0x3", mx_x64_shift_i(a, MX_X64_SHIFT_SHL, MX_X64_R13, 3));
    FORM("shr rax,0x3", mx_x64_shift_i(a, MX_X64_SHIFT_SHR, MX_X64_RAX, 3));
    FORM("shr r13,0x3", mx_x64_shift_i(a, MX_X64_SHIFT_SHR, MX_X64_R13, 3));
    FORM("shl edx,cl", mx_x64_shl32_cl(a, MX_X64_RDX));
    FORM("shl r9d,cl", mx_x64_shl32_cl(a, MX_X64_R9));
    FORM("btc rax,0x3f", mx_x64_btc_i(a, MX_X64_RAX, 63));
    FORM("btc r13,0x3f", mx_x64_btc_i(a, MX_X64_R13, 63));
    FORM("cqo", mx_x64_cqo(a));
    FORM("idiv rcx", mx_x64_idiv(a, MX_X64_RCX));
    FORM("idiv r13", mx_x64_idiv(a, MX_X64_R13));
    FORM("push rbx", mx_x64_push(a, MX_X64_RBX));
    FORM("push r12", mx_x64_push(a, MX_X64_R12));
    FORM("pop rbx", mx_x64_pop(a, MX_X64_RBX));
    FORM("pop r12", mx_x64_pop(a, MX_X64_R12));
    FORM("ret", mx_x64_ret(a));
    FORM("call rax", mx_x64_call_r(a, MX_X64_RAX));
    FORM("jmp rdx", mx_x64_jmp_r(a, MX_X64_RDX));
    FORM("jmp QWORD PTR [rax+0x8]", mx_x64_jmp_m(a, MX_X64_RAX, 8));
    FORM("call QWORD PTR [rip+0xff0]", mx_x64_call_rip(a, a->p + 6 + 0xff0));
    FORM("jmp +0x1000", mx_x64_jmp(a, a->p + 0x1000));

    static const struct {
        mx_x64_cc_t cc;
        const char *name;
    } ccs[] = {
        {MX_X64_CC_E, "je"},
        {MX_X64_CC_NE, "jne"},
        {MX_X64_CC_B, "jb"},
        {MX_X64_CC_AE, "jae"},
        {MX_X64_CC_BE, "jbe"},
        {MX_X64_CC_A, "ja"},
        {MX_X64_CC_L, "jl"},
        {MX_X64_CC_GE, "jge"},
        {MX_X64_CC_LE, "jle"},
        {MX_X64_CC_G, "jg"},
        {MX_X64_CC_P, "jp"},
        {MX_X64_CC_NP, "jnp"},
    };
    for (size_t i = 0; i < sizeof(ccs) / sizeof(ccs[0]); i++) {
        char text[32];
        snprintf(text, sizeof(text), "%s +0x1000", ccs[i].name);
        FORM(text, mx_x64_jcc(a, ccs[i].cc, a->p + 0x1000));
    }

    FORM("movq xmm0,rax", mx_x64_movq_xr(a, 0, MX_X64_RAX));
    FORM("movq xmm9,r13", mx_x64_movq_xr(a, 9, MX_X64_R13));
    FORM("movq rax,xmm0", mx_x64_movq_rx(a, MX_X64_RAX, 0));
    FORM("movq r13,xmm9", mx_x64_movq_rx(a, MX_X64_R13, 9));
    FORM("movsd xmm0,QWORD PTR [rbx+0x8]", mx_x64_movsd_load(a, 0, MX_X64_RBX, 8));
    FORM("movsd xmm9,QWORD PTR [r12+0x1000]", mx_x64_movsd_load(a, 9, MX_X64_R12, 0x1000));
    FORM("movsd QWORD PTR [rbx+0x8],xmm0", mx_x64_movsd_store(a, MX_X64_RBX, 8, 0));
    FORM("movsd QWORD PTR [r12+0x1000],xmm9", mx_x64_movsd_store(a, MX_X64_R12, 0x1000, 9));
    FORM("movsd xmm15,QWORD PTR [rip+0x1000]", mx_x64_movsd_rip(a, 15, a->p + 9 + 0x1000));
    FORM("movsd xmm1,QWORD PTR [rip+0x1000]", mx_x64_movsd_rip(a, 1, a->p + 8 + 0x1000));
    FORM("mulsd xmm1,QWORD PTR [rip+0x1000]", mx_x64_sse_rip(a, MX_X64_SSE_MUL, 1, a->p + 8 + 0x1000));
    FORM("addsd xmm9,QWORD PTR [rip+0x1000]", mx_x64_sse_rip(a, MX_X64_SSE_ADD, 9, a->p + 9 + 0x1000));
    FORM("movaps xmm0,xmm1", mx_x64_movaps(a, 0, 1));
    FORM("movaps xmm9,xmm10", mx_x64_movaps(a, 9, 10));

    static const struct {
        mx_x64_sse_t op;
        const char *name;
    } sses[] = {
        {MX_X64_SSE_ADD, "addsd"},
        {MX_X64_SSE_SUB, "subsd"},
        {MX_X64_SSE_MUL, "mulsd"},
        {MX_X64_SSE_DIV, "divsd"},
    };
    for (size_t i = 0; i < sizeof(sses) / sizeof(sses[0]); i++) {
        char text[64];
        snprintf(text, sizeof(text), "%s xmm0,xmm1", sses[i].name);
        FORM(text, mx_x64_sse_rr(a, sses[i].op, 0, 1));
        snprintf(text, sizeof(text), "%s xmm9,xmm15", sses[i].name);
        FORM(text, mx_x64_sse_rr(a, sses[i].op, 9, 15));
        snprintf(text, sizeof(text), "%s xmm0,QWORD PTR [rbx+0x8]", sses[i].name);
        FORM(text, mx_x64_sse_rm(a, sses[i].op, 0, MX_X64_RBX, 8));
        snprintf(text, sizeof(text), "%s xmm9,QWORD PTR [r12+0x1000]", sses[i].name);
        FORM(text, mx_x64_sse_rm(a, sses[i].op, 9, MX_X64_R12, 0x1000));
    }
    FORM("ucomisd xmm0,xmm1", mx_x64_ucomisd_rr(a, 0, 1));
    FORM("ucomisd xmm9,xmm15", mx_x64_ucomisd_rr(a, 9, 15));
    FORM("ucomisd xmm0,QWORD PTR [rbx+0x8]", mx_x64_ucomisd_rm(a, 0, MX_X64_RBX, 8));
    FORM("cvtsi2sd xmm0,rax", mx_x64_cvtsi2sd_rr(a, 0, MX_X64_RAX));
    FORM("cvtsi2sd xmm9,r13", mx_x64_cvtsi2sd_rr(a, 9, MX_X64_R13));
    FORM("cvtsi2sd xmm0,QWORD PTR [rbx+0x8]", mx_x64_cvtsi2sd_rm(a, 0, MX_X64_RBX, 8));
    FORM("or BYTE PTR [r12+0x10],0x2", mx_x64_alu8_mi(a, MX_X64_ALU_OR, MX_X64_R12, 0x10, 2));
    FORM("and BYTE PTR [r12+0x1000],0xfd", mx_x64_alu8_mi(a, MX_X64_ALU_AND, MX_X64_R12, 0x1000, 0xfd));
    FORM("add DWORD PTR [r12+0x10],0x1", mx_x64_alu32_mi8(a, MX_X64_ALU_ADD, MX_X64_R12, 0x10, 1));
    FORM("sub DWORD PTR [rax+0x1000],0x1", mx_x64_alu32_mi8(a, MX_X64_ALU_SUB, MX_X64_RAX, 0x1000, 1));
    FORM("sqrtsd xmm0,xmm1", mx_x64_sqrtsd_rr(a, 0, 1));
    FORM("sqrtsd xmm9,xmm15", mx_x64_sqrtsd_rr(a, 9, 15));
    FORM("cvttsd2si rax,xmm1", mx_x64_cvttsd2si(a, MX_X64_RAX, 1));
    FORM("cvttsd2si r13,xmm9", mx_x64_cvttsd2si(a, MX_X64_R13, 9));
    FORM("btr rax,0x3f", mx_x64_btr_i(a, MX_X64_RAX, 63));
    FORM("xorpd xmm0,xmm1", mx_x64_xorpd(a, 0, 1));
    FORM("xorpd xmm15,xmm15", mx_x64_xorpd(a, 15, 15));
    return 0;
}
