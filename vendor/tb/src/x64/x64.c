#include "x64.h"
#include "x64_emitter.h"
#include "../objects/win64eh.h"

#include <tb_x64.h>
#include "x64_disasm.c"

enum {
    CG_REGISTER_CLASSES = 2
};

enum {
    REG_CLASS_GPR,
    REG_CLASS_XMM,

    FIRST_GPR = 0,
    FIRST_XMM = 16, // we're getting more GPRs in intel APX so this might change :)
};

static const struct ParamDescriptor {
    int gpr_count;
    int xmm_count;
    uint16_t caller_saved_xmms; // XMM0 - XMMwhatever
    uint16_t caller_saved_gprs; // bitfield

    GPR gprs[6];
} param_descs[] = {
    // win64
    { 4, 4, 6, WIN64_ABI_CALLER_SAVED,   { RCX, RDX, R8,  R9,  0,  0 } },
    // system v
    { 6, 4, 5, SYSV_ABI_CALLER_SAVED,    { RDI, RSI, RDX, RCX, R8, R9 } },
    // syscall
    { 6, 4, 5, SYSCALL_ABI_CALLER_SAVED, { RDI, RSI, RDX, R10, R8, R9 } },
};

#include "../generic_cg.h"

static size_t emit_prologue(Ctx* restrict ctx);
static void emit_epilogue(Ctx* restrict ctx);

// initialize register allocator state
static void init_regalloc(Ctx* restrict ctx) {
    // Generate intervals for physical registers
    FOREACH_N(i, 0, 32) {
        bool is_gpr = i < 16;
        int reg = i % 16;

        dyn_array_put(ctx->intervals, (LiveInterval){
                .reg_class = is_gpr ? REG_CLASS_GPR : REG_CLASS_XMM,
                .dt = is_gpr ? TB_X86_TYPE_QWORD : TB_X86_TYPE_XMMWORD,
                .reg = reg, .assigned = reg, .hint = -1, .split_kid = -1,
            });

        LiveInterval* it = &ctx->intervals[i];
        it->ranges = tb_platform_heap_alloc(4 * sizeof(LiveRange));
        it->range_count = 1;
        it->range_cap = 4;
        it->ranges[0] = (LiveRange){ INT_MAX, INT_MAX };
    }
}

static void mark_callee_saved_constraints(Ctx* restrict ctx, uint64_t callee_saved[CG_REGISTER_CLASSES]) {
    bool is_sysv = (ctx->target_abi == TB_ABI_SYSTEMV);
    const struct ParamDescriptor* restrict desc = &param_descs[is_sysv ? 1 : 0];

    // don't include RBP and RSP, those are special cases
    uint32_t callee_saved_gprs = ~desc->caller_saved_gprs;
    callee_saved_gprs &= ~(1u << RBP);
    callee_saved_gprs &= ~(1u << RSP);
    callee_saved[0] = callee_saved_gprs;

    // mark XMM callees
    callee_saved[1] = 0;
    FOREACH_N(i, desc->caller_saved_xmms, 16) {
        callee_saved[1] |= (1ull << i);
    }
}

// *out_mask of 0 means no mask
static TB_X86_DataType legalize_int(TB_DataType dt, uint64_t* out_mask) {
    assert(dt.type == TB_INT || dt.type == TB_PTR);
    if (dt.type == TB_PTR) return *out_mask = 0, TB_X86_TYPE_QWORD;

    TB_X86_DataType t = TB_X86_TYPE_NONE;
    int bits = 0;

    if (dt.data <= 8) bits = 8, t = TB_X86_TYPE_BYTE;
    else if (dt.data <= 16) bits = 16, t = TB_X86_TYPE_WORD;
    else if (dt.data <= 32) bits = 32, t = TB_X86_TYPE_DWORD;
    else if (dt.data <= 64) bits = 64, t = TB_X86_TYPE_QWORD;

    assert(bits != 0 && "TODO: large int support");
    uint64_t mask = dt.data == 0 ? 0 :  ~UINT64_C(0) >> (64 - dt.data);

    *out_mask = (dt.data == bits) ? 0 : mask;
    return t;
}

static TB_X86_DataType legalize_int2(TB_DataType dt) {
    uint64_t m;
    return legalize_int(dt, &m);
}

static TB_X86_DataType legalize_float(TB_DataType dt) {
    assert(dt.type == TB_FLOAT);
    return (dt.data == TB_FLT_64 ? TB_X86_TYPE_SSE_SD : TB_X86_TYPE_SSE_SS);
}

static TB_X86_DataType legalize(TB_DataType dt) {
    if (dt.type == TB_FLOAT) {
        return legalize_float(dt);
    } else {
        uint64_t m;
        return legalize_int(dt, &m);
    }
}

static int classify_reg_class(TB_DataType dt) {
    return dt.type == TB_FLOAT ? REG_CLASS_XMM : REG_CLASS_GPR;
}

static bool wont_spill_around(int t) {
    return t == INST_LABEL || t == TEST || t == CMP || t == JMP || (t >= JO && t <= JG);
}

static bool is_terminator(int t) {
    return t == INST_TERMINATOR || t == INT3 || t == UD2;
}

static bool try_for_imm32(Ctx* restrict ctx, int bits, TB_Node* n, int32_t* out_x) {
    if (n->type != TB_INTEGER_CONST) {
        return false;
    }

    TB_NodeInt* i = TB_NODE_GET_EXTRA(n);
    if (bits > 32) {
        bool sign = (i->value >> 31ull) & 1;
        uint64_t top = i->value >> 32ull;

        // if the sign matches the rest of the top bits, we can sign extend just fine
        if (top != (sign ? 0xFFFFFFFF : 0)) {
            return false;
        }
    }

    *out_x = i->value;
    return true;
}

static int get_stack_slot(Ctx* restrict ctx, TB_Node* n) {
    ptrdiff_t search = nl_map_get(ctx->stack_slots, n);
    if (search >= 0) {
        return ctx->stack_slots[search].v;
    } else {
        TB_NodeLocal* local = TB_NODE_GET_EXTRA(n);

        int pos = STACK_ALLOC(local->size, local->align);
        nl_map_put(ctx->stack_slots, n, pos);

        add_debug_local(ctx, n, pos);
        return pos;
    }
}

static Inst* inst_jmp(int target) {
    Inst* i = alloc_inst(JMP, TB_TYPE_VOID, 0, 0, 0);
    i->flags = INST_NODE;
    i->l = target;
    return i;
}

static Inst* inst_jmp_reg(int target) {
    Inst* i = alloc_inst(JMP, TB_TYPE_VOID, 0, 1, 0);
    i->operands[0] = target;
    return i;
}

static Inst* inst_jcc(int target, Cond cc) {
    Inst* i = alloc_inst(JO + cc, TB_TYPE_VOID, 0, 0, 0);
    i->flags = INST_NODE;
    i->l = target;
    return i;
}

// store(binop(load(a), b))
static int can_folded_store(Ctx* restrict ctx, TB_Node* mem, TB_Node* addr, TB_Node* src) {
    switch (src->type) {
        default: return -1;

        case TB_AND:
        case TB_OR:
        case TB_XOR:
        case TB_ADD:
        case TB_SUB: {
            if (src->inputs[1]->type == TB_LOAD &&
                src->inputs[1]->inputs[1] == mem &&
                src->inputs[1]->inputs[2] == addr &&
                on_last_use(ctx, src) &&
                on_last_use(ctx, src->inputs[1])) {
                const static InstType ops[] = { AND, OR, XOR, ADD, SUB };
                return ops[src->type - TB_AND];
            }

            return -1;
        }
    }
}

// generates an LEA for computing the address of n.
static Inst* isel_addr(Ctx* restrict ctx, TB_Node* n, int dst, int store_op, int src) {
    bool has_second_in = store_op < 0 && src >= 0;

    int64_t offset = 0;
    if (n->type == TB_SYMBOL) {
        TB_Symbol* sym = TB_NODE_GET_EXTRA_T(n, TB_NodeSymbol)->sym;
        assert(sym->tag != 0);

        Inst* i = alloc_inst(LEA, TB_TYPE_PTR, 1, 1+has_second_in, 0);
        i->flags = INST_GLOBAL;
        if (store_op < 0) {
            i->operands[0] = dst;
            if (has_second_in) {
                i->mem_slot = 2;
                i->operands[1] = src;
                i->operands[2] = RSP;
            } else {
                i->mem_slot = 1;
                i->operands[1] = RSP;
            }
        } else {
            i->type = store_op;
            i->mem_slot = 0;
            i->operands[0] = RSP;
            i->operands[1] = src;
        }
        i->s = sym;
        return i;
    } else if (n->type == TB_VA_START) {
        assert(ctx->module->target_abi == TB_ABI_WIN64 && "How does va_start even work on SysV?");

        // on Win64 va_start just means whatever is one parameter away from
        // the parameter you give it (plus in Win64 the parameters in the stack
        // are 8bytes, no fanciness like in SysV):
        // void printf(const char* fmt, ...) {
        //     va_list args;
        //     va_start(args, fmt); // args = ((char*) &fmt) + 8;
        //     ...
        // }
        offset = 8;

        use(ctx, n);
        n = n->inputs[1];
    } else if (n->type == TB_MEMBER_ACCESS) {
        offset = TB_NODE_GET_EXTRA_T(n, TB_NodeMember)->offset;

        use(ctx, n);
        n = n->inputs[1];
    }

    Scale scale = SCALE_X1;
    int index = -1;

    if (n->type == TB_ARRAY_ACCESS) {
        TB_Node* base = n->inputs[1];
        int64_t stride = TB_NODE_GET_EXTRA_T(n, TB_NodeArray)->stride;

        use(ctx, n);
        n = n->inputs[2];

        int32_t x;
        if (n->type == TB_SHL && try_for_imm32(ctx, 64, n->inputs[2], &x)) {
            use(ctx, n);
            use(ctx, n->inputs[2]);

            n = n->inputs[1];
            stride *= (1ull << x);
        }

        index = input_reg(ctx, n);
        int addr = index;

        // compute index
        if (stride == 1) {
            // no scaling required
        } else if (tb_is_power_of_two(stride)) {
            scale = tb_ffs(stride) - 1;

            if (scale > 3) {
                addr = DEF(NULL, TB_TYPE_I64);

                // we can't fit this into an LEA, might as well just do a shift
                SUBMIT(inst_op_rri(SHL, TB_TYPE_I64, addr, index, scale));
                index = addr, scale = 0;
            }
        } else {
            // needs a proper multiply (we may wanna invest in a few special patterns
            // for reducing simple multiplies into shifts)
            //
            //   a * 24 => (a * 8) * 3
            //                b    * 3 => b<<1 + b
            //
            // thus
            //
            //   LEA b,   [a * 8]
            //   LEA dst, [b * 2 + b]
            addr = DEF(NULL, TB_TYPE_I64);
            SUBMIT(inst_op_rri(IMUL, TB_TYPE_I64, addr, index, stride));
            index = addr;
        }

        n = base;
    }

    int base;
    if (n->type == TB_LOCAL) {
        use(ctx, n);
        offset += get_stack_slot(ctx, n);
        base = RBP;
    } else {
        base = input_reg(ctx, n);
    }

    // compute base
    if (store_op < 0) {
        if (has_second_in) {
            return inst_op_rrm(LEA, n->dt, dst, src, base, index, scale, offset);
        } else {
            return inst_op_rm(LEA, n->dt, dst, base, index, scale, offset);
        }
    } else {
        return inst_op_mr(store_op, n->dt, base, index, scale, offset, src);
    }
}

