#include "../passes.h"
#include "../codegen/emitter.h"
#include <inttypes.h>
#include <log.h>

static thread_local bool reg_alloc_log;
static TB_CFG* muh_______cfg;

enum {
    CG_VAL_UNRESOLVED = 0,
    CG_VAL_FLAGS      = 1,
    CG_VAL_REGISTER   = 2,
};

enum {
    INST_LABEL = 1024,
    INST_LINE,

    // inline machine code
    INST_INLINE,

    // marks the terminator
    INST_TERMINATOR,
    INST_EPILOGUE,

    // this is where parameters come from
    INST_ENTRY,

    //    XORPS xmm0, xmm0
    // or XOR   eax,  eax
    INST_ZERO,
};

typedef struct Inst Inst;

// the first set of indices are reserved for physical registers, the
// rest are allocated as virtual registers.
typedef int RegIndex;
_Static_assert(sizeof(TB_PhysicalReg) == sizeof(RegIndex), "these should be the same");

typedef struct MachineBB {
    Inst* first;

    // what's the terminator, it helps us walk successors
    TB_Node* end_node;

    int start, end;
    int terminator;

    // local live sets
    Set gen, kill;
    // global
    Set live_in, live_out;
} MachineBB;

typedef struct MachineReg {
    uint8_t class, num;
} MachineReg;

typedef struct {
    int uses;
    RegIndex vreg;
} ValueDesc;

typedef struct LiveInterval LiveInterval;
typedef NL_Map(TB_Node*, MachineBB) MachineBBs;

typedef struct {
    uint32_t* pos;
    TB_Node* target;
} JumpTablePatch;

typedef struct {
    TB_CGEmitter emit;

    TB_Module* module;
    TB_Function* f;
    TB_ABI target_abi;

    int caller_usage;
    TB_Node* fallthrough;

    TB_Passes* p;

    int bb_count;
    int* bb_order;

    // Scheduling
    TB_CFG cfg;
    Worklist worklist; // reusing from TB_Passes.
    ValueDesc* values; // the indices match the GVN.

    DynArray(PhiVal) phi_vals;
    DynArray(JumpTablePatch) jump_table_patches;

    // Regalloc
    DynArray(LiveInterval) intervals;

    // machine output sequences
    Inst *first, *head;
    MachineBBs machine_bbs;

    // Line info
    DynArray(TB_Location) locations;

    // Stack
    uint32_t stack_usage;
    NL_Map(TB_Node*, int) stack_slots;
    DynArray(TB_StackSlot) debug_stack_slots;

    uint64_t regs_to_save;
} Ctx;

static bool fits_into_int8(uint64_t x) {
    int8_t y = x & 0xFF;
    return (int64_t)y == x;
}

static bool fits_into_int32(uint64_t x) {
    uint32_t hi = x >> 32ull;
    return hi == 0 || hi == 0xFFFFFFFF;
}

static void init_regalloc(Ctx* restrict ctx);

static TB_X86_DataType legalize(TB_DataType dt);
static bool is_terminator(int type);
static bool wont_spill_around(int type);
static int classify_reg_class(TB_DataType dt);
static void isel(Ctx* restrict ctx, TB_Node* n, int dst);
static void disassemble(TB_CGEmitter* e, int id, size_t pos, size_t end);
static bool should_rematerialize(TB_Node* n);

static void emit_code(Ctx* restrict ctx, TB_FunctionOutput* restrict func_out, int end);
static void mark_callee_saved_constraints(Ctx* restrict ctx, uint64_t callee_saved[CG_REGISTER_CLASSES]);

static void add_debug_local(Ctx* restrict ctx, TB_Node* n, int pos) {
    ptrdiff_t search = nl_map_get(ctx->f->attribs, n);
    if (search < 0) {
        return;
    }

    // could be costly if you had more than like 50 attributes per stack slot... which you
    // wouldn't do right?
    DynArray(TB_Attrib) attribs = ctx->f->attribs[search].v;
    dyn_array_for(i, attribs) {
        TB_Attrib* a = &attribs[i];
        if (a->tag == TB_ATTRIB_VARIABLE) {
            TB_StackSlot s = {
                .position = pos,
                .storage_type = a->var.storage,
                .name = a->var.name,
            };
            dyn_array_put(ctx->debug_stack_slots, s);
            break;
        }
    }
}

static const char* reg_name(int rg, int num) {
    return (rg == REG_CLASS_XMM ? XMM_NAMES : GPR_NAMES)[num];
}

