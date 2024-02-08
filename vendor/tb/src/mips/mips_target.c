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

typedef struct {
    TB_Node* base;
    int offset;
} AuxAddress;

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

static const char* gpr_names[32] = {
    "zr",
    "at",
    "v0", "v1",
    "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9",
    "k0", "k1",
    "gp",
    "sp",
    "fp",
    "ra",
};

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
    TB_Arch arch = ctx->module->target_arch;
    assert(arch == TB_ARCH_MIPS32 || arch == TB_ARCH_MIPS64);

    ctx->sched = greedy_scheduler;
    ctx->regalloc = tb__lsra;
    ctx->abi_index = arch == TB_ARCH_MIPS64;
    ctx->stack_slot_size = arch == TB_ARCH_MIPS64 ? 8 : 4;

    uint32_t not_tmps = (1 << ZR) | (1 << AT) | (1 << SP) | (1 << K0) | (1 << K1) | (1 << GP);

    ctx->num_regs[REG_CLASS_GPR] = 32;
    ctx->normie_mask[REG_CLASS_GPR] = REGMASK(GPR, UINT32_MAX & ~not_tmps);

    uint32_t volatile_gprs = ((UINT32_MAX >> 7) << 1) & ~(0xFF << S0);
    volatile_gprs |= 1u << RA;
    ctx->callee_saved[REG_CLASS_GPR] = ~volatile_gprs;
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

static bool is_regpair(Ctx* restrict ctx, TB_DataType dt) {
    return ctx->abi_index == 0 && dt.type == TB_INT && dt.data > 32;
}

static int reg_count(Ctx* restrict ctx, TB_Node* n) {
    if (n->dt.type == TB_INT) {
        assert(n->dt.data <= 64);
        return ctx->abi_index == 0 && n->dt.data > 32 ? 2 : 1;
    } else if (n->dt.type == TB_PTR) {
        return 1;
    } else if (n->dt.type == TB_FLOAT) {
        return 1;
    } else {
        return 0;
    }
}