static Inst* isel_addr2(Ctx* restrict ctx, TB_Node* n, int dst, int store_op, int src) {
    // compute base
    if (n->type == TB_ARRAY_ACCESS && (ctx->values[n->gvn].uses >  2 || ctx->values[n->gvn].vreg >= 0)) {
        int base = input_reg(ctx, n);
        if (store_op < 0) {
            if (src >= 0) {
                return inst_op_rrm(LEA, TB_TYPE_PTR, dst, src, base, -1, SCALE_X1, 0);
            } else {
                return inst_op_rm(LEA, TB_TYPE_PTR, dst, base, -1, SCALE_X1, 0);
            }
        } else {
            return inst_op_mr(store_op, TB_TYPE_PTR, base, -1, SCALE_X1, 0, src);
        }
    } else {
        return isel_addr(ctx, n, dst, store_op, src);
    }
}

static Cond isel_cmp(Ctx* restrict ctx, TB_Node* n) {
    bool invert = false;
    if (n->type == TB_CMP_EQ && n->dt.type == TB_INT && n->dt.data == 1 && n->inputs[2]->type == TB_INTEGER_CONST) {
        TB_NodeInt* b = TB_NODE_GET_EXTRA_T(n->inputs[2], TB_NodeInt);
        if (b->value == 0) {
            invert = true;
            n = n->inputs[1];
        }
    }

    if (n->type >= TB_CMP_EQ && n->type <= TB_CMP_FLE) {
        TB_DataType cmp_dt = TB_NODE_GET_EXTRA_T(n, TB_NodeCompare)->cmp_dt;

        Cond cc = -1;
        use(ctx, n);

        if (TB_IS_FLOAT_TYPE(cmp_dt)) {
            int lhs = input_reg(ctx, n->inputs[1]);
            int rhs = input_reg(ctx, n->inputs[2]);
            SUBMIT(inst_op_rr_no_dst(FP_UCOMI, cmp_dt, lhs, rhs));

            switch (n->type) {
                case TB_CMP_EQ:  cc = E;  break;
                case TB_CMP_NE:  cc = NE; break;
                case TB_CMP_FLT: cc = B;  break;
                case TB_CMP_FLE: cc = BE; break;
                default: tb_unreachable();
            }
        } else {
            bool invert = false;
            int32_t x;
            int lhs = input_reg(ctx, n->inputs[1]);
            if (try_for_imm32(ctx, cmp_dt.type == TB_PTR ? 64 : cmp_dt.data, n->inputs[2], &x)) {
                use(ctx, n->inputs[2]);

                if (x == 0 && (n->type == TB_CMP_EQ || n->type == TB_CMP_NE)) {
                    SUBMIT(inst_op_rr_no_dst(TEST, cmp_dt, lhs, lhs));
                } else {
                    SUBMIT(inst_op_ri(CMP, cmp_dt, lhs, x));
                }
            } else if (n->inputs[2]->type == TB_LOAD && on_last_use(ctx, n->inputs[2])) {
                use(ctx, n->inputs[2]);

                Inst* inst = isel_addr2(ctx, n->inputs[2]->inputs[2], lhs, -1, lhs);
                inst->type = CMP;
                inst->dt = legalize(cmp_dt);
                SUBMIT(inst);
            } else {
                int rhs = input_reg(ctx, n->inputs[2]);
                SUBMIT(inst_op_rr_no_dst(CMP, cmp_dt, lhs, rhs));
            }

            switch (n->type) {
                case TB_CMP_EQ: cc = E; break;
                case TB_CMP_NE: cc = NE; break;
                case TB_CMP_SLT: cc = invert ? G  : L;  break;
                case TB_CMP_SLE: cc = invert ? GE : LE; break;
                case TB_CMP_ULT: cc = invert ? A  : B;  break;
                case TB_CMP_ULE: cc = invert ? NB : BE; break;
                default: tb_unreachable();
            }
        }

        return cc ^ invert;
    } else {
        int src = input_reg(ctx, n);

        TB_DataType dt = n->dt;
        if (TB_IS_FLOAT_TYPE(dt)) {
            int tmp = DEF(n, n->dt);
            SUBMIT(inst_op_zero(dt, tmp));
            SUBMIT(inst_op_rr_no_dst(FP_UCOMI, dt, src, tmp));
        } else {
            SUBMIT(inst_op_rr_no_dst(TEST, dt, src, src));
        }
        return NE ^ invert;
    }
}

static bool should_rematerialize(TB_Node* n) {
    if ((n->type == TB_INT2FLOAT || n->type == TB_INT2PTR) && n->inputs[1]->type == TB_INTEGER_CONST) {
        return true;
    }

    return (n->type == TB_PROJ && (n->dt.type == TB_CONT || n->inputs[0]->type == TB_START)) ||
        n->type == TB_FLOAT32_CONST || n->type == TB_FLOAT64_CONST ||
        n->type == TB_INTEGER_CONST || n->type == TB_MEMBER_ACCESS ||
        n->type == TB_LOCAL || n->type == TB_SYMBOL;
}