////////////////////////////////
// Instructions
////////////////////////////////
typedef enum {
    INST_LOCK   = 1,
    INST_REP    = 2,
    INST_REPNE  = 4,

    // operands
    INST_MEM    = 16,
    INST_GLOBAL = 32,  // operand to TB_Symbol*
    INST_NODE   = 64,  // operand to TB_Node*
    INST_ATTRIB = 128, // operand to TB_Attrib*
    INST_IMM    = 256, // operand in imm
    INST_ABS    = 512, // operand in abs

    // memory op
    INST_INDEXED = 1024,
    INST_SPILL   = 2048,
} InstFlags;

struct Inst {
    Inst* next;

    // prefixes
    InstType type;
    InstFlags flags;

    TB_X86_DataType dt;
    int time, mem_slot;

    union {
        TB_Symbol* s;
        TB_Node* n;
        TB_Attrib* a;
    };

    union {
        int32_t imm;
        uint64_t abs;
    };

    int32_t disp;

    uint8_t scale;
    uint8_t out_count, in_count, tmp_count;

    // operands all go after the instruction in memory.
    //
    //    RegIndex outs[out_count];
    //    RegIndex ins[in_count];
    //    RegIndex tmps[tmp_count];
    //
    RegIndex operands[];
};

// generic instructions
static Inst* inst_jmp(TB_Node* target);

static Inst* inst_label(TB_Node* n) {
    Inst* i = TB_ARENA_ALLOC(tmp_arena, Inst);
    *i = (Inst){ .type = INST_LABEL, .flags = INST_NODE, .n = n };
    return i;
}

static Inst* inst_line(TB_Attrib* a) {
    Inst* i = TB_ARENA_ALLOC(tmp_arena, Inst);
    *i = (Inst){ .type = INST_LINE, .flags = INST_ATTRIB, .a = a };
    return i;
}

#define SUBMIT(i) append_inst(ctx, i)
static void append_inst(Ctx* restrict ctx, Inst* inst) {
    ctx->head->next = inst;
    ctx->head = inst;
}

static Inst* alloc_inst(int type, TB_DataType dt, int outs, int ins, int tmps) {
    int total = outs + ins + tmps;
    Inst* i = tb_arena_alloc(tmp_arena, sizeof(Inst) + (total * sizeof(RegIndex)));
    *i = (Inst){ .type = type, .dt = legalize(dt), .out_count = outs, ins, tmps };
    return i;
}

static Inst* inst_move(TB_DataType dt, RegIndex dst, RegIndex src) {
    assert(dst >= 0);
    int machine_dt = legalize(dt);

    Inst* i = tb_arena_alloc(tmp_arena, sizeof(Inst) + (2 * sizeof(RegIndex)));
    *i = (Inst){ .type = machine_dt >= TB_X86_TYPE_SSE_SS ? FP_MOV : MOV, .dt = machine_dt, .out_count = 1, 1 };
    i->operands[0] = dst;
    i->operands[1] = src;
    return i;
}

static Inst* inst_op_global(int type, TB_DataType dt, RegIndex dst, TB_Symbol* s) {
    Inst* i = alloc_inst(type, dt, 1, 1, 0);
    i->flags = INST_GLOBAL;
    i->mem_slot = 1;
    i->operands[0] = dst;
    i->operands[1] = RSP;
    i->s = s;
    return i;
}

static Inst* inst_op_abs(int type, TB_DataType dt, RegIndex dst, uint64_t imm) {
    Inst* i = alloc_inst(type, dt, 1, 0, 0);
    i->flags = INST_ABS;
    i->operands[0] = dst;
    i->abs = imm;
    return i;
}

static Inst* inst_op_rm(int type, TB_DataType dt, RegIndex dst, RegIndex base, RegIndex index, Scale scale, int32_t disp) {
    Inst* i = alloc_inst(type, dt, 1, index >= 0 ? 2 : 1, 0);
    i->flags = INST_MEM | (index >= 0 ? INST_INDEXED : 0);
    i->mem_slot = 1;
    i->operands[0] = dst;
    i->operands[1] = base;
    if (index >= 0) {
        i->operands[2] = index;
    }
    i->disp = disp;
    i->scale = scale;
    return i;
}

static Inst* inst_op_rrm(int type, TB_DataType dt, RegIndex dst, RegIndex src, RegIndex base, RegIndex index, Scale scale, int32_t disp) {
    Inst* i = alloc_inst(type, dt, 1, index >= 0 ? 3 : 2, 0);
    i->flags = INST_MEM | (index >= 0 ? INST_INDEXED : 0);
    i->mem_slot = 2;
    i->operands[0] = dst;
    i->operands[1] = src;
    i->operands[2] = base;
    if (index >= 0) {
        i->operands[3] = index;
    }
    i->disp = disp;
    i->scale = scale;
    return i;
}

