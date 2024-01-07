// Linear scan register allocator:
//   https://ssw.jku.at/Research/Papers/Wimmer04Master/Wimmer04Master.pdf
#include "codegen.h"

#define FOREACH_SET(it, set) \
FOREACH_N(_i, 0, ((set).capacity + 63) / 64) FOREACH_BIT(it, _i*64, (set).data[_i])

typedef struct {
    Ctx* ctx;
    TB_Arena* arena;

    int spills;
    int num_classes;
    int* num_regs;
    uint64_t* callee_saved;

    // time when the physical registers will be free again
    int* free_pos;
    int* block_pos;

    size_t stack_slot_cap;

    // waiting to get registers, sorted such that the top most item is the youngest
    DynArray(LiveInterval*) unhandled;
    DynArray(LiveInterval*) inactive;
    DynArray(CalleeSpill) callee_spills;
    RegMask* normie_mask;

    Set active_set[MAX_REG_CLASSES];
    LiveInterval** active[MAX_REG_CLASSES];
    LiveInterval* fixed[MAX_REG_CLASSES];
} LSRA;

// Forward decls... yay
static void insert_split_move(LSRA* restrict ra, int t, LiveInterval* old_it, LiveInterval* new_it);
static void cuiksort_defs(LiveInterval** intervals, ptrdiff_t lo, ptrdiff_t hi);
static bool update_interval(LSRA* restrict ra, LiveInterval* interval, bool is_active, int time, int inactive_index);
static ptrdiff_t allocate_free_reg(LSRA* restrict ra, LiveInterval* interval);
static LiveInterval* split_intersecting(LSRA* restrict ra, int pos, LiveInterval* interval, RegMask new_mask);
static void move_to_active(LSRA* restrict ra, LiveInterval* interval);

static const char* GPR_NAMES[] = { "RAX", "RCX", "RDX", "RBX", "RSP", "RBP", "RSI", "RDI", "R8",  "R9", "R10", "R11", "R12", "R13", "R14", "R15" };

// static const char* GPR_NAMES[] = { "X0", "X1", "X2", "X3", "X4", "X5", "X6", "X7", "X8",  "X9", "X10", "X11", "X12", "X13", "X14", "X15" };
static void print_reg_name(int rg, int num) {
    if (rg == 1) {
        printf("R%d", num);
        // printf("%s", GPR_NAMES[num]);
    } else if (rg == 2) {
        printf("XMM%d", num);
    } else if (rg == REG_CLASS_STK) {
        printf("[sp + %d]", num*8);
    } else {
        tb_todo();
    }
}

// Helpers
static int interval_start(LiveInterval* l) { return l->ranges[l->range_count - 1].start; }
static int interval_end(LiveInterval* l)   { return l->ranges[1].end; }
static int interval_class(LiveInterval* l) { return l->mask.class; }

static LiveInterval* split_interval_at(LiveInterval* interval, int pos) {
    if (interval == NULL) {
        return interval;
    }

    // skip past previous intervals
    while (interval->split_kid && pos > interval_end(interval)) {
        interval = interval->split_kid;
    }

    // assert(interval->reg >= 0 || pos <= interval_end(interval));
    return interval;
}

static int range_intersect(LiveRange* a, LiveRange* b) {
    if (b->start <= a->end && a->start <= b->end) {
        return a->start > b->start ? a->start : b->start;
    } else {
        return -1;
    }
}

static int interval_intersect(LiveInterval* a, LiveInterval* b) {
    FOREACH_REVERSE_N(i, 1, a->active_range+1) {
        FOREACH_REVERSE_N(j, 1, b->active_range+1) {
            int t = range_intersect(&a->ranges[i], &b->ranges[j]);
            if (t >= 0) {
                return t;
            }
        }
    }

    return -1;
}

static void add_use_pos(LSRA* restrict ra, LiveInterval* interval, int t, bool may_spill) {
    if (interval->use_cap == interval->use_count) {
        if (interval->use_cap == 0) {
            interval->use_cap = 4;
        } else {
            interval->use_cap *= 2;
        }
        interval->uses = tb_arena_realloc(ra->arena, interval->uses, interval->use_cap * sizeof(UsePos));
    }

    interval->uses[interval->use_count++] = (UsePos){ may_spill, t };
}

