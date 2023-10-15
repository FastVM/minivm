// https://ssw.jku.at/Research/Papers/Wimmer04Master/Wimmer04Master.pdf
#ifdef NDEBUG
#define REG_ALLOC_LOG if (0)
#else
#define REG_ALLOC_LOG if (reg_alloc_log)
#endif

typedef struct {
    int start, end;
} LiveRange;

typedef struct {
    int pos;

    // this is used to denote folded reloads
    //
    //   # r2 is spilled
    //   add r0, r1, r2
    //
    // when we codegen, we don't need to allocate
    // a register for r2 here.
    //
    //   add r0, r1, [sp - 24]
    enum {
        USE_OUT,
        USE_REG,
        USE_MEM_OR_REG,
    } kind;
} UsePos;

struct LiveInterval {
    int reg_class;

    TB_Node* n;
    TB_X86_DataType dt;

    // results of regalloc
    int assigned;

    // register num, -1 if the interval isn't a physical reg
    int reg, hint;

    // whole interval
    int start, end;

    // spill point, -1 if there's none
    int spill, split_kid;

    // help speed up some of the main allocation loop
    int active_range;

    // we're gonna have so much memory to clean up...
    DynArray(LiveRange) ranges;
    DynArray(UsePos) uses;
};

typedef DynArray(RegIndex) IntervalList;

typedef struct {
    TB_ABI abi;

    DynArray(LiveInterval) intervals;
    DynArray(RegIndex) inactive;
    IntervalList unhandled;
    Inst* first;

    int stack_usage;

    // time when the physical registers will be free again
    int* free_pos;
    int* block_pos;

    int endpoint;
    uint64_t callee_saved[CG_REGISTER_CLASSES];

    Set active_set[CG_REGISTER_CLASSES];
    RegIndex active[CG_REGISTER_CLASSES][16];

    Inst* cache;
} LSRA;

static LiveRange* last_range(LiveInterval* i) {
    return &i->ranges[dyn_array_length(i->ranges) - 1];
}

////////////////////////////////
// Generate intervals
////////////////////////////////
static void add_use_pos(LiveInterval* interval, int t, int kind) {
    UsePos u = { t, kind };
    dyn_array_put(interval->uses, u);
}

// interval->start is filled in by the definition
static void add_range(LiveInterval* interval, int start, int end) {
    assert(start <= end);
    size_t count = dyn_array_length(interval->ranges);
    if (count > 0 && interval->ranges[count - 1].start <= end) {
        LiveRange* top = &interval->ranges[count - 1];

        // coalesce
        top->start = TB_MIN(top->start, start);
        top->end   = TB_MAX(top->end,   end);
    } else {
        LiveRange r = { start, end };
        dyn_array_put(interval->ranges, r);
    }

    if (end > interval->end) interval->end = end;
}

static void reverse_bb_walk(LSRA* restrict ra, MachineBB* bb, Inst* inst) {
    Inst* next = inst->next;
    if (next && next->type != INST_LABEL) {
        reverse_bb_walk(ra, bb, next);
    }

    // mark outputs, inputs and temps
    //
    // TODO(NeGate): on x86 we can have one memory operand per instruction.
    // we shouldn't force only register uses or else we'll make spilling more
    // prominent.
    RegIndex* ops = inst->operands;
    bool dst_use_reg = inst->type == IMUL || inst->type == INST_ZERO || (inst->flags & (INST_MEM | INST_GLOBAL));

    FOREACH_N(i, 0, inst->out_count) {
        assert(*ops >= 0);
        LiveInterval* interval = &ra->intervals[*ops++];

        if (interval->ranges == NULL) {
            add_range(interval, inst->time, inst->time);
        } else {
            interval->ranges[dyn_array_length(interval->ranges) - 1].start = inst->time;
        }

        interval->start = inst->time;
        add_use_pos(interval, inst->time, dst_use_reg ? USE_REG : USE_OUT);
    }

    FOREACH_N(i, 0, inst->in_count) {
        assert(*ops >= 0);
        LiveInterval* interval = &ra->intervals[*ops++];

        add_range(interval, bb->start, inst->time);
        add_use_pos(interval, inst->time, USE_REG);
    }

    // calls use the temporaries for clobbers
    bool is_call = (inst->type == CALL || inst->type == SYSCALL);
    FOREACH_N(i, 0, inst->tmp_count) {
        assert(*ops >= 0);
        LiveInterval* interval = &ra->intervals[*ops++];

        add_range(interval, inst->time, inst->time + 1);
        if (!is_call) {
            add_use_pos(interval, inst->time, USE_REG);
        }
    }
}