static Inst* inst_op_mr(int type, TB_DataType dt, RegIndex base, RegIndex index, Scale scale, int32_t disp, RegIndex src) {
    Inst* i = alloc_inst(type, dt, 0, index >= 0 ? 3 : 2, 0);
    i->flags = INST_MEM | (index >= 0 ? INST_INDEXED : 0);
    i->mem_slot = 0;
    if (index >= 0) {
        i->operands[0] = base;
        i->operands[1] = index;
        i->operands[2] = src;
    } else {
        i->operands[0] = base;
        i->operands[1] = src;
    }
    i->disp = disp;
    i->scale = scale;
    return i;
}

static Inst* inst_op_rri(int type, TB_DataType dt, RegIndex dst, RegIndex src, int32_t imm) {
    Inst* i = alloc_inst(type, dt, 1, 1, 0);
    i->flags = INST_IMM;
    i->operands[0] = dst;
    i->operands[1] = src;
    i->imm = imm;
    return i;
}

static Inst* inst_op_rrr(int type, TB_DataType dt, RegIndex dst, RegIndex lhs, RegIndex rhs) {
    Inst* i = alloc_inst(type, dt, 1, 2, 0);
    i->operands[0] = dst;
    i->operands[1] = lhs;
    i->operands[2] = rhs;
    return i;
}

static Inst* inst_op_rri_tmp(int type, TB_DataType dt, RegIndex dst, RegIndex src, int32_t imm, RegIndex tmp) {
    Inst* i = alloc_inst(type, dt, 1, 1, 1);
    i->flags = INST_IMM;
    i->operands[0] = dst;
    i->operands[1] = src;
    i->operands[2] = tmp;
    i->imm = imm;
    return i;
}

static Inst* inst_op_rrr_tmp(int type, TB_DataType dt, RegIndex dst, RegIndex lhs, RegIndex rhs, RegIndex tmp) {
    Inst* i = alloc_inst(type, dt, 1, 2, 1);
    i->operands[0] = dst;
    i->operands[1] = lhs;
    i->operands[2] = rhs;
    i->operands[3] = tmp;
    return i;
}

static Inst* inst_op_imm(int type, TB_DataType dt, RegIndex dst, int32_t imm) {
    Inst* i = alloc_inst(type, dt, 1, 0, 0);
    i->flags = INST_IMM;
    i->operands[0] = dst;
    i->imm = imm;
    return i;
}

static Inst* inst_op_ri(int type, TB_DataType dt, RegIndex src, int32_t imm) {
    Inst* i = alloc_inst(type, dt, 0, 1, 0);
    i->flags = INST_IMM;
    i->operands[0] = src;
    i->imm = imm;
    return i;
}

static Inst* inst_op_r(int type, TB_DataType dt, RegIndex dst) {
    Inst* i = alloc_inst(type, dt, 1, 0, 0);
    i->operands[0] = dst;
    return i;
}

static Inst* inst_op_rr(int type, TB_DataType dt, RegIndex dst, RegIndex src) {
    Inst* i = alloc_inst(type, dt, 1, 1, 0);
    i->operands[0] = dst;
    i->operands[1] = src;
    return i;
}

static Inst* inst_op_rr_no_dst(int type, TB_DataType dt, RegIndex lhs, RegIndex rhs) {
    Inst* i = alloc_inst(type, dt, 0, 2, 0);
    i->operands[0] = lhs;
    i->operands[1] = rhs;
    return i;
}

static Inst* inst_op_zero(TB_DataType dt, RegIndex dst) {
    Inst* i = alloc_inst(INST_ZERO, dt, 1, 0, 0);
    i->operands[0] = dst;
    return i;
}

////////////////////////////////
// Register allocation
////////////////////////////////
#include "reg_alloc.h"

#define DEF(n, dt) alloc_vreg(ctx, n, dt)
static int alloc_vreg(Ctx* restrict ctx, TB_Node* n, TB_DataType dt) {
    int i = dyn_array_length(ctx->intervals);
    dyn_array_put(ctx->intervals, (LiveInterval){
            .reg_class = classify_reg_class(dt),
            .n = n, .reg = -1, .hint = -1, .assigned = -1,
            .dt = legalize(dt), .split_kid = -1
        });

    LiveRange r = { INT_MAX, INT_MAX };
    dyn_array_put(ctx->intervals[i].ranges, r);
    return i;
}

static void hint_reg(Ctx* restrict ctx, int i, int j) {
    if (ctx->intervals[i].hint < 0) {
        ctx->intervals[i].hint = j;
    }
}