static void isel(Ctx* restrict ctx, TB_Node* n, const int dst) {
    TB_NodeTypeEnum type = n->type;
    switch (type) {
        case TB_PHI: break;
        case TB_REGION: break;

        case TB_POISON: {
            Inst* inst = alloc_inst(INST_INLINE, TB_TYPE_VOID, 1, 0, 0);
            inst->operands[0] = dst;
            append_inst(ctx, inst);
            break;
        }

        case TB_START: {
            TB_NodeRegion* start = TB_NODE_GET_EXTRA(n);
            const TB_FunctionPrototype* restrict proto = ctx->f->prototype;
            bool is_sysv = (ctx->target_abi == TB_ABI_SYSTEMV);

            const GPR* gpr_params  = is_sysv ? SYSV_GPR_PARAMETERS : WIN64_GPR_PARAMETERS;
            size_t gpr_param_count = is_sysv ? COUNTOF(SYSV_GPR_PARAMETERS) : COUNTOF(WIN64_GPR_PARAMETERS);
            int xmm_param_count    = is_sysv ? 8 : 4;

            Inst* prev = ctx->head;

            int out_count = 0;
            RegIndex outs[16];

            // handle known parameters
            int used_gpr = 0, used_xmm = 0;
            TB_Node** params = ctx->f->params;
            FOREACH_N(i, 0, ctx->f->param_count) {
                TB_Node* proj = params[3 + i];
                bool is_float = proj->dt.type == TB_FLOAT;

                // copy from parameter
                int reg_class = (is_float ? REG_CLASS_XMM : REG_CLASS_GPR);
                int id = is_float ? used_xmm : used_gpr;
                if (is_sysv) {
                    if (is_float) used_xmm += 1;
                    else used_gpr += 1;
                } else {
                    // win64 will expend the slot regardless of if it's used
                    used_gpr += 1;
                    used_xmm += 1;
                }

                int reg_limit = is_float ? xmm_param_count : gpr_param_count;
                if (id < reg_limit) {
                    ValueDesc* v = lookup_val(ctx, proj);
                    if (v != NULL) {
                        assert(v->vreg < 0 && "shouldn't have been initialized yet?");
                        v->vreg = DEF(proj, proj->dt);

                        int reg_num = is_float ? id : gpr_params[id];
                        int vreg = (is_float ? FIRST_XMM : 0) + reg_num;

                        hint_reg(ctx, v->vreg, vreg);
                        SUBMIT(inst_move(proj->dt, v->vreg, vreg));

                        outs[out_count++] = vreg;
                    }
                }
            }

            // insert INST_ENTRY (this is where parameter register come from)
            Inst* entry_inst = alloc_inst(INST_ENTRY, TB_TYPE_I64, out_count, 0, 0);
            memcpy(entry_inst->operands, outs, out_count * sizeof(RegIndex));

            entry_inst->next = prev->next;
            if (prev->next == NULL) {
                ctx->head = entry_inst;
            }
            prev->next = entry_inst;

            // walk the entry to find any parameter stack slots
            bool has_param_slots = false;
            FOREACH_N(i, 0, ctx->f->param_count) {
                TB_Node* proj = params[3 + i];
                User* use = find_users(ctx->p, proj);
                if (use == NULL || use->next != NULL || use->slot == 0) {
                    continue;
                }

                TB_Node* store_op = use->n;
                if (store_op->type != TB_STORE || tb_get_parent_region(store_op->inputs[0]) != n) {
                    continue;
                }

                TB_Node* addr = store_op->inputs[2];
                if (addr->type != TB_LOCAL) {
                    continue;
                }

                int pos = 16 + (i * 8);
                nl_map_put(ctx->stack_slots, addr, pos);

                if (i >= 4 && ctx->target_abi == TB_ABI_WIN64) {
                    // marks as visited (stores don't return so we can -1)
                    put_val(ctx, store_op, 0);
                }

                // add parameter to debug info
                add_debug_local(ctx, addr, pos);
                has_param_slots = true;
            }

            if (has_param_slots) {
                ctx->stack_usage += 16 + (proto->param_count * 8);
            } else {
                ctx->stack_usage += 16;
            }

            // Handle unknown parameters (if we have varargs)
            if (proto->has_varargs) {
                const GPR* parameter_gprs = is_sysv ? SYSV_GPR_PARAMETERS : WIN64_GPR_PARAMETERS;

                // spill the rest of the parameters (assumes they're all in the GPRs)
                size_t gpr_count = is_sysv ? 6 : 4;
                size_t extra_param_count = proto->param_count > gpr_count ? 0 : gpr_count - proto->param_count;

                FOREACH_N(i, 0, extra_param_count) {
                    size_t param_num = proto->param_count + i;

                    int dst_pos = 16 + (param_num * 8);
                    GPR src = parameter_gprs[param_num];

                    SUBMIT(inst_op_mr(MOV, TB_TYPE_I64, RBP, GPR_NONE, SCALE_X1, dst_pos, src));
                }

                ctx->stack_usage += (extra_param_count * 8);
            }
            break;
        }

        case TB_INTEGER_CONST: {
            uint64_t x = TB_NODE_GET_EXTRA_T(n, TB_NodeInt)->value;

            // mask off bits
            uint64_t bits_in_type = n->dt.type == TB_PTR ? 64 : n->dt.data;
            if (bits_in_type < 64) {
                x &= (1ull << bits_in_type) - 1;
            }

            if (x == 0) {
                SUBMIT(inst_op_zero(n->dt, dst));
            } else if ((x >> 32ull) == UINT32_MAX) {
                // mov but zero ext
                SUBMIT(inst_op_imm(MOV, TB_TYPE_I32, dst, x));
            } else if (bits_in_type <= 32 || (x >> 31ull) == 0) {
                SUBMIT(inst_op_imm(MOV, n->dt, dst, x));
            } else {
                // movabs reg, imm64
                SUBMIT(inst_op_abs(MOVABS, n->dt, dst, x));
            }
            break;
        }

        case TB_SELECT: {
            assert(n->dt.type != TB_FLOAT);
            int lhs = input_reg(ctx, n->inputs[2]);
            int rhs = input_reg(ctx, n->inputs[3]);

            Cond cc = isel_cmp(ctx, n->inputs[1]);
            SUBMIT(inst_move(n->dt, dst, rhs));
            SUBMIT(inst_op_rr(CMOVO + cc, n->dt, dst, lhs));
            break;
        }

        case TB_AND:
        case TB_OR:
        case TB_XOR:
        case TB_ADD:
        case TB_SUB: {
            const static InstType ops[] = { AND, OR, XOR, ADD, SUB };
            InstType op = ops[type - TB_AND];

            int lhs = input_reg(ctx, n->inputs[1]);
            hint_reg(ctx, dst, lhs);

            int32_t x;
            if (n->inputs[2]->type == TB_LOAD && on_last_use(ctx, n->inputs[2])) {
                use(ctx, n->inputs[2]);

                SUBMIT(inst_move(n->dt, dst, lhs));

                Inst* inst = isel_addr2(ctx, n->inputs[2]->inputs[2], dst, -1, dst);
                inst->type = op;
                inst->dt = legalize(n->dt);
                SUBMIT(inst);
            } else if (try_for_imm32(ctx, n->dt.data, n->inputs[2], &x)) {
                use(ctx, n->inputs[2]);

                if (0 && type == TB_ADD) {
                    SUBMIT(inst_op_rm(LEA, TB_TYPE_I64, dst, lhs, -1, SCALE_X1, x));
                } else {
                    SUBMIT(inst_move(n->dt, dst, lhs));
                    SUBMIT(inst_op_rri(op, n->dt, dst, dst, x));
                }
            } else {
                int rhs = input_reg(ctx, n->inputs[2]);

                SUBMIT(inst_move(n->dt, dst, lhs));
                SUBMIT(inst_op_rrr(op, n->dt, dst, dst, rhs));
            }
            break;
        }

        case TB_MUL: {
            int lhs = input_reg(ctx, n->inputs[1]);
            hint_reg(ctx, dst, lhs);

            // promote any <16bit multiplies up a bit:
            //   should be fair game to compute the multiply
            //   with garbage bits at the top as long as we
            //   don't read them.
            TB_DataType dt = n->dt;
            assert(dt.type == TB_INT);
            if (dt.data < 16) {
                dt.data = 16;
            }

            int32_t x;
            if (try_for_imm32(ctx, dt.data, n->inputs[2], &x)) {
                use(ctx, n->inputs[2]);

                SUBMIT(inst_move(dt, dst, lhs));
                SUBMIT(inst_op_rri(IMUL, dt, dst, dst, x));
            } else {
                int rhs = input_reg(ctx, n->inputs[2]);

                SUBMIT(inst_move(dt, dst, lhs));
                SUBMIT(inst_op_rrr(IMUL, dt, dst, dst, rhs));
            }
            break;
        }
        case TB_MULPAIR: {
            // returns into both lo and hi
            TB_Node* projs[2] = { 0 };
            for (User* u = n->users; u; u = u->next) {
                if (u->n->type == TB_PROJ) {
                    int index = TB_NODE_GET_EXTRA_T(u->n, TB_NodeProj)->index;
                    projs[index] = u->n;
                }
            }

            // at least one needs to be alive
            assert(projs[0] != NULL || projs[1] != NULL);

            TB_DataType dt = projs[0] ? projs[0]->dt : projs[1]->dt;
            {
                Inst* inst = alloc_inst(MUL, dt, 2, 2, 0);
                inst->operands[0] = RAX;
                inst->operands[1] = RDX;
                inst->operands[2] = RAX;
                inst->operands[3] = RDX;
                SUBMIT(inst);
            }

            if (projs[0]) SUBMIT(inst_move(dt, input_reg(ctx, projs[0]), RAX));
            if (projs[1]) SUBMIT(inst_move(dt, input_reg(ctx, projs[1]), RDX));
            break;
        }

        // bit magic
        case TB_CTZ:
        case TB_CLZ: {
            int op = type == TB_CLZ ? BSR : BSF;
            int lhs = input_reg(ctx, n->inputs[1]);
            hint_reg(ctx, dst, lhs);

            // we only wanna deal with 32 or 64 ops for
            // this (16 is annoying and 8 is unavailable)
            TB_DataType dt = n->dt;
            if (dt.data < 64) {
                // make sure the bits are zero'd above
                if (dt.data < 32) {
                    assert(type == TB_CLZ && "clz is different, and im stupid");
                    SUBMIT(inst_op_zero(TB_TYPE_I32, dst));
                }

                dt.data = 32;
            }

            Inst* inst = inst_op_rr(op, dt, dst, lhs);
            if (type == TB_CLZ) {
                // the difference between bsf and tzcnt
                inst->flags |= INST_REP;
            }
            SUBMIT(inst);

            // flip bits to make CLZ instead of bitscanreverse
            SUBMIT(inst_op_rri(XOR, dt, dst, dst, 63));
            break;
        }

        // bit shifts
        case TB_SHL:
        case TB_SHR:
        case TB_SAR:
        case TB_ROL:
        case TB_ROR: {
            const static InstType ops[] = { SHL, SHR, SAR, ROL, ROR };
            InstType op = ops[type - TB_SHL];

            int lhs = input_reg(ctx, n->inputs[1]);
            hint_reg(ctx, dst, lhs);

            int32_t x;
            if (try_for_imm32(ctx, n->inputs[2]->dt.data, n->inputs[2], &x) && x >= 0 && x < 64) {
                use(ctx, n->inputs[2]);
                SUBMIT(inst_move(n->dt, dst, lhs));
                SUBMIT(inst_op_rri(op, n->dt, dst, dst, x));
                break;
            }

            // the shift operations need their right hand side in CL (RCX's low 8bit)
            int rhs = input_reg(ctx, n->inputs[2]);

            SUBMIT(inst_move(n->dt, dst, lhs));
            SUBMIT(inst_move(n->dt, RCX, rhs));
            SUBMIT(inst_op_rrr_tmp(op, n->dt, dst, dst, RCX, RCX));
            break;
        }

        case TB_UDIV:
        case TB_SDIV:
        case TB_UMOD:
        case TB_SMOD: {
            bool is_signed = (type == TB_SDIV || type == TB_SMOD);
            bool is_div    = (type == TB_UDIV || type == TB_SDIV);

            TB_DataType dt = n->dt;
            assert(dt.type == TB_INT);

            int op = MOV;
            if (dt.data <= 8)       op = is_signed ? MOVSXB : MOVZXB;
            else if (dt.data <= 16) op = is_signed ? MOVSXW : MOVZXW;

            // division is scaled up to 32bit
            if (dt.data < 32) dt.data = 32;

            // mov rax, lhs
            int lhs = input_reg(ctx, n->inputs[1]);
            SUBMIT(inst_op_rr(op, dt, RAX, lhs));

            int rhs = input_reg(ctx, n->inputs[2]);
            if (n->dt.data < 32) {
                // add cast
                int new_rhs = DEF(n->inputs[2], TB_TYPE_I32);
                SUBMIT(inst_op_rr(op, TB_TYPE_I32, new_rhs, rhs));
                rhs = new_rhs;
            }

            // if signed:
            //   cqo/cdq (sign extend RAX into RDX)
            // else:
            //   xor rdx, rdx
            if (is_signed) {
                Inst* inst = alloc_inst(CAST, dt, 1, 1, 0);
                inst->operands[0] = RDX;
                inst->operands[1] = RAX;
                SUBMIT(inst);
            } else {
                SUBMIT(inst_op_zero(dt, RDX));
            }

            {
                Inst* inst = alloc_inst(is_signed ? IDIV : DIV, dt, 2, 3, 0);
                inst->operands[0] = is_div ? RAX : RDX;
                inst->operands[1] = is_div ? RDX : RAX;
                inst->operands[2] = rhs;
                inst->operands[3] = RDX;
                inst->operands[4] = RAX;
                SUBMIT(inst);
            }

            hint_reg(ctx, dst, is_div ? RAX : RDX);
            SUBMIT(inst_move(dt, dst, is_div ? RAX : RDX));
            break;
        }

        case TB_FLOAT32_CONST: {
            assert(n->dt.type == TB_FLOAT);
            uint32_t imm = (Cvt_F32U32) { .f = TB_NODE_GET_EXTRA_T(n, TB_NodeFloat32)->value }.i;

            if (imm == 0) {
                SUBMIT(inst_op_zero(n->dt, dst));
            } else {
                TB_Global* g = tb__small_data_intern(ctx->module, sizeof(float), &imm);
                SUBMIT(inst_op_global(FP_MOV, n->dt, dst, (TB_Symbol*) g));
            }
            break;
        }
        case TB_FLOAT64_CONST: {
            assert(n->dt.type == TB_FLOAT);
            uint64_t imm = (Cvt_F64U64){ .f = TB_NODE_GET_EXTRA_T(n, TB_NodeFloat64)->value }.i;

            if (imm == 0) {
                SUBMIT(inst_op_zero(n->dt, dst));
            } else {
                TB_Global* g = tb__small_data_intern(ctx->module, sizeof(double), &imm);
                SUBMIT(inst_op_global(FP_MOV, n->dt, dst, (TB_Symbol*) g));
            }
            break;
        }
        case TB_FLOAT_EXT: {
            int src = input_reg(ctx, n->inputs[1]);
            SUBMIT(inst_op_rr(FP_CVT, n->inputs[1]->dt, dst, src));
            break;
        }
        case TB_NEG:
        case TB_NOT: {
            if (n->dt.type != TB_FLOAT) {
                int src = input_reg(ctx, n->inputs[1]);

                SUBMIT(inst_move(n->dt, dst, src));
                SUBMIT(inst_op_rr(type == TB_NOT ? NOT : NEG, n->dt, dst, dst));
            } else {
                if (type == TB_NEG) {
                    TB_Global* g = NULL;
                    if (n->dt.data == TB_FLT_32) {
                        uint32_t buffer[4] = { 1u << 31u, 1u << 31u, 1u << 31u, 1u << 31u };
                        g = tb__small_data_intern(ctx->module, 16, buffer);
                    } else if (n->dt.data == TB_FLT_64) {
                        uint64_t buffer[4] = { 1ull << 63ull, 1ull << 63ull };
                        g = tb__small_data_intern(ctx->module, 16, buffer);
                    } else {
                        tb_todo();
                    }

                    SUBMIT(inst_op_global(FP_XOR, n->dt, dst, (TB_Symbol*) g));
                } else {
                    tb_todo();
                }
            }
            break;
        }
        case TB_FADD:
        case TB_FSUB:
        case TB_FMUL:
        case TB_FDIV:
        case TB_FMAX:
        case TB_FMIN: {
            const static InstType ops[] = { FP_ADD, FP_SUB, FP_MUL, FP_DIV, FP_MAX, FP_MIN };

            int lhs = input_reg(ctx, n->inputs[1]);
            hint_reg(ctx, dst, lhs);
            SUBMIT(inst_move(n->dt, dst, lhs));

            if (n->inputs[2]->type == TB_LOAD && on_last_use(ctx, n->inputs[2])) {
                use(ctx, n->inputs[2]);

                Inst* inst = isel_addr2(ctx, n->inputs[2]->inputs[2], dst, -1, dst);
                inst->type = ops[type - TB_FADD];
                inst->dt = legalize(n->dt);
                SUBMIT(inst);
            } else {
                int rhs = input_reg(ctx, n->inputs[2]);
                SUBMIT(inst_op_rrr(ops[type - TB_FADD], n->dt, dst, dst, rhs));
            }
            break;
        }
        case TB_UINT2FLOAT:
        case TB_INT2FLOAT: {
            TB_DataType src_dt = n->inputs[1]->dt;
            assert(src_dt.type == TB_INT);

            // it's either 32bit or 64bit conversion
            //   CVTSI2SS r/m32, xmm1
            //   CVTSI2SD r/m64, xmm1
            bool is_64bit = src_dt.data > 32;

            int src = input_reg(ctx, n->inputs[1]);
            SUBMIT(inst_op_rr(is_64bit ? FP_CVT64 : FP_CVT32, n->dt, dst, src));
            break;
        }

        case TB_FLOAT2INT:
        case TB_FLOAT2UINT: {
            TB_DataType src_dt = n->inputs[1]->dt;
            assert(src_dt.type == TB_FLOAT);

            // it's either 32bit or 64bit conversion
            // F3 0F 2C /r            CVTTSS2SI xmm1, r/m32
            // F3 REX.W 0F 2C /r      CVTTSS2SI xmm1, r/m64
            // F2 0F 2C /r            CVTTSD2SI xmm1, r/m32
            // F2 REX.W 0F 2C /r      CVTTSD2SI xmm1, r/m64
            int src = input_reg(ctx, n->inputs[1]);
            SUBMIT(inst_op_rr(FP_CVTT, src_dt, dst, src));
            break;
        }

        // pointer arithmatic
        case TB_LOCAL:
        case TB_VA_START:
        case TB_MEMBER_ACCESS:
        case TB_ARRAY_ACCESS: {
            SUBMIT(isel_addr(ctx, n, dst, -1, -1));
            break;
        }

        // bitcasting
        case TB_BITCAST: {
            TB_DataType src_dt = n->inputs[1]->dt;
            int src = input_reg(ctx, n->inputs[1]);

            if (src_dt.type == TB_FLOAT && n->dt.type == TB_INT) {
                // float -> int
                SUBMIT(inst_op_rr(MOV_F2I, n->dt, dst, src));
            } else if (src_dt.type == TB_INT && n->dt.type == TB_FLOAT) {
                // int -> float
                SUBMIT(inst_op_rr(MOV_I2F, src_dt, dst, src));
            } else {
                SUBMIT(inst_move(n->dt, dst, src));
            }
            break;
        }

        // downcasting
        case TB_PTR2INT:
        case TB_TRUNCATE: {
            int src = input_reg(ctx, n->inputs[1]);

            if (n->dt.type == TB_FLOAT) {
                SUBMIT(inst_op_rr(FP_CVT, n->inputs[1]->dt, dst, src));
            } else {
                SUBMIT(inst_move(n->dt, dst, src));
            }
            break;
        }

        // upcasting
        case TB_INT2PTR:
        case TB_SIGN_EXT:
        case TB_ZERO_EXT: {
            TB_Node* src = n->inputs[1];

            TB_DataType src_dt = src->dt;
            bool sign_ext = (type == TB_SIGN_EXT);
            int bits_in_type = src_dt.type == TB_PTR ? 64 : src_dt.data;

            int32_t imm;
            if (try_for_imm32(ctx, bits_in_type, src, &imm)) {
                #define MASK_UPTO(pos) (~UINT64_C(0) >> (64 - pos))
                use(ctx, src);

                uint64_t src = imm;
                uint64_t sign_bit = (src >> (bits_in_type - 1)) & 1;
                uint64_t mask = MASK_UPTO(64) & ~MASK_UPTO(bits_in_type);

                src = (src & ~mask) | (sign_bit ? mask : 0);
                if (!fits_into_int32(src)) {
                    // movabs reg, imm64
                    SUBMIT(inst_op_abs(MOVABS, n->dt, dst, src));
                } else {
                    SUBMIT(inst_op_imm(MOV, n->dt, dst, src));
                }

                #undef MASK_UPTO
            } else {
                TB_DataType dt = n->dt;

                int op = MOV;
                if (bits_in_type <= 8) op = sign_ext ? MOVSXB : MOVZXB;
                else if (bits_in_type <= 16) op = sign_ext ? MOVSXW : MOVZXW;
                else if (bits_in_type <= 32) {
                    if (sign_ext) op = MOVSXD;
                    else dt = src_dt;
                } else if (bits_in_type <= 64) op = MOV;
                else tb_todo();

                int val = input_reg(ctx, src);
                SUBMIT(inst_op_rr(op, dt, dst, val));
            }
            break;
        }

        case TB_TAILCALL:
        case TB_SYSCALL:
        case TB_CALL: {
            static int ret_gprs[2] = { RAX, RDX };

            bool is_sysv = (ctx->target_abi == TB_ABI_SYSTEMV);
            const struct ParamDescriptor* restrict desc = &param_descs[is_sysv ? 1 : 0];
            if (type == TB_SYSCALL) {
                desc = &param_descs[2];
            }

            uint32_t caller_saved_gprs = desc->caller_saved_gprs;
            uint32_t caller_saved_xmms = ~0ull >> (64 - desc->caller_saved_xmms);

            TB_FunctionPrototype* proto = TB_NODE_GET_EXTRA_T(n, TB_NodeCall)->proto;

            TB_Node* ret_nodes[2] = { 0 };
            int rets[2] = { -1, -1 };
            int ret_count = 0;
            int proj_base = type == TB_TAILCALL ? 3 : 2;

            if (n->type != TB_TAILCALL) {
                assert(proto->return_count <= 2);
                FOREACH_N(i, 0, proto->return_count) {
                    TB_Node* ret_node = TB_NODE_GET_EXTRA_T(n, TB_NodeCall)->projs[proj_base + i];
                    if (!has_users(ctx, ret_node)) {
                        ret_node = NULL;
                    }

                    if (ret_node != NULL) {
                        ret_nodes[i] = ret_node;
                        rets[i] = input_reg(ctx, ret_node);
                        ret_count++;

                        bool use_xmm_ret = TB_IS_FLOAT_TYPE(ret_node->dt);
                        if (use_xmm_ret) {
                            caller_saved_xmms &= ~(1ull << (XMM0 + i));
                        } else {
                            caller_saved_gprs &= ~(1ull << ret_gprs[i]);
                        }
                    }
                }
            }

            // system calls don't count, we track this for ABI
            // and stack allocation purposes.
            if (ctx->caller_usage < n->input_count - 3) {
                ctx->caller_usage = n->input_count - 3;
            }

            // parameter passing is separate from eval from regalloc reasons
            size_t in_count = 0;
            RegIndex ins[64];
            RegIndex param_srcs[64];

            int vararg_cutoff = proto && proto->has_varargs ? proto->param_count : n->input_count-2;

            size_t xmms_used = 0, gprs_used = 0;
            FOREACH_N(i, 3, n->input_count) {
                TB_Node* param = n->inputs[i];
                TB_DataType param_dt = param->dt;

                bool use_xmm = TB_IS_FLOAT_TYPE(param_dt);
                int reg = use_xmm ? xmms_used : gprs_used;
                if (is_sysv) {
                    if (use_xmm) {
                        xmms_used++;
                    } else {
                        gprs_used++;
                        caller_saved_gprs &= ~(1u << gprs_used);
                    }
                } else {
                    // win64 will always expend a register
                    xmms_used++;
                    gprs_used++;
                }

                // first few parameters are passed as inputs to the CALL instruction.
                // the rest are written into the stack at specific places.
                RegIndex src = input_reg(ctx, param);
                if (reg >= desc->gpr_count) {
                    SUBMIT(inst_op_mr(use_xmm ? FP_MOV : MOV, param->dt, RSP, GPR_NONE, SCALE_X1, reg * 8, src));
                } else {
                    int phys_reg = use_xmm ? reg : desc->gprs[reg];
                    int dst = (use_xmm ? FIRST_XMM : FIRST_GPR) + phys_reg;

                    hint_reg(ctx, src, dst);

                    param_srcs[in_count] = src;
                    ins[in_count] = dst;
                    in_count += 1;

                    if (use_xmm) {
                        caller_saved_xmms &= ~(1ull << phys_reg);
                    } else {
                        caller_saved_gprs &= ~(1ull << phys_reg);
                    }
                }
            }

            // perform last minute copies (this avoids keeping parameter registers alive for too long)
            FOREACH_N(i, 0, in_count) {
                TB_DataType dt = n->inputs[3 + i]->dt;

                bool use_xmm = TB_IS_FLOAT_TYPE(dt);
                SUBMIT(inst_move(dt, ins[i], param_srcs[i]));

                // in win64, float params past the vararg cutoff are
                // duplicated in their respective GPR slot
                if (use_xmm && i >= vararg_cutoff && i < desc->gpr_count) {
                    int phys_reg = desc->gprs[i];
                    SUBMIT(inst_op_rr(MOV_F2I, TB_TYPE_I64, phys_reg, ins[i]));
                    ins[in_count++] = FIRST_GPR + phys_reg;
                }
            }

            // compute the target (unless it's a symbol) before the
            // registers all need to be forcibly shuffled
            TB_Node* target = n->inputs[2];
            bool static_call = n->type == TB_CALL && target->type == TB_SYMBOL;

            int target_val = RSP; // placeholder really
            if (!static_call) {
                target_val = input_reg(ctx, target);
            }

            if (type == TB_TAILCALL) {
                hint_reg(ctx, target_val, RAX);
                SUBMIT(inst_move(TB_TYPE_I64, RAX, target_val));
            } else if (type == TB_SYSCALL) {
                ins[in_count++] = FIRST_GPR + RAX;
                hint_reg(ctx, target_val, RAX);
                SUBMIT(inst_move(target->dt, RAX, target_val));
            } else {
                // the number of float parameters is written into AL
                if (proto->has_varargs && is_sysv) {
                    SUBMIT(inst_op_imm(MOV, TB_TYPE_I8, RAX, xmms_used));
                    ins[in_count++] = FIRST_GPR + RAX;
                    caller_saved_gprs &= ~(1ull << RAX);
                }
            }

            // all these registers need to be spilled and reloaded if they're used across
            // the function call boundary... you might see why inlining could be nice to implement
            size_t clobber_count = tb_popcount(caller_saved_gprs) + tb_popcount(caller_saved_xmms);

            int op = SYSCALL;
            if (type == TB_CALL) op = CALL;
            if (type == TB_TAILCALL) op = NOP;

            Inst* call_inst = alloc_inst(op, TB_TYPE_PTR, ret_count, 1 + in_count, clobber_count);

            // mark clobber list
            {
                RegIndex* clobbers = &call_inst->operands[call_inst->out_count + call_inst->in_count];
                FOREACH_N(i, 0, 16) if (caller_saved_gprs & (1u << i)) {
                    *clobbers++ = FIRST_GPR + i;
                }

                FOREACH_N(i, 0, 16) if (caller_saved_xmms & (1u << i)) {
                    *clobbers++ = FIRST_XMM + i;
                }
            }

            // return value (either XMM0 or RAX)
            RegIndex* dst_ins = call_inst->operands;
            FOREACH_N(i, 0, 2) if (ret_nodes[i] != NULL) {
                bool use_xmm_ret = TB_IS_FLOAT_TYPE(ret_nodes[i]->dt);
                if (use_xmm_ret) {
                    *dst_ins++ = FIRST_XMM + i;
                } else {
                    *dst_ins++ = ret_gprs[i];
                }
            }

            // write inputs
            if (static_call) {
                call_inst->flags |= INST_GLOBAL;
                call_inst->mem_slot = call_inst->out_count;
                call_inst->s = TB_NODE_GET_EXTRA_T(target, TB_NodeSymbol)->sym;
            }

            *dst_ins++ = target_val;
            memcpy(dst_ins, ins, in_count * sizeof(RegIndex));

            SUBMIT(call_inst);

            if (type == TB_TAILCALL) {
                Inst* i = alloc_inst(INST_EPILOGUE, TB_TYPE_VOID, 0, 0, 0);
                append_inst(ctx, i);

                SUBMIT(inst_jmp_reg(RAX));
            } else {
                // copy out return
                FOREACH_N(i, 0, 2) if (ret_nodes[i] != NULL) {
                    assert(rets[i] >= 0);
                    TB_DataType dt = ret_nodes[i]->dt;
                    bool use_xmm_ret = TB_IS_FLOAT_TYPE(dt);
                    if (use_xmm_ret) {
                        hint_reg(ctx, rets[i], FIRST_XMM + i);
                        SUBMIT(inst_move(dt, rets[i], FIRST_XMM + i));
                    } else {
                        hint_reg(ctx, rets[i], ret_gprs[i]);
                        SUBMIT(inst_move(dt, rets[i], ret_gprs[i]));
                    }
                }
            }
            break;
        }

        case TB_CMP_EQ:
        case TB_CMP_NE:
        case TB_CMP_SLT:
        case TB_CMP_SLE:
        case TB_CMP_ULT:
        case TB_CMP_ULE:
        case TB_CMP_FLT:
        case TB_CMP_FLE: {
            // SUBMIT(inst_op_zero(n->dt, dst));

            // use SETcc to convert into integer
            Cond cc = isel_cmp(ctx, n);
            SUBMIT(inst_op_r(SETO + cc, TB_TYPE_I8, dst));
            break;
        }

        // table lookup constant -> constant
        case TB_LOOKUP: {
            TB_NodeLookup* l = TB_NODE_GET_EXTRA(n);

            TB_LookupEntry* min = &l->entries[1];
            TB_LookupEntry* max = &l->entries[l->entry_count - 1];

            // "+ 2" because of the inclusive range + the default case
            int64_t range = (max->key - min->key) + 2;

            double dist_avg = 0;
            double inv_key_count = 1.0 / (l->entry_count - 1);

            int64_t last = l->entries[1].key;
            FOREACH_N(i, 2, l->entry_count) {
                int64_t key = l->entries[i].key;

                dist_avg += (key - last) * inv_key_count;
                last = key;
            }

            // we wanna figure out how many bits per table entry
            assert(n->dt.type == TB_INT);
            size_t bits = tb_next_pow2((n->dt.data + 7) / 8) * 8;
            if (ctx->p->universe.arena != NULL) {
                // lattice is the superior type, trust it over the
                // TB_DataType, in this case if we see a 1bit value
                // let's just do bitsets.
                Lattice* l = lattice_universe_get(&ctx->p->universe, n);
                if (l->tag == LATTICE_INT && l->_int.min == 0 && l->_int.max == 1) {
                    bits = 1;
                }
            }

            // in QWORDs
            size_t table_size = ((range * bits) + 63) / 64;

            // flat table from start to finish (first element is the default)
            TB_Function* f = ctx->f;
            TB_Global* table_sym = tb_global_create(f->super.module, 0, NULL, NULL, TB_LINKAGE_PRIVATE);
            tb_global_set_storage(f->super.module, tb_module_get_rdata(f->super.module), table_sym, table_size * sizeof(uint64_t), 8, 1);
            uint64_t* table_data = tb_global_add_region(f->super.module, table_sym, 0, table_size * sizeof(uint64_t));

            memset(table_data, 0, table_size * sizeof(uint64_t));

            // encode every entry
            int amt = tb_ffs(64 / bits) - 1;
            FOREACH_N(i, 0, l->entry_count) {
                uint64_t index = 0;
                if (i > 0) {
                    index = (l->entries[i].key - min->key) + 1;
                }

                uint64_t mask = (1ull << bits) - 1;
                uint64_t shift = index & ((1ull << amt) - 1);
                table_data[index >> amt] |= (l->entries[i].val & mask) << shift;
            }

            TB_DataType dt = n->dt;
            int key = input_reg(ctx, n->inputs[1]);

            int index = DEF(NULL, dt);
            hint_reg(ctx, index, key);
            SUBMIT(inst_move(dt, index, key));
            // Simple range check:
            //   if ((key - min) >= (max - min)) goto default
            if (1) {
                int zero = DEF(NULL, dt);
                SUBMIT(inst_op_zero(dt, zero));

                if (min != 0) {
                    SUBMIT(inst_op_rri(SUB, dt, index, index, min->key - 1));
                }
                SUBMIT(inst_op_ri(CMP, dt, index, range));
                SUBMIT(inst_op_rr(CMOVA, n->dt, index, zero));
            }
            //   lea table, [rip + TABLE]
            int table = DEF(NULL, TB_TYPE_I64);
            SUBMIT(inst_op_global(LEA, TB_TYPE_I64, table, (TB_Symbol*) table_sym));

            // word_index = key
            int word_index = DEF(NULL, dt);
            hint_reg(ctx, word_index, index);
            SUBMIT(inst_move(dt, word_index, index));
            if (bits != 64) {
                // word_index /= (64 / bits)
                SUBMIT(inst_op_rri(SHR, TB_TYPE_I64, word_index, word_index, amt));
            }
            //   mov table, [table + index*8]
            SUBMIT(inst_op_rm(MOV, TB_TYPE_I64, dst, table, word_index, SCALE_X8, 0));
            // we need to extract bits
            if (bits != 64) {
                //   mov RCX, index
                hint_reg(ctx, index, RCX);
                SUBMIT(inst_move(dt, RCX, index));
                //   and dst, amt_mask
                uint64_t amt_mask = (1ull << amt) - 1;
                if (amt_mask != 0x3F) {
                    SUBMIT(inst_op_rri(AND, TB_TYPE_I64, RCX, RCX, amt_mask));
                }
                //   shr dst, RCX
                SUBMIT(inst_op_rrr(SHR, TB_TYPE_I64, dst, dst, RCX));
                //   and dst, mask
                uint64_t mask = (1ull << bits) - 1;
                SUBMIT(inst_op_rri(AND, TB_TYPE_I64, dst, dst, mask));
            }
            break;
        }

        case TB_BRANCH: {
            TB_Node* bb = tb_get_parent_region(n);
            TB_NodeBranch* br = TB_NODE_GET_EXTRA(n);

            // the arena on the function should also be available at this time, we're
            // in the TB_Passes
            TB_Arena* arena = ctx->f->arena;
            TB_ArenaSavepoint sp = tb_arena_save(arena);
            int* succ = tb_arena_alloc(arena, br->succ_count * sizeof(int));

            // fill successors
            bool has_default = false;
            for (User* u = n->users; u; u = u->next) {
                if (u->n->type == TB_PROJ) {
                    int index = TB_NODE_GET_EXTRA_T(u->n, TB_NodeProj)->index;
                    TB_Node* succ_n = cfg_next_bb_after_cproj(u->n);

                    if (index == 0) {
                        has_default = !cfg_is_unreachable(succ_n);
                    }

                    succ[index] = nl_map_get_checked(ctx->cfg.node_to_block, succ_n).id;
                }
            }

            TB_DataType dt = n->inputs[1]->dt;

            SUBMIT(alloc_inst(INST_TERMINATOR, TB_TYPE_VOID, 0, 0, 0));

            if (br->succ_count == 1) {
                assert(0 && "degenerate branch? that's odd");
            } else if (br->succ_count == 2) {
                int f = succ[1], t = succ[0];

                Cond cc;
                if (br->keys[0] == 0) {
                    cc = isel_cmp(ctx, n->inputs[1]);
                } else {
                    int key = input_reg(ctx, n->inputs[1]);
                    SUBMIT(inst_op_ri(CMP, dt, key, br->keys[0]));
                    cc = NE;
                }

                // if flipping avoids a jmp, do that
                if (ctx->fallthrough == t) {
                    SUBMIT(inst_jcc(f, cc ^ 1));
                } else {
                    SUBMIT(inst_jcc(t, cc));
                    if (ctx->fallthrough != f) {
                        SUBMIT(inst_jmp(f));
                    }
                }
            } else {
                int key = input_reg(ctx, n->inputs[1]);

                // check if there's at most only one space between entries
                int64_t last = br->keys[0];
                int64_t min = last, max = last;

                double dist_avg = 0;
                double inv_succ_count = 1.0 / (br->succ_count - 2);

                FOREACH_N(i, 2, br->succ_count) {
                    int64_t key = br->keys[i - 1];
                    min = (min > key) ? key : min;
                    max = (max > key) ? max : key;

                    dist_avg += (key - last) * inv_succ_count;
                    last = key;
                }

                enum {
                    IF_ELSE_CHAIN,
                    JUMP_TABLE,
                } r = IF_ELSE_CHAIN;

                int64_t range = (max - min) + 1;
                if (range >= 4 && dist_avg < 2.0) {
                    r = JUMP_TABLE;
                }

                switch (r) {
                    case IF_ELSE_CHAIN: {
                        // Basic if-else chain
                        FOREACH_N(i, 1, br->succ_count) {
                            uint64_t curr_key = br->keys[i-1];

                            if (fits_into_int32(curr_key)) {
                                SUBMIT(inst_op_ri(CMP, dt, key, curr_key));
                            } else {
                                int tmp = DEF(n, dt);
                                SUBMIT(inst_op_abs(MOVABS, dt, tmp, curr_key));
                                SUBMIT(inst_op_rr(CMP, dt, key, tmp));
                            }
                            SUBMIT(inst_jcc(succ[i], E));
                        }
                        SUBMIT(inst_jmp(succ[0]));
                        break;
                    }

                    case JUMP_TABLE: {
                        // make a jump table with 4 byte relative pointers for each target
                        TB_Function* f = ctx->f;
                        TB_Global* jump_table = tb_global_create(f->super.module, -1, "jumptbl", NULL, TB_LINKAGE_PRIVATE);
                        tb_global_set_storage(f->super.module, tb_module_get_rdata(f->super.module), jump_table, range*4, 4, 1);

                        // generate patches for later
                        uint32_t* jump_entries = tb_global_add_region(f->super.module, jump_table, 0, range*4);

                        Set entries_set = set_create_in_arena(arena, range);
                        FOREACH_N(i, 1, br->succ_count) {
                            uint64_t key_idx = br->keys[i - 1] - min;
                            assert(key_idx < range);

                            JumpTablePatch p;
                            p.pos = &jump_entries[key_idx];
                            p.target = succ[i];
                            dyn_array_put(ctx->jump_table_patches, p);
                            set_put(&entries_set, key_idx);
                        }

                        // handle default cases
                        FOREACH_N(i, 0, range) {
                            if (!set_get(&entries_set, i)) {
                                JumpTablePatch p;
                                p.pos = &jump_entries[i];
                                p.target = succ[0];
                                dyn_array_put(ctx->jump_table_patches, p);
                            }
                        }

                        int tmp = DEF(NULL, dt);
                        hint_reg(ctx, tmp, key);
                        if (dt.data >= 32) {
                            SUBMIT(inst_move(dt, tmp, key));
                        } else if (dt.data == 16) {
                            dt = TB_TYPE_I32;
                            SUBMIT(inst_op_rr(MOVZXW, dt, tmp, key));
                        } else if (dt.data == 8) {
                            dt = TB_TYPE_I32;
                            SUBMIT(inst_op_rr(MOVZXB, dt, tmp, key));
                        } else {
                            dt = TB_TYPE_I32;
                            uint64_t mask = tb__mask(dt.data);

                            SUBMIT(inst_move(dt, tmp, key));
                            SUBMIT(inst_op_rri(AND, dt, tmp, tmp, mask));
                        }

                        // Simple range check:
                        //   if ((key - min) >= (max - min)) goto default
                        if (has_default) {
                            if (min != 0) {
                                SUBMIT(inst_op_rri(SUB, dt, tmp, tmp, min));
                            }
                            SUBMIT(inst_op_ri(CMP, dt, tmp, range));
                            SUBMIT(inst_jcc(succ[0], NB));
                        }
                        //   lea target, [rip + f]
                        int target = DEF(NULL, TB_TYPE_I64);
                        SUBMIT(inst_op_global(LEA, TB_TYPE_I64, target, (TB_Symbol*) f));
                        //   lea table, [rip + JUMP_TABLE]
                        int table = DEF(NULL, TB_TYPE_I64);
                        SUBMIT(inst_op_global(LEA, TB_TYPE_I64, table, (TB_Symbol*) jump_table));
                        //   movsxd table, [table + key*4]
                        SUBMIT(inst_op_rm(MOVSXD, TB_TYPE_I64, table, table, tmp, SCALE_X4, 0));
                        //   add target, table
                        SUBMIT(inst_op_rrr(ADD, TB_TYPE_I64, target, target, table));
                        //   jmp target
                        SUBMIT(inst_jmp_reg(target));
                        break;
                    }
                }
            }

            tb_arena_restore(arena, sp);
            break;
        }

        case TB_DEBUGBREAK: {
            SUBMIT(alloc_inst(INT3, TB_TYPE_VOID, 0, 0, 0));
            break;
        }

        case TB_UNREACHABLE:
        case TB_TRAP: {
            SUBMIT(alloc_inst(UD2, TB_TYPE_VOID, 0, 0, 0));
            break;
        }

        case TB_SYMBOL: {
            TB_NodeSymbol* s = TB_NODE_GET_EXTRA(n);
            SUBMIT(inst_op_global(LEA, n->dt, dst, s->sym));
            break;
        }
        case TB_LOAD:
        case TB_ATOMIC_LOAD: {
            int mov_op = TB_IS_FLOAT_TYPE(n->dt) ? FP_MOV : MOV;
            TB_Node* addr = n->inputs[2];

            Inst* ld_inst = isel_addr2(ctx, addr, dst, -1, -1);
            ld_inst->type = mov_op;
            ld_inst->dt = legalize(n->dt);
            if (n->type == TB_ATOMIC_LOAD) {
                ld_inst->flags |= INST_LOCK;
            }

            SUBMIT(ld_inst);
            break;
        }
        case TB_SAFEPOINT_POLL: {
            TB_Node* addr = n->inputs[2];

            tb_todo();

            // force uses of the inputs
            /*FOREACH_N(i, 3, n->input_count) {
                input_reg(ctx, n->inputs[i]);
            }*/

            // test tmp, dword [poll_site]
            int tmp = DEF(n, TB_TYPE_I32);
            Inst* ld_inst = isel_addr2(ctx, addr, tmp, -1, -1);
            ld_inst->type = TEST;
            ld_inst->dt = TB_X86_TYPE_DWORD;
            SUBMIT(ld_inst);
            break;
        }

        case TB_STORE: {
            if (dst >= 0) {
                use(ctx, n->inputs[2]);
                use(ctx, n->inputs[3]);
                break;
            }

            TB_DataType store_dt = n->inputs[3]->dt;

            // if we can couple the LOAD & STORE
            TB_Node* addr = n->inputs[2];
            TB_Node* src = n->inputs[3];
            int store_op = can_folded_store(ctx, n->inputs[1], addr, n->inputs[3]);
            if (store_op >= 0) {
                use(ctx, src);
                use(ctx, addr);
                use(ctx, src->inputs[1]);
                use(ctx, src->inputs[1]->inputs[1]);

                src = src->inputs[2];
            } else {
                store_op = TB_IS_FLOAT_TYPE(store_dt) ? FP_MOV : MOV;
            }

            int32_t imm;
            if (try_for_imm32(ctx, src->dt.type == TB_PTR ? 64 : src->dt.data, src, &imm)) {
                use(ctx, src);

                Inst* st_inst = isel_addr2(ctx, addr, dst, store_op, -1);
                st_inst->in_count -= 1;
                st_inst->dt = legalize(store_dt);
                st_inst->flags |= INST_IMM;
                st_inst->imm = imm;
                assert(st_inst->flags & (INST_MEM | INST_GLOBAL));

                SUBMIT(st_inst);
            } else {
                int src_reg = input_reg(ctx, src);

                Inst* st_inst = isel_addr2(ctx, addr, dst, store_op, src_reg);
                st_inst->dt = legalize(store_dt);
                assert(st_inst->flags & (INST_MEM | INST_GLOBAL));

                SUBMIT(st_inst);
            }
            break;
        }
        case TB_MEMSET: {
            TB_DataType ptr_dt = TB_TYPE_I64;
            int rdi = input_reg(ctx, n->inputs[2]);
            int rax = input_reg(ctx, n->inputs[3]);
            int rcx = input_reg(ctx, n->inputs[4]);

            hint_reg(ctx, rdi, RDI);
            hint_reg(ctx, rax, RAX);
            hint_reg(ctx, rcx, RCX);
            SUBMIT(inst_move(ptr_dt,     RDI, rdi));
            SUBMIT(inst_move(TB_TYPE_I8, RAX, rax));
            SUBMIT(inst_move(ptr_dt,     RCX, rcx));

            Inst* i = alloc_inst(STOSB, TB_TYPE_VOID, 0, 3, 0);
            i->flags |= INST_REP;
            i->operands[0] = RDI;
            i->operands[1] = RAX;
            i->operands[2] = RCX;
            SUBMIT(i);
            break;
        }
        case TB_MEMCPY: {
            TB_DataType ptr_dt = TB_TYPE_I64;
            int rdi = input_reg(ctx, n->inputs[2]);
            int rsi = input_reg(ctx, n->inputs[3]);
            int rcx = input_reg(ctx, n->inputs[4]);

            hint_reg(ctx, rdi, RDI);
            hint_reg(ctx, rsi, RSI);
            hint_reg(ctx, rcx, RCX);
            SUBMIT(inst_move(ptr_dt, RDI, rdi));
            SUBMIT(inst_move(ptr_dt, RSI, rsi));
            SUBMIT(inst_move(ptr_dt, RCX, rcx));

            Inst* i = alloc_inst(MOVSB, TB_TYPE_VOID, 0, 3, 0);
            i->flags |= INST_REP;
            i->operands[0] = RDI;
            i->operands[1] = RSI;
            i->operands[2] = RCX;
            SUBMIT(i);
            break;
        }

        case TB_END: {
            assert(n->input_count <= 5 && "At most 2 return values :(");
            static int ret_gprs[2] = { RAX, RDX };

            int rets = n->input_count - 3;
            FOREACH_N(i, 0, rets) {
                int src = input_reg(ctx, n->inputs[3+i]);

                // copy to return register
                TB_DataType dt = n->inputs[3+i]->dt;
                if (dt.type == TB_FLOAT) {
                    hint_reg(ctx, src, FIRST_XMM + i);
                    SUBMIT(inst_move(dt, FIRST_XMM + i, src));
                } else {
                    hint_reg(ctx, src, ret_gprs[i]);
                    SUBMIT(inst_move(dt, ret_gprs[i], src));
                }
            }

            // we don't really need a fence if we're about to exit but we do
            // need to mark that it's the epilogue to tell regalloc where callee
            // regs need to get restored.
            Inst* i = alloc_inst(INST_EPILOGUE, TB_TYPE_VOID, 0, 0, 0);
            i->flags |= INST_RET;
            append_inst(ctx, i);
            break;
        }

        case TB_MACHINE_OP: {
            TB_NodeMachineOp* mach = TB_NODE_GET_EXTRA(n);
            size_t total = mach->outs + mach->ins + mach->tmps;

            if (total == 0) tb_todo();

            Inst* inst = alloc_inst(INST_INLINE, TB_TYPE_VOID, mach->outs, mach->ins, mach->tmps);
            memcpy(inst->operands, mach->regs, total * sizeof(TB_PhysicalReg));
            append_inst(ctx, inst);
            break;
        }

        // atomic RMW
        case TB_ATOMIC_XCHG:
        case TB_ATOMIC_ADD:
        case TB_ATOMIC_SUB:
        case TB_ATOMIC_AND:
        case TB_ATOMIC_XOR:
        case TB_ATOMIC_OR: {
            // tbl is for normal locking operations which don't care about the result,
            // fetch will need to worry about it which means slightly different codegen.
            const static int tbl[]       = { MOV, ADD, SUB, AND, XOR, OR };
            const static int fetch_tbl[] = { XCHG, XADD, XADD, 0, 0, 0 };

            TB_NodeAtomic* a = TB_NODE_GET_EXTRA(n);
            TB_DataType dt = a->proj1->dt;
            bool dst_users = !has_users(ctx, a->proj1);

            int op = (dst_users ? fetch_tbl : tbl)[type - TB_ATOMIC_XCHG];
            if (op == 0) {
                tb_todo(); // unsupported atomic op, we need to emulate it :(
            }

            int src = input_reg(ctx, n->inputs[3]);

            // copy src into proj1 because the operation will be swapping SRC with
            // the old value of the address.
            int proj1 = input_reg(ctx, a->proj1);
            hint_reg(ctx, proj1, src);
            SUBMIT(inst_move(dt, proj1, src));

            // ATOMIC-OP [addr], proj1
            Inst* st_inst = isel_addr2(ctx, n->inputs[2], -1, op, proj1);
            st_inst->flags |= INST_LOCK;
            st_inst->dt = legalize(dt);
            assert(st_inst->flags & INST_MEM);
            SUBMIT(st_inst);
            break;
        }

        case TB_PREFETCH: {
            TB_NodePrefetch* p = TB_NODE_GET_EXTRA(n);

            // we might wanna fold these into each other but yea
            int addr = DEF(n, TB_TYPE_I64);
            SUBMIT(isel_addr(ctx, n->inputs[2], addr, -1, -1));

            // prefetch op
            Inst* i = alloc_inst(PREFETCHNTA + p->level, TB_TYPE_I32, 0, 1, 0);
            i->flags = INST_MEM;
            i->mem_slot = 0;
            i->operands[1] = addr;
            i->disp = 0;
            i->scale = SCALE_X1;
            SUBMIT(i);
            break;
        }

        case TB_CYCLE_COUNTER: {
            // rdtsc
            Inst* inst = alloc_inst(RDTSC, TB_TYPE_I64, 2, 0, 0);
            inst->operands[0] = RDX;
            inst->operands[1] = RAX;
            SUBMIT(inst);

            // shl rdx, 32
            SUBMIT(inst_op_rri(SHL, TB_TYPE_I64, RDX, RDX, 32));
            // or rax, rdx
            SUBMIT(inst_op_rrr(OR, TB_TYPE_I64, RAX, RAX, RDX));
            // mov dst, rax
            hint_reg(ctx, dst, RAX);
            SUBMIT(inst_move(TB_TYPE_I64, dst, RAX));
            break;
        }

        case TB_PROJ: {
            if (n->inputs[0]->type == TB_START) {
                int index = TB_NODE_GET_EXTRA_T(n, TB_NodeProj)->index - 3;
                int param_gpr_count = ctx->target_abi == TB_ABI_WIN64 ? 4 : 6;

                // past the first register parameters, it's all stack
                if (index >= param_gpr_count) {
                    InstType i = n->dt.type == TB_FLOAT ? FP_MOV : MOV;
                    SUBMIT(inst_op_rm(i, n->dt, dst, RBP, GPR_NONE, SCALE_X1, 16 + index*8));
                }
            }
            break;
        }

        default: tb_todo();
    }
}

