#include "../tb_internal.h"

#ifdef TB_HAS_AARCH64
// NOTE(NeGate): THIS IS VERY INCOMPLETE
#include "../emitter.h"
#include "aarch64_emitter.h"

enum {
    // register classes
    REG_CLASS_GPR,
    REG_CLASS_COUNT,
};

enum {
    ALL_GPRS = 0xFFFFFFFF & ~((1 << SP) | (1 << LR)),
};

#include "../codegen_impl.h"

enum {
    //   OP reg, imm
    TILE_HAS_IMM = 1
};

// true for 64bit
static bool legalize_int(TB_DataType dt) { return dt.type == TB_PTR || (dt.type == TB_INT && dt.data > 32); }

static bool try_for_imm12(int bits, TB_Node* n, int32_t* out_x) {
    if (n->type != TB_INTEGER_CONST) {
        return false;
    }

    TB_NodeInt* i = TB_NODE_GET_EXTRA(n);
    if (bits > 12) {
        bool sign = (i->value >> 11ull) & 1;
        uint64_t top = i->value >> 12ull;

        // if the sign matches the rest of the top bits, we can sign extend just fine
        if (top != (sign ? 0xFFFFFFFFFFFFF : 0)) {
            return false;
        }
    }

    *out_x = i->value;
    return true;
}

static void init_ctx(Ctx* restrict ctx, TB_ABI abi) {
    ctx->sched = greedy_scheduler;
    ctx->regalloc = tb__lsra;

    ctx->num_regs[0] = 32;

    uint32_t all_gprs = UINT32_MAX & ~(1u << SP);
    if (ctx->features.gen & TB_FEATURE_FRAME_PTR) {
        all_gprs &= ~(1u << FP);
    }
    ctx->normie_mask[0] = REGMASK(GPR, all_gprs);

    // x19 - x29 are callee saved
    ctx->callee_saved[0] = ((1u << 10u) - 1) << 19u;
}

static RegMask isel_node(Ctx* restrict ctx, Tile* dst, TB_Node* n) {
    switch (n->type) {
        case TB_ROOT: {
            TileInput* ins = tile_set_ins(ctx, dst, n, 3, n->input_count);
            int rets = n->input_count - 3;

            assert(rets <= 2 && "At most 2 return values :(");
            FOREACH_N(i, 0, rets) {
                ins[i].mask = REGMASK(GPR, 1 << i);
            }
            return REGEMPTY;
        }

        case TB_PROJ:
        int i = TB_NODE_GET_EXTRA_T(n, TB_NodeProj)->index;
        if (n->inputs[0]->type == TB_ROOT) {
            return REGMASK(GPR, i >= 3 ? (1u << (i - 3)) : 0);
        } else if (n->inputs[0]->type == TB_CALL) {
            if (i == 2)      return REGMASK(GPR, 1 << X0);
            else if (i == 3) return REGMASK(GPR, 1 << X1);
            else return REGEMPTY;
        } else if (n->inputs[0]->type == TB_BRANCH) {
            return REGEMPTY;
        } else {
            tb_todo();
        }

        case TB_ARRAY_ACCESS:
        tile_broadcast_ins(ctx, dst, n, 1, n->input_count, REGMASK(GPR, ALL_GPRS));
        return ctx->normie_mask[0];

        case TB_INTEGER_CONST:
        return ctx->normie_mask[0];

        case TB_MUL:
        case TB_UDIV:
        case TB_SDIV:
        tile_broadcast_ins(ctx, dst, n, 1, n->input_count, REGMASK(GPR, ALL_GPRS));
        return ctx->normie_mask[0];

        // binary ops
        case TB_SHL:
        case TB_SHR:
        case TB_ADD:
        case TB_SUB: {
            int32_t x;
            if (try_for_imm12(n->dt.data, n->inputs[2], &x)) {
                tile_broadcast_ins(ctx, dst, n, 1, 2, REGMASK(GPR, ALL_GPRS));
                dst->flags |= TILE_HAS_IMM;
            } else {
                tile_broadcast_ins(ctx, dst, n, 1, n->input_count, REGMASK(GPR, ALL_GPRS));
            }
            return ctx->normie_mask[0];
        }

        default:
        tb_todo();
    }
}

