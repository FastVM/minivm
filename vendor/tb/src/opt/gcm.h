#include <chunked_array.h>

// Scheduling: "Global Code Motion Global Value Numbering", Cliff Click 1995
// https://courses.cs.washington.edu/courses/cse501/06wi/reading/click-pldi95.pdf
typedef struct Elem {
    struct Elem* parent;
    TB_ArenaSavepoint sp;
    TB_Node* n;
    int i;
} Elem;

////////////////////////////////
// Late scheduling
////////////////////////////////
// schedule nodes such that they appear the least common
// ancestor to all their users
static TB_BasicBlock* find_lca(TB_Passes* p, TB_BasicBlock* a, TB_BasicBlock* b) {
    if (a == NULL) return b;

    // line both up
    while (a->dom_depth > b->dom_depth) a = a->dom;
    while (b->dom_depth > a->dom_depth) b = b->dom;

    while (a != b) {
        b = b->dom;
        a = a->dom;
    }

    return a;
}

// writes out to the p->scheduled array, it's at the top of the tmp_arena
void tb_pass_schedule(TB_Passes* p, TB_CFG cfg, bool renumber) {
    assert(p->scheduled == NULL && "make sure when you're done with the schedule, you throw away the old one");

    CUIK_TIMED_BLOCK("schedule") {
        TB_Function* f = p->f;
        Worklist* restrict ws = &p->worklist;

        // arraychads stay up
        p->scheduled = tb_arena_alloc(tmp_arena, f->node_count * sizeof(TB_BasicBlock*));
        memset(p->scheduled, 0, f->node_count * sizeof(TB_BasicBlock*));

        TB_ArenaSavepoint sp = tb_arena_save(tmp_arena);
        TB_Node** saved = tb_arena_alloc(tmp_arena, cfg.block_count * sizeof(TB_Node*));
        memcpy(saved, &ws->items[0], cfg.block_count * sizeof(TB_Node*));

        CUIK_TIMED_BLOCK("dominators") {
            // jarvis pull up the dommies
            tb_compute_dominators(p->f, p, cfg);

            FOREACH_N(i, 0, cfg.block_count) {
                TB_Node* n = ws->items[i];
                TB_BasicBlock* bb = &nl_map_get_checked(cfg.node_to_block, n);

                bb->items = nl_hashset_alloc(32);
                nl_hashset_put(&bb->items, n);
                p->scheduled[n->gvn] = bb;
            }

            worklist_clear_visited(ws);
        }

        TB_BasicBlock* start_bb = p->scheduled[p->worklist.items[0]->gvn];
        NL_ChunkedArr pins = nl_chunked_arr_alloc(tmp_arena);

        CUIK_TIMED_BLOCK("pinned schedule") {
            // schedule root's users
            {
                TB_Node* root = f->root_node;
                FOR_USERS(u, root) {
                    TB_Node* out = u->n;
                    if (!worklist_test_n_set(ws, out)) {
                        dyn_array_put(ws->items, out);
                    }
                }
            }

            for (size_t i = cfg.block_count + 1; i < dyn_array_length(ws->items); i++) {
                TB_Node* n = ws->items[i];

                if (is_pinned(n)) {
                    // a region might refer to itself, but a node within a
                    // BB will refer to it's parent (who should've been scheduled
                    // by now)
                    TB_BasicBlock* bb = NULL;
                    if (n->type == TB_PROJ && n->inputs[0]->type == TB_ROOT) {
                        bb = start_bb;
                    } else {
                        TB_Node* curr = n;
                        do {
                            bb = p->scheduled[curr->gvn];
                            curr = curr->inputs[0];
                        } while (!bb);
                    }

                    nl_hashset_put(&bb->items, n);
                    p->scheduled[n->gvn] = bb;
                    nl_chunked_arr_put(&pins, n);

                    DO_IF(TB_OPTDEBUG_GCM)(printf("%s: v%u pinned to .bb%d\n", f->super.name, n->gvn, bb->id));
                }

                FOR_USERS(u, n) {
                    TB_Node* out = u->n;
                    if (!worklist_test_n_set(ws, out)) {
                        dyn_array_put(ws->items, out);
                    }
                }
            }

            DO_IF(TB_OPTDEBUG_GCM)(printf("%s: scheduled %zu nodes (%zu recorded in the graph)\n", f->super.name, dyn_array_length(ws->items) - cfg.block_count, f->node_count));

            if (renumber) {
                // TODO(NeGate): reorder lattice universe
                tb_todo();

                f->node_count = dyn_array_length(ws->items) - cfg.block_count;

                FOREACH_N(i, cfg.block_count, dyn_array_length(ws->items)) {
                    // we'll reassign IDs here such that we have an easier
                    // time with node count based tables (quite a few in codegen).
                    ws->items[i]->gvn = i;
                }
            }
        }

        CUIK_TIMED_BLOCK("early schedule") {
            nl_chunked_arr_trim(&pins);

            // we're gonna use this space to store the DFS order, we'll walk it in reverse for
            // late sched
            worklist_clear_visited(ws);
            dyn_array_set_length(ws->items, cfg.block_count);

            for (NL_ArrChunk* restrict chk = pins.first; chk; chk = chk->next) {
                FOREACH_N(i, 0, chk->count) {
                    TB_Node* pin_n = chk->elems[i];

                    TB_ArenaSavepoint sp = tb_arena_save(tmp_arena);
                    Elem* top = tb_arena_alloc(tmp_arena, sizeof(Elem));
                    top->parent = NULL;
                    top->sp = sp;
                    top->n = pin_n;
                    top->i = pin_n->input_count;

                    // DFS nodes by inputs
                    while (top) {
                        TB_Node* n = top->n;

                        if (top->i > 0) {
                            // push next unvisited in
                            TB_Node* in = n->inputs[--top->i];

                            // pinned nodes can't be rescheduled
                            if (in && !is_pinned(in) && !worklist_test_n_set(ws, in)) {
                                TB_ArenaSavepoint sp = tb_arena_save(tmp_arena);
                                Elem* new_top = tb_arena_alloc(tmp_arena, sizeof(Elem));
                                new_top->parent = top;
                                new_top->sp = sp;
                                new_top->n = in;
                                new_top->i = in->input_count;
                                top = new_top;
                            }
                            continue;
                        }

                        if (n != pin_n) { // only pinned node in the stack
                            // start at the entry point
                            int best_depth = 0;
                            TB_BasicBlock* best = start_bb;

                            // choose deepest block
                            FOREACH_N(i, 0, n->input_count) if (n->inputs[i]) {
                                if (n->inputs[i]->type == TB_ROOT) {
                                    DO_IF(TB_OPTDEBUG_GCM)(printf("  in v%u @ bb0\n", n->inputs[i]->gvn));
                                    continue;
                                }

                                TB_BasicBlock* bb = p->scheduled[n->inputs[i]->gvn];
                                if (bb == NULL) {
                                    // input has no scheduling... weird?
                                    DO_IF(TB_OPTDEBUG_GCM)(printf("  in v%u @ dead\n", n->inputs[i]->gvn));
                                    continue;
                                }

                                DO_IF(TB_OPTDEBUG_GCM)(printf("  in v%u @ bb%d\n", n->inputs[i]->gvn, bb->id));
                                if (best_depth < bb->dom_depth) {
                                    best_depth = bb->dom_depth;
                                    best = bb;
                                }
                            }

                            DO_IF(TB_OPTDEBUG_GCM)(printf("%s: v%u into .bb%d\n", p->f->super.name, n->gvn, best->id));

                            p->scheduled[n->gvn] = best;
                            nl_hashset_put(&best->items, n);
                            dyn_array_put(ws->items, n);
                        }

                        struct Elem* parent = top->parent;
                        tb_arena_restore(tmp_arena, top->sp);
                        top = parent;
                    }
                }
            }
        }

        // move nodes closer to their usage site
        CUIK_TIMED_BLOCK("late schedule") {
            FOREACH_REVERSE_N(i, cfg.block_count, dyn_array_length(ws->items)) {
                TB_Node* n = ws->items[i];

                DO_IF(TB_OPTDEBUG_GCM)(printf("%s: try late v%u\n", p->f->super.name, n->gvn));

                // we're gonna find the least common ancestor
                TB_BasicBlock* lca = NULL;
                FOR_USERS(use, n) {
                    TB_Node* y = use->n;
                    TB_BasicBlock* use_block = p->scheduled[y->gvn];
                    if (use_block == NULL) { continue; } // dead

                    DO_IF(TB_OPTDEBUG_GCM)(printf("  user v%u @ bb%d\n", y->gvn, use_block->id));
                    if (y->type == TB_PHI) {
                        TB_Node* use_node = y->inputs[0];
                        assert(use_node->type == TB_REGION);

                        if (y->input_count != use_node->input_count + 1) {
                            tb_panic("phi has parent with mismatched predecessors");
                        }

                        ptrdiff_t j = 1;
                        for (; j < y->input_count; j++) {
                            if (y->inputs[j] == n) {
                                break;
                            }
                        }
                        assert(j >= 0);

                        TB_BasicBlock* bb = p->scheduled[use_node->inputs[j - 1]->gvn];
                        if (bb) { use_block = bb; }
                    }

                    lca = find_lca(p, lca, use_block);
                }

                if (lca != NULL) {
                    TB_BasicBlock* old = p->scheduled[n->gvn];
                    // i dont think it should be possible to schedule something here
                    // which didn't already get scheduled in EARLY
                    assert(old && "huh?");

                    // replace old BB entry
                    if (old != lca && lca->dom_depth > old->dom_depth) {
                        TB_OPTDEBUG(GCM)(
                            printf("  LATE  v%u into .bb%d: ", n->gvn, lca->id),
                            print_node_sexpr(n, 0),
                            printf("\n")
                        );

                        p->scheduled[n->gvn] = lca;
                        nl_hashset_remove(&old->items, n);
                        nl_hashset_put(&lca->items, n);
                    }
                }
            }
        }

        CUIK_TIMED_BLOCK("copy CFG back in") {
            memcpy(ws->items, saved, cfg.block_count * sizeof(TB_Node*));
            dyn_array_set_length(ws->items, cfg.block_count);

            worklist_clear_visited(ws);
            tb_arena_restore(tmp_arena, sp);
        }
    }
}
