#include "../tb_internal.h"

#ifdef TB_HAS_MIPS
// NOTE(NeGate): THIS IS VERY INCOMPLETE
#include "../emitter.h"

enum {
    //   OP reg, imm
    TILE_HAS_IMM = 1,
};

enum {
    // register classes
    REG_CLASS_GPR = 1,
    REG_CLASS_COUNT,
};

typedef enum {
    ZR,                             // zero reg.
    AT,                             // reserved for assembler.
    V0, V1,                         // returns.
    A0, A1, A2, A3,                 // call params.
    T0, T1, T2, T3, T4, T5, T6, T7, // temporaries (volatile)
    S0, S1, S2, S3, S4, S5, S6, S7, // temporaries (non-volatile)
    T8, T9,                         // temporaries (volatile)
    K0, K1,                         // kernel regs.
    GP,                             // global ptr
    SP,                             // stack ptr
    FP,                             // frame ptr
    RA,                             // return addr
} GPR;

#include "../codegen_impl.h"

static bool fits_into_int16(uint64_t x) {
    uint64_t hi = x >> 16ull;
    return hi == 0 || hi == 0xFFFFFFFFFFFF;
}

static bool try_for_imm16(int bits, TB_Node* n, int32_t* out_x) {
    if (n->type != TB_INTEGER_CONST) {
        return false;
    }

    TB_NodeInt* i = TB_NODE_GET_EXTRA(n);
    if (bits > 16) {
        bool sign = (i->value >> 15ull) & 1;
        uint64_t top = i->value >> 16ull;

        // if the sign matches the rest of the top bits, we can sign extend just fine
        if (top != (sign ? 0xFFFFFFFF : 0)) {
            return false;
        }
    }

    *out_x = i->value;
    return true;
}

static void init_ctx(Ctx* restrict ctx, TB_ABI abi) {
    ctx->sched = greedy_scheduler;
    ctx->regalloc = tb__lsra;

    uint32_t not_tmps = (1 << ZR) | (1 << AT) | (1 << SP) | (1 << K0) | (1 << K1) | (1 << GP);

    ctx->num_regs[REG_CLASS_GPR] = 32;
    ctx->normie_mask[REG_CLASS_GPR] = REGMASK(GPR, UINT32_MAX & ~not_tmps);
}

static int bits_in_type(Ctx* restrict ctx, TB_DataType dt) {
    switch (dt.type) {
        case TB_INT: return dt.data;
        case TB_PTR: return 64;
        default: tb_todo();
    }
}

static RegMask normie_mask(Ctx* restrict ctx, TB_DataType dt) {
    return ctx->normie_mask[REG_CLASS_GPR];
}