////////////////////////////////
// Data flow analysis
////////////////////////////////
static int liveness(Ctx* restrict ctx, TB_Function* f) {
    size_t interval_count = dyn_array_length(ctx->intervals);
    TB_Arena* arena = tmp_arena;

    // find BB boundaries in sequences
    MachineBBs seq_bb = NULL;
    nl_map_create(seq_bb, ctx->bb_count);

    TB_Node** bbs = ctx->worklist.items;
    int* bb_order = ctx->bb_order;
    FOREACH_N(i, 0, ctx->bb_count) {
        TB_Node* n = bbs[bb_order[i]];
        TB_BasicBlock* bb = &nl_map_get_checked(ctx->cfg.node_to_block, n);

        MachineBB mbb = {
            .end_node = bb->end,
            .gen = set_create_in_arena(arena, interval_count),
            .kill = set_create_in_arena(arena, interval_count),
            .live_in = set_create_in_arena(arena, interval_count),
            .live_out = set_create_in_arena(arena, interval_count)
        };

        nl_map_put(seq_bb, n, mbb);
    }

    // generate local live sets
    int timeline = 4, epilogue = -1;
    // CUIK_TIMED_BLOCK("local liveness")
    {
        if (ctx->first) {
            Inst* restrict inst = ctx->first;
            assert(inst->type == INST_LABEL);

            // initial label
            TB_Node* bb = ctx->worklist.items[0];
            MachineBB* mbb = &nl_map_get_checked(seq_bb, bb);
            mbb->first = inst;
            mbb->start = 2;
            inst->time = 2;
            inst = inst->next;

            for (; inst; inst = inst->next) {
                if (inst->type == INST_LABEL) {
                    nl_map_get_checked(seq_bb, bb).end = timeline;
                    timeline += 2; // reserved two extra spaces at the end of the BB

                    tb_assert(inst->flags & INST_NODE, "label instruction has no TB_Node* for the region");
                    bb = inst->n;
                    mbb = &nl_map_get_checked(seq_bb, bb);
                    mbb->first = inst->next;
                    mbb->start = timeline;
                } else if (is_terminator(inst->type) && mbb->terminator == 0) {
                    mbb->terminator = timeline;
                } else if (inst->type == INST_EPILOGUE) {
                    epilogue = timeline;
                }

                Set* restrict gen = &mbb->gen;
                Set* restrict kill = &mbb->kill;

                inst->time = timeline;
                timeline += 2;

                RegIndex* ins = inst->operands + inst->out_count;
                FOREACH_N(i, 0, inst->in_count) {
                    if (!set_get(kill, ins[i])) {
                        set_put(gen, ins[i]);
                    }
                }

                RegIndex* outs = inst->operands;
                FOREACH_N(i, 0, inst->out_count) {
                    set_put(kill, outs[i]);
                }
            }

            mbb->end = timeline;
        }
    }

    // generate global live sets
    size_t base = dyn_array_length(ctx->worklist.items);

    // all nodes go into the worklist
    FOREACH_REVERSE_N(i, 0, ctx->bb_count) {
        TB_Node* n = bbs[bb_order[i]];
        dyn_array_put(ctx->worklist.items, n);

        // in(bb) = use(bb)
        MachineBB* mbb = &nl_map_get_checked(seq_bb, n);
        set_copy(&mbb->live_in, &mbb->gen);
    }

    while (dyn_array_length(ctx->worklist.items) > base) // CUIK_TIMED_BLOCK("global iter")
    {
        TB_Node* bb = dyn_array_pop(ctx->worklist.items);
        MachineBB* mbb = &nl_map_get_checked(seq_bb, bb);

        Set* restrict live_out = &mbb->live_out;
        set_clear(live_out);

        // walk all successors
        TB_Node* end = mbb->end_node;
        for (User* u = end->users; u; u = u->next) {
            if (cfg_is_control(u->n)) {
                // union with successor's lives
                TB_Node* succ = cfg_get_fallthru(u->n);
                set_union(live_out, &nl_map_get_checked(seq_bb, succ).live_in);
            }
        }

        Set* restrict live_in = &mbb->live_in;
        Set* restrict kill = &mbb->kill;
        Set* restrict gen = &mbb->gen;

        // live_in = (live_out - live_kill) U live_gen
        bool changes = false;
        FOREACH_N(i, 0, (interval_count + 63) / 64) {
            bool changes = false;
            uint64_t new_in = (live_out->data[i] & ~kill->data[i]) | gen->data[i];

            changes |= (live_in->data[i] != new_in);
            live_in->data[i] = new_in;
        }

        // if we have changes, mark the predeccesors
        if (changes) {
            FOREACH_N(i, 0, bb->input_count) {
                dyn_array_put(ctx->worklist.items, get_block_begin(bb->inputs[i]));
            }
        }
    }

    /*if (!strcmp(f->super.name, "WinMain")) {
        FOREACH_N(i, 0, ctx->bb_count) {
            TB_Node* n = bbs[bb_order[i]];
            MachineBB* mbb = &nl_map_get_checked(seq_bb, n);

            bool in = set_get(&mbb->live_in, 83);
            bool out = set_get(&mbb->live_out, 83);
            printf(".bb%d: %s %s\n", bb_order[i], in?"in":"", out?"out":"");
        }
    }*/

    ctx->machine_bbs = seq_bb;
    return epilogue;
}