static int range_intersect(int start, int end, LiveRange* b) {
    if (b->start <= end && start <= b->end) {
        return start > b->start ? start : b->start;
    } else {
        return -1;
    }
}

static int interval_intersect(LiveInterval* a, LiveInterval* b) {
    if (!(b->start <= a->end && a->start <= b->end)) {
        return -1; // don't intersect at all
    }

    FOREACH_N(i, 0, dyn_array_length(a->ranges)) {
        LiveRange a_range = a->ranges[i];

        FOREACH_N(j, 0, dyn_array_length(b->ranges)) {
            LiveRange b_range = b->ranges[j];

            // if the end is greater than the start, then we've overshot
            if (a_range.start >= b_range.end) {
                break;
            }

            if (a_range.end >= b_range.start) {
                return b_range.start > a_range.start ? b_range.start : a_range.start;
            }
        }
    }

    return -1;
}

#define FOREACH_SET(it, set) \
FOREACH_N(_i, 0, ((set).capacity + 63) / 64) \
for (uint64_t bits = (set).data[_i], it = _i*64; bits; bits >>= 1, it++) if (bits & 1)

static int next_use(LSRA* restrict ra, LiveInterval* interval, int time) {
    for (;;) {
        FOREACH_N(i, 0, dyn_array_length(interval->uses)) {
            if (interval->uses[i].pos > time) {
                return interval->uses[i].pos;
            }
        }

        if (interval->split_kid >= 0) {
            interval = &ra->intervals[interval->split_kid];
            continue;
        }
    }

    return INT_MAX;
}

// if < 0, then it's -x - 1 where x is the nearest starting point
static int covers(LiveInterval* it, int start, int end) {
    size_t i = 0, count = dyn_array_length(it->ranges);
    for (; i < count; i++) {
        // if the end is greater than the start, then we've overshot
        if (start > it->ranges[i].end) {
            return -1;
        }

        if (end >= it->ranges[i].start) {
            return i;
        }
    }

    return -1;
}

static LiveInterval* get_active(LSRA* restrict ra, int rc, int reg) {
    if (!set_get(&ra->active_set[rc], reg)) {
        return NULL;
    }

    return &ra->intervals[ra->active[rc][reg]];
}

static void insert_split_move(LSRA* restrict ra, int t, int old_reg, int new_reg) {
    Inst *prev, *inst;
    TB_X86_DataType dt = ra->intervals[old_reg].dt;

    // invalidate
    if (ra->cache->time >= t) {
        ra->cache = ra->first;
    }

    prev = ra->cache, inst = prev->next;
    CUIK_TIMED_BLOCK("walk") {
        while (inst != NULL) {
            if (inst->time > t) {
                ra->cache = prev;
                break;
            }

            prev = inst, inst = inst->next;
        }
    }

    // folded spill
    if (inst && inst->type == MOV && inst->flags == 0 && inst->operands[0] == old_reg) {
        inst->operands[0] = new_reg;
        return;
    }

    Inst* new_inst = tb_arena_alloc(tmp_arena, sizeof(Inst) + (2 * sizeof(RegIndex)));
    *new_inst = (Inst){ .type = MOV, .flags = INST_SPILL, .dt = dt, .out_count = 1, 1 };
    new_inst->operands[0] = new_reg;
    new_inst->operands[1] = old_reg;
    new_inst->time = prev->time + 1;
    new_inst->next = prev->next;
    prev->next = new_inst;
}