static bool clobbers(Ctx* restrict ctx, Tile* t, uint64_t clobbers[MAX_REG_CLASSES]) {
    return false;
}

static void pre_emit(Ctx* restrict ctx, TB_CGEmitter* e, TB_Node* n) {
    // TODO(NeGate): optionally preserve the frame pointer
    // TODO(NeGate): stack allocation
    ctx->prologue_length = ctx->emit.count;
}

static GPR gpr_at(LiveInterval* l) { assert(!l->is_spill); return l->assigned; }
static void emit_tile(Ctx* restrict ctx, TB_CGEmitter* e, Tile* t) {
    if (t->tag == TILE_SPILL_MOVE) {
        GPR dst = gpr_at(t->interval);
        GPR src = gpr_at(t->ins[0].src);
        if (dst != src) {
            emit_mov(e, dst, src, true);
        }
    } else {
        TB_Node* n = t->n;
        switch (n->type) {
            // projections don't manage their own work, that's the
            // TUPLE node's job.
            case TB_PROJ: break;

            case TB_ROOT: {
                emit_ret(e, LR);
                break;
            }

            case TB_INTEGER_CONST: {
                bool is_64bit = false;
                GPR dst = gpr_at(t->interval);
                uint64_t imm = TB_NODE_GET_EXTRA_T(n, TB_NodeInt)->value;

                int shift = 0;
                bool first = true;
                while (imm) {
                    if (imm & 0xFFFF) {
                        emit_movimm(e, dst, imm & 0xFFFF, shift / 16, is_64bit, first);
                        first = false;
                    }
                    imm >>= 16, shift += 16;
                }
                break;
            }

            case TB_SDIV: {
                bool is_64bit = legalize_int(n->dt);
                GPR dst = gpr_at(t->interval);
                GPR lhs = gpr_at(t->ins[0].src);
                GPR rhs = gpr_at(t->ins[1].src);
                emit_dp_r(e, SDIV, dst, lhs, rhs, 0, 0, is_64bit);
                break;
            }

            case TB_SHL:
            case TB_SHR:
            case TB_SAR: {
                bool is_64bit = legalize_int(n->dt);
                GPR dst = gpr_at(t->interval);
                GPR lhs = gpr_at(t->ins[0].src);
                if (t->flags & TILE_HAS_IMM) {
                    assert(n->inputs[2]->type == TB_INTEGER_CONST);
                    TB_NodeInt* i = TB_NODE_GET_EXTRA(n->inputs[2]);

                    int amt = 31 - i->value;
                    emit_bitfield(e, UBFM, dst, lhs, amt + 1, amt, is_64bit);
                } else {
                    // GPR rhs = gpr_at(t->ins[1].src);
                    tb_todo();
                }
                break;
            }

            case TB_ADD:
            case TB_SUB: {
                bool is_64bit = legalize_int(n->dt);
                GPR dst = gpr_at(t->interval);
                GPR lhs = gpr_at(t->ins[0].src);
                int op = n->type == TB_ADD ? ADD : SUB;

                if (t->flags & TILE_HAS_IMM) {
                    assert(n->inputs[2]->type == TB_INTEGER_CONST);
                    TB_NodeInt* i = TB_NODE_GET_EXTRA(n->inputs[2]);
                    emit_dp_imm(e, op, dst, lhs, i->value, 0, is_64bit);
                } else {
                    GPR rhs = gpr_at(t->ins[1].src);
                    emit_dp_r(e, op, dst, lhs, rhs, 0, 0, is_64bit);
                }
                break;
            }

            case TB_MUL: {
                GPR dst = gpr_at(t->interval);
                GPR lhs = gpr_at(t->ins[0].src);
                GPR rhs = gpr_at(t->ins[1].src);

                emit_dp3(e, MADD, dst, lhs, rhs, ZR, legalize_int(n->dt));
                break;
            }

            default: tb_todo();
        }
    }
}