static void inst2_print(TB_CGEmitter* restrict e, InstType type, Val* dst, Val* src, TB_X86_DataType dt) {
    if (dt >= TB_X86_TYPE_SSE_SS && dt <= TB_X86_TYPE_SSE_PD) {
        inst2sse(e, type, dst, src, dt);
    } else {
        inst2(e, type, dst, src, dt);
    }
}

static int resolve_interval(Ctx* restrict ctx, Inst* inst, int i, Val* val) {
    LiveInterval* interval = &ctx->intervals[inst->operands[i]];

    if ((inst->flags & (INST_MEM | INST_GLOBAL)) && i == inst->mem_slot) {
        tb_assert(!interval->is_spill, "cannot use spilled value for memory operand");
        if (inst->flags & INST_MEM) {
            *val = (Val){
                .type = VAL_MEM,
                .reg  = interval->assigned,
                .index = GPR_NONE,
                .scale = inst->scale,
                .imm = inst->disp,
            };

            if (inst->flags & INST_INDEXED) {
                interval = &ctx->intervals[inst->operands[i + 1]];
                tb_assert(!interval->is_spill, "cannot use spilled value for memory operand");

                val->index = interval->assigned;
                return 2;
            } else {
                return 1;
            }
        } else {
            *val = val_global(inst->s, inst->disp);
            return 1;
        }
    }

    if (interval->is_spill) {
        *val = (Val){
            .type = VAL_MEM,
            .reg = RBP,
            .index = GPR_NONE,
            .imm = -interval->spill->pos,
        };
    } else {
        *val = (Val){
            .type = interval->reg_class == REG_CLASS_XMM ? VAL_XMM : VAL_GPR,
            .reg  = interval->assigned
        };
    }

    return 1;
}