static RegMask isel_node(Ctx* restrict ctx, Tile* dst, TB_Node* n) {
    switch (n->type) {
        case TB_REGION:
        case TB_TRAP:
        case TB_CALLGRAPH:
        case TB_SPLITMEM:
        case TB_MERGEMEM:
        case TB_UNREACHABLE:
        case TB_DEBUGBREAK:
        return REGEMPTY;

        case TB_PHI:
        return normie_mask(ctx, n->dt);

        case TB_PROJ: {
            int i = TB_NODE_GET_EXTRA_T(n, TB_NodeProj)->index;
            if (n->inputs[0]->type == TB_ROOT) {
                if (i == 2) {
                    return REGMASK(GPR, 1u << RA); // RPC is the RA reg
                } else if (i >= 3) {
                    return REGMASK(GPR, 1u << ((i - 3) + A0));
                } else {
                    return REGEMPTY;
                }
            } else if (n->inputs[0]->type == TB_BRANCH) {
                return REGEMPTY;
            } else if (n->inputs[0]->type == TB_SPLITMEM) {
                return REGEMPTY;
            } else {
                tb_todo();
            }
            break;
        }

        case TB_ROOT: {
            int rets = (n->input_count - 4);

            dst->ins = tb_arena_alloc(tmp_arena, (1+rets) * sizeof(TileInput));
            dst->in_count = 1+rets;
            dst->ins[0].mask = REGMASK(GPR, 1u << RA);
            dst->ins[0].src = get_tile(ctx, n->inputs[2], true)->interval;

            assert(rets <= 2 && "At most 2 return values :(");
            if (rets >= 1) {
                dst->ins[1].mask = REGMASK(GPR, 1u << V0);
                dst->ins[1].src = get_tile(ctx, n->inputs[4], true)->interval;
            }

            if (rets >= 2) {
                dst->ins[2].mask = REGMASK(GPR, 1u << V1);
                dst->ins[2].src = get_tile(ctx, n->inputs[5], true)->interval;
            }
            return REGEMPTY;
        }

        case TB_INTEGER_CONST:
        return ctx->normie_mask[REG_CLASS_GPR];

        case TB_AND:
        case TB_OR:
        case TB_XOR:
        case TB_ADD:
        case TB_SUB:
        case TB_MUL:
        case TB_SAR:
        case TB_SHL:
        case TB_SHR: {
            int32_t x;
            if (n->type != TB_MUL && try_for_imm16(n->dt.data, n->inputs[2], &x)) {
                tile_broadcast_ins(ctx, dst, n, 1, 2, ctx->normie_mask[REG_CLASS_GPR]);
                dst->flags |= TILE_HAS_IMM;
            } else {
                tile_broadcast_ins(ctx, dst, n, 1, 3, ctx->normie_mask[REG_CLASS_GPR]);
            }
            return ctx->normie_mask[REG_CLASS_GPR];
        }

        // no rotate ops, we'll emulate it:
        case TB_ROL:
        case TB_ROR: {
            int32_t x;
            bool fits_16 = try_for_imm16(n->dt.data, n->inputs[2], &x);

            dst->in_count = fits_16 ? 3 : 2;
            dst->ins = tb_arena_alloc(tmp_arena, dst->in_count * sizeof(TileInput));
            dst->ins[0].src = get_tile(ctx, n->inputs[1], true)->interval;
            dst->ins[0].mask = ctx->normie_mask[REG_CLASS_GPR];
            dst->ins[1].src = NULL;
            dst->ins[1].mask = ctx->normie_mask[REG_CLASS_GPR];
            if (fits_16) {
                dst->flags |= TILE_HAS_IMM;
            } else {
                dst->ins[2].src = get_tile(ctx, n->inputs[1], true)->interval;
                dst->ins[2].mask = ctx->normie_mask[REG_CLASS_GPR];
            }
            return ctx->normie_mask[REG_CLASS_GPR];
        }

        case TB_SIGN_EXT:
        case TB_ZERO_EXT: {
            tile_broadcast_ins(ctx, dst, n, 1, n->input_count, ctx->normie_mask[REG_CLASS_GPR]);
            return ctx->normie_mask[REG_CLASS_GPR];
        }

        case TB_ARRAY_ACCESS: {
            tile_broadcast_ins(ctx, dst, n, 1, n->input_count, ctx->normie_mask[REG_CLASS_GPR]);
            return ctx->normie_mask[REG_CLASS_GPR];
        }

        case TB_LOAD: {
            TB_Node* base = n->inputs[2];
            int16_t offset = 0;

            if (base->type == TB_MEMBER_ACCESS) {
                uint64_t raw = TB_NODE_GET_EXTRA_T(n, TB_NodeMember)->offset;
                if (fits_into_int16(raw)) {
                    base = base->inputs[1];
                    offset = raw;
                }
            }

            dst->ins = tb_arena_alloc(tmp_arena, 2 * sizeof(TileInput));
            dst->in_count = 2;
            dst->ins[0].mask = ctx->normie_mask[REG_CLASS_GPR];
            dst->ins[0].src = get_tile(ctx, base, true)->interval;
            dst->aux = (void*) (intptr_t) offset;
            return REGEMPTY;
        }

        case TB_STORE: {
            TB_Node* base = n->inputs[2];
            int16_t offset = 0;

            if (base->type == TB_MEMBER_ACCESS) {
                uint64_t raw = TB_NODE_GET_EXTRA_T(n, TB_NodeMember)->offset;
                if (fits_into_int16(raw)) {
                    base = base->inputs[1];
                    offset = raw;
                }
            }

            dst->ins = tb_arena_alloc(tmp_arena, 2 * sizeof(TileInput));
            dst->in_count = 2;
            dst->ins[0].mask = ctx->normie_mask[REG_CLASS_GPR];
            dst->ins[1].mask = ctx->normie_mask[REG_CLASS_GPR];
            dst->ins[0].src = get_tile(ctx, base, true)->interval;
            dst->ins[1].src = get_tile(ctx, n->inputs[3], true)->interval;
            dst->aux = (void*) (intptr_t) offset;
            return REGEMPTY;
        }

        case TB_BRANCH: {
            TB_Node* cmp = n->inputs[1];
            TB_NodeBranch* br = TB_NODE_GET_EXTRA(n);

            if (br->succ_count > 2) {
                tb_todo();
            } else {
                dst->ins = tb_arena_alloc(tmp_arena, 1 * sizeof(TileInput));
                dst->in_count = 1;
                dst->ins[0].src = get_tile(ctx, cmp, true)->interval;
                dst->ins[0].mask = ctx->normie_mask[REG_CLASS_GPR];
            }
            return REGEMPTY;
        }

        default: tb_todo();
    }
}