static LiveInterval* split_interval_at(LSRA* restrict ra, LiveInterval* interval, int pos) {
    // skip past previous intervals
    while (interval->split_kid >= 0 && pos > interval->end) {
        interval = &ra->intervals[interval->split_kid];
    }

    assert(interval->reg >= 0 || pos <= interval->end);
    return interval;
}

// any uses after `pos` after put into the new interval
static int split_intersecting(LSRA* restrict ra, int current_time, int pos, LiveInterval* interval, bool is_spill) {
    assert(interval->reg < 0);

    int ri = interval - ra->intervals;
    if (interval->spill > 0) {
        REG_ALLOC_LOG printf("  \x1b[33m#   v%d: reload [RBP - %d] at t=%d\x1b[0m\n", ri, interval->spill, pos);
    } else {
        // allocate stack slot
        int size = 8;
        ra->stack_usage = align_up(ra->stack_usage + size, size);

        REG_ALLOC_LOG printf("  \x1b[33m#   v%d: spill %s to [RBP - %d] at t=%d\x1b[0m\n", ri, reg_name(interval->reg_class, interval->assigned), ra->stack_usage, pos);
        if (current_time >= pos && interval->assigned >= 0) {
            if (set_get(&ra->active_set[interval->reg_class], interval->assigned) && ra->active[interval->reg_class][interval->assigned] == ri) {
                REG_ALLOC_LOG printf("  \x1b[33m#   v%d: expired during split\x1b[0m\n", ri);
                set_remove(&ra->active_set[interval->reg_class], interval->assigned);
            }
        }
    }

    // split lifetime
    LiveInterval it = *interval;
    if (is_spill) {
        it.spill = ra->stack_usage;
    } else {
        it.spill = -1;
        it.reg = -1;
    }
    it.assigned = it.reg = -1;
    it.start = pos;
    it.end = interval->end;
    it.uses = NULL;
    it.ranges = NULL;
    it.n = NULL;
    it.split_kid = -1;
    interval->end = pos;

    int old_reg = interval - ra->intervals;
    int new_reg = dyn_array_length(ra->intervals);
    interval->split_kid = new_reg;

    dyn_array_put(ra->intervals, it);
    interval = &ra->intervals[old_reg];

    if (!is_spill) {
        // since the split is starting at pos and pos is at the top of the
        // unhandled list... we can push this to the top wit no problem
        size_t i = 0, count = dyn_array_length(ra->unhandled);
        for (; i < count; i++) {
            if (pos > ra->intervals[ra->unhandled[i]].start) break;
        }

        // we know where to insert
        dyn_array_put(ra->unhandled, 0);
        memmove(&ra->unhandled[i + 1], &ra->unhandled[i], (count - i) * sizeof(RegIndex));
        ra->unhandled[i] = new_reg;
    }

    // split uses
    size_t use_count = dyn_array_length(interval->uses);
    FOREACH_REVERSE_N(i, 0, use_count) {
        size_t split_count = use_count - (i + 1);
        if (interval->uses[i].pos > pos && split_count > 0) {
            // split
            DynArray(UsePos) uses = dyn_array_create(UsePos, split_count);
            dyn_array_set_length(uses, split_count);
            memcpy(uses, &interval->uses[i + 1], split_count * sizeof(UsePos));

            dyn_array_set_length(interval->uses, i + 1);
            it.uses = interval->uses;
            interval->uses = uses;
            break;
        }
    }

    // split ranges
    for (size_t i = 0; i < dyn_array_length(interval->ranges);) {
        LiveRange* range = &interval->ranges[i];
        if (range->start > pos) {
            dyn_array_put(it.ranges, *range);
            interval = &ra->intervals[old_reg];

            // remove-shift
            size_t shift = dyn_array_length(interval->ranges) - (i + 1);
            if (shift > 0) {
                memmove(range, range + 1, shift * sizeof(LiveRange));
            }
            dyn_array_pop(interval->ranges);
        } else if (range->end > pos) {
            // intersects pos, we need to split the range
            LiveRange r = { pos, range->end };
            range->end = pos;
            dyn_array_put(it.ranges, r);

            interval = &ra->intervals[old_reg];
            continue;
        }

        i++;
    }

    ra->intervals[new_reg] = it;

    // insert move (the control flow aware moves are inserted later)
    insert_split_move(ra, pos, old_reg, new_reg);

    // reload before next use
    if (is_spill) {
        FOREACH_REVERSE_N(i, 0, dyn_array_length(it.uses)) {
            if (it.uses[i].kind == USE_REG) {
                // new split
                split_intersecting(ra, current_time, it.uses[i].pos - 1, &ra->intervals[new_reg], false);
                break;
            }
        }
    }

    return new_reg;
}