static void emit_code(Ctx* restrict ctx, TB_FunctionOutput* restrict func_out) {
    TB_CGEmitter* e = &ctx->emit;

    // resolve stack usage
    {
        size_t caller_usage = ctx->caller_usage;
        if (ctx->target_abi == TB_ABI_WIN64 && caller_usage > 0 && caller_usage < 4) {
            caller_usage = 4;
        }

        size_t usage = ctx->stack_usage + (caller_usage * 8);

        // Align stack usage to 16bytes + 8 to accommodate for the RIP being pushed by CALL
        ctx->stack_usage = align_up(usage, 16);
    }

    // emit prologue
    func_out->prologue_length = emit_prologue(ctx);

    Inst* prev_line = NULL;
    for (Inst* restrict inst = ctx->first; inst; inst = inst->next) {
        size_t in_base = inst->out_count;
        size_t inst_table_size = sizeof(inst_table) / sizeof(*inst_table);
        InstCategory cat = inst->type >= inst_table_size ? INST_BINOP : inst_table[inst->type].cat;

        if (0) {
            printf("  \x1b[32m# %s t=%d { outs:", inst->type < inst_table_size ? inst_table[inst->type].mnemonic : "???", inst->time);
            FOREACH_N(i, 0, inst->out_count) {
                printf(" v%d", inst->operands[i]);
            }
            printf(", ins: ");
            FOREACH_N(i, inst->out_count, inst->out_count + inst->in_count) {
                printf(" v%d", inst->operands[i]);
            }
            printf("}\x1b[0m\n");
        }

        if (inst->type == INST_ENTRY || inst->type == INST_TERMINATOR) {
            // does nothing
        } else if (inst->type == INST_LABEL) {
            TB_Node* bb = inst->n;
            uint32_t pos = GET_CODE_POS(&ctx->emit);

            int id = nl_map_get_checked(ctx->cfg.node_to_block, bb).id;
            tb_resolve_rel32(&ctx->emit, &ctx->emit.labels[id], pos);
        } else if (inst->type == INST_INLINE) {
            if (inst->n) {
                TB_NodeMachineOp* mach = TB_NODE_GET_EXTRA(inst->n);

                void* dst = tb_cgemit_reserve(&ctx->emit, mach->length);
                memcpy(dst, mach->data, mach->length);
                tb_cgemit_commit(&ctx->emit, mach->length);
            }
        } else if (inst->type == INST_EPILOGUE) {
            // just a marker for regalloc
            emit_epilogue(ctx);

            if (inst->flags & INST_RET) {
                // ret
                EMIT1(&ctx->emit, 0xC3);
            }
        } else if (inst->type == INST_LINE) {
            TB_Function* f = ctx->f;
            TB_NodeSafepoint* loc = TB_NODE_GET_EXTRA(inst->n);
            uint32_t pos = GET_CODE_POS(&ctx->emit);

            size_t top = dyn_array_length(ctx->locations);
            if (prev_line == NULL || (prev_line->next != inst && !is_same_location(TB_NODE_GET_EXTRA(prev_line->n), loc))) {
                TB_Location l = {
                    .file = loc->file,
                    .line = loc->line,
                    .column = loc->column,
                    .pos = pos
                };
                dyn_array_put(ctx->locations, l);
                prev_line = inst;
            }
            continue;
        } else if (cat == INST_BYTE || cat == INST_BYTE_EXT) {
            // we don't emit NOPs, just annotate for the regalloc
            if (inst->type != NOP) {
                if (inst->flags & INST_REP) EMIT1(e, 0xF3);
                inst0(e, inst->type, inst->dt);
            }
        } else if (inst->type == INST_ZERO) {
            Val dst;
            resolve_interval(ctx, inst, 0, &dst);

            bool is_xmm = inst->dt >= TB_X86_TYPE_PBYTE && inst->dt <= TB_X86_TYPE_XMMWORD;
            inst2_print(e, is_xmm ? FP_XOR : XOR, &dst, &dst, inst->dt);
        } else if (inst->type >= JMP && inst->type <= JG) {
            Val target;
            if (inst->flags & INST_NODE) {
                target = val_label(inst->l);
            } else if (inst->flags & INST_GLOBAL) {
                target = val_global(inst->s, inst->disp);
            } else {
                assert(inst->in_count == 1);
                resolve_interval(ctx, inst, in_base, &target);
            }

            inst1(e, inst->type, &target, inst->dt);
        } else if (inst->type == CALL) {
            Val target;
            size_t i = resolve_interval(ctx, inst, in_base, &target);
            inst1(e, CALL, &target, TB_X86_TYPE_QWORD);
        } else {
            int mov_op = inst->dt >= TB_X86_TYPE_PBYTE && inst->dt <= TB_X86_TYPE_XMMWORD ? FP_MOV : MOV;

            // prefix
            if (inst->flags & INST_LOCK) {
                EMIT1(e, 0xF0);
            }

            if (inst->flags & INST_REP) {
                EMIT1(e, 0xF3);
            }

            TB_X86_DataType dt = inst->dt;
            if (dt == TB_X86_TYPE_XMMWORD) {
                dt = TB_X86_TYPE_SSE_PD;
            }

            // resolve output
            Val out;
            int i = 0;
            if (inst->out_count == 1) {
                i += resolve_interval(ctx, inst, i, &out);
                assert(i == in_base);
            } else {
                i = in_base;
            }

            // first parameter
            bool ternary = false;
            if (inst->in_count > 0) {
                Val lhs;
                i += resolve_interval(ctx, inst, i, &lhs);

                ternary = (i < in_base + inst->in_count) || (inst->flags & (INST_IMM | INST_ABS));
                if (ternary && inst->type == IMUL && (inst->flags & INST_IMM)) {
                    // there's a special case for ternary IMUL r64, r/m64, imm32
                    inst2(e, IMUL3, &out, &lhs, dt);
                    if (inst->dt == TB_X86_TYPE_WORD) {
                        EMIT2(e, inst->imm);
                    } else {
                        EMIT4(e, inst->imm);
                    }
                    continue;
                }

                if (inst->out_count == 0) {
                    out = lhs;
                } else if (inst->type == IDIV || inst->type == DIV) {
                    inst1(e, inst->type, &lhs, dt);
                    continue;
                }  else if (cat == INST_UNARY || cat == INST_UNARY_EXT) {
                    if (!is_value_match(&out, &lhs)) {
                        inst2_print(e, MOV, &out, &lhs, dt);
                    }
                } else {
                    if (ternary || inst->type == MOV || inst->type == FP_MOV) {
                        if (!is_value_match(&out, &lhs)) {
                            inst2_print(e, mov_op, &out, &lhs, dt);
                        }
                    } else {
                        inst2_print(e, inst->type, &out, &lhs, dt);
                    }
                }
            }

            // unary ops
            if (cat == INST_UNARY || cat == INST_UNARY_EXT) {
                inst1(e, inst->type, &out, dt);
                continue;
            }

            if (inst->flags & INST_IMM) {
                Val rhs = val_imm(inst->imm);
                inst2_print(e, inst->type, &out, &rhs, dt);
            } else if (inst->flags & INST_ABS) {
                Val rhs = val_abs(inst->abs);
                inst2_print(e, inst->type, &out, &rhs, dt);
            } else if (ternary) {
                Val rhs;
                i += resolve_interval(ctx, inst, i, &rhs);

                if (inst->type != MOV || (inst->type == MOV && !is_value_match(&out, &rhs))) {
                    inst2_print(e, inst->type, &out, &rhs, dt);
                }
            }
        }
    }

    // pad to 16bytes
    static const uint8_t nops[8][8] = {
        { 0x90 },
        { 0x66, 0x90 },
        { 0x0F, 0x1F, 0x00 },
        { 0x0F, 0x1F, 0x40, 0x00 },
        { 0x0F, 0x1F, 0x44, 0x00, 0x00 },
        { 0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00 },
        { 0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00 },
        { 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 },
    };

    size_t pad = 16 - (ctx->emit.count & 15);
    if (pad < 16) {
        uint8_t* dst = tb_cgemit_reserve(&ctx->emit, pad);
        tb_cgemit_commit(&ctx->emit, pad);

        if (pad > 8) {
            size_t rem = pad - 8;
            memset(dst, 0x66, rem);
            pad -= rem, dst += rem;
        }
        memcpy(dst, nops[pad - 1], pad);
    }
}

