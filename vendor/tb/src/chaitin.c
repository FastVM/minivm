// TODO(NeGate): implement Chaitin-Briggs, if you wanna contribute this would be cool to work
// with someone else on.
#include "codegen.h"

typedef struct {
    Ctx* ctx;
    TB_Arena* arena;

    int stack_usage;

    int num_classes;
    int* num_regs;
    uint64_t* callee_saved;
    RegMask* normie_mask;

    size_t ifg_len;
    uint64_t* ifg;
} Chaitin;

void tb__chaitin(Ctx* restrict ctx, TB_Arena* arena) {
    Chaitin ra = { .ctx = ctx, .arena = arena, .stack_usage = ctx->stack_usage };

    CUIK_TIMED_BLOCK("build IFG") {
        tb_todo();
    }

    ctx->stack_usage = ra.stack_usage;
}