static ValueDesc* lookup_val(Ctx* restrict ctx, TB_Node* n) {
    return worklist_test(&ctx->worklist, n) ? &ctx->values[n->gvn] : NULL;
}

static void put_val(Ctx* restrict ctx, TB_Node* n, int src) {
    ValueDesc* val = lookup_val(ctx, n);
    if (val) val->vreg = src;
}

// generated lazily to avoid allocating one for a node which is
// always folded.
static RegIndex input_reg(Ctx* restrict ctx, TB_Node* n) {
    ValueDesc* val = lookup_val(ctx, n);
    if (val == NULL) {
        DO_IF(TB_OPTDEBUG_CODEGEN)(log_debug("%s: materialize on the spot for node %zu", ctx->f->super.name, n->gvn));
        int tmp = DEF(n, n->dt);
        isel(ctx, n, tmp);
        return tmp;
    }

    val->uses -= 1;

    if (val->vreg >= 0) {
        return val->vreg;
    } else if (should_rematerialize(n)) {
        DO_IF(TB_OPTDEBUG_CODEGEN)(log_debug("%s: materialize on the spot for node %zu", ctx->f->super.name, n->gvn));
        int tmp = DEF(n, n->dt);
        isel(ctx, n, tmp);
        return tmp;
    } else {
        int i = DEF(n, n->dt);
        return (val->vreg = i);
    }
}

static void use(Ctx* restrict ctx, TB_Node* n) {
    ValueDesc* v = lookup_val(ctx, n);
    if (v != NULL) { v->uses -= 1; }
}

static void fake_unuse(Ctx* restrict ctx, TB_Node* n) {
    ValueDesc* v = lookup_val(ctx, n);
    assert(v != NULL);
    v->uses += 1;
}

static bool on_last_use(Ctx* restrict ctx, TB_Node* n) {
    ValueDesc* v = lookup_val(ctx, n);
    return v ? v->uses == 1 : false;
}

static bool has_users(Ctx* restrict ctx, TB_Node* n) {
    if (n != NULL) {
        ValueDesc* v = lookup_val(ctx, n);
        return v ? v->vreg >= 0 || v->uses > 0 : false;
    }
    return false;
}

static void isel_set_location(Ctx* restrict ctx, TB_Node* n) {
    ptrdiff_t search = nl_map_get(ctx->f->attribs, n);
    if (search < 0) {
        return;
    }

    DynArray(TB_Attrib) attribs = ctx->f->attribs[search].v;
    dyn_array_for(i, attribs) {
        if (attribs[i].tag == TB_ATTRIB_LOCATION) {
            SUBMIT(inst_line(&attribs[i]));
            return;
        }
    }
}