static void emit_win64eh_unwind_info(TB_Emitter* e, TB_FunctionOutput* out_f, uint64_t stack_usage) {
    size_t patch_pos = e->count;
    UnwindInfo unwind = {
        .version = 1,
        .flags = 0, // UNWIND_FLAG_EHANDLER,
        .prolog_length = out_f->prologue_length,
        .code_count = 0,
        .frame_register = RBP,
        .frame_offset = 0,
    };
    tb_outs(e, sizeof(UnwindInfo), &unwind);

    size_t code_count = 0;
    if (stack_usage > 0) {
        UnwindCode codes[] = {
            // sub rsp, stack_usage
            { .code_offset = 8, .unwind_op = UNWIND_OP_ALLOC_SMALL, .op_info = (stack_usage / 8) - 1 },
            // mov rbp, rsp
            { .code_offset = 4, .unwind_op = UNWIND_OP_SET_FPREG, .op_info = 0 },
            // push rbp
            { .code_offset = 1, .unwind_op = UNWIND_OP_PUSH_NONVOL, .op_info = RBP },
        };
        tb_outs(e, sizeof(codes), codes);
        code_count += 3;
    }

    tb_patch1b(e, patch_pos + offsetof(UnwindInfo, code_count), code_count);
}

static size_t emit_prologue(Ctx* restrict ctx) {
    uint64_t stack_usage = ctx->stack_usage;
    if (stack_usage <= 16) {
        return 0;
    }

    TB_CGEmitter* e = &ctx->emit;

    // push rbp
    if (stack_usage > 0) {
        EMIT1(e, 0x50 + RBP);

        // mov rbp, rsp
        EMIT1(e, rex(true, RSP, RBP, 0));
        EMIT1(e, 0x89);
        EMIT1(e, mod_rx_rm(MOD_DIRECT, RSP, RBP));
    }

    // if there's more than 4096 bytes of stack, we need to insert a chkstk
    if (ctx->target_abi == TB_ABI_WIN64 && stack_usage >= 4096) {
        assert(ctx->f->super.module->chkstk_extern);
        Val sym = val_global(ctx->f->super.module->chkstk_extern, 0);
        Val imm = val_imm(stack_usage);
        Val rax = val_gpr(RAX);
        Val rsp = val_gpr(RSP);

        inst2_print(e, MOV, &rax, &imm, TB_X86_TYPE_DWORD);
        inst1(e, CALL, &sym, TB_X86_TYPE_QWORD);
        inst2_print(e, SUB, &rsp, &rax, TB_X86_TYPE_QWORD);
    } else if (stack_usage > 0) {
        if (stack_usage == (int8_t)stack_usage) {
            // sub rsp, stack_usage
            EMIT1(e, rex(true, 0x00, RSP, 0));
            EMIT1(e, 0x83);
            EMIT1(e, mod_rx_rm(MOD_DIRECT, 0x05, RSP));
            EMIT1(e, stack_usage);
        } else {
            // sub rsp, stack_usage
            EMIT1(e, rex(true, 0x00, RSP, 0));
            EMIT1(e, 0x81);
            EMIT1(e, mod_rx_rm(MOD_DIRECT, 0x05, RSP));
            EMIT4(e, stack_usage);
        }
    }

    return e->count;
}