// returns -1 if no registers are available
static ptrdiff_t allocate_free_reg(LSRA* restrict ra, LiveInterval* interval) {
    int rc = interval->reg_class;

    // callee saved will be biased to have nearer free positions to avoid incurring
    // a spill on them early.
    int half_free = 1 << 16;
    FOREACH_N(i, 0, 16) {
        ra->free_pos[i] = (ra->callee_saved[rc] & (1ull << i)) ? half_free : INT_MAX;
        // ra->free_pos[i] = INT_MAX;
    }

    // for each active reg, set the free pos to 0
    FOREACH_SET(i, ra->active_set[rc]) {
        ra->free_pos[i] = 0;
    }

    // for each inactive which intersects current
    dyn_array_for(i, ra->inactive) {
        LiveInterval* it = &ra->intervals[ra->inactive[i]];
        int fp = ra->free_pos[it->assigned];
        if (fp > 0) {
            int p = range_intersect(interval->start, interval->end, &it->ranges[it->active_range]);
            if (p >= 0 && p < fp) {
                ra->free_pos[it->assigned] = p;
            }
        }
    }

    if (rc == REG_CLASS_GPR) {
        // reserved regs
        ra->free_pos[RBP] = 0;
        ra->free_pos[RSP] = 0;
    }

    // try hint
    int highest = -1;
    int hint_reg = -1;

    if (interval->hint >= 0) {
        LiveInterval* hint = &ra->intervals[interval->hint];
        assert(hint->reg_class == rc);
        hint_reg = hint->assigned;

        if (interval->end <= ra->free_pos[hint_reg]) {
            highest = hint_reg;
        }
    }

    // pick highest free pos
    if (highest < 0) {
        highest = 0;
        FOREACH_N(i, 1, 16) if (ra->free_pos[i] > ra->free_pos[highest]) {
            highest = i;
        }
    }

    int pos = ra->free_pos[highest];
    if (UNLIKELY(pos == 0)) {
        // alloc failure
        return -1;
    } else {
        if (UNLIKELY(ra->callee_saved[rc] & (1ull << highest))) {
            ra->callee_saved[rc] &= ~(1ull << highest);

            REG_ALLOC_LOG printf("  #   spill callee saved register %s\n", reg_name(rc, highest));

            int size = rc ? 16 : 8;
            int vreg = (rc ? FIRST_XMM : FIRST_GPR) + highest;
            ra->stack_usage = align_up(ra->stack_usage + size, size);

            LiveInterval it = {
                .spill = ra->stack_usage,
                .dt = ra->intervals[vreg].dt,
                .assigned = -1,
                .reg = -1,
                .split_kid = -1,
            };

            int old_reg = interval - ra->intervals;
            int spill_slot = dyn_array_length(ra->intervals);
            dyn_array_put(ra->intervals, it);

            // insert spill and reload
            insert_split_move(ra, 0,            vreg, spill_slot);
            insert_split_move(ra, ra->endpoint, spill_slot, vreg);

            // adding to intervals might resized this
            interval = &ra->intervals[old_reg];
        }

        if (interval->end <= pos) {
            // we can steal it completely
            REG_ALLOC_LOG printf("  #   assign to %s", reg_name(rc, highest));

            if (interval->hint >= 0) {
                if (highest == hint_reg) {
                    REG_ALLOC_LOG printf(" (HINTED)\n");
                } else {
                    REG_ALLOC_LOG printf(" (FAILED HINT %s)\n", reg_name(rc, hint_reg));
                }
            } else {
                REG_ALLOC_LOG printf("\n");
            }
        } else {
            // TODO(NeGate): split current at optimal position before current
            interval->assigned = highest;
            split_intersecting(ra, interval->start, pos - 1, interval, true);
        }

        return highest;
    }
}