static void add_range(LSRA* restrict ra, LiveInterval* interval, int start, int end) {
    assert(start <= end);
    assert(interval->range_count > 0);

    if (interval->ranges[interval->range_count - 1].start <= end) {
        LiveRange* top = &interval->ranges[interval->range_count - 1];

        // coalesce
        top->start = TB_MIN(top->start, start);
        top->end   = TB_MAX(top->end,   end);
    } else {
        if (interval->range_cap == interval->range_count) {
            interval->range_cap *= 2;
            interval->ranges = tb_arena_realloc(ra->arena, interval->ranges, interval->range_cap * sizeof(LiveRange));
        }

        interval->active_range = interval->range_count;
        interval->ranges[interval->range_count++] = (LiveRange){ start, end };
    }
}

LiveInterval* gimme_interval_for_mask(Ctx* restrict ctx, TB_Arena* arena, LSRA* restrict ra, RegMask mask, TB_DataType dt) {
    // not so fixed interval? we need a unique interval then
    int reg = fixed_reg_mask(mask);
    if (reg >= 0) {
        return &ctx->fixed[mask.class][reg];
    } else {
        LiveInterval* interval = tb_arena_alloc(arena, sizeof(LiveInterval));
        *interval = (LiveInterval){
            .id = ctx->interval_count++,
            .mask = mask,
            .dt   = dt,
            .hint = NULL,
            .reg  = -1,
            .assigned = -1,
            .range_cap = 4, .range_count = 1,
            .ranges = tb_arena_alloc(arena, 4 * sizeof(LiveRange))
        };
        interval->ranges[0] = (LiveRange){ INT_MAX, INT_MAX };
        return interval;
    }
}

static Tile* tile_at_time(LSRA* restrict ra, int t) {
    // find which BB
    MachineBB* mbb = NULL;
    FOREACH_N(i, 0, ra->ctx->bb_count) {
        mbb = &ra->ctx->machine_bbs[i];
        if (t <= mbb->end->time) break;
    }

    Tile* curr = mbb->start;
    while (curr != NULL) {
        if (curr->time > t) {
            return curr;
        }
        curr = curr->next;
    }

    return NULL;
}