static void emit_epilogue(Ctx* restrict ctx) {
    uint64_t saved = ctx->regs_to_save, stack_usage = ctx->stack_usage;
    TB_CGEmitter* e = &ctx->emit;

    if (stack_usage <= 16) {
        return;
    }

    size_t start = e->count;

    // add rsp, N
    if (stack_usage > 0) {
        if (stack_usage == (int8_t)stack_usage) {
            EMIT1(&ctx->emit, rex(true, 0x00, RSP, 0));
            EMIT1(&ctx->emit, 0x83);
            EMIT1(&ctx->emit, mod_rx_rm(MOD_DIRECT, 0x00, RSP));
            EMIT1(&ctx->emit, (int8_t) stack_usage);
        } else {
            EMIT1(&ctx->emit, rex(true, 0x00, RSP, 0));
            EMIT1(&ctx->emit, 0x81);
            EMIT1(&ctx->emit, mod_rx_rm(MOD_DIRECT, 0x00, RSP));
            EMIT4(&ctx->emit, stack_usage);
        }
    }

    // pop rbp
    if (stack_usage > 0) {
        EMIT1(&ctx->emit, 0x58 + RBP);
    }
}

static size_t emit_call_patches(TB_Module* restrict m, TB_FunctionOutput* out_f) {
    size_t r = 0;
    uint32_t src_section = out_f->section;

    for (TB_SymbolPatch* patch = out_f->first_patch; patch; patch = patch->next) {
        if (patch->target->tag == TB_SYMBOL_FUNCTION) {
            uint32_t dst_section = ((TB_Function*) patch->target)->output->section;

            // you can't do relocations across sections
            if (src_section == dst_section) {
                assert(patch->pos < out_f->code_size);

                // x64 thinks of relative addresses as being relative
                // to the end of the instruction or in this case just
                // 4 bytes ahead hence the +4.
                size_t actual_pos = out_f->code_pos + patch->pos + 4;

                uint32_t p = ((TB_Function*) patch->target)->output->code_pos - actual_pos;
                memcpy(&out_f->code[patch->pos], &p, sizeof(uint32_t));

                r += 1;
                patch->internal = true;
            }
        }
    }

    return out_f->patch_count - r;
}
#undef E