static void isel_region(Ctx* restrict ctx, TB_Node* bb_start, TB_Node* end, size_t rpo_index) {
    assert(dyn_array_length(ctx->worklist.items) == ctx->cfg.block_count);
    TB_Scheduled scheduled = ctx->p->scheduled;
    TB_BasicBlock* bb = nl_map_get_checked(scheduled, bb_start);

    // phase 1: logical schedule
    DynArray(PhiVal) phi_vals = ctx->phi_vals;
    CUIK_TIMED_BLOCK("phase 1") {
        sched_walk(ctx->p, &ctx->worklist, &phi_vals, bb, end, true);

        // schedule params
        if (rpo_index == 0) {
            for (User* use = ctx->f->start_node->users; use; use = use->next) {
                TB_Node* use_n = use->n;
                if (use_n->type == TB_PROJ && !worklist_test_n_set(&ctx->worklist, use_n)) {
                    dyn_array_put(ctx->worklist.items, use_n);
                }
            }
        }
    }

    // phase 2: define all the nodes in this BB
    CUIK_TIMED_BLOCK("phase 2") {
        FOREACH_N(i, ctx->cfg.block_count, dyn_array_length(ctx->worklist.items)) {
            TB_Node* n = ctx->worklist.items[i];

            // track non-dead users
            size_t use_count = 0;
            for (User* use = find_users(ctx->p, n); use; use = use->next) {
                if (nl_map_get(scheduled, use->n) >= 0) use_count++;
            }

            // we don't have to worry about resizing here which is really nice
            ctx->values[n->gvn].uses = use_count;
            ctx->values[n->gvn].vreg = -1;
        }
    }

    // phase 3: within the BB, the phi nodes should view itself as the previous value
    // not the new one we're producing.
    size_t our_phis = dyn_array_length(phi_vals);
    CUIK_TIMED_BLOCK("phase 3") {
        FOREACH_N(i, 0, our_phis) {
            PhiVal* v = &phi_vals[i];

            // mark the proper output, especially before we make the BB-local ones
            v->dst = input_reg(ctx, v->phi);
        }

        if (bb_start->type == TB_REGION) {
            for (User* use = find_users(ctx->p, bb_start); use; use = use->next) {
                if (use->n->type == TB_PHI && use->n->dt.type != TB_MEMORY) {
                    ValueDesc* val = &ctx->values[use->n->gvn];

                    // copy PHI into temporary
                    PhiVal p = { .phi = use->n, .dst = input_reg(ctx, use->n) };
                    dyn_array_put(phi_vals, p);

                    TB_DataType dt = p.phi->dt;
                    int tmp = DEF(NULL, dt);
                    SUBMIT(inst_move(dt, tmp, p.dst));

                    // assign temporary as the PHI until the end of the BB
                    val->vreg = tmp;
                }
            }
        }

        if (rpo_index == 0) {
            isel(ctx, ctx->f->start_node, -1);
        }
    }

    // phase 4: walk all nodes (we're allowed to fold nodes into those which appear later)
    //
    // isel is emitting start->end but we're iterating in reverse order so we need
    // to reverse the instruction stream as we go, it's a linked list so it's not
    // hard.
    int bbid = nl_map_get_checked(ctx->cfg.node_to_block, bb_start).id;
    TB_OPTDEBUG(CODEGEN)(printf("BB %d\n", bbid));

    CUIK_TIMED_BLOCK("phase 4") {
        Inst *head = ctx->head, *last = NULL;
        TB_Node* prev_effect = NULL;
        FOREACH_REVERSE_N(i, ctx->cfg.block_count, dyn_array_length(ctx->worklist.items)) {
            TB_Node* n = ctx->worklist.items[i];
            if (n->type == TB_START) {
                continue;
            }

            ValueDesc* val = lookup_val(ctx, n);

            // if the value hasn't been asked for yet and
            if (n != end && val->vreg < 0 && should_rematerialize(n)) {
                DO_IF(TB_OPTDEBUG_CODEGEN)(
                    printf("  DISCARD %u: ", n->gvn),
                    print_node_sexpr(n, 0),
                    printf("\n")
                );
                continue;
            }

            // attach to dummy list
            Inst dummy;
            dummy.next = NULL;
            ctx->head = &dummy;

            if (n->dt.type == TB_TUPLE || n->dt.type == TB_CONTROL || n->dt.type == TB_MEMORY) {
                TB_OPTDEBUG(CODEGEN)(
                    printf("  EFFECT %u: ", n->gvn),
                    print_node_sexpr(n, 0),
                    printf("\n")
                );


                isel(ctx, n, val->vreg);

                if ((n->input_count > 0 && n->inputs[0]->type == TB_START) || n->type != TB_PROJ) {
                    if (prev_effect != NULL) {
                        isel_set_location(ctx, prev_effect);
                    }
                    prev_effect = n;

                    // find next line
                    /* FOREACH_N(j, i + 1, dyn_array_length(ctx->worklist.items)) {
                        TB_Node* m = ctx->worklist.items[j];
                        if (m->type != TB_PROJ && (m->dt.type == TB_TUPLE || m->dt.type == TB_CONTROL || m->dt.type == TB_MEMORY)) {
                            break;
                        }
                    }*/
                }
            } else if (val->uses > 0 || val->vreg >= 0) {
                if (val->vreg < 0) {
                    val->vreg = DEF(n, n->dt);
                }

                TB_OPTDEBUG(CODEGEN)(
                    printf("  DATA %u: ", n->gvn),
                    print_node_sexpr(n, 0),
                    printf("\n")
                );

                isel(ctx, n, val->vreg);
            } else {
                TB_OPTDEBUG(CODEGEN)(
                    printf("  DEAD %u: ", n->gvn),
                    print_node_sexpr(n, 0),
                    printf("\n")
                );
            }

            Inst* seq_start = dummy.next;
            Inst* seq_end   = ctx->head;
            assert(seq_end->next == NULL);

            if (seq_start != NULL) {
                if (last == NULL) {
                    last = seq_end;
                    head->next = dummy.next;
                } else {
                    Inst* old_next = head->next;
                    head->next = seq_start;
                    seq_end->next = old_next;
                }
            }
        }

        // write location for the top effect
        if (prev_effect != NULL) {
            // attach to dummy list
            Inst dummy;
            dummy.next = NULL;
            ctx->head = &dummy;

            isel_set_location(ctx, prev_effect);

            Inst* seq_start = dummy.next;
            Inst* seq_end   = ctx->head;
            assert(seq_end->next == NULL);

            if (seq_start != NULL) {
                if (last == NULL) {
                    last = seq_end;
                    head->next = dummy.next;
                } else {
                    Inst* old_next = head->next;
                    head->next = seq_start;
                    seq_end->next = old_next;
                }
            }
        }

        // restore the PHI value to normal
        FOREACH_N(i, our_phis, dyn_array_length(phi_vals)) {
            PhiVal* v = &phi_vals[i];
            lookup_val(ctx, v->phi)->vreg = v->dst;
        }

        dyn_array_clear(phi_vals);
        ctx->phi_vals = phi_vals;
        ctx->head = last ? last : head;

        if (end->type != TB_END    && end->type != TB_TRAP &&
            end->type != TB_BRANCH && end->type != TB_UNREACHABLE) {
            TB_OPTDEBUG(CODEGEN)(
                printf("  TERMINATOR %u: ", end->gvn),
                print_node_sexpr(end, 0),
                printf("\n")
            );

            // writeback PHIs
            FOREACH_N(i, 0, our_phis) {
                PhiVal* v = &phi_vals[i];
                TB_DataType dt = v->phi->dt;

                int src = input_reg(ctx, v->n);

                hint_reg(ctx, v->dst, src);
                SUBMIT(inst_move(dt, v->dst, src));
            }

            // implicit goto
            TB_Node* succ = cfg_next_control(end);
            if (ctx->fallthrough != succ) {
                SUBMIT(inst_jmp(succ));
            }
        }
    }

    dyn_array_set_length(ctx->worklist.items, ctx->cfg.block_count);
}