static void pre_emit(Ctx* restrict ctx, TB_CGEmitter* e, TB_Node* n) {
    // TODO(NeGate): optionally preserve the frame pointer
    // TODO(NeGate): stack allocation
    ctx->prologue_length = ctx->emit.count;
}

#define RTYPE(op, funct) ((op<<26) | (funct))
#define ITYPE(op)        (op<<26)

enum {
    // R-types
    SLLV, SRLV, SRAV, SLL, SRL, SRA,
    AND, ADD, ADDU, SUB, SUBU, OR, XOR, NOR, JR, MUL,

    // I-type
    ADDI, ANDI, ORI, XORI, LUI,

    LB, LW, LD, // loads
    SB, SH, SW, // stores

    BEQ,

    INST_MAX,
};

static uint32_t insts[INST_MAX] = {
    // special
    [SLLV] = RTYPE(0b000000, 0b000100),
    [SRLV] = RTYPE(0b000000, 0b000110),
    [SRAV] = RTYPE(0b000000, 0b000111),
    [SLL]  = RTYPE(0b000000, 0b000000),
    [SRL]  = RTYPE(0b000000, 0b000010),
    [SRA]  = RTYPE(0b000000, 0b000011),
    [ADD]  = RTYPE(0b000000, 0b100000),
    [ADDU] = RTYPE(0b000000, 0b100001),
    [AND]  = RTYPE(0b000000, 0b100100),
    [OR]   = RTYPE(0b000000, 0b100101),
    [XOR]  = RTYPE(0b000000, 0b100110),
    [NOR]  = RTYPE(0b000000, 0b100111),
    [JR]   = RTYPE(0b000000, 0b001000),

    // special2
    [MUL] = RTYPE(0b011100, 0b000010),

    // i-types
    [ADDI] = ITYPE(0b001000),
    [ANDI] = ITYPE(0b001100),
    [ORI]  = ITYPE(0b001101),
    [XORI] = ITYPE(0b001110),
    [LUI]  = ITYPE(0b001111),

    [LB]   = ITYPE(0b100000),
    [LW]   = ITYPE(0b100011),
    [LD]   = ITYPE(0b110111),

    [SB]   = ITYPE(0b101000),
    [SH]   = ITYPE(0b101001),
    [SW]   = ITYPE(0b101011),

    [BEQ]  = ITYPE(0b000100),
};

static void nop(TB_CGEmitter* e) {
    EMIT4(e, 0);
}

static void rtype(TB_CGEmitter* e, int op, uint32_t rd, uint32_t rs, uint32_t rt, uint32_t shamt) {
    assert(op >= 0 && op < INST_MAX);
    EMIT4(e, insts[op] | (rs<<21) | (rt<<16) | (rd<<11) | (shamt<<6));
}

static void itype(TB_CGEmitter* e, int op, uint32_t rt, uint32_t rs, uint32_t imm) {
    assert(op >= 0 && op < INST_MAX);
    EMIT4(e, insts[op] | (rs<<21) | (rt<<16) | (imm&0xFFFF));
}