#define OUT1(m) (dst->outs[0]->dt = n->dt, dst->outs[0]->mask = (m))
static void isel_node(Ctx* restrict ctx, Tile* dst, TB_Node* n) {
    switch (n->type) {
        // 0-input normie ops
        case TB_REGION:
        case TB_ROOT:
        case TB_TRAP:
        case TB_CALLGRAPH:
        case TB_SPLITMEM:
        case TB_MERGEMEM:
        case TB_UNREACHABLE:
        case TB_DEBUGBREAK:
        case TB_INTEGER_CONST:
        break;

        case TB_LOCAL:
        try_init_stack_slot(ctx, n);
        break;

        case TB_PHI:
        if (n->dt.type == TB_INT || n->dt.type == TB_PTR || n->dt.type == TB_FLOAT) {
            RegMask rm = normie_mask(ctx, n->dt);
            rm.may_spill = true;
            OUT1(rm);
        }
        return;

        case TB_PROJ: {
            if (dst->out_count) {
                RegMask rm = { 0 };
                int i = TB_NODE_GET_EXTRA_T(n, TB_NodeProj)->index;

                if (n->inputs[0]->type == TB_ROOT) {
                    if (i == 2) {
                        // RPC is the RA reg
                        rm = REGMASK(GPR, 1u << RA);
                    } else if (i >= 3) {
                        rm = REGMASK(GPR, 1u << ((i - 3) + A0));
                    }
                } else if (n->inputs[0]->type == TB_CALL) {
                    if (n->dt.type == TB_FLOAT) {
                        tb_todo();
                    } else if (i >= 2) {
                        rm = REGMASK(GPR, 1 << (V0+i));
                    }
                } else if (n->inputs[0]->type == TB_BRANCH) {
                } else if (n->inputs[0]->type == TB_SPLITMEM) {
                } else {
                    tb_todo();
                }

                OUT1(rm);
            }
            return;
        }

        case TB_RETURN: {
            int rets = (n->input_count - 3);
            bool pair = false;
            if (rets > 0 && is_regpair(ctx, n->inputs[3]->dt)) {
                assert(rets == 1 && "At most 1 doubleword return :(");
                pair = true;
                rets = 2;
            }

            dst->ins = tb_arena_alloc(tmp_arena, (1+rets) * sizeof(TileInput));
            dst->in_count = 1+rets;
            dst->ins[0].mask = REGMASK(GPR, 1u << RA);
            dst->ins[0].src = get_interval(ctx, n->inputs[2], 0);

            if (pair) {
                dst->ins[1].mask = REGMASK(GPR, 1u << V0);
                dst->ins[1].src = get_interval(ctx, n->inputs[3], 0);
                dst->ins[2].mask = REGMASK(GPR, 1u << V1);
                dst->ins[2].src = get_interval(ctx, n->inputs[3], 1);
            } else {
                assert(rets <= 2 && "At most 2 return values :(");
                if (rets >= 1) {
                    dst->ins[1].mask = REGMASK(GPR, 1u << V0);
                    dst->ins[1].src = get_interval(ctx, n->inputs[3], 0);
                }

                if (rets >= 2) {
                    dst->ins[2].mask = REGMASK(GPR, 1u << V1);
                    dst->ins[2].src = get_interval(ctx, n->inputs[4], 0);
                }
            }
            break;
        }

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
            break;
        }

        // no rotate ops, we'll emulate it:
        case TB_ROL:
        case TB_ROR: {
            int32_t x;
            bool fits_16 = try_for_imm16(n->dt.data, n->inputs[2], &x);

            dst->in_count = fits_16 ? 3 : 2;
            dst->ins = tb_arena_alloc(tmp_arena, dst->in_count * sizeof(TileInput));
            dst->ins[0].src = get_interval(ctx, n->inputs[1], 0);
            dst->ins[0].mask = ctx->normie_mask[REG_CLASS_GPR];
            dst->ins[1].src = NULL;
            dst->ins[1].mask = ctx->normie_mask[REG_CLASS_GPR];
            if (fits_16) {
                dst->flags |= TILE_HAS_IMM;
            } else {
                dst->ins[2].src = get_interval(ctx, n->inputs[1], 0);
                dst->ins[2].mask = ctx->normie_mask[REG_CLASS_GPR];
            }
            break;
        }

        case TB_SIGN_EXT:
        case TB_ZERO_EXT: {
            tile_broadcast_ins(ctx, dst, n, 1, n->input_count, ctx->normie_mask[REG_CLASS_GPR]);
            break;
        }

        case TB_ARRAY_ACCESS: {
            tile_broadcast_ins(ctx, dst, n, 1, n->input_count, ctx->normie_mask[REG_CLASS_GPR]);
            break;
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

            if (base->type == TB_LOCAL) {
                try_init_stack_slot(ctx, base);

                dst->ins = NULL;
                dst->in_count = 0;
            } else {
                dst->ins = tb_arena_alloc(tmp_arena, 1 * sizeof(TileInput));
                dst->in_count = 1;
                dst->ins[0].mask = ctx->normie_mask[REG_CLASS_GPR];
                dst->ins[0].src = get_interval(ctx, base, 0);
            }

            AuxAddress* aux = tb_arena_alloc(tmp_arena, sizeof(AuxAddress));
            aux->base = base;
            aux->offset = offset;
            dst->aux = aux;
            break;
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

            if (base->type == TB_LOCAL) {
                try_init_stack_slot(ctx, base);

                dst->ins = tb_arena_alloc(tmp_arena, 1 * sizeof(TileInput));
                dst->in_count = 1;
            } else {
                dst->ins = tb_arena_alloc(tmp_arena, 2 * sizeof(TileInput));
                dst->in_count = 2;
                dst->ins[1].mask = ctx->normie_mask[REG_CLASS_GPR];
                dst->ins[1].src = get_interval(ctx, base, 0);
            }
            dst->ins[0].mask = ctx->normie_mask[REG_CLASS_GPR];
            dst->ins[0].src = get_interval(ctx, n->inputs[3], 0);

            AuxAddress* aux = tb_arena_alloc(tmp_arena, sizeof(AuxAddress));
            aux->base = base;
            aux->offset = offset;
            dst->aux = aux;
            break;
        }

        case TB_CALL: {
            uint32_t volatile_gprs = ((UINT32_MAX >> 7) << 1) & ~(0xFF << S0);
            volatile_gprs |= 1u << RA;

            int param_count = n->input_count - 3;
            if (ctx->num_regs[0] < param_count) {
                ctx->num_regs[0] = param_count;
            }

            FOREACH_N(i, 0, param_count > 4 ? 4 : param_count) {
                volatile_gprs &= ~(1u << (A0 + i));
            }

            size_t clobber_count = tb_popcount(volatile_gprs);
            size_t input_start = n->inputs[2]->type == TB_SYMBOL ? 3 : 2;
            size_t input_count = (n->input_count - input_start) + clobber_count;

            TileInput* ins;
            if (n->inputs[2]->type == TB_SYMBOL) {
                // CALL symbol
                ins = dst->ins = tb_arena_alloc(tmp_arena, input_count * sizeof(TileInput));
                dst->in_count = input_count;
            } else {
                // CALL r/m
                ins = dst->ins = tb_arena_alloc(tmp_arena, input_count * sizeof(TileInput));
                dst->in_count = input_count;

                ins[0].src = get_interval(ctx, n->inputs[2], 0);
                ins[0].mask = ctx->normie_mask[REG_CLASS_GPR];
                ins += 1;
            }

            FOREACH_N(i, 0, param_count) {
                ins[i].src = get_interval(ctx, n->inputs[i + 3], 0);

                if (i < 4) {
                    if (TB_IS_FLOAT_TYPE(n->inputs[i + 3]->dt)) {
                        tb_todo();
                    } else {
                        ins[i].mask = REGMASK(GPR, 1u << (A0 + i));
                    }
                } else {
                    // stack slots go into [RSP + 8i]
                    ins[i].mask = REGMASK(STK, i);
                }
            }

            int j = param_count;
            FOREACH_N(i, 0, ctx->num_regs[1]) {
                if (volatile_gprs & (1u << i)) {
                    ins[j].src = NULL;
                    ins[j].mask = REGMASK(GPR, 1u << i);
                    j++;
                }
            }

            assert(j == input_count - (n->inputs[2]->type != TB_SYMBOL));
            return;
        }

        case TB_BRANCH: {
            TB_Node* cmp = n->inputs[1];
            TB_NodeBranch* br = TB_NODE_GET_EXTRA(n);

            if (br->succ_count > 2) {
                tb_todo();
            } else {
                dst->ins = tb_arena_alloc(tmp_arena, 1 * sizeof(TileInput));
                dst->in_count = 1;
                dst->ins[0].src = get_interval(ctx, cmp, 0);
                dst->ins[0].mask = ctx->normie_mask[REG_CLASS_GPR];
            }
            break;
        }

        default: tb_todo();
    }

    if (dst->out_count == 1) {
        dst->outs[0]->dt = n->dt;
        dst->outs[0]->mask = normie_mask(ctx, n->dt);
    } else if (dst->out_count == 2) {
        dst->outs[0]->dt = TB_TYPE_I32;
        dst->outs[0]->mask = normie_mask(ctx, n->dt);
        dst->outs[1]->dt = TB_TYPE_I32;
        dst->outs[1]->mask = normie_mask(ctx, n->dt);
    } else if (dst->out_count != 0) {
        tb_todo();
    }
}

