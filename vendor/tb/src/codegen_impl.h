// See codegen.h for more details, this is the implementation file for it, each target
// will include this to define their own copy of the codegen.
//
// Your job is to implement:
//   isel_node, init_ctx, emit_tile, disassemble.
//
#include "codegen.h"

static RegMask isel_node(Ctx* restrict ctx, Tile* dst, TB_Node* n);
static void init_ctx(Ctx* restrict ctx, TB_ABI abi);

static bool clobbers(Ctx* restrict ctx, Tile* t, uint64_t clobbers[MAX_REG_CLASSES]);

// This is where we do the byte emitting phase
static void emit_tile(Ctx* restrict ctx, TB_CGEmitter* e, Tile* t);

// Used for post-processing after regalloc (prologue mostly)
static void pre_emit(Ctx* restrict ctx, TB_CGEmitter* e, TB_Node* n);

// Write bytes after every tile's emitted, used by x86 for NOP padding
static void post_emit(Ctx* restrict ctx, TB_CGEmitter* e);

// Disassembles a basic block
static void disassemble(TB_CGEmitter* e, Disasm* restrict d, int bb, size_t pos, size_t end);

static uint32_t node_to_bb_hash(void* ptr) { return (((uintptr_t) ptr) * 11400714819323198485ull) >> 32ull; }
static MachineBB* node_to_bb(Ctx* restrict ctx, TB_Node* n) {
    uint32_t h = node_to_bb_hash(n);

    size_t mask = (1 << ctx->node_to_bb.exp) - 1;
    size_t first = h & mask, i = first;
    do {
        if (ctx->node_to_bb.entries[i].k == n) {
            return ctx->node_to_bb.entries[i].v;
        }

        i = (i + 1) & mask;
    } while (i != first);

    abort();
}

static void node_to_bb_put(Ctx* restrict ctx, TB_Node* n, MachineBB* bb) {
    uint32_t h = node_to_bb_hash(n);

    size_t mask = (1 << ctx->node_to_bb.exp) - 1;
    size_t first = h & mask, i = first;
    do {
        if (ctx->node_to_bb.entries[i].k == NULL) {
            ctx->node_to_bb.entries[i].k = n;
            ctx->node_to_bb.entries[i].v = bb;
            return;
        }

        i = (i + 1) & mask;
    } while (i != first);

    abort();
}

static ValueDesc* val_at(Ctx* restrict ctx, TB_Node* n) {
    if (ctx->values[n->gvn].use_count < 0) {
        int count = 0;
        FOR_USERS(u, n) count++;
        ctx->values[n->gvn].use_count = count;
    }

    return &ctx->values[n->gvn];
}

static Tile* get_tile(Ctx* restrict ctx, TB_Node* n, bool alloc_interval) {
    ValueDesc* val = val_at(ctx, n);
    if (val->tile == NULL) {
        Tile* tile = TB_ARENA_ALLOC(tmp_arena, Tile);
        *tile = (Tile){ .tag = TILE_NORMAL, .n = n };
        if (alloc_interval) {
            tile->interval = TB_ARENA_ALLOC(tmp_arena, LiveInterval);
        }
        val->tile = tile;
    }

    return val->tile;
}

// you're expected to set the masks in the returned array
static TileInput* tile_set_ins(Ctx* restrict ctx, Tile* t, TB_Node* n, int start, int end) {
    t->ins = tb_arena_alloc(tmp_arena, (end - start) * sizeof(TileInput));
    t->in_count = end - start;
    FOREACH_N(i, start, end) {
        t->ins[i - start].src = get_tile(ctx, n->inputs[i], true)->interval;
    }
    return t->ins;
}

// fills all inputs with the same mask
static TileInput* tile_broadcast_ins(Ctx* restrict ctx, Tile* t, TB_Node* n, int start, int end, RegMask rm) {
    t->ins = tb_arena_alloc(tmp_arena, (end - start) * sizeof(TileInput));
    t->in_count = end - start;
    FOREACH_N(i, start, end) {
        t->ins[i - start].src = get_tile(ctx, n->inputs[i], true)->interval;
        t->ins[i - start].mask = rm;
    }
    return t->ins;
}