static void tb_emit_rel16(TB_CGEmitter* restrict e, uint32_t* head, uint32_t pos) {
    uint32_t curr = *head;
    if (curr & 0x80000000) {
        // the label target is resolved, we need to do the relocation now
        uint32_t target = curr & 0x7FFFFFFF;
        int32_t rel = target - (pos + 4);
        PATCH2(e, pos+2, rel*4);
    } else {
        PATCH2(e, pos+2, curr);
        *head = pos;
    }
}

static void tb_resolve_rel16(TB_CGEmitter* restrict e, uint32_t* head, uint32_t target) {
    // walk previous relocations
    uint32_t curr = *head;
    while (curr != 0 && (curr & 0x80000000) == 0) {
        uint32_t next;
        memcpy(&next, &e->data[curr], 4);
        int32_t rel = target - (curr + 4);
        PATCH2(e, curr+2, rel*4);
        curr = next;
    }

    // store the target and mark it as resolved
    *head = 0x80000000 | target;
}

static void branch(TB_CGEmitter* e, int op, uint32_t rt, uint32_t rs, int id) {
    assert(op == BEQ);
    EMIT4(e, insts[op] | (rs<<21) | (rt<<16));
    EMIT4(e, 0); // TODO(NeGate): delay slots

    tb_emit_rel16(e, &e->labels[id], GET_CODE_POS(e) - 8);
}

static void on_basic_block(Ctx* restrict ctx, TB_CGEmitter* e, int bbid) {
    tb_resolve_rel16(e, &e->labels[bbid], e->count);
}