enum {
    #define R(name, op, funct) name,
    #define I(name, op)        name,
    #define J(name, op)        name,
    #include "mips_insts.inc"

    INST_MAX,
};

#define __(t, op, ...) t ## type(e, op, __VA_ARGS__)
#define COMMENT(...) (e->has_comments ? tb_emit_comment(e, tmp_arena, __VA_ARGS__) : (void)0)

static uint32_t insts[INST_MAX] = {
    #define R(name, op, funct) [name] = ((op<<26) | (funct)),
    #define I(name, op)        [name] = (op<<26),
    #define J(name, op)        [name] = (op<<26),
    #include "mips_insts.inc"
};

static void nop(TB_CGEmitter* e) {
    EMIT4(e, 0);
}

static void jtype(TB_CGEmitter* e, int op, uint32_t imm) {
    EMIT4(e, insts[op] | (imm & 0x3FFFFFF));
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
    assert(op == beq);
    EMIT4(e, insts[op] | (rs<<21) | (rt<<16));
    EMIT4(e, 0); // TODO(NeGate): delay slots

    tb_emit_rel16(e, &e->labels[id], GET_CODE_POS(e) - 8);
}

static void on_basic_block(Ctx* restrict ctx, TB_CGEmitter* e, int bbid) {
    tb_resolve_rel16(e, &e->labels[bbid], e->count);
}

static void loadimm(TB_CGEmitter* e, int dst, uint32_t imm) {
    GPR curr = ZR;
    if (imm >> 16ull) {
        itype(e, lui, dst, curr, imm >> 16ull);
        curr = dst;
    }

    itype(e, ori, dst, curr, imm & 0xFFFF);
}