static ptrdiff_t allocate_blocked_reg(LSRA* restrict ra, LiveInterval* interval) {
    int rc = interval->reg_class;
    int* use_pos = ra->free_pos;

    FOREACH_N(i, 0, 16) ra->block_pos[i] = INT_MAX;
    FOREACH_N(i, 0, 16) use_pos[i] = INT_MAX;

    // mark non-fixed intervals
    FOREACH_SET(i, ra->active_set[rc]) {
        LiveInterval* it = &ra->intervals[ra->active[rc][i]];
        if (it->reg_class == rc && it->reg < 0) {
            use_pos[i] = next_use(ra, it, interval->start);
        }
    }

    dyn_array_for(i, ra->inactive) {
        LiveInterval* it = &ra->intervals[ra->inactive[i]];
        if (it->reg_class == rc && it->reg < 0) {
            use_pos[i] = next_use(ra, it, interval->start);
        }
    }

    // mark fixed intervals
    FOREACH_SET(i, ra->active_set[rc]) {
        LiveInterval* it = &ra->intervals[ra->active[rc][i]];
        if (it->reg_class == rc && it->reg >= 0) {
            use_pos[i] = 0;
            ra->block_pos[i] = 0;
        }
    }

    dyn_array_for(i, ra->inactive) {
        LiveInterval* it = &ra->intervals[ra->inactive[i]];
        if (it->reg_class == rc && it->reg >= 0) {
            int bp = ra->block_pos[it->assigned];
            if (bp > 0) {
                int p = range_intersect(interval->start, interval->end, &it->ranges[it->active_range]);
                if (p >= 0 && p < bp) {
                    ra->block_pos[it->assigned] = p;
                }
            }
        }
    }

    if (rc == REG_CLASS_GPR) {
        // reserved regs
        use_pos[RBP] = 0;
        use_pos[RSP] = 0;
    }

    // pick highest use pos
    int highest = 0;
    FOREACH_N(i, 1, 16) if (use_pos[i] > use_pos[highest]) {
        highest = i;
    }

    int pos = use_pos[highest];
    int first_use = interval->uses[dyn_array_length(interval->uses) - 1].pos;

    bool spilled = false;
    if (first_use > pos) {
        int size = 8;

        // spill interval
        ra->stack_usage = align_up(ra->stack_usage + size, size);
        interval->spill = ra->stack_usage;

        // split at optimal spot before first use that requires a register
        FOREACH_REVERSE_N(i, 0, dyn_array_length(interval->uses)) {
            if (interval->uses[i].pos >= pos && interval->uses[i].kind == USE_REG) {
                split_intersecting(ra, interval->start, interval->uses[i].pos - 1, interval, false);
                break;
            }
        }

        spilled = true;
    } else {
        int split_pos = (interval->start & ~1) - 1;

        // split active or inactive interval reg
        LiveInterval* to_split = get_active(ra, rc, highest);
        if (to_split != NULL) {
            split_intersecting(ra, interval->start, split_pos, to_split, true);
        }

        // split any inactive interval for reg at the end of it's lifetime hole
        dyn_array_for(i, ra->inactive) {
            LiveInterval* it = &ra->intervals[ra->inactive[i]];
            LiveRange* r = &it->ranges[it->active_range];

            if (it->reg_class == rc && it->assigned == highest && r->start <= pos+1 && pos <= r->end) {
                split_intersecting(ra, interval->start, split_pos, it, true);
            }
        }
    }

    // split active reg if it intersects with fixed interval
    LiveInterval* fix_interval = &ra->intervals[(rc ? FIRST_XMM : FIRST_GPR) + highest];
    if (dyn_array_length(fix_interval->ranges)) {
        int p = range_intersect(interval->start, interval->end, &fix_interval->ranges[fix_interval->active_range]);
        if (p >= 0) {
            split_intersecting(ra, interval->start, p, fix_interval, true);
        }
    }

    return spilled ? -1 : highest;
}