// Codegen through here is done in phases
static void compile_function(TB_Passes* restrict p, TB_FunctionOutput* restrict func_out, const TB_FeatureSet* features, uint8_t* out, size_t out_capacity, bool emit_asm) {
    verify_tmp_arena(p);

    TB_Function* restrict f = p->f;
    DO_IF(TB_OPTDEBUG_PEEP)(log_debug("%s: starting codegen with %d nodes", f->super.name, f->node_count));

    #if 0
    if (!strcmp(f->super.name, "load_jpeg_image")) {
        reg_alloc_log = true;
        tb_pass_print(p);
    } else {
        reg_alloc_log = false;
    }
    #endif

    Ctx ctx = {
        .module = f->super.module,
        .f = f,
        .p = p,
        .target_abi = f->super.module->target_abi,
        .emit = {
            .f = f,
            .output = func_out,
            .data = out,
            .capacity = out_capacity,
        }
    };

    CUIK_TIMED_BLOCK("init regalloc") {
        init_regalloc(&ctx);
    }

    worklist_clear(&p->worklist);
    ctx.values = tb_arena_alloc(tmp_arena, f->node_count * sizeof(ValueDesc));

    // We need to generate a CFG
    ctx.cfg = tb_compute_rpo(f, p);
    muh_______cfg = &ctx.cfg;

    // And perform global scheduling
    tb_pass_schedule(p, ctx.cfg);
    ctx.worklist = p->worklist;

    // allocate more stuff now that we've run stats on the IR
    nl_map_create(ctx.emit.labels, ctx.cfg.block_count);
    nl_map_create(ctx.stack_slots, 8);
    dyn_array_create(ctx.debug_stack_slots, 8);

    worklist_clear_visited(&ctx.worklist);

    // Instruction selection:
    //   we just decide which instructions to emit, which operands are
    //   fixed and which need allocation. For now regalloc is handled
    //   immediately but in theory it could be delayed until all selection
    //   is done.
    ctx.bb_count = 0;
    int* bb_order = ctx.bb_order = tb_arena_alloc(tmp_arena, ctx.cfg.block_count * sizeof(int));

    CUIK_TIMED_BLOCK("isel") {
        assert(dyn_array_length(ctx.worklist.items) == ctx.cfg.block_count);

        // define all PHIs early and sort BB order
        int stop_bb = -1;
        FOREACH_N(i, 0, ctx.cfg.block_count) {
            TB_Node* bb = ctx.worklist.items[i];

            for (User* use = find_users(p, bb); use; use = use->next) {
                TB_Node* n = use->n;
                if (n->type == TB_PHI && n->dt.type != TB_MEMORY) {
                    worklist_test_n_set(&ctx.worklist, n);
                    ctx.values[n->gvn].uses = INT_MAX;
                    ctx.values[n->gvn].vreg = -1;
                }
            }

            TB_Node* end = nl_map_get_checked(ctx.cfg.node_to_block, bb).end;
            if (end->type == TB_END) {
                stop_bb = i;
            } else {
                bb_order[ctx.bb_count++] = i;
            }
        }

        // enter END block at the... end
        if (stop_bb >= 0) {
            bb_order[ctx.bb_count++] = stop_bb;
        }

        TB_Node** bbs = ctx.worklist.items;
        FOREACH_N(i, 0, ctx.bb_count) {
            TB_Node* bb = bbs[bb_order[i]];

            nl_map_put(ctx.emit.labels, bb, 0);

            // find next BB
            ctx.fallthrough = i + 1 < ctx.bb_count ? bbs[bb_order[i + 1]] : NULL;

            Inst* label = inst_label(bb);
            if (ctx.first == NULL) {
                ctx.first = ctx.head = label;
            } else {
                append_inst(&ctx, label);
            }

            TB_Node* end = nl_map_get_checked(ctx.cfg.node_to_block, bb).end;
            isel_region(&ctx, bb, end, i);
        }
    }
    p->worklist = ctx.worklist;

    {
        int end;
        CUIK_TIMED_BLOCK("data flow") {
            end = liveness(&ctx, f);
        }

        // we can in theory have other regalloc solutions and eventually will put
        // graph coloring here.
        ctx.stack_usage = linear_scan(&ctx, f, ctx.stack_usage, end);

        // Arch-specific: convert instruction buffer into actual instructions
        CUIK_TIMED_BLOCK("emit code") {
            emit_code(&ctx, func_out, end);
        }

        // Fill jump table entries
        CUIK_TIMED_BLOCK("jump tables") {
            dyn_array_for(i, ctx.jump_table_patches) {
                uint32_t target = nl_map_get_checked(ctx.emit.labels, ctx.jump_table_patches[i].target);
                assert((target & 0x80000000) && "target label wasn't resolved... what?");
                *ctx.jump_table_patches[i].pos = target & ~0x80000000;
            }
        }
    }

    if (emit_asm) {
        printf("%s:\n", f->super.name);

        // dump epilogue
        disassemble(&ctx.emit, -1, 0, func_out->prologue_length);

        TB_Node** bbs = ctx.worklist.items;
        FOREACH_N(i, 0, ctx.bb_count) {
            TB_Node* bb = bbs[bb_order[i]];

            uint32_t start = nl_map_get_checked(ctx.emit.labels, bb) & ~0x80000000;
            uint32_t end   = ctx.emit.count;
            if (i + 1 < ctx.bb_count) {
                TB_Node* next = bbs[bb_order[i + 1]];
                end = nl_map_get_checked(ctx.emit.labels, next) & ~0x80000000;
            }

            disassemble(&ctx.emit, i, start, end);
        }
    }

    tb_free_cfg(&ctx.cfg);
    nl_map_free(ctx.emit.labels);
    nl_map_free(ctx.machine_bbs);
    dyn_array_destroy(ctx.jump_table_patches);
    dyn_array_destroy(ctx.intervals);
    dyn_array_destroy(ctx.phi_vals);

    if (dyn_array_length(ctx.locations)) {
        ctx.locations[0].pos = 0; // func_out->prologue_length;
    }

    // we're done, clean up
    func_out->asm_out = ctx.emit.head_asm;
    func_out->code = ctx.emit.data;
    func_out->code_size = ctx.emit.count;
    func_out->stack_usage = ctx.stack_usage;
    func_out->locations = ctx.locations;
    func_out->stack_slots = ctx.debug_stack_slots;
    nl_map_free(ctx.stack_slots);
}

static void get_data_type_size(TB_DataType dt, size_t* out_size, size_t* out_align) {
    switch (dt.type) {
        case TB_INT: {
            // above 64bits we really dont care that much about natural alignment
            bool is_big_int = dt.data > 64;

            // round up bits to a byte
            int bits = is_big_int ? ((dt.data + 7) / 8) : tb_next_pow2(dt.data - 1);

            *out_size  = ((bits+7) / 8);
            *out_align = is_big_int ? 8 : ((dt.data + 7) / 8);
            break;
        }
        case TB_FLOAT: {
            int s = 0;
            if (dt.data == TB_FLT_32) s = 4;
            else if (dt.data == TB_FLT_64) s = 8;
            else tb_unreachable();

            *out_size = s;
            *out_align = s;
            break;
        }
        case TB_PTR: {
            *out_size = 8;
            *out_align = 8;
            break;
        }
        default: tb_unreachable();
    }
}