void tb__lsra(Ctx* restrict ctx, TB_Arena* arena) {
    LSRA ra = { .ctx = ctx, .arena = arena, .spills = ctx->num_regs[0] };
    int initial_spills = ctx->num_regs[0];

    // create timeline & insert moves
    CUIK_TIMED_BLOCK("insert legalizing moves") {
        int timeline = 4;
        FOREACH_N(i, 0, ctx->bb_count) {
            MachineBB* mbb = &ctx->machine_bbs[i];

            for (Tile* t = mbb->start; t; t = t->next) {
                LiveInterval* interval = t->interval;

                // insert input copies
                FOREACH_N(j, 0, t->in_count) {
                    LiveInterval* in_def = t->ins[j].src;
                    RegMask in_mask = t->ins[j].mask;
                    int hint = fixed_reg_mask(in_mask);

                    if (in_def == NULL) {
                        continue;
                    }

                    RegMask in_def_mask = in_def->mask;
                    if (hint >= 0) {
                        in_def->hint = &ctx->fixed[in_mask.class][hint];
                    }

                    bool both_fixed = hint >= 0 && in_def_mask.mask == in_mask.mask;

                    // we resolve def-use conflicts with a spill move, either when:
                    //   * the use and def classes don't match.
                    //   * the use mask is more constrained than the def.
                    //   * it's on both ends to avoid stretching fixed intervals.
                    if (in_def_mask.class != in_mask.class || (in_def_mask.mask & in_mask.mask) != in_def_mask.mask || both_fixed) {
                        if (both_fixed) {
                            in_def_mask = ctx->normie_mask[in_def_mask.class];
                        }

                        TB_OPTDEBUG(REGALLOC)(printf("  TEMP "), tb__print_regmask(in_def_mask), printf(" -> "), tb__print_regmask(in_mask), printf("\n"));

                        // construct copy (either to a fixed interval or a new masked interval)
                        Tile* tmp = tb_arena_alloc(arena, sizeof(Tile));
                        *tmp = (Tile){
                            .prev = t->prev,
                            .next = t,
                            .tag = TILE_SPILL_MOVE,
                            .time = timeline
                        };
                        assert(in_def->dt.raw);
                        tmp->spill_dt = in_def->dt;
                        tmp->ins = tb_arena_alloc(tmp_arena, sizeof(Tile*));
                        tmp->in_count = 1;
                        tmp->ins[0].src  = in_def;
                        tmp->ins[0].mask = in_def_mask;
                        t->prev->next = tmp;
                        t->prev = tmp;

                        // replace use site with temporary that legalized the constraint
                        tmp->interval = gimme_interval_for_mask(ctx, arena, &ra, in_mask, in_def->dt);
                        t->ins[j].src = tmp->interval;

                        timeline += 2;
                    }
                }

                // place on timeline
                t->time = timeline;
                // 2addr ops will have a bit of room for placing the hypothetical copy
                timeline += t->n && ctx->_2addr(t->n) ? 4 : 2;

                // insert copy if we're writing to a fixed interval
                if (interval && interval->mask.mask) {
                    // if we're writing to a fixed interval, insert copy
                    // such that we only guarentee a fixed location at the
                    // def site.
                    int reg = fixed_reg_mask(interval->mask);
                    if (reg >= 0 && t->tag != TILE_SPILL_MOVE) {
                        RegMask rm = interval->mask;
                        LiveInterval* fixed = &ctx->fixed[rm.class][reg];

                        // interval: FIXED(t) => NORMIE_MASK
                        interval->mask = ctx->normie_mask[rm.class];
                        interval->hint = fixed;

                        // insert copy such that the def site is the only piece which "requires"
                        // the fixed range.
                        Tile* tmp = tb_arena_alloc(arena, sizeof(Tile));
                        *tmp = (Tile){
                            .prev = t,
                            .next = t->next,
                            .tag = TILE_SPILL_MOVE,
                            .time = timeline,
                        };
                        assert(interval->dt.raw);
                        tmp->spill_dt = interval->dt;
                        tmp->ins = tb_arena_alloc(tmp_arena, sizeof(Tile*));
                        tmp->in_count = 1;
                        tmp->ins[0].src  = fixed;
                        tmp->ins[0].mask = rm;
                        t->next->prev = tmp;
                        t->next = tmp;

                        // replace def site with fixed interval
                        tmp->interval = interval;
                        t->interval = fixed;

                        // skip this move op
                        t = tmp;
                        timeline += 2;
                    }
                }
            }

            timeline += 2;
        }
    }

    // build intervals from dataflow
    CUIK_TIMED_BLOCK("build intervals") {
        Set visited = set_create_in_arena(arena, ctx->interval_count);

        FOREACH_REVERSE_N(i, 0, ctx->bb_count) {
            MachineBB* mbb = &ctx->machine_bbs[i];

            int bb_start = mbb->start->time;
            int bb_end = mbb->end->time + 2;

            // live outs define a full range across the BB (if they're defined
            // in the block, the later reverse walk will fix that up)
            Set* live_out = &mbb->live_out;
            FOREACH_N(j, 0, (ctx->interval_count + 63) / 64) {
                uint64_t bits = live_out->data[j];
                if (bits == 0) continue;

                FOREACH_N(k, 0, 64) if (bits & (1ull << k)) {
                    add_range(&ra, ctx->id2interval[j*64 + k], bb_start, bb_end);
                }
            }

            for (Tile* t = mbb->end; t; t = t->prev) {
                LiveInterval* interval = t->interval;

                int time = t->time;

                // mark output
                if (interval != NULL && interval->mask.mask) {
                    if (!set_get(&visited, interval->id)) {
                        set_put(&visited, interval->id);
                        if (interval->reg < 0) {
                            dyn_array_put(ra.unhandled, interval);
                        }
                    }

                    if (interval->range_count == 1) {
                        add_range(&ra, interval, time, time);
                    } else {
                        interval->ranges[interval->range_count - 1].start = time;
                    }
                }

                // 2 address ops will interfere with their own inputs (except for
                // shared dst/src)
                bool _2addr = t->tag == TILE_SPILL_MOVE || (t->n && ctx->_2addr(t->n));

                // mark inputs
                FOREACH_N(j, 0, t->in_count) {
                    LiveInterval* in_def = t->ins[j].src;
                    RegMask in_mask = t->ins[j].mask;
                    int hint = fixed_reg_mask(in_mask);

                    // clobber fixed input
                    if (in_def == NULL) {
                        LiveInterval* tmp;
                        if (hint >= 0) {
                            tmp = &ctx->fixed[in_mask.class][hint];
                        } else {
                            tmp = gimme_interval_for_mask(ctx, arena, &ra, in_mask, TB_TYPE_I64);
                            dyn_array_put(ra.unhandled, tmp);
                            t->ins[j].src = tmp;
                        }

                        add_range(&ra, tmp, time, time + 1);
                        continue;
                    }

                    RegMask in_def_mask = in_def->mask;
                    if (hint >= 0) {
                        in_def->hint = &ctx->fixed[in_mask.class][hint];
                    }

                    int use_time = time;
                    if (_2addr) {
                        if (j != 0) {
                            // extend
                            use_time += 2;
                        } else if (interval->hint == NULL) {
                            // hint as copy
                            interval->hint = in_def;
                        }
                    }

                    add_range(&ra, in_def, bb_start, use_time);
                    add_use_pos(&ra, in_def, use_time, in_mask.may_spill);
                }
            }
        }
    }

    int max_regs_in_class = 0;
    CUIK_TIMED_BLOCK("pre-pass on fixed intervals") {
        ra.num_classes = ctx->num_classes;
        ra.num_regs = ctx->num_regs;
        ra.normie_mask = ctx->normie_mask;
        ra.callee_saved = ctx->callee_saved;

        FOREACH_N(i, 0, ctx->num_classes) {
            if (max_regs_in_class < ctx->num_regs[i]) {
                max_regs_in_class = ctx->num_regs[i];
            }

            // add range at beginning such that all fixed intervals are "awake"
            FOREACH_N(j, 0, ctx->num_regs[i]) {
                add_range(&ra, &ctx->fixed[i][j], 0, 1);
                dyn_array_put(ra.unhandled, &ctx->fixed[i][j]);
            }

            size_t cap = ctx->num_regs[i];
            if (i == REG_CLASS_STK) {
                // stack slots have infinite colors we so need to make sure we can resize things
                if (cap < 128) cap = 128;
                else cap = tb_next_pow2(cap + 1);

                ra.stack_slot_cap = cap;
            }

            ra.active_set[i] = set_create_in_arena(arena, cap);
            ra.active[i] = tb_arena_alloc(arena, cap * sizeof(LiveInterval*));
            ra.fixed[i] = ctx->fixed[i];
            memset(ra.active[i], 0, ctx->num_regs[i] * sizeof(LiveInterval*));
        }

        // only need enough to store for the biggest register class
        ra.free_pos  = TB_ARENA_ARR_ALLOC(tmp_arena, max_regs_in_class, int);
        ra.block_pos = TB_ARENA_ARR_ALLOC(tmp_arena, max_regs_in_class, int);
    }

    // sort intervals:
    CUIK_TIMED_BLOCK("sort intervals") {
        cuiksort_defs(ra.unhandled, 0, dyn_array_length(ra.unhandled) - 1);
    }

    // linear scan:
    //   expire old => allocate free or spill/split => rinse & repeat.
    CUIK_TIMED_BLOCK("linear scan") {
        while (dyn_array_length(ra.unhandled)) {
            LiveInterval* interval = dyn_array_pop(ra.unhandled);

            int time = interval_start(interval);
            int end = interval_end(interval);

            assert(time != INT_MAX);

            #if TB_OPTDEBUG_REGALLOC
            printf("  # v%-4d t=[%-4d - %4d) ", interval->id, time, end);
            tb__print_regmask(interval->mask);
            printf("    ");
            if (interval->tile && interval->tile->n) {
                print_node_sexpr(interval->tile->n, 0);
            }
            printf("\n");
            #endif

            // update intervals (inactive <-> active along with expiring)
            FOREACH_N(rc, 0, ctx->num_classes) {
                FOREACH_SET(reg, ra.active_set[rc]) {
                    update_interval(&ra, ra.active[rc][reg], true, time, -1);
                }
            }

            for (size_t i = 0; i < dyn_array_length(ra.inactive);) {
                LiveInterval* inactive = ra.inactive[i];
                if (update_interval(&ra, inactive, false, time, i)) {
                    continue;
                }
                i++;
            }

            // allocate free register (or stack slot)
            ptrdiff_t reg = interval->reg;
            if (reg < 0) {
                if (interval->mask.class == REG_CLASS_STK) {
                    assert(interval->mask.mask == 0);

                    // add new stack slot
                    if (reg < 0) {
                        int next_stk_regs = ra.num_regs[0]++;
                        if (next_stk_regs == ra.stack_slot_cap) {
                            size_t old_cap = ra.stack_slot_cap;
                            ra.stack_slot_cap *= 2;

                            Set new_set = set_create_in_arena(arena, ra.stack_slot_cap);
                            set_copy(&new_set, &ra.active_set[REG_CLASS_STK]);

                            LiveInterval** new_active = tb_arena_alloc(arena, ra.stack_slot_cap * sizeof(LiveInterval*));
                            memcpy(new_active, ra.active, ra.stack_slot_cap * sizeof(LiveInterval*));

                            ra.active_set[REG_CLASS_STK] = new_set;
                            ra.active[REG_CLASS_STK] = new_active;
                            ra.active[REG_CLASS_STK][next_stk_regs] = NULL;
                        }

                        reg = ra.spills++;
                    }

                    TB_OPTDEBUG(REGALLOC)(printf("  #   assign to [SP + %"PRId64"]\n", reg*8));
                } else {
                    reg = allocate_free_reg(&ra, interval);
                    assert(reg >= 0 && "despair");
                }
            }

            // add to active set
            if (reg >= 0) {
                interval->class = interval->mask.class;
                interval->assigned = reg;
                move_to_active(&ra, interval);
            }

            // display active set
            #if TB_OPTDEBUG_REGALLOC
            static const char* classes[] = { "STK", "GPR", "VEC" };
            FOREACH_N(rc, 0, ctx->num_classes) {
                printf("  \x1b[32m%s { ", classes[rc]);
                FOREACH_SET(reg, ra.active_set[rc]) {
                    LiveInterval* l = ra.active[rc][reg];
                    printf("v%d:", l->id);
                    print_reg_name(rc, reg);
                    printf(" ");
                }
                printf("}\x1b[0m\n");
            }
            #endif
        }
    }

    // move resolver:
    //   when a split happens, all indirect paths that cross the split will have
    //   moves inserted.
    CUIK_TIMED_BLOCK("move resolver") {
        FOREACH_N(i, 0, ctx->bb_count) {
            MachineBB* mbb = &ctx->machine_bbs[i];
            TB_Node* end_node = mbb->end_n;
            int terminator = mbb->end->time;

            for (User* u = end_node->users; u; u = u->next) {
                if (cfg_is_control(u->n)) {
                    TB_Node* succ = end_node->type == TB_BRANCH ? cfg_next_bb_after_cproj(u->n) : u->n;
                    MachineBB* target = node_to_bb(ctx, succ);
                    int start_time = target->start->time;

                    // for all live-ins, we should check if we need to insert a move
                    FOREACH_SET(k, target->live_in) {
                        LiveInterval* interval = ctx->id2interval[k];

                        // if the value changes across the edge, insert move
                        LiveInterval* start = split_interval_at(interval, terminator);
                        LiveInterval* end = split_interval_at(interval, start_time);

                        if (start != end) {
                            __debugbreak();
                            /* if (start->spill > 0) {
                                assert(end->spill == start->spill && "TODO: both can't be spills yet");
                                insert_split_move(&ra, start_time, start, end);
                            } else {
                                insert_split_move(&ra, terminator - 1, start, end);
                            } */
                        }
                    }
                }
            }
        }
    }

    // resolve all split interval references
    CUIK_TIMED_BLOCK("split resolver") {
        FOREACH_REVERSE_N(i, 0, ctx->bb_count) {
            MachineBB* mbb = &ctx->machine_bbs[i];

            for (Tile* t = mbb->start; t; t = t->next) {
                int pos = t->time;
                t->interval = split_interval_at(t->interval, pos - 1);

                FOREACH_N(i, 0, t->in_count) {
                    t->ins[i].src = split_interval_at(t->ins[i].src, pos);
                }
            }
        }
    }

    ctx->stack_usage += (ra.spills - initial_spills) * 8;
    ctx->callee_spills = ra.callee_spills;
}