static void move_to_active(LSRA* restrict ra, LiveInterval* interval) {
    int rc = interval->reg_class, reg = interval->assigned;
    int ri = interval - ra->intervals;

    if (set_get(&ra->active_set[rc], reg)) {
        tb_panic("intervals should never be forced out, we should've accomodated them in the first place");
    }

    set_put(&ra->active_set[rc], reg);
    ra->active[rc][reg] = ri;
}

// update active range to match where the position is currently
static bool update_interval(LSRA* restrict ra, LiveInterval* restrict interval, bool is_active, int time, int inactive_index) {
    int ri = interval - ra->intervals;

    // get to the right range first
    while (interval->ranges[interval->active_range].end <= time) {
        interval->active_range -= 1;
    }

    int hole_end = interval->ranges[interval->active_range].start;
    int active_end = interval->ranges[interval->active_range].end;
    bool is_now_active = time >= hole_end;

    int rc = interval->reg_class;
    int reg = interval->assigned;

    if (time >= interval->end) { // expired
        if (is_active) {
            REG_ALLOC_LOG printf("  #   active %s has expired (v%d)\n", reg_name(rc, reg), ri);
            set_remove(&ra->active_set[rc], reg);
        } else {
            REG_ALLOC_LOG printf("  #   inactive %s has expired (v%d)\n", reg_name(rc, reg), ri);
            dyn_array_remove(ra->inactive, inactive_index);
            return true;
        }
    } else if (is_now_active != is_active) { // if we moved, change which list we're in
        if (is_now_active) { // inactive -> active
            REG_ALLOC_LOG printf("  #   inactive %s is active again (until t=%d, v%d)\n", reg_name(rc, reg), active_end, ri);

            move_to_active(ra, interval);
            dyn_array_remove(ra->inactive, inactive_index);
            return true;
        } else { // active -> inactive
            REG_ALLOC_LOG printf("  #   active %s is going quiet for now (until t=%d, v%d)\n", reg_name(rc, reg), hole_end, ri);

            set_remove(&ra->active_set[rc], reg);
            dyn_array_put(ra->inactive, ri);
        }
    }

    return false;
}

