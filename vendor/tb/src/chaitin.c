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

    size_t ifg_stride;
    size_t ifg_len;
    uint64_t* ifg;
    int* degree;
} Chaitin;

static void ifg_edge(Chaitin* ra, int i, int j) {
    ra->ifg[i*ra->ifg_stride + j/64] |= 1ull << (j % 64);
}

static bool ifg_test(Chaitin* ra, int i, int j) {
    return ra->ifg[i*ra->ifg_stride + j/64] & (1ull << (j % 64));
}

static void ifg_remove(Chaitin* ra, int i, int j) {
    printf("remove v%d -- v%d\n", i, j);
    assert(ifg_test(ra, i, j));
    ra->ifg[i*ra->ifg_stride + j/64] &= ~(1ull << (j % 64));
}

static void ifg_compute_degree(Chaitin* ra) {
    FOREACH_N(i, 0, ra->ifg_len) {
        int sum = 0;
        FOREACH_N(j, 0, ra->ifg_stride) {
            sum += tb_popcount64(ra->ifg[i*ra->ifg_stride + j]);
        }
        ra->degree[i] = sum;
    }
}

static void ifg_remove_edges(Chaitin* ra, int i) {
    FOREACH_N(j, 0, ra->ifg_stride) {
        uint64_t bits = ra->ifg[i*ra->ifg_stride + j];
        if (bits == 0) continue;

        FOREACH_N(k, 0, 64) if ((bits >> k) & 1) {
            ifg_remove(ra, j*64 + k, i);
            ra->degree[j*64 + k] -= 1;
        }

        // reset all the bits
        ra->ifg[i*ra->ifg_stride + j] = 0;
    }
    ra->degree[i] = 0;
}

static bool ifg_empty(Chaitin* ra) {
    FOREACH_N(i, 0, ra->ifg_len) {
        if (ra->degree[i]) return false;
    }

    return true;
}

static bool reg_mask_intersect(RegMask a, RegMask b) {
    return a.class == b.class && (a.mask & b.mask) != 0;
}