#define E(fmt, ...) tb_asm_print(e, fmt, ## __VA_ARGS__)
static void disassemble_operands(TB_CGEmitter* e, Disasm* restrict d, TB_X86_Inst inst, size_t pos, int start) {
    bool mem = true, imm = true;
    for (int i = 0; i < 4; i++) {
        if (inst->regs[i] == -1) {
            if (mem && (inst->flags & TB_X86_INSTR_USE_MEMOP)) {
                if (i > 0) E(", ");

                mem = false;

                if (inst->flags & TB_X86_INSTR_USE_RIPMEM) {
                    bool is_label = inst->opcode == 0xE8 || inst->opcode == 0xE9
                        || (inst->opcode >= 0x70   && inst->opcode <= 0x7F)
                        || (inst->opcode >= 0x0F80 && inst->opcode <= 0x0F8F);

                    if (!is_label) E("[");

                    if (d->patch && d->patch->pos == pos + inst->length - 4) {
                        const TB_Symbol* target = d->patch->target;

                        if (target->name[0] == 0) {
                            E("sym%p", target);
                        } else {
                            E("%s", target->name);
                        }
                        d->patch = d->patch->next;
                    } else {
                        uint32_t target = pos + inst->length + inst->disp;
                        int bb = tb_emit_get_label(e, target);
                        uint32_t landed = e->labels[bb] & 0x7FFFFFFF;

                        if (landed != target) {
                            E(".bb%d + %d", bb, (int)target - (int)landed);
                        } else {
                            E(".bb%d", bb);
                        }
                    }

                    if (!is_label) E("]");
                } else {
                    E("%s [", tb_x86_type_name(inst->data_type));
                    if (inst->base != 255) {
                        E("%s", tb_x86_reg_name(inst->base, TB_X86_TYPE_QWORD));
                    }

                    if (inst->index != 255) {
                        E(" + %s*%d", tb_x86_reg_name(inst->index, TB_X86_TYPE_QWORD), 1 << inst->scale);
                    }

                    if (inst->disp > 0) {
                        E(" + %d", inst->disp);
                    } else if (inst->disp < 0) {
                        E(" - %d", -inst->disp);
                    }

                    E("]");
                }
            } else if (imm && (inst->flags & (TB_X86_INSTR_IMMEDIATE | TB_X86_INSTR_ABSOLUTE))) {
                if (i > 0) E(", ");

                imm = false;
                if (inst->flags & TB_X86_INSTR_ABSOLUTE) {
                    E("%#llx", inst->abs);
                } else {
                    E("%d", inst->imm);
                }
            } else {
                break;
            }
        } else {
            if (i > 0) {
                E(", ");

                // special case for certain ops with two data types
                if (inst->flags & TB_X86_INSTR_TWO_DATA_TYPES) {
                    E("%s", tb_x86_reg_name(inst->regs[i], inst->data_type2));
                    continue;
                }
            }

            E("%s", tb_x86_reg_name(inst->regs[i], inst->data_type));
        }
    }
}

static void disassemble(TB_CGEmitter* e, Disasm* restrict d, int bb, size_t pos, size_t end) {
    if (bb >= 0) {
        E(".bb%d:\n", bb);
    }

    while (pos < end) {
        while (d->loc != d->end && d->loc->pos == pos) {
            E("  // %s : line %d\n", d->loc->file->path, d->loc->line);
            d->loc++;
        }

        TB_X86_Inst inst;
        if (!tb_x86_disasm(&inst, end - pos, &e->data[pos])) {
            E("  ERROR\n");
            pos += 1; // skip ahead once... cry
            continue;
        }

        // special syntax:
        //   mov rax, rcx
        //   add rax, rdx   INTO   add rax, rcx, rdx
        if (inst.opcode >= 0x88 && inst.opcode <= 0x8B && inst.regs[0] >= 0) {
            size_t saved = pos;
            size_t next = pos + inst.length;
            pos = next;

            // skip REX
            if ((e->data[pos] & 0xF0) == 0x40) {
                pos += 1;
            }

            // are we an x86 integer op
            int op = (e->data[pos] >> 3) & 7;
            if (op != 0) {
                pos = saved;
                goto normie;
            }

            TB_X86_Inst inst2;
            if (!tb_x86_disasm(&inst2, end - next, &e->data[next]) || inst2.regs[0] != inst.regs[0]) {
                pos = saved;
                goto normie;
            }

            const char* mnemonic = tb_x86_mnemonic(&inst2);
            E("%s ", mnemonic);

            // print operands for mov instruction
            disassemble_operands(e, d, &inst, pos, 0);
            E(", ");
            // print operands for data op without the destination
            disassemble_operands(e, d, &inst2, next, 1);
            E("\n");

            pos = next + inst2.length;
            continue;
        }

        normie:
        const char* mnemonic = tb_x86_mnemonic(&inst);
        E("  ");
        if (inst.flags & TB_X86_INSTR_REP) {
            E("rep ");
        }
        if (inst.flags & TB_X86_INSTR_LOCK) {
            E("lock ");
        }
        E("%s", mnemonic);
        if (inst.data_type >= TB_X86_TYPE_SSE_SS && inst.data_type <= TB_X86_TYPE_SSE_PD) {
            static const char* strs[] = { "ss", "sd", "ps", "pd" };
            E(strs[inst.data_type - TB_X86_TYPE_SSE_SS]);
        }
        E(" ");

        disassemble_operands(e, d, &inst, pos, 0);
        E("\n");

        pos += inst.length;
    }
}
#undef E

ICodeGen tb__x64_codegen = {
    .minimum_addressable_size = 8,
    .pointer_size = 64,
    .emit_win64eh_unwind_info = emit_win64eh_unwind_info,
    .emit_call_patches  = emit_call_patches,
    .get_data_type_size = get_data_type_size,
    .compile_function   = compile_function,
};