static void compute_free_pos(LSRA* restrict ra, LiveInterval* interval, int class, uint64_t mask) {
    // callee saved will be biased to have nearer free positions to avoid incurring
    // a spill on them early.
    int half_free = interval_end(interval);
    FOREACH_N(i, 0, ra->num_regs[class]) {
        int p = 0;
        if (mask == 0 || (mask & (1u << i))) {
            // it's in use rn, stop trying to steal
            if (!set_get(&ra->active_set[class], i)) {
                p = INT_MAX;
                if (ra->callee_saved[class] & (1ull << i)) {
                    p = half_free;
                }
            }
        }

        ra->free_pos[i] = p;
    }

    // for each inactive which intersects current
    dyn_array_for(i, ra->inactive) {
        LiveInterval* other = ra->inactive[i];
        int fp = ra->free_pos[other->assigned];
        if (fp > 0) {
            int p = interval_intersect(interval, other);
            if (p >= 0 && p < fp) {
                ra->free_pos[other->assigned] = p;
            }
        }
    }
}

// returns -1 if no registers are available
static ptrdiff_t allocate_free_reg(LSRA* restrict ra, LiveInterval* interval) {
    int rc = interval->mask.class;
    compute_free_pos(ra, interval, interval->mask.class, interval->mask.mask);

    int highest = -1;
    int hint_reg = interval->hint ? interval->hint->assigned : -1;

    // it's better in the long run to aggressively split based on hints
    if (hint_reg >= 0 && interval_end(interval) <= ra->free_pos[hint_reg]) {
        highest = hint_reg;
    }

    // pick highest free pos
    if (highest < 0) {
        highest = 0;
        FOREACH_N(i, 1, ra->num_regs[rc]) if (ra->free_pos[i] > ra->free_pos[highest]) {
            highest = i;
        }
    }

    int pos = ra->free_pos[highest];
    if (UNLIKELY(pos == 0)) {
        int reg = -1;
        FOREACH_N(i, 0, ra->num_regs[rc]) {
            if (set_get(&ra->active_set[rc], i) && ra->active[rc][i]->reg < 0) {
                reg = i;
                break;
            }
        }
        assert(reg >= 0 && "no way they're all in fixed-use lmao");

        // alloc failure, split any
        LiveInterval* active_user = ra->active[rc][reg];
        set_remove(&ra->active_set[rc], reg);

        // split whatever is using the interval right now
        split_intersecting(ra, interval_start(interval) - 3, active_user, REGMASK(STK, 0));
        return reg;
    } else {
        if (UNLIKELY(ra->callee_saved[rc] & (1ull << highest))) {
            ra->callee_saved[rc] &= ~(1ull << highest);

            TB_OPTDEBUG(REGALLOC)(printf("  #   spill callee saved register "), print_reg_name(rc, highest), printf("\n"));
            LiveInterval* fixed = &ra->fixed[rc][highest];

            // mark callee move
            CalleeSpill c = { fixed->class, fixed->reg, ra->spills++ };
            dyn_array_put(ra->callee_spills, c);
        }

        if (interval_end(interval) <= pos) {
            // we can steal it completely
            TB_OPTDEBUG(REGALLOC)(printf("  #   assign to "), print_reg_name(rc, highest));

            if (hint_reg >= 0) {
                if (highest == hint_reg) {
                    TB_OPTDEBUG(REGALLOC)(printf(" (HINTED)\n"));
                } else {
                    TB_OPTDEBUG(REGALLOC)(printf(" (FAILED HINT "), print_reg_name(rc, hint_reg), printf(")\n"));
                }
            } else {
                TB_OPTDEBUG(REGALLOC)(printf("\n"));
            }
        } else {
            // TODO(NeGate): split at optimal position before current
            interval->class = interval->mask.class;
            interval->assigned = highest;
            split_intersecting(ra, pos - 3, interval, REGMASK(STK, 0));
            TB_OPTDEBUG(REGALLOC)(printf("  #   stole "), print_reg_name(rc, highest), printf("\n"));
        }

        return highest;
    }
}