static void cuiksort_defs(LiveInterval* intervals, ptrdiff_t lo, ptrdiff_t hi, RegIndex* arr);
static int linear_scan(Ctx* restrict ctx, TB_Function* f, int stack_usage, int end) {
    LSRA ra = { .abi = f->super.module->target_abi, .first = ctx->first, .cache = ctx->first, .intervals = ctx->intervals, .stack_usage = stack_usage };

    FOREACH_N(i, 0, CG_REGISTER_CLASSES) {
        ra.active_set[i] = set_create_in_arena(tmp_arena, 16);
    }

    // build intervals:
    //   we also track when uses happen to aid in splitting
    MachineBBs mbbs = ctx->machine_bbs;
    size_t interval_count = dyn_array_length(ra.intervals);
    CUIK_TIMED_BLOCK("build intervals") {
        FOREACH_N(i, 0, ctx->block_count) {
            TB_Node* bb = ctx->worklist.items[i];
            MachineBB* mbb = &nl_map_get_checked(mbbs, bb);

            int bb_start = mbb->start;
            int bb_end = mbb->end + 2;

            // for anything that's live out, add the entire range
            Set* live_out = &mbb->live_out;
            FOREACH_N(j, 0, (interval_count + 63) / 64) {
                uint64_t bits = live_out->data[j];
                if (bits == 0) continue;

                FOREACH_N(k, 0, 64) if (bits & (1ull << k)) {
                    add_range(&ra.intervals[j*64 + k], bb_start, bb_end);
                }
            }

            // for all instruction in BB (in reverse), add ranges
            if (mbb->first) {
                reverse_bb_walk(&ra, mbb, mbb->first);
            }
        }
    }

    // we use every fixed interval at the very start to force them into
    // the inactive set.
    FOREACH_N(i, 0, 32) if (ra.intervals[i].ranges) {
        ra.intervals[i].start = 0;
        add_range(&ra.intervals[i], 0, 1);
    }

    dyn_array_destroy(ra.intervals[RBP].ranges);
    dyn_array_destroy(ra.intervals[RSP].ranges);

    ra.endpoint = end;
    mark_callee_saved_constraints(ctx, ra.callee_saved);

    // generate unhandled interval list (sorted by starting point)
    ra.unhandled = dyn_array_create(LiveInterval*, (interval_count * 4) / 3);
    FOREACH_N(i, 0, interval_count) dyn_array_put(ra.unhandled, i);
    cuiksort_defs(ctx->intervals, 0, interval_count - 1, ra.unhandled);

    // only need enough to store for the biggest register class
    ra.free_pos  = TB_ARENA_ARR_ALLOC(tmp_arena, 16, int);
    ra.block_pos = TB_ARENA_ARR_ALLOC(tmp_arena, 16, int);

    // linear scan main loop
    CUIK_TIMED_BLOCK("reg alloc") {
        while (dyn_array_length(ra.unhandled)) {
            RegIndex ri = dyn_array_pop(ra.unhandled);
            LiveInterval* interval = &ra.intervals[ri];

            // unused interval, skip
            if (interval->ranges == NULL) continue;

            int time = interval->start;

            int before_next_time = interval->start;
            if (dyn_array_length(ra.unhandled)) {
                int before = ra.intervals[ra.unhandled[dyn_array_length(ra.unhandled) - 1]].start - 1;
                if (before > before_next_time) {
                    before_next_time = before;
                }
            }

            if (interval->reg >= 0) {
                REG_ALLOC_LOG printf("  # %-5s t=[%-4d - %4d)\n", reg_name(interval->reg_class, interval->reg), time, interval->end);
            } else if (interval->spill > 0) {
                REG_ALLOC_LOG {
                    printf("  # v%-4d t=[%-4d - %4d) SPILLED [RBP - %d]\n", ri, time, interval->end, interval->spill);
                }
                continue;
            } else {
                REG_ALLOC_LOG {
                    printf("  # v%-4d t=[%-4d - %4d)   ", ri, time, interval->end);
                    if (interval->n != NULL) {
                        print_node_sexpr(interval->n, 0);
                    }
                    printf("\n");
                }
            }

            // expire intervals
            FOREACH_N(rc, 0, CG_REGISTER_CLASSES) {
                FOREACH_SET(reg, ra.active_set[rc]) {
                    RegIndex active_i = ra.active[rc][reg];
                    update_interval(&ra, &ra.intervals[active_i], true, time, -1);
                }
            }

            int rc = interval->reg_class;
            for (size_t i = 0; i < dyn_array_length(ra.inactive);) {
                RegIndex inactive_i = ra.inactive[i];
                LiveInterval* it = &ra.intervals[inactive_i];

                if (update_interval(&ra, it, false, time, i)) {
                    interval = &ra.intervals[ri]; // might've resized the intervals
                    continue;
                }

                i++;
            }

            ptrdiff_t reg = interval->reg;
            if (reg < 0) {
                // find register for virtual interval
                if (reg < 0) {
                    reg = allocate_free_reg(&ra, interval);
                    interval = &ra.intervals[ri]; // might've resized the intervals
                }

                // alloc failure
                if (reg < 0) {
                    reg = allocate_blocked_reg(&ra, interval);
                    interval = &ra.intervals[ri]; // might've resized the intervals

                    // tb_assert(reg >= 0, "regalloc failure");
                }
            }

            // add to active set
            if (reg >= 0) {
                interval->assigned = reg;
                interval->active_range = dyn_array_length(interval->ranges) - 1;
                move_to_active(&ra, interval);
            }

            // display active set
            REG_ALLOC_LOG {
                printf("  \x1b[32m{ ");
                FOREACH_N(rc, 0, CG_REGISTER_CLASSES) {
                    FOREACH_SET(reg, ra.active_set[rc]) {
                        int id = ra.active[rc][reg];

                        if (ra.intervals[id].reg >= 0) {
                            printf("%s ", reg_name(rc, ra.intervals[id].reg));
                        } else {
                            printf("v%d:%s ", ra.active[rc][reg], reg_name(rc, reg));
                        }
                    }
                }
                printf("}\x1b[0m\n");
            }
        }
    }

    // move resolver
    CUIK_TIMED_BLOCK("move resolver") {
        FOREACH_N(i, 0, ctx->block_count) {
            TB_Node* bb = ctx->worklist.items[i];
            assert(bb->type == TB_START || bb->type == TB_REGION);

            MachineBB* mbb = &nl_map_get_checked(mbbs, bb);
            TB_NodeRegion* r = TB_NODE_GET_EXTRA(bb);

            if (r->end->type != TB_BRANCH) {
                continue;
            }

            TB_NodeBranch* br = TB_NODE_GET_EXTRA(r->end);
            FOREACH_REVERSE_N(i, 0, br->succ_count) {
                TB_Node* bb = br->succ[i];
                MachineBB* target = &nl_map_get_checked(mbbs, bb);

                // for all live-ins, we should check if we need to insert a move
                FOREACH_SET(i, target->live_in) {
                    LiveInterval* interval = &ra.intervals[i];

                    // if the value changes across the edge, insert move
                    LiveInterval* start = split_interval_at(&ra, interval, mbb->end);
                    LiveInterval* end = split_interval_at(&ra, interval, target->start);

                    if (start != end) {
                        if (start->spill > 0) {
                            assert(end->spill <= 0 && "TODO: both can't be spills yet");
                            insert_split_move(&ra, target->start + 1, start - ra.intervals, end - ra.intervals);
                        } else {
                            insert_split_move(&ra, mbb->terminator - 1, start - ra.intervals, end - ra.intervals);
                        }
                    }
                }

                // the moves are inserted either at the end of block from or at the beginning of block to,
                // depending on the control flow
                // resolver.find_insert_position(from, to)

                // insert all moves in correct order (without overwriting registers that are used later)
                // resolver.resolve_mappings()
            }
        }
    }

    // resolve all split interval references
    CUIK_TIMED_BLOCK("split resolver") {
        for (Inst* restrict inst = ra.first; inst; inst = inst->next) {
            if (inst->flags & INST_SPILL) {
                continue;
            }

            int pos = inst->time;
            FOREACH_N(i, 0, inst->out_count + inst->in_count + inst->tmp_count) {
                inst->operands[i] = split_interval_at(&ra, &ra.intervals[inst->operands[i]], pos) - ra.intervals;
            }
        }
    }

    CUIK_TIMED_BLOCK("free intervals") {
        dyn_array_for(i, ra.intervals) {
            dyn_array_destroy(ra.intervals[i].ranges);
            dyn_array_destroy(ra.intervals[i].uses);
        }
    }

    ctx->intervals = ra.intervals;
    return ra.stack_usage;
}