void tb__chaitin(Ctx* restrict ctx, TB_Arena* arena) {
    Chaitin ra = { .ctx = ctx, .arena = arena, .stack_usage = ctx->stack_usage };

    int k_colors[2] = { 16, 16 };
    CUIK_TIMED_BLOCK("build IFG") {
        size_t normie_nodes = ctx->interval_count - ctx->num_fixed;

        ra.ifg_stride = (normie_nodes + 63) / 64;
        ra.ifg_len    = normie_nodes;
        ra.ifg        = tb_arena_alloc(arena, ra.ifg_len * ra.ifg_stride * sizeof(uint64_t));
        ra.degree     = tb_arena_alloc(arena, ra.ifg_len * sizeof(int));

        memset(ra.ifg,    0, ra.ifg_len * ra.ifg_stride * sizeof(uint64_t));
        memset(ra.degree, 0, ra.ifg_len * sizeof(int));

        Set live = set_create_in_arena(arena, ctx->interval_count);
        FOREACH_REVERSE_N(i, 0, ctx->bb_count) {
            MachineBB* mbb = &ctx->machine_bbs[i];

            Set* live_out = &mbb->live_out;
            set_copy(&live, live_out);

            for (Tile* t = mbb->end; t; t = t->prev) {
                LiveInterval* interval = t->interval;
                if (interval) {
                    set_remove(&live, interval->id);

                    // interfere
                    FOREACH_N(j, 0, ra.ifg_len) {
                        if (!set_get(&live, j)) continue;

                        LiveInterval* other = ctx->id2interval[j];
                        if (!reg_mask_intersect(interval->mask, other->mask)) continue;

                        printf("v%d -- v%td\n", interval->id, j);
                        ifg_edge(&ra, interval->id, j);
                        ifg_edge(&ra, j, interval->id);
                    }

                    // 2 address ops will interfere with their own inputs (except for
                    // shared dst/src)
                    if (t->n && ctx->_2addr(t->n)) {
                        FOREACH_N(j, 1, t->in_count) {
                            LiveInterval* in_def = t->ins[j].src;
                            if (in_def == NULL) continue;
                            if (!reg_mask_intersect(interval->mask, in_def->mask)) continue;

                            printf("v%d -- v%d\n", interval->id, in_def->id);
                            ifg_edge(&ra, interval->id, in_def->id);
                            ifg_edge(&ra, in_def->id, interval->id);
                        }
                    }
                }

                // uses are live now
                FOREACH_N(j, 0, t->in_count) {
                    LiveInterval* in_def = t->ins[j].src;
                    if (in_def) set_put(&live, in_def->id);
                }
            }
        }

        // clone before doing all the fancy node removals
        uint64_t* ifg_copy = tb_arena_alloc(arena, ra.ifg_len * ra.ifg_stride * sizeof(uint64_t));
        int* deg_copy      = tb_arena_alloc(arena, ra.ifg_len * sizeof(int));
        memcpy(ifg_copy, ra.ifg,    ra.ifg_len * ra.ifg_stride * sizeof(uint64_t));
        memcpy(deg_copy, ra.degree, ra.ifg_len * sizeof(int));

        // simplify/select stack
        int cnt = 0, cap = ra.ifg_len;
        LiveInterval** stk = tb_arena_alloc(arena, ra.ifg_len * sizeof(LiveInterval*));

        // simplify phase:
        //   push all nodes with a degree < k
        FOREACH_N(i, 0, ra.ifg_len) {
            int class = ctx->id2interval[i]->mask.class;
            if (ra.degree[i] < k_colors[class]) {
                assert(cnt < cap);
                stk[cnt++] = ctx->id2interval[i];
                ifg_remove_edges(&ra, i);
            }
        }

        // split phase
        if (!ifg_empty(&ra)) {
            float* weights = tb_arena_alloc(arena, ra.ifg_len * sizeof(float));
            NL_ChunkedArr spills = nl_chunked_arr_alloc(tmp_arena);

            // compute spill weights
            FOREACH_N(i, 0, normie_nodes) {
                weights[i] = deg_copy[i];
            }

            do {
                int spill = ctx->num_fixed;

                // TODO(NeGate): pick best spill slot
                FOREACH_N(i, ctx->num_fixed + 1, ra.ifg_len) {
                    tb_todo();
                }

                nl_chunked_arr_put(&spills, ctx->id2interval[spill]);
                ifg_remove_edges(&ra, spill);
            } while (!ifg_empty(&ra));

            // insert splitting code
            for (NL_ArrChunk* restrict chk = spills.first; chk; chk = chk->next) {
                FOREACH_N(i, 0, chk->count) {
                    LiveInterval* spill = chk->elems[i];
                    tb_todo();
                }
            }
            nl_chunked_arr_trim(&spills);
        }

        ra.ifg    = ifg_copy;
        ra.degree = deg_copy;

        // select phase
        while (cnt) {
            LiveInterval* interval = stk[--cnt];
            int i = interval->id;

            uint64_t mask = interval->mask.mask;
            FOREACH_N(j, 0, ra.ifg_stride) {
                uint64_t bits = ra.ifg[i*ra.ifg_stride + j];
                if (bits == 0) continue;

                FOREACH_N(k, 0, 64) if ((bits >> k) & 1) {
                    assert(ctx->id2interval[j*64 + k]->mask.class == interval->mask.class);
                    uint64_t assign_mask = 1ull << ctx->id2interval[j*64 + k]->assigned;
                    mask &= ~assign_mask;
                }
            }

            assert(mask != 0 && "couldn't select color :(");
            interval->assigned = tb_ffs64(mask) - 1;
            printf("v%d = R%d\n", i, interval->assigned);
        }
    }

    ctx->stack_usage = ra.stack_usage;
}
