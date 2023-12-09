// Scheduling: "Global Code Motion Global Value Numbering", Cliff Click 1995
// https://courses.cs.washington.edu/courses/cse501/06wi/reading/click-pldi95.pdf
static uint32_t node_hash(void* a) { return ((TB_Node*) a)->gvn; }
static bool node_compare(void* a, void* b) { return a == b; }

static float node_cost(TB_Node* n) {
    switch (n->type) {
        case TB_AND:
        case TB_OR:
        case TB_XOR:
        case TB_ADD:
        case TB_SUB:
        case TB_MUL:
        case TB_SHL:
        case TB_SHR:
        case TB_SAR:
        case TB_CMP_EQ:
        case TB_CMP_NE:
        case TB_CMP_SLT:
        case TB_CMP_SLE:
        case TB_CMP_ULT:
        case TB_CMP_ULE:
        case TB_MEMBER_ACCESS:
        case TB_ARRAY_ACCESS:
        case TB_LOAD:
        return 2.0f;

        // we don't wanna just hoist things we haven't thought about
        default:
        return 0.0f;
    }
}

////////////////////////////////
// Early scheduling
////////////////////////////////
static void schedule_early(TB_Passes* p, TB_Node* n) {
    // already visited
    if (worklist_test_n_set(&p->worklist, n)) {
        return;
    }

    // push node, late scheduling will process this list
    dyn_array_put(p->worklist.items, n);

    // schedule inputs first
    FOREACH_N(i, 0, n->input_count) if (n->inputs[i]) {
        schedule_early(p, n->inputs[i]);
    }

    // schedule unpinned nodes
    if (!is_pinned(n) || n->input_count == 0) {
        // start at the entry point
        TB_BasicBlock* best = nl_map_get_checked(p->scheduled, p->worklist.items[0]);
        int best_depth = 0;

        // choose deepest block
        FOREACH_N(i, 0, n->input_count) if (n->inputs[i]) {
            ptrdiff_t search = nl_map_get(p->scheduled, n->inputs[i]);
            if (search < 0) {
                // input has no scheduling... weird?
                continue;
            }

            TB_BasicBlock* bb = p->scheduled[search].v;
            if (best_depth < bb->dom_depth) {
                best_depth = bb->dom_depth;
                best = bb;
            }
        }

        DO_IF(TB_OPTDEBUG_GCM)(printf("%s: v%u into .bb%d\n", p->f->super.name, n->gvn, best->id));

        nl_hashset_put2(&best->items, n, node_hash, node_compare);
        nl_map_put(p->scheduled, n, best);
    }
}

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

static void schedule_late(TB_Passes* p, TB_Node* n) {
    // pinned nodes can't be rescheduled
    if (!is_pinned(n)) {
        DO_IF(TB_OPTDEBUG_GCM)(printf("%s: try late v%u\n", p->f->super.name, n->gvn));

        // we're gonna find the least common ancestor
        TB_BasicBlock* lca = NULL;
        for (User* use = n->users; use; use = use->next) {
            TB_Node* y = use->n;

            ptrdiff_t search = nl_map_get(p->scheduled, y);
            if (search < 0) continue; // dead

            TB_BasicBlock* use_block = p->scheduled[search].v;
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

                ptrdiff_t search = nl_map_get(p->scheduled, use_node->inputs[j - 1]);
                if (search >= 0) use_block = p->scheduled[search].v;
            }

            lca = find_lca(p, lca, use_block);
        }

        // tb_assert(lca, "missing least common ancestor");
        if (lca != NULL) {
            TB_OPTDEBUG(GCM)(
                printf("  LATE  v%u into .bb%d: ", n->gvn, lca->id),
                print_node_sexpr(n, 0),
                printf("\n")
            );

            ptrdiff_t search = nl_map_get(p->scheduled, n);
            // i dont think it should be possible to schedule something here
            // which didn't already get scheduled in EARLY
            assert(search >= 0 && "huh?");

            // replace old BB entry
            TB_BasicBlock* old = p->scheduled[search].v;
            if (old != lca) {
                p->scheduled[search].v = lca;
                nl_hashset_remove2(&old->items, n, node_hash, node_compare);
                nl_hashset_put2(&lca->items, n, node_hash, node_compare);
            }
        }
    }
}

void tb_pass_schedule(TB_Passes* p, TB_CFG cfg) {
    if (p->scheduled != NULL) {
        nl_map_free(p->scheduled);
    }

    CUIK_TIMED_BLOCK("schedule") {
        Worklist* restrict ws = &p->worklist;
        nl_map_create(p->scheduled, 256);

        CUIK_TIMED_BLOCK("dominators") {
            // jarvis pull up the dommies
            tb_compute_dominators(p->f, p, cfg);

            worklist_clear_visited(ws);
            FOREACH_N(i, 0, cfg.block_count) {
                TB_BasicBlock* best = &nl_map_get_checked(cfg.node_to_block, ws->items[i]);
                best->items = nl_hashset_alloc(32);
            }
        }

        CUIK_TIMED_BLOCK("pinned schedule") {
            FOREACH_REVERSE_N(i, 0, cfg.block_count) {
                TB_Node* bb_node = ws->items[i];
                TB_BasicBlock* bb = &nl_map_get_checked(cfg.node_to_block, bb_node);

                if (i == 0) {
                    // schedule START node
                    TB_Node* start = p->f->start_node;
                    nl_hashset_put2(&bb->items, start, node_hash, node_compare);
                    nl_map_put(p->scheduled, start, bb);
                }

                TB_Node* n = bb->end;
                for (;;) {
                    DO_IF(TB_OPTDEBUG_GCM)(printf("%s: v%u pinned to .bb%d\n", p->f->super.name, n->gvn, bb->id));
                    nl_hashset_put2(&bb->items, n, node_hash, node_compare);
                    nl_map_put(p->scheduled, n, bb);

                    // mark projections into the same block
                    for (User* use = n->users; use; use = use->next) {
                        TB_Node* proj = use->n;
                        if (use->slot == 0 && (proj->type == TB_PROJ || proj->type == TB_PHI)) {
                            if (nl_map_get(p->scheduled, proj) < 0) {
                                DO_IF(TB_OPTDEBUG_GCM)(printf("%s: proj v%u pinned to .bb%d\n", p->f->super.name, proj->gvn, bb->id));
                                nl_hashset_put2(&bb->items, proj, node_hash, node_compare);
                                nl_map_put(p->scheduled, proj, bb);
                            }
                        }
                    }

                    if (n == bb_node) break;
                    n = n->inputs[0];
                }
            }
        }

        CUIK_TIMED_BLOCK("early schedule") {
            FOREACH_REVERSE_N(i, 0, cfg.block_count) {
                TB_Node* end = nl_map_get_checked(cfg.node_to_block, ws->items[i]).end;
                schedule_early(p, end);
            }
        }

        // move nodes closer to their usage site
        CUIK_TIMED_BLOCK("late schedule") {
            FOREACH_N(i, cfg.block_count, dyn_array_length(ws->items)) {
                schedule_late(p, ws->items[i]);
            }

            worklist_clear_visited(ws);
            dyn_array_set_length(ws->items, cfg.block_count);
        }
    }
}