////////////////////////////////
// Sorting unhandled list
////////////////////////////////
static size_t partition(LiveInterval* intervals, ptrdiff_t lo, ptrdiff_t hi, RegIndex* arr) {
    int pivot = intervals[arr[(hi - lo) / 2 + lo]].start; // middle

    ptrdiff_t i = lo - 1, j = hi + 1;
    for (;;) {
        // Move the left index to the right at least once and while the element at
        // the left index is less than the pivot
        do { i += 1; } while (intervals[arr[i]].start > pivot);

        // Move the right index to the left at least once and while the element at
        // the right index is greater than the pivot
        do { j -= 1; } while (intervals[arr[j]].start < pivot);

        // If the indices crossed, return
        if (i >= j) return j;

        // Swap the elements at the left and right indices
        SWAP(RegIndex, arr[i], arr[j]);
    }
}

static void cuiksort_defs(LiveInterval* intervals, ptrdiff_t lo, ptrdiff_t hi, RegIndex* arr) {
    if (lo >= 0 && hi >= 0 && lo < hi) {
        // get pivot
        size_t p = partition(intervals, lo, hi, arr);

        // sort both sides
        cuiksort_defs(intervals, lo, p, arr);
        cuiksort_defs(intervals, p + 1, hi, arr);
    }
}
