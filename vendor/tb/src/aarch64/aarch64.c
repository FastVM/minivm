#if 0
// NOTE(NeGate): THIS IS VERY INCOMPLETE
#include "../tb_internal.h"
#include "../codegen/emitter.h"

typedef struct Ctx Ctx;

enum {
    AARCH64_REG_CLASS_GPR
};

// Xn refers to the 64bit variants of the registers,
// usually the 32bit aliases are Wn (we don't have enums
// for them because it's not that important, they're equal)
typedef enum {
    X0,  X1,   X2,  X3,  X4,  X5,  X6,  X7,
    X8,  X9,  X10, X11, X12, X13, X14, X15,
    X16, X17, X18, X19, X20, X21, X22, X23,
    X24, X25, X26, X27, X28, X29, X30,

    // It's context specific because ARM lmao
    ZR = 0x1F, SP = 0x1F,

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

typedef enum ValType {
    VAL_NONE,
    VAL_FLAGS,
    VAL_GPR,
    VAL_FPR,
    VAL_IMM,
    VAL_MEM,
    VAL_GLOBAL,
} ValType;

typedef struct {
    int8_t type;

    // if VAL_MEM then this is the base
    int8_t reg;

    uint8_t base;

    union {

        int reg;
        GPR gpr;
        uint8_t fpr;
        struct {
            bool is_rvalue;
            int32_t disp;
        } mem;

        int32_t disp;
        uint64_t num;
    };
} Val;

#include "../x64/generic_cg.h"
#include "aarch64_emitter.h"

static int classify_reg_class(TB_DataType dt) {

}

static int isel(Ctx* restrict ctx, Sequence* restrict seq, TB_Node* n) {
    /*TB_Node* restrict n = &f->nodes[r];
    TB_NodeTypeEnum type = n->type;

    switch (type) {
        case TB_INTEGER_CONST: {
            assert(n->integer.num_words == 1);
            GAD_VAL dst = GAD_FN(regalloc)(ctx, f, r, AARCH64_REG_CLASS_GPR);

            uint64_t x = n->integer.single_word;
            emit_movz(&ctx->emit, dst.reg, x & 0xFFFF, 0, true);

            uint8_t shift = 16;
            x >>= 16;

            while (x & 0xFFFF) {
                emit_movk(&ctx->emit, dst.reg, x & 0xFFFF, shift, true);
                x >>= 16, shift += 16;
            }
            return dst;
        }

        case TB_ADD: {
            GAD_VAL a = ctx->values[n->i_arith.a];
            GAD_VAL b = ctx->values[n->i_arith.b];
            GAD_VAL dst = GAD_FN(regalloc)(ctx, f, r, AARCH64_REG_CLASS_GPR);

            bool is_64bit = n->dt.type == TB_PTR || (n->dt.type == TB_INT && n->dt.data == 64);
            emit_dp_r(&ctx->emit, ADD, dst.reg, a.reg, b.reg, 0, 0, is_64bit);
            return dst;
        }

        default:
        tb_todo();
        return (GAD_VAL){ 0 };
    }*/
}

static void emit_sequence(Ctx* restrict ctx, Sequence* restrict seq, TB_Node* n) {
    assert(seq->inst_count == 0 && "TODO");
}

static void patch_local_labels(Ctx* restrict ctx) {

}

static void resolve_stack_usage(Ctx* restrict ctx, size_t caller_usage) {
    tb_todo();
}

static void copy_value(Ctx* restrict ctx, Sequence* seq, TB_Node* phi, int dst, TB_Node* src, TB_DataType dt) {
    tb_todo();
}

static size_t aarch64_emit_call_patches(TB_Module* restrict m) {
    return 0;
}

static void aarch64_return(Ctx* restrict ctx, TB_Function* f, TB_Node* restrict n) {
    GAD_VAL* src = &ctx->values[n->ret.value];

    if (src->type != VAL_GPR || src->gpr != X0) {
        emit_mov(&ctx->emit, X0, src->gpr, true);
    }
}

static void aarch64_goto(Ctx* restrict ctx, TB_Label label) {
    // jmp(&ctx->emit, label);
    tb_todo();
}

static void aarch64_ret_jmp(Ctx* restrict ctx) {
    ctx->emit.ret_patches[ctx->emit.ret_patch_count++] = GET_CODE_POS(&ctx->emit);
    EMIT4(&ctx->emit, (0b101 << 25));
}

static size_t aarch64_emit_prologue(uint8_t* out, uint64_t saved, uint64_t stack_usage) {
    return 0;
}

static size_t aarch64_emit_epilogue(uint8_t* out, uint64_t saved, uint64_t stack_usage) {
    // RET
    memcpy(out, &(uint32_t){ 0xD65F03C0 }, 4);
    return 4;
}

static void aarch64_initial_reg_alloc(Ctx* restrict ctx) {
    ctx->callee_saved[0] = 0;

    // SP/ZR is the only reserved register
    ctx->free_regs[0] = set_create(32);
    set_put(&ctx->free_regs[0], SP);
    ctx->active[0] = 0, ctx->active_count += 1;
}

static int get_data_type_size(const TB_DataType dt) {
    assert(dt.width <= 2 && "Vector width too big!");

    switch (dt.type) {
        case TB_INT: {
            // round up bits to a byte
            bool is_big_int = dt.data > 64;
            int bits = is_big_int ? ((dt.data + 7) / 8) : tb_next_pow2(dt.data);

            return ((bits+7) / 8) << dt.width;
        }
        case TB_FLOAT: {
            int s = 0;
            if (dt.data == TB_FLT_32) s = 4;
            else if (dt.data == TB_FLT_64) s = 8;
            else tb_unreachable();

            return s << dt.width;
        }
        case TB_PTR: {
            return 8;
        }
        default: {
            tb_unreachable();
            return 0;
        }
    }
}

#if _MSC_VER
_Pragma("warning (push)") _Pragma("warning (disable: 4028)")
#endif
ICodeGen tb__aarch64_codegen = {
    .minimum_addressable_size = 8,
    .pointer_size = 64,

    .get_data_type_size  = aarch64_get_data_type_size,
    .emit_call_patches   = aarch64_emit_call_patches,
    .emit_prologue       = aarch64_emit_prologue,
    .emit_epilogue       = aarch64_emit_epilogue,

    .fast_path    = aarch64_compile_function,
    //.complex_path = x64_complex_compile_function
};
#if _MSC_VER
_Pragma("warning (pop)")
#endif
#endif