static void insert_split_move(LSRA* restrict ra, int t, LiveInterval* old_it, LiveInterval* new_it) {
    // find which BB
    MachineBB* mbb = NULL;
    FOREACH_N(i, 0, ra->ctx->bb_count) {
        mbb = &ra->ctx->machine_bbs[i];
        if (t <= mbb->end->time) break;
    }

    Tile *prev = NULL, *curr = mbb->start;
    while (curr != NULL) {
        if (curr->time > t) {
            break;
        }
        prev = curr, curr = curr->next;
    }

    Tile* move = tb_arena_alloc(ra->arena, sizeof(Tile));
    *move = (Tile){
        .tag = TILE_SPILL_MOVE,
        .interval = new_it
    };
    assert(old_it->dt.raw);
    move->spill_dt = old_it->dt;
    move->ins = tb_arena_alloc(ra->arena, sizeof(Tile*));
    move->in_count = 1;
    move->ins[0].src  = old_it;
    move->ins[0].mask = old_it->mask;
    if (prev) {
        move->time = prev->time + 1;
        move->prev = prev;
        move->next = prev->next;
        if (prev->next == NULL) {
            prev->next = move;
            mbb->end = move;
        } else {
            prev->next->prev = move;
            prev->next = move;
        }
    } else {
        move->time = t;
        move->next = mbb->start;
        mbb->start->prev = move;
        mbb->start = move;
    }
}