static void post_emit(Ctx* restrict ctx, TB_CGEmitter* e) {
}

#define E(fmt, ...) tb_asm_print(e, fmt, ## __VA_ARGS__)
static void print_gpr_sp(TB_CGEmitter* e, int i, bool is_64bit) {
    if (i == ZR) {
        E("sp");
    } else if (i == LR) {
        E("lr");
    } else {
        E("%c%d", is_64bit["wx"], i);
    }
}

static void print_gpr(TB_CGEmitter* e, int i, bool is_64bit) {
    if (i == ZR) {
        E("zr");
    } else if (i == LR) {
        E("lr");
    } else {
        E("%c%d", is_64bit["wx"], i);
    }
}

static void disassemble(TB_CGEmitter* e, Disasm* restrict d, int bb, size_t pos, size_t end) {
    if (bb >= 0) {
        E(".bb%d:\n", bb);
    }

    static const char* dp_strs[] = { "add", "adds", "sub", "subs" };
    static const char* sh_strs[] = { "lsl", "lsr", "asr", "ror" };

    while (pos < end) {
        uint32_t inst = *((uint32_t*) &e->data[pos]);
        pos += 4;

        bool is_64bit = inst >> 31u;
        uint32_t family = (inst >> 25u) & 0xF;
        switch (family) {
            // Data processing (immediate)
            case 0b1000:
            case 0b1001: {
                uint32_t op1 = (inst >> 23u) & 0b111;
                uint32_t opc = (inst >> 29u) & 0b11;

                if (op1 == 2) {
                    uint32_t imm  = (inst >> 10u) & 0xFFF;
                    uint32_t rn   = (inst >> 5u) & 0x1F;
                    uint32_t rd   = (inst >> 0u) & 0x1F;

                    E("  %s ", dp_strs[opc]);
                    print_gpr(e, rd, is_64bit);
                    E(", ");
                    print_gpr(e, rn, is_64bit);
                    E(", #%#x", imm);
                } else if (op1 == 5) {
                    // Move wide
                    if (opc == 1) {
                        E("  ERROR ");
                    } else {
                        const char* str = "n_zk";
                        uint32_t hw  = (inst >> 21u) & 3;
                        uint32_t imm = (inst >> 5u) & 0xFFFF;
                        uint32_t rn  = inst & 0x1F;

                        E("  mov%c ", str[opc]);
                        print_gpr(e, rn, is_64bit);
                        E(", #%#x", imm);
                        if (hw) {
                            E(", lsl #%d", hw*16);
                        }
                    }
                } else if (op1 == 6) {
                    // Bitfield
                    static const char* strs[] = { "sbfm", "bfm", "ubfm", "error" };
                    uint32_t immr = (inst >> 16u) & 0b111111;
                    uint32_t imms = (inst >> 10u) & 0b111111;
                    uint32_t rn   = (inst >> 5u) & 0x1F;
                    uint32_t rd   = (inst >> 0u) & 0x1F;

                    if (opc == 2 && immr == imms + 1) {
                        // lsl
                        int amt = (is_64bit ? 63 : 31) - imms;
                        E("  lsl ");
                        print_gpr(e, rd, is_64bit);
                        E(", #%#x", amt);
                    } else {
                        E("  %s ", strs[opc]);
                        print_gpr(e, rd, is_64bit);
                        E(", ");
                        print_gpr(e, rn, is_64bit);
                        E(", #%#x, #%#x", immr, imms);
                    }
                } else {
                    E("  DATA IMM ");
                }
                break;
            }

            // Data processing (register)
            case 0b0101:
            case 0b1101: {
                uint32_t op0 = (inst >> 30u) & 1;
                uint32_t op1 = (inst >> 28u) & 1;
                uint32_t op2 = (inst >> 21u) & 0xF;
                uint32_t rm  = (inst >> 16u) & 0x1F;
                uint32_t rn  = (inst >> 5u) & 0x1F;
                uint32_t rd  = (inst >> 0u) & 0x1F;

                if (op0 == 0 && op1 == 1 && op2 == 0b0110) {
                    // uint32_t S = (inst >> 29u) & 1;
                    uint32_t opc  = (inst >> 10u) & 0x3F;

                    if (opc == 2) {
                        E("  udiv ");
                    } else if (opc == 3) {
                        E("  sdiv ");
                    } else {
                        E("ERROR ");
                    }
                    print_gpr(e, rd, is_64bit);
                    E(", ");
                    print_gpr(e, rn, is_64bit);
                    E(", ");
                    print_gpr(e, rm, is_64bit);
                } else if (op1 == 0 && (op2 & 0b1001) == 0b1000) {
                    uint32_t opc  = (inst >> 29u) & 3;
                    uint32_t imm  = (inst >> 10u) & 0x3F;
                    uint32_t sh   = (inst >> 22u) & 3;

                    E("  %s ", dp_strs[opc]);
                    print_gpr(e, rd, is_64bit);
                    E(", ");
                    print_gpr(e, rn, is_64bit);
                    E(", ");
                    print_gpr(e, rm, is_64bit);
                    if (imm) {
                        E(", %s #%#x", sh_strs[sh], imm);
                    }
                } else if (op1 == 1 && op2 >> 3) {
                    // Data processing (3 source)
                    uint32_t ra = (inst >> 10u) & 0x1F;

                    if ((inst >> 29u) & 3) {
                        E("ERROR");
                    } else {
                        uint32_t opc = (((inst >> 21u) & 7) << 1) | ((inst >> 15u) & 1);
                        switch (opc) {
                            case 0b0000: E("  madd "); break;
                            case 0b0001: E("  msub "); break;
                            default: tb_todo();
                        }
                    }
                    print_gpr(e, rd, is_64bit);
                    E(", ");
                    print_gpr(e, rn, is_64bit);
                    E(", ");
                    print_gpr(e, rm, is_64bit);
                    E(", ");
                    print_gpr(e, ra, is_64bit);
                } else {
                    E("  DATA REG ");
                }
                break;
            }

            // Branches, Exceptions, Syscalls
            case 0b1010:
            case 0b1011: {
                uint32_t op0 = (inst >> 29u) & 0b111;
                uint32_t op1 = (inst >> 12u) & 0b11111111111111;

                if (op0 == 0b110 && op1 >> 13u) {
                    // Unconditional
                    //
                    //    opc   op2    op3   op4
                    //   0010 11111 000000 00000  RET
                    uint32_t opc = (inst >> 21u) & 0xF;
                    uint32_t op2 = (inst >> 16u) & 0x1F;
                    uint32_t op3 = (inst >> 10u) & 0x1F;
                    uint32_t rn  = (inst >>  5u) & 0x1F;
                    uint32_t op4 = (inst >>  0u) & 0x1F;
                    if (opc == 2 && op2 == 0x1F && op3 == 0 && op4 == 0) {
                        if (rn == LR) { E("  ret"); }
                        else  { E("  ret v%d", rn); }
                    } else {
                        tb_todo();
                    }
                } else {
                    E("  BRANCH(op1=%#x) ", op1);
                }
                break;
            }

            default: tb_todo();
        }
        E("\n");
    }
}
#undef E

static size_t emit_call_patches(TB_Module* restrict m, TB_FunctionOutput* out_f) {
    return 0;
}

ICodeGen tb__aarch64_codegen = {
    .minimum_addressable_size = 8,
    .pointer_size = 64,
    .emit_win64eh_unwind_info = NULL,
    .emit_call_patches  = emit_call_patches,
    .get_data_type_size = get_data_type_size,
    .compile_function   = compile_function,
};
#else
ICodeGen tb__aarch64_codegen;
#endif