static GPR gpr_at(LiveInterval* l) { assert(l->class == REG_CLASS_GPR); return l->assigned; }
static void emit_tile(Ctx* restrict ctx, TB_CGEmitter* e, Tile* t) {
    if (t->tag == TILE_SPILL_MOVE) {
        GPR dst = gpr_at(t->interval);
        GPR src = gpr_at(t->ins[0].src);
        if (dst != src) {
            rtype(e, OR, dst, src, 0, 0);
        }
    } else if (t->tag == TILE_GOTO) {
        MachineBB* mbb = node_to_bb(ctx, t->succ);
        if (ctx->fallthrough != mbb->id) {
            branch(e, BEQ, ZR, ZR, mbb->id);
        }
    } else {
        TB_Node* n = t->n;
        switch (n->type) {
            // projections don't manage their own work, that's the
            // TUPLE node's job.
            case TB_PROJ:
            case TB_REGION:
            case TB_PHI:
            case TB_POISON:
            case TB_UNREACHABLE:
            case TB_SPLITMEM:
            case TB_MERGEMEM:
            case TB_CALLGRAPH:
            break;

            case TB_ROOT: {
                rtype(e, JR, 0, RA, 0, 0);
                nop(e); // TODO(NeGate): delay slots
                break;
            }

            case TB_BRANCH: {
                GPR src = gpr_at(t->ins[0].src);
                itype(e, BEQ, src, 0, 0);
                nop(e); // TODO(NeGate): delay slots
                break;
            }

            case TB_LOAD: {
                intptr_t offset = (intptr_t) t->aux;
                GPR dst  = gpr_at(t->interval);
                GPR base = gpr_at(t->ins[0].src);
                itype(e, LW, dst, base, offset);
                break;
            }

            case TB_STORE: {
                intptr_t offset = (intptr_t) t->aux;
                GPR base = gpr_at(t->ins[0].src);
                GPR src = gpr_at(t->ins[1].src);
                itype(e, SW, src, base, offset);
                break;
            }

            case TB_INTEGER_CONST: {
                GPR dst = gpr_at(t->interval);
                uint64_t imm = TB_NODE_GET_EXTRA_T(n, TB_NodeInt)->value;

                GPR curr = ZR;
                if (imm > UINT16_MAX) {
                    itype(e, LUI, dst, curr, (imm >> 16ull) & 0xFFFF);
                    curr = dst;
                }
                itype(e, ORI, dst, curr, imm & 0xFFFF);
                break;
            }

            case TB_ARRAY_ACCESS: {
                intptr_t offset = (intptr_t) t->aux;
                GPR dst  = gpr_at(t->interval);
                GPR base = gpr_at(t->ins[0].src);
                GPR idx  = gpr_at(t->ins[1].src);
                int64_t stride = TB_NODE_GET_EXTRA_T(n, TB_NodeArray)->stride;

                uint64_t log2 = tb_ffs(stride) - 1;
                if (UINT64_C(1) << log2 == stride) {
                    rtype(e, SLL, dst, 0, idx, log2);
                } else {
                    tb_todo();
                }
                itype(e, ADD, dst, base, offset);
                break;
            }

            case TB_AND:
            case TB_OR:
            case TB_XOR:
            case TB_ADD:
            case TB_SUB:
            case TB_MUL: {
                const static int rops[] = { AND,  OR,  XOR,  ADD,  SUB,  MUL };
                const static int iops[] = { ANDI, ORI, XORI, ADDI, ADDI, -1  };

                GPR dst = gpr_at(t->interval);
                GPR lhs = gpr_at(t->ins[0].src);
                if (t->flags & TILE_HAS_IMM) {
                    assert(n->inputs[2]->type == TB_INTEGER_CONST);
                    uint64_t i = TB_NODE_GET_EXTRA_T(n->inputs[2], TB_NodeInt)->value;
                    if (n->type == TB_SUB) {
                        // no SUBI, we just negate and do ADDI
                        i = -i;
                    }

                    itype(e, iops[n->type - TB_AND], dst, lhs, i);
                } else {
                    GPR rhs = gpr_at(t->ins[1].src);
                    rtype(e, rops[n->type - TB_AND], dst, lhs, rhs, 0);
                }
                break;
            }

            case TB_ZERO_EXT:
            case TB_SIGN_EXT: {
                GPR dst = gpr_at(t->interval);
                GPR src = gpr_at(t->ins[0].src);
                int op = n->type == TB_SIGN_EXT ? SRA : SRL;

                int32_t x = 64;
                int32_t y = x - bits_in_type(ctx, n->dt);
                rtype(e, SLL, dst, 0, src, x);
                rtype(e, op,  dst, 0, src, y);
                break;
            }

            case TB_SAR:
            case TB_SHL:
            case TB_SHR: {
                const static int rops[] = { SLLV, SRLV, SRAV };
                const static int iops[] = { SLL,  SRL,  SRA  };

                GPR dst = gpr_at(t->interval);
                GPR lhs = gpr_at(t->ins[0].src);
                if (t->flags & TILE_HAS_IMM) {
                    assert(n->inputs[2]->type == TB_INTEGER_CONST);
                    uint64_t x = TB_NODE_GET_EXTRA_T(n->inputs[2], TB_NodeInt)->value;
                    rtype(e, iops[n->type - TB_SHL], dst, 0, lhs, x);
                } else {
                    GPR rhs = gpr_at(t->ins[1].src);
                    rtype(e, rops[n->type - TB_SHL], dst, lhs, rhs, 0);
                }
                break;
            }

            case TB_ROL: {
                GPR dst = gpr_at(t->interval);
                GPR src = gpr_at(t->ins[0].src);
                GPR tmp = gpr_at(t->ins[1].src);

                if (t->flags & TILE_HAS_IMM) {
                    assert(n->inputs[2]->type == TB_INTEGER_CONST);
                    uint64_t x = TB_NODE_GET_EXTRA_T(n->inputs[2], TB_NodeInt)->value;

                    int32_t y = 32 - x;
                    rtype(e, SLL, tmp, 0, src, x);   //   sll     tmp,src,X
                    rtype(e, SRL, dst, 0, src, y);   //   srl     dst,src,Y
                    rtype(e, OR,  dst, dst, tmp, 0); //   or      dst,dst,tmp
                } else {
                    tb_todo();
                }
                break;
            }

            default: tb_todo();
        }
    }
}

static void post_emit(Ctx* restrict ctx, TB_CGEmitter* e) {
}

static void disassemble(TB_CGEmitter* e, Disasm* restrict d, int bb, size_t pos, size_t end) {

}

static size_t emit_call_patches(TB_Module* restrict m, TB_FunctionOutput* out_f) {
    return 0;
}

ICodeGen tb__mips64_codegen = {
    .minimum_addressable_size = 8,
    .pointer_size = 64,
    .emit_win64eh_unwind_info = NULL,
    .emit_call_patches  = emit_call_patches,
    .get_data_type_size = get_data_type_size,
    .compile_function   = compile_function,
};
#endif