static LiveInterval* split_intersecting(LSRA* restrict ra, int pos, LiveInterval* interval, RegMask new_mask) {
    cuikperf_region_start("split", NULL);
    int rc = interval->class;

    LiveInterval* restrict new_it = TB_ARENA_ALLOC(ra->arena, LiveInterval);
    *new_it = *interval;

    assert(interval->mask.class != new_mask.class);
    new_it->mask = new_mask;

    if (interval->assigned >= 0) {
        TB_OPTDEBUG(REGALLOC)(printf("  \x1b[33m#   v%d: split ", interval->id), print_reg_name(rc, interval->assigned), printf(" at t=%d\x1b[0m\n", pos));
    } else {
        TB_OPTDEBUG(REGALLOC)(printf("  \x1b[33m#   v%d: split *undecided* at t=%d\x1b[0m\n", interval->id, pos));
    }

    // split lifetime
    new_it->assigned = new_it->reg = -1;
    new_it->uses = NULL;
    new_it->split_kid = NULL;
    new_it->range_count = 0;

    assert(interval->split_kid == NULL && "cannot spill while spilled");
    interval->split_kid = new_it;

    {
        // since the split is starting at pos and pos is at the top of the
        // unhandled list... we can push this to the top wit no problem
        size_t i = 0, count = dyn_array_length(ra->unhandled);
        for (; i < count; i++) {
            if (pos > interval_start(ra->unhandled[i])) break;
        }

        // we know where to insert
        dyn_array_put(ra->unhandled, NULL);
        memmove(&ra->unhandled[i + 1], &ra->unhandled[i], (count - i) * sizeof(LiveInterval*));
        ra->unhandled[i] = new_it;
    }

    // split ranges
    size_t end = interval->range_count;
    FOREACH_REVERSE_N(i, 1, end) {
        LiveRange* range = &interval->ranges[i];
        if (range->end > pos) {
            bool clean_split = pos < range->start;

            LiveRange old = interval->ranges[interval->active_range];

            new_it->range_count = new_it->range_cap = i + 1;
            new_it->active_range = new_it->range_count - 1;
            new_it->ranges = interval->ranges;

            // move interval up, also insert INT_MAX and potentially
            size_t start = new_it->range_count - !clean_split;

            interval->range_count = interval->range_cap = (end - start) + 1;
            interval->ranges = tb_arena_alloc(ra->arena, interval->range_count * sizeof(LiveRange));
            interval->active_range -= start - 1;
            interval->ranges[0] = (LiveRange){ INT_MAX, INT_MAX };

            FOREACH_N(j, start, end) {
                assert(j - start + 1 < interval->range_count);
                interval->ranges[j - start + 1] = new_it->ranges[j];
            }

            // assert(interval->ranges[interval->active_range].start == old.start);
            // assert(interval->ranges[interval->active_range].end == old.end);

            if (range->start <= pos) {
                interval->ranges[1].end = pos;
                new_it->ranges[new_it->range_count - 1].start = pos;
            }
            break;
        }
    }
    assert(new_it->range_count != 0);

    // split uses
    size_t i = 0, use_count = interval->use_count;
    while (i < use_count + 1) {
        size_t split_count = use_count - i;
        if (i == use_count || interval->uses[i].pos < pos) {
            new_it->use_count = new_it->use_cap = i;
            new_it->uses = interval->uses;

            interval->use_count = interval->use_cap = split_count;
            interval->uses = &interval->uses[i];
            break;
        }
        i++;
    }

    // insert move (the control flow aware moves are inserted later)
    insert_split_move(ra, pos, interval, new_it);

    // reload before next use that requires the original regclass
    if (new_mask.class == REG_CLASS_STK) {
        FOREACH_REVERSE_N(i, 0, new_it->use_count) {
            if (!new_it->uses[i].may_spill) {
                split_intersecting(ra, new_it->uses[i].pos - 3, new_it, interval->mask);
                break;
            }
        }
    }

    cuikperf_region_end();
    return new_it;
}