static LiveInterval* tile_make_interval(Ctx* restrict ctx, TB_Arena* arena, LiveInterval* interval) {
    if (interval == NULL) {
        interval = TB_ARENA_ALLOC(arena, LiveInterval);
    }

    // construct live interval
    *interval = (LiveInterval){
        .id = ctx->interval_count++,
        .reg = -1,
        .assigned = -1,
        .range_cap = 4, .range_count = 1,
        .ranges = tb_platform_heap_alloc(4 * sizeof(LiveRange))
    };
    interval->ranges[0] = (LiveRange){ INT_MAX, INT_MAX };
    return interval;
}

static int try_init_stack_slot(Ctx* restrict ctx, TB_Node* n) {
    if (n->type == TB_LOCAL) {
        TB_NodeLocal* local = TB_NODE_GET_EXTRA(n);
        ptrdiff_t search = nl_map_get(ctx->stack_slots, n);
        if (search >= 0) {
            return ctx->stack_slots[search].v;
        } else {
            ctx->stack_usage = align_up(ctx->stack_usage + local->size, local->align);
            nl_map_put(ctx->stack_slots, n, ctx->stack_usage);
            return ctx->stack_usage;
        }
    } else {
        return 0;
    }
}

static int get_stack_slot(Ctx* restrict ctx, TB_Node* n) {
    int pos = nl_map_get_checked(ctx->stack_slots, n);
    return ctx->stack_usage - pos;
}

static LiveInterval* canonical_interval(Ctx* restrict ctx, LiveInterval* interval, RegMask mask) {
    int reg = fixed_reg_mask(mask);
    if (reg >= 0) {
        return &ctx->fixed[mask.class][reg];
    } else {
        return interval;
    }
}

void tb__print_regmask(RegMask mask) {
    if (mask.class == REG_CLASS_STK) {
        if (mask.mask == 0) {
            printf("[any spill]");
        } else {
            printf("[SP + %"PRId64"]", mask.mask*8);
        }
    } else {
        int i = 0;
        bool comma = false;
        uint64_t bits = mask.mask;

        printf("[");
        while (bits) {
            // skip zeros
            int skip = __builtin_ffs(bits) - 1;
            i += skip, bits >>= skip;

            if (!comma) {
                comma = true;
            } else {
                printf(", ");
            }

            // find sequence of ones
            int len = __builtin_ffs(~bits) - 1;
            printf("R%d", i);
            if (len > 1) {
                printf(" .. R%d", i+len-1);
            }

            // skip ones
            bits >>= len, i += len;
        }

        if (mask.may_spill) {
            printf(" | SPILL");
        }
        printf("]");
    }
}