static int stk_offset(Ctx* ctx, int reg) {
    int pos = reg * ctx->stack_slot_size;
    if (reg >= ctx->num_regs[0]) {
        return (ctx->stack_usage - pos) + ctx->stack_slot_size;
    } else {
        return pos;
    }
}

static void pre_emit(Ctx* restrict ctx, TB_CGEmitter* e, TB_Node* n) {
    // TODO(NeGate): optionally preserve the frame pointer
    TB_FunctionPrototype* proto = ctx->f->prototype;

    size_t stack_usage = (ctx->num_regs[0] + proto->param_count) * ctx->stack_slot_size;
    ctx->stack_usage = stack_usage;

    if (stack_usage > 0) {
        __(i, addi, SP, SP, -stack_usage);
    }

    FOREACH_N(i, 0, dyn_array_length(ctx->callee_spills)) {
        int pos = stk_offset(ctx, ctx->callee_spills[i].stk);
        int rc = ctx->callee_spills[i].class;
        assert(rc == REG_CLASS_GPR);

        GPR src = ctx->callee_spills[i].reg;
        __(i, sw, src, SP, stk_offset(ctx, pos));
    }

    ctx->prologue_length = ctx->emit.count;
}

static GPR gpr_at(LiveInterval* l) { assert(l->class == REG_CLASS_GPR); return l->assigned; }
static void emit_tile(Ctx* restrict ctx, TB_CGEmitter* e, Tile* t) {
    if (t->tag == TILE_SPILL_MOVE) {
        if (t->outs[0]->class == REG_CLASS_STK && t->ins[0].src->class == REG_CLASS_GPR) {
            COMMENT("spill v%d -> v%d", t->outs[0]->id, t->ins[0].src->id);
            GPR src = gpr_at(t->ins[0].src);
            int dst = t->outs[0]->assigned;
            itype(e, sw, src, SP, stk_offset(ctx, dst));
        } else if (t->outs[0]->class == REG_CLASS_GPR && t->ins[0].src->class == REG_CLASS_STK) {
            COMMENT("reload v%d -> v%d", t->outs[0]->id, t->ins[0].src->id);
            GPR dst = gpr_at(t->outs[0]);
            int src = t->ins[0].src->assigned;
            itype(e, lw, dst, SP, stk_offset(ctx, src));
        } else {
            GPR dst = gpr_at(t->outs[0]);
            GPR src = gpr_at(t->ins[0].src);
            if (dst != src) {
                rtype(e, or, dst, src, 0, 0);
            }
        }
    } else if (t->tag == TILE_GOTO) {
        MachineBB* mbb = node_to_bb(ctx, t->succ);
        if (ctx->fallthrough != mbb->id) {
            branch(e, beq, ZR, ZR, mbb->id);
        }
    } else {
        TB_Node* n = t->n;
        switch (n->type) {
            // projections don't manage their own work, that's the
            // TUPLE node's job.
            case TB_PROJ:
            case TB_REGION:
            case TB_PHI:
            case TB_ROOT:
            case TB_POISON:
            case TB_UNREACHABLE:
            case TB_SPLITMEM:
            case TB_MERGEMEM:
            case TB_CALLGRAPH:
            break;

            case TB_RETURN: {
                // return can run in parallel to the stack free
                __(r, jr, 0, RA, 0, 0);
                if (ctx->stack_usage > 0) {
                    __(i, addi, SP, SP, ctx->stack_usage);
                } else {
                    nop(e);
                }
                break;
            }

            case TB_BRANCH: {
                GPR src = gpr_at(t->ins[0].src);
                __debugbreak();
                // branch(e, beq, src, ZR, 0);
                // branch(e, beq, src, ZR, mbb->id);
                break;
            }

            case TB_CALL: {
                if (n->inputs[2]->type == TB_SYMBOL) {
                    TB_Symbol* sym = TB_NODE_GET_EXTRA_T(n->inputs[2], TB_NodeSymbol)->sym;

                    __(j, jal, 0);
                    tb_emit_symbol_patch(e->output, sym, e->count - 4);
                } else {
                    tb_todo();
                }
                nop(e); // TODO(NeGate): delay slots
                break;
            }

            case TB_LOAD: {
                AuxAddress* aux = t->aux;
                GPR dst  = gpr_at(t->outs[0]);
                int32_t offset = aux->offset;

                GPR base;
                if (aux->base->type == TB_LOCAL) {
                    offset += get_stack_slot(ctx, aux->base);
                    base = SP;
                } else {
                    base = gpr_at(t->ins[0].src);
                }
                itype(e, lw, dst, base, offset);
                break;
            }

            case TB_STORE: {
                AuxAddress* aux = t->aux;
                GPR src = gpr_at(t->ins[0].src);
                int32_t offset = aux->offset;

                GPR base;
                if (aux->base->type == TB_LOCAL) {
                    offset += get_stack_slot(ctx, aux->base);
                    base = SP;
                } else {
                    base = gpr_at(t->ins[1].src);
                }
                itype(e, sw, src, base, offset);
                break;
            }

            case TB_LOCAL: {
                GPR dst = gpr_at(t->outs[0]);
                int pos = get_stack_slot(ctx, n);
                itype(e, addi, dst, SP, pos);
                break;
            }

            case TB_INTEGER_CONST: {
                GPR dst = gpr_at(t->outs[0]);
                uint64_t imm = TB_NODE_GET_EXTRA_T(n, TB_NodeInt)->value;

                loadimm(e, dst, imm);
                if (t->out_count == 2) {
                    GPR dst2 = gpr_at(t->outs[1]);
                    loadimm(e, dst2, imm >> 32ull);
                }
                break;
            }

            case TB_ARRAY_ACCESS: {
                intptr_t offset = (intptr_t) t->aux;
                GPR dst  = gpr_at(t->outs[0]);
                GPR base = gpr_at(t->ins[0].src);
                GPR idx  = gpr_at(t->ins[1].src);
                int64_t stride = TB_NODE_GET_EXTRA_T(n, TB_NodeArray)->stride;

                uint64_t log2 = tb_ffs(stride) - 1;
                if (UINT64_C(1) << log2 == stride) {
                    rtype(e, sll, dst, 0, idx, log2);
                } else {
                    tb_todo();
                }
                rtype(e, add, dst, base, offset, 0);
                break;
            }

            case TB_AND:
            case TB_OR:
            case TB_XOR:
            case TB_ADD:
            case TB_SUB:
            case TB_MUL: {
                const static int rops[] = { and,  or,  xor,  add,  sub,  mul };
                const static int iops[] = { andi, ori, xori, addi, addi, -1  };

                GPR dst = gpr_at(t->outs[0]);
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
                GPR dst = gpr_at(t->outs[0]);
                GPR src = gpr_at(t->ins[0].src);
                int op = n->type == TB_SIGN_EXT ? sra : srl;

                int32_t x = 64;
                int32_t y = x - bits_in_type(ctx, n->dt);
                rtype(e, sll, dst, 0, src, x);
                rtype(e, op,  dst, 0, src, y);
                break;
            }

            case TB_SAR:
            case TB_SHL:
            case TB_SHR: {
                const static int rops[] = { sllv, srlv, srav };
                const static int iops[] = { sll,  srl,  sra  };

                GPR dst = gpr_at(t->outs[0]);
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
                GPR dst = gpr_at(t->outs[0]);
                GPR src = gpr_at(t->ins[0].src);
                GPR tmp = gpr_at(t->ins[1].src);

                if (t->flags & TILE_HAS_IMM) {
                    assert(n->inputs[2]->type == TB_INTEGER_CONST);
                    uint64_t x = TB_NODE_GET_EXTRA_T(n->inputs[2], TB_NodeInt)->value;

                    int32_t y = 32 - x;
                    rtype(e, sll, tmp, 0, src, x);   //   sll     tmp,src,X
                    rtype(e, srl, dst, 0, src, y);   //   srl     dst,src,Y
                    rtype(e, or,  dst, dst, tmp, 0); //   or      dst,dst,tmp
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

typedef struct {
    const char* name;
    int op;
    enum { ITYPE, RTYPE, JTYPE } family;
} Op;

static Op decode(uint32_t inst) {
    #define R(name, op, funct) if ((inst >> 26) == op && (inst & 0b111111) == funct) { return (Op){ #name, name, RTYPE }; }
    #define I(name, op)        if ((inst >> 26) == op) { return (Op){ #name, name, ITYPE }; }
    #define J(name, op)        if ((inst >> 26) == op) { return (Op){ #name, name, JTYPE }; }
    #include "mips_insts.inc"

    return (Op){ "error", RTYPE };
}

#define E(fmt, ...) tb_asm_print(e, fmt, ## __VA_ARGS__)
static void disassemble(TB_CGEmitter* e, Disasm* restrict d, int bb, size_t pos, size_t end) {
    if (bb >= 0) {
        E(".bb%d:\n", bb);
    }

    while (pos < end) {
        while (d->loc != d->end && d->loc->pos == pos) {
            E("  // %s : line %d\n", d->loc->file->path, d->loc->line);
            d->loc++;
        }

        uint32_t inst;
        memcpy(&inst, &e->data[pos], sizeof(uint32_t));
        pos += 4;

        // special case
        uint64_t line_start = e->total_asm;
        if (inst == 0) {
            E("  nop");
        } else if ((inst >> 26) == 0 && (inst & 0b111111) == 0b100101 && ((inst >> 16) & 0b11111) == 0) {
            int a = (inst >> 11) & 0b11111;
            int c = (inst >> 21) & 0b11111;
            E("  move $%s, $%s", gpr_names[a], gpr_names[c]);
        } else if ((inst >> 26) == 0b001101 && ((inst >> 21) & 0b11111) == 0) {
            int a = (inst >> 16) & 0b11111;
            E("  lui $%s, %d", gpr_names[a], inst & 0xFFFF);
        } else {
            // figure out what instruction type it is (everything else is trivial from there)
            Op op = decode(inst);
            E("  %s ", op.name);
            switch (op.family) {
                case RTYPE: {
                    int a = (inst >> 11) & 0b11111;
                    int b = (inst >> 16) & 0b11111;
                    int c = (inst >> 21) & 0b11111;
                    E("$%s, $%s, $%s", gpr_names[a], gpr_names[b], gpr_names[c]);
                    break;
                }

                case ITYPE: {
                    int b = (inst >> 16) & 0b11111;
                    int c = (inst >> 21) & 0b11111;
                    if (op.op >= lb && op.op <= sw) {
                        E("$%s, $%s(%d)", gpr_names[b], gpr_names[c], tb__sxt(inst & 0xFFFF, 16, 64));
                    } else {
                        E("$%s, $%s, %d", gpr_names[b], gpr_names[c], tb__sxt(inst & 0xFFFF, 16, 64));
                    }
                    break;
                }

                case JTYPE: {
                    int32_t disp = (inst & 0x3FFFFFF) * 2;
                    if (d->patch && d->patch->pos == pos - 4) {
                        const TB_Symbol* target = d->patch->target;
                        d->patch = d->patch->next;

                        if (target->name[0] == 0) {
                            E("sym%p", target);
                        } else {
                            E("%s", target->name);
                        }

                        if (disp) {
                            E(" + %"PRIu64, disp);
                        }
                    } else {
                        E("%"PRIu64, disp);
                    }
                    break;
                }

                default: tb_todo();
            }
        }

        int offset = e->total_asm - line_start;
        if (d->comment && d->comment->pos == pos) {
            TB_OPTDEBUG(ANSI)(E("\x1b[32m"));
            E("  // ");
            bool out_of_line = false;
            do {
                if (out_of_line) {
                    // tack on a newline
                    E("%*s  // ", offset, "");
                }

                E("%.*s\n", d->comment->line_len, d->comment->line);
                d->comment = d->comment->next;
                out_of_line = true;
            } while  (d->comment && d->comment->pos == pos);
            TB_OPTDEBUG(ANSI)(E("\x1b[0m"));
        } else {
            E("\n");
        }
    }
}
#undef E

static size_t emit_call_patches(TB_Module* restrict m, TB_FunctionOutput* out_f) {
    return 0;
}

ICodeGen tb__mips32_codegen = {
    .minimum_addressable_size = 8,
    .pointer_size = 32,
    .emit_win64eh_unwind_info = NULL,
    .emit_call_patches  = emit_call_patches,
    .get_data_type_size = get_data_type_size,
    .compile_function   = compile_function,
};

ICodeGen tb__mips64_codegen = {
    .minimum_addressable_size = 8,
    .pointer_size = 64,
    .emit_win64eh_unwind_info = NULL,
    .emit_call_patches  = emit_call_patches,
    .get_data_type_size = get_data_type_size,
    .compile_function   = compile_function,
};
#endif