// update active range to match where the position is currently
static bool update_interval(LSRA* restrict ra, LiveInterval* interval, bool is_active, int time, int inactive_index) {
    // get to the right range first
    while (time >= interval->ranges[interval->active_range].end) {
        assert(interval->active_range > 0);
        interval->active_range -= 1;
    }

    int hole_end = interval->ranges[interval->active_range].start;
    int active_end = interval->ranges[interval->active_range].end;
    bool is_now_active = time >= hole_end;

    int rc = interval_class(interval);
    int reg = interval->assigned;

    if (interval->active_range == 0) { // expired
        if (is_active) {
            TB_OPTDEBUG(REGALLOC)(printf("  #   active "), print_reg_name(rc, reg), printf(" has expired at t=%d (v%d)\n", interval_end(interval), interval->id));
            set_remove(&ra->active_set[rc], reg);
        } else {
            TB_OPTDEBUG(REGALLOC)(printf("  #   inactive "), print_reg_name(rc, reg), printf(" has expired at t=%d (v%d)\n", interval_end(interval), interval->id));
            dyn_array_remove(ra->inactive, inactive_index);
            return true;
        }
    } else if (is_now_active != is_active) { // if we moved, change which list we're in
        if (is_now_active) { // inactive -> active
            TB_OPTDEBUG(REGALLOC)(printf("  #   inactive "), print_reg_name(rc, reg), printf(" is active again (until t=%d, v%d)\n", active_end, interval->id));

            move_to_active(ra, interval);
            dyn_array_remove(ra->inactive, inactive_index);
            return true;
        } else { // active -> inactive
            TB_OPTDEBUG(REGALLOC)(printf("  #   active "), print_reg_name(rc, reg), printf(" is going quiet for now (until t=%d, v%d)\n", active_end, interval->id));

            set_remove(&ra->active_set[rc], reg);
            dyn_array_put(ra->inactive, interval);
        }
    }

    return false;
}