static void compile_function(TB_Passes* restrict p, TB_FunctionOutput* restrict func_out, const TB_FeatureSet* features, TB_Arena* code_arena, bool emit_asm) {
    verify_tmp_arena(p);

    TB_Arena* arena = tmp_arena;
    TB_ArenaSavepoint sp = tb_arena_save(arena);

    TB_Function* restrict f = p->f;
    TB_OPTDEBUG(CODEGEN)(tb_pass_print(p));

    Ctx ctx = {
        .module = f->super.module,
        .f = f,
        .p = p,
        .num_classes = REG_CLASS_COUNT,
        .emit = {
            .output = func_out,
            .arena = arena,
        }
    };

    // allocate entire top of the code arena (we'll trim it later if possible)
    ctx.emit.capacity = code_arena->high_point - code_arena->watermark;
    ctx.emit.data = tb_arena_alloc(code_arena, ctx.emit.capacity);

    if (features == NULL) {
        ctx.features = (TB_FeatureSet){ 0 };
    } else {
        ctx.features = *features;
    }

    Worklist* restrict ws = &p->worklist;
    worklist_clear(ws);

    TB_CFG cfg;
    CUIK_TIMED_BLOCK("global sched") {
        // We need to generate a CFG
        cfg = tb_compute_rpo(f, p);
        // And perform global scheduling
        tb_pass_schedule(p, cfg, false);
    }

    nl_map_create(ctx.stack_slots, 4);
    init_ctx(&ctx, f->super.module->target_abi);

    ctx.values = tb_arena_alloc(arena, f->node_count * sizeof(Tile*));
    memset(ctx.values, 0, f->node_count * sizeof(Tile*));

    ctx.values = tb_arena_alloc(arena, f->node_count * sizeof(ValueDesc));
    FOREACH_N(i, 0, f->node_count) {
        ctx.values[i].use_count = -1;
        ctx.values[i].mat_count = 0;
        ctx.values[i].tile = NULL;
    }

    // allocate more stuff now that we've run stats on the IR
    ctx.emit.label_count = cfg.block_count;
    ctx.emit.labels = tb_arena_alloc(arena, cfg.block_count * sizeof(uint32_t));
    memset(ctx.emit.labels, 0, cfg.block_count * sizeof(uint32_t));

    int bb_count = 0;
    MachineBB* restrict machine_bbs = tb_arena_alloc(arena, cfg.block_count * sizeof(MachineBB));
    TB_Node** bbs = ws->items;

    size_t cap = ((cfg.block_count * 4) / 3);
    ctx.node_to_bb.exp = 64 - __builtin_clzll((cap < 4 ? 4 : cap) - 1);
    ctx.node_to_bb.entries = tb_arena_alloc(arena, (1u << ctx.node_to_bb.exp) * sizeof(NodeToBB));
    memset(ctx.node_to_bb.entries, 0, (1u << ctx.node_to_bb.exp) * sizeof(NodeToBB));

    CUIK_TIMED_BLOCK("isel") {
        assert(dyn_array_length(ws->items) == cfg.block_count);

        // define all PHIs early and sort BB order
        int stop_bb = -1;
        FOREACH_N(i, 0, cfg.block_count) {
            TB_Node* end = nl_map_get_checked(cfg.node_to_block, bbs[i]).end;
            if (end->type == TB_ROOT) {
                stop_bb = i;
            } else {
                machine_bbs[bb_count++] = (MachineBB){ i };
            }
        }

        // enter END block at the... end
        if (stop_bb >= 0) {
            machine_bbs[bb_count++] = (MachineBB){ stop_bb };
        }

        // build blocks in reverse
        DynArray(PhiVal) phi_vals = NULL;
        FOREACH_REVERSE_N(i, 0, bb_count) {
            int bbid = machine_bbs[i].id;
            TB_Node* bb_start = bbs[bbid];
            TB_BasicBlock* bb = p->scheduled[bb_start->gvn];

            node_to_bb_put(&ctx, bb_start, &machine_bbs[i]);
            size_t base = dyn_array_length(ws->items);

            // phase 1: logical schedule
            CUIK_TIMED_BLOCK("phase 1") {
                dyn_array_clear(phi_vals);
                ctx.sched(p, &cfg, ws, &phi_vals, bb, bb->end);
            }

            // phase 2: reverse walk to generate tiles (greedily)
            CUIK_TIMED_BLOCK("phase 2") {
                TB_OPTDEBUG(CODEGEN)(printf("BB %d\n", bbid));

                // force materialization of phi ins
                FOREACH_N(i, 0, dyn_array_length(phi_vals)) {
                    PhiVal* v = &phi_vals[i];
                    get_tile(&ctx, v->n, true);
                }

                Tile* top = NULL;
                Tile* bot = NULL;
                TB_NodeLocation* last_loc = NULL;
                FOREACH_REVERSE_N(i, cfg.block_count, dyn_array_length(ws->items)) {
                    TB_Node* n = ws->items[i];
                    if (n->type == TB_PHI) {
                        continue;
                    } else if (n->type != TB_MULPAIR && (n->dt.type == TB_TUPLE || n->dt.type == TB_CONTROL || n->dt.type == TB_MEMORY)) {
                        // these are always run because they're cool effect nodes
                        TB_NodeLocation* v;
                        if (v = nl_table_get(&f->locations, n), v) {
                            if (last_loc) {
                                TB_OPTDEBUG(CODEGEN)(printf("  LOCATION\n"));

                                Tile* loc = TB_ARENA_ALLOC(arena, Tile);
                                *loc = (Tile){ .tag = TILE_LOCATION, .loc = last_loc };
                                loc->next = top;

                                // attach to list
                                if (top) top->prev = loc;
                                if (!bot) bot = loc;
                                top = loc;
                            }
                            last_loc = v;
                        }
                    } else {
                        if (ctx.values[n->gvn].tile == NULL) {
                            TB_OPTDEBUG(CODEGEN)(printf("  FOLDED "), print_node_sexpr(n, 0), printf("\n"));
                            continue;
                        }
                    }

                    TB_OPTDEBUG(CODEGEN)(printf("  TILE "), print_node_sexpr(n, 0), printf("\n"));

                    Tile* tile = get_tile(&ctx, n, false);
                    tile->next = top;

                    // attach to list
                    if (top) top->prev = tile;
                    if (!bot) bot = tile;
                    top = tile;

                    RegMask mask = isel_node(&ctx, tile, n);
                    if (mask.mask != 0) {
                        // construct live interval
                        tile->interval = tile_make_interval(&ctx, arena, tile->interval);
                        tile->interval->tile = tile;
                        tile->interval->dt = n->dt;
                        tile->interval->mask = mask;

                        TB_OPTDEBUG(CODEGEN)(printf("    v%d ", tile->interval->id), tb__print_regmask(mask), printf("\n"));
                    } else {
                        assert(tile->interval == NULL && "shouldn't have allocated an interval... tf");
                        TB_OPTDEBUG(CODEGEN)(printf("    no def\n"));
                    }

                    FOREACH_N(j, 0, tile->in_count) {
                        TB_OPTDEBUG(CODEGEN)(printf("    IN[%zu] = ", j), tb__print_regmask(tile->ins[j].mask), printf("\n"));
                    }
                }

                if (last_loc) {
                    TB_OPTDEBUG(CODEGEN)(printf("  LOCATION\n"));

                    Tile* loc = TB_ARENA_ALLOC(arena, Tile);
                    *loc = (Tile){ .tag = TILE_LOCATION, .loc = last_loc };
                    loc->next = top;

                    // attach to list
                    if (top) top->prev = loc;
                    if (!bot) bot = loc;
                    top = loc;
                }

                // if the endpoint is a not a terminator, we've hit some implicit GOTO edge
                TB_Node* end = bb->end;
                if (!cfg_is_terminator(end)) {
                    TB_OPTDEBUG(CODEGEN)(printf("  TERMINATOR %u: ", end->gvn), print_node_sexpr(end, 0), printf("\n"));

                    // writeback phis
                    FOREACH_N(i, 0, dyn_array_length(phi_vals)) {
                        PhiVal* v = &phi_vals[i];

                        Tile* phi_tile = get_tile(&ctx, v->phi, false);

                        // PHIs are weird because they have multiple tiles with the same destination.
                        // post phi elimination we don't have "SSA" really.
                        phi_tile->interval = tile_make_interval(&ctx, arena, phi_tile->interval);
                        phi_tile->interval->tile = phi_tile;
                        phi_tile->interval->dt = v->phi->dt;
                        phi_tile->interval->mask = isel_node(&ctx, phi_tile, v->phi);

                        LiveInterval* src = get_tile(&ctx, v->n, true)->interval;

                        TB_OPTDEBUG(CODEGEN)(printf("  PHI %u: ", v->phi->gvn), print_node_sexpr(v->phi, 0), printf("\n"));
                        TB_OPTDEBUG(CODEGEN)(printf("    v%d ", phi_tile->interval->id), tb__print_regmask(phi_tile->interval->mask), printf("\n"));

                        Tile* move = TB_ARENA_ALLOC(arena, Tile);
                        *move = (Tile){ .prev = bot, .tag = TILE_SPILL_MOVE, .interval = phi_tile->interval };
                        move->spill_dt = v->phi->dt;
                        move->n = v->phi;
                        move->ins = tb_arena_alloc(tmp_arena, sizeof(Tile*));
                        move->in_count = 1;
                        move->ins[0].src  = src;
                        move->ins[0].mask = phi_tile->interval->mask;
                        bot->next = move;
                        bot = move;
                    }

                    Tile* tile = TB_ARENA_ALLOC(arena, Tile);
                    TB_Node* succ_n = cfg_next_control(end);
                    *tile = (Tile){ .prev = bot, .tag = TILE_GOTO, .succ = succ_n };
                    bot->next = tile;
                    bot = tile;
                }
                dyn_array_set_length(ws->items, base);

                machine_bbs[bbid].start = top;
                machine_bbs[bbid].end = bot;
                machine_bbs[bbid].n = bb_start;
                machine_bbs[bbid].end_n = end;
            }
        }
        dyn_array_destroy(phi_vals);
        log_debug("%s: tmp_arena=%.1f KiB (postISel)", f->super.name, tb_arena_current_size(arena) / 1024.0f);
    }

    CUIK_TIMED_BLOCK("create physical intervals") {
        FOREACH_N(i, 0, ctx.num_classes) {
            LiveInterval* intervals = tb_arena_alloc(arena, ctx.num_regs[i] * sizeof(LiveInterval));
            FOREACH_N(j, 0, ctx.num_regs[i]) {
                intervals[j] = (LiveInterval){
                    .id = ctx.interval_count++,
                    .assigned = -1, .hint = NULL, .reg = j,
                    .mask = { i, 0, 1u << j },
                    .class = i,
                    .range_cap = 4,
                    .range_count = 1,
                };
                intervals[j].ranges = tb_arena_alloc(arena, 4 * sizeof(LiveRange));
                intervals[j].ranges[0] = (LiveRange){ INT_MAX, INT_MAX };
            }
            ctx.fixed[i] = intervals;
            ctx.num_fixed += ctx.num_regs[i];
        }
    }

    CUIK_TIMED_BLOCK("liveness") {
        int interval_count = ctx.interval_count;
        ctx.id2interval = tb_arena_alloc(arena, interval_count * sizeof(Tile*));

        // local liveness (along with placing tiles on a timeline)
        FOREACH_N(i, 0, bb_count) {
            MachineBB* mbb = &machine_bbs[i];
            mbb->live_in = set_create_in_arena(arena, interval_count);
            mbb->live_out = set_create_in_arena(arena, interval_count);
        }

        // we don't need to keep the GEN and KILL sets, this doesn't save us
        // much memory but it would potentially mean not using new cachelines
        // in a few of the later stages.
        TB_ArenaSavepoint sp = tb_arena_save(arena);
        CUIK_TIMED_BLOCK("local") {
            FOREACH_N(i, 0, bb_count) {
                MachineBB* mbb = &machine_bbs[i];
                int bbid = mbb->id;

                mbb->gen = set_create_in_arena(arena, interval_count);
                mbb->kill = set_create_in_arena(arena, interval_count);

                Set* gen = &mbb->gen;
                Set* kill = &mbb->kill;
                for (Tile* t = mbb->start; t; t = t->next) {
                    FOREACH_N(j, 0, t->in_count) {
                        LiveInterval* in_def = t->ins[j].src;
                        if (in_def && !set_get(kill, in_def->id)) {
                            set_put(gen, in_def->id);
                        }
                    }

                    LiveInterval* interval = t->interval;
                    if (interval) {
                        set_put(kill, interval->id);
                        ctx.id2interval[interval->id] = interval;
                    }
                }
            }
        }

        // generate global live sets
        CUIK_TIMED_BLOCK("global") {
            size_t base = dyn_array_length(ws->items);

            // all BB go into the worklist
            FOREACH_REVERSE_N(i, 0, bb_count) {
                // in(bb) = use(bb)
                set_copy(&machine_bbs[i].live_in, &machine_bbs[i].gen);

                TB_Node* n = bbs[machine_bbs[i].id];
                dyn_array_put(ws->items, n);
            }

            Set visited = set_create_in_arena(arena, bb_count);
            while (dyn_array_length(ws->items) > base) CUIK_TIMED_BLOCK("iter")
            {
                TB_Node* bb = dyn_array_pop(ws->items);
                MachineBB* mbb = node_to_bb(&ctx, bb);
                set_remove(&visited, mbb - machine_bbs);

                Set* live_out = &mbb->live_out;
                set_clear(live_out);

                // walk all successors
                TB_Node* end = mbb->end_n;
                if (end->type == TB_BRANCH) {
                    FOR_USERS(u, end) {
                        if (u->n->type == TB_PROJ) {
                            // union with successor's lives
                            TB_Node* succ = cfg_next_bb_after_cproj(u->n);
                            set_union(live_out, &node_to_bb(&ctx, succ)->live_in);
                        }
                    }
                } else {
                    // union with successor's lives
                    TB_Node* succ = cfg_next_control(end);
                    if (succ) set_union(live_out, &node_to_bb(&ctx, succ)->live_in);
                }

                Set* restrict live_in = &mbb->live_in;
                Set* restrict kill = &mbb->kill;
                Set* restrict gen = &mbb->gen;

                // live_in = (live_out - live_kill) U live_gen
                bool changes = false;
                FOREACH_N(i, 0, (interval_count + 63) / 64) {
                    uint64_t new_in = (live_out->data[i] & ~kill->data[i]) | gen->data[i];

                    changes |= (live_in->data[i] != new_in);
                    live_in->data[i] = new_in;
                }

                // if we have changes, mark the predeccesors
                if (changes && !(bb->type == TB_PROJ && bb->inputs[0]->type == TB_ROOT)) {
                    FOREACH_N(i, 0, bb->input_count) {
                        TB_Node* pred = get_pred_cfg(&cfg, bb, i);
                        if (pred->input_count > 0) {
                            MachineBB* pred_mbb = node_to_bb(&ctx, pred);
                            if (!set_get(&visited, pred_mbb - machine_bbs)) {
                                set_put(&visited, pred_mbb - machine_bbs);
                                dyn_array_put(ws->items, pred);
                            }
                        }
                    }
                }
            }
            dyn_array_set_length(ws->items, base);
        }

        #if TB_OPTDEBUG_DATAFLOW
        // log live ins and outs
        FOREACH_N(i, 0, bb_count) {
            MachineBB* mbb = &machine_bbs[i];

            printf("BB%d:\n  live-ins:", mbb->id);
            FOREACH_N(j, 0, interval_count) if (set_get(&mbb->live_in, j)) {
                printf(" v%zu", j);
            }
            printf("\n  live-outs:");
            FOREACH_N(j, 0, interval_count) if (set_get(&mbb->live_out, j)) {
                printf(" v%zu", j);
            }
            printf("\n  gen:");
            FOREACH_N(j, 0, interval_count) if (set_get(&mbb->gen, j)) {
                printf(" v%zu", j);
            }
            printf("\n  kill:");
            FOREACH_N(j, 0, interval_count) if (set_get(&mbb->kill, j)) {
                printf(" v%zu", j);
            }
            printf("\n");
        }
        #endif

        log_debug("%s: tmp_arena=%.1f KiB (dataflow)", f->super.name, tb_arena_current_size(arena) / 1024.0f);
        tb_arena_restore(arena, sp);
    }

    CUIK_TIMED_BLOCK("regalloc") {
        log_debug("%s: tmp_arena=%.1f KiB (preRA)", f->super.name, tb_arena_current_size(arena) / 1024.0f);

        ctx.bb_count = bb_count;
        ctx.machine_bbs = machine_bbs;
        ctx.regalloc(&ctx, arena);

        log_debug("%s: tmp_arena=%.1f KiB (postRA)", f->super.name, tb_arena_current_size(arena) / 1024.0f);
    }

    CUIK_TIMED_BLOCK("emit") {
        TB_CGEmitter* e = &ctx.emit;
        pre_emit(&ctx, e, f->root_node);

        FOREACH_N(i, 0, bb_count) {
            int bbid = machine_bbs[i].id;
            Tile* t = machine_bbs[i].start;

            if (i + 1 < bb_count) {
                ctx.fallthrough = machine_bbs[i + 1].id;
            } else {
                ctx.fallthrough = INT_MAX;
            }
            ctx.current_emit_bb = &machine_bbs[i];
            ctx.current_emit_bb_pos = GET_CODE_POS(e);

            // line info is BB-local
            TB_NodeLocation* last_loc = NULL;

            // mark label
            tb_resolve_rel32(e, &e->labels[bbid], e->count);
            while (t) {
                if (t->tag == TILE_LOCATION) {
                    TB_Location l = {
                        .file = t->loc->file,
                        .line = t->loc->line,
                        .column = t->loc->column,
                        .pos = GET_CODE_POS(e)
                    };

                    size_t top = dyn_array_length(ctx.locations);
                    if (top == 0 || (ctx.locations[top - 1].pos != l.pos && !is_same_location(&l, &ctx.locations[top - 1]))) {
                        dyn_array_put(ctx.locations, l);
                    }
                } else {
                    emit_tile(&ctx, e, t);
                }
                t = t->next;
            }
        }

        post_emit(&ctx, e);

        // Fill jump table entries
        CUIK_TIMED_BLOCK("jump tables") {
            dyn_array_for(i, ctx.jump_table_patches) {
                uint32_t target = ctx.emit.labels[ctx.jump_table_patches[i].target];
                assert((target & 0x80000000) && "target label wasn't resolved... what?");
                *ctx.jump_table_patches[i].pos = target & ~0x80000000;
            }
        }
    }

    if (ctx.locations) {
        ctx.locations[0].pos = 0;
    }

    // trim code arena (it fits in a single chunk so just arena free the top)
    code_arena->watermark = (char*) &ctx.emit.data[ctx.emit.count];
    tb_arena_realign(code_arena);

    // TODO(NeGate): move the assembly output to code arena
    if (emit_asm) CUIK_TIMED_BLOCK("dissassembly") {
        dyn_array_for(i, ctx.debug_stack_slots) {
            TB_StackSlot* s = &ctx.debug_stack_slots[i];
            EMITA(&ctx.emit, "// %s = [rsp + %d]\n", s->name, s->storage.offset);
        }
        EMITA(&ctx.emit, "%s:\n", f->super.name);

        Disasm d = { func_out->first_patch, ctx.locations, &ctx.locations[dyn_array_length(ctx.locations)] };

        if (ctx.prologue_length) {
            disassemble(&ctx.emit, &d, -1, 0, ctx.prologue_length);
        }

        FOREACH_N(i, 0, bb_count) {
            int bbid = machine_bbs[i].id;
            TB_Node* bb = bbs[bbid];

            uint32_t start = ctx.emit.labels[bbid] & ~0x80000000;
            uint32_t end   = ctx.emit.count;
            if (i + 1 < bb_count) {
                end = ctx.emit.labels[machine_bbs[i + 1].id] & ~0x80000000;
            }

            disassemble(&ctx.emit, &d, bbid, start, end);
        }
    }

    // cleanup memory
    nl_map_free(ctx.stack_slots);
    tb_free_cfg(&cfg);

    log_debug("%s: code_arena=%.1f KiB", f->super.name, tb_arena_current_size(code_arena) / 1024.0f);
    tb_arena_restore(arena, sp);
    p->scheduled = NULL;

    // we're done, clean up
    func_out->asm_out = ctx.emit.head_asm;
    func_out->code = ctx.emit.data;
    func_out->code_size = ctx.emit.count;
    func_out->locations = ctx.locations;
    func_out->stack_slots = ctx.debug_stack_slots;
    func_out->stack_usage = ctx.stack_usage;
    func_out->prologue_length = ctx.prologue_length;
    func_out->epilogue_length = ctx.epilogue_length;
    func_out->nop_pads = ctx.nop_pads;
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