static void move_to_active(LSRA* restrict ra, LiveInterval* interval) {
    int rc = interval_class(interval), reg = interval->assigned;
    if (set_get(&ra->active_set[rc], reg)) {
        tb_panic("v%d: interval v%d should never be forced out, we should've accomodated them in the first place", interval->id, ra->active[rc][reg]->id);
    }

    set_put(&ra->active_set[rc], reg);
    ra->active[rc][reg] = interval;
}

////////////////////////////////
// Sorting unhandled list
////////////////////////////////
static size_t partition(LiveInterval** intervals, ptrdiff_t lo, ptrdiff_t hi) {
    int pivot = interval_start(intervals[(hi - lo) / 2 + lo]); // middle

    ptrdiff_t i = lo - 1, j = hi + 1;
    for (;;) {
        // Move the left index to the right at least once and while the element at
        // the left index is less than the pivot
        do { i += 1; } while (interval_start(intervals[i]) > pivot);

        // Move the right index to the left at least once and while the element at
        // the right index is greater than the pivot
        do { j -= 1; } while (interval_start(intervals[j]) < pivot);

        // If the indices crossed, return
        if (i >= j) return j;

        // Swap the elements at the left and right indices
        SWAP(LiveInterval*, intervals[i], intervals[j]);
    }
}

static void cuiksort_defs(LiveInterval** intervals, ptrdiff_t lo, ptrdiff_t hi) {
    if (lo >= 0 && hi >= 0 && lo < hi) {
        // get pivot
        size_t p = partition(intervals, lo, hi);

        // sort both sides
        cuiksort_defs(intervals, lo, p);
        cuiksort_defs(intervals, p + 1, hi);
    }
}
