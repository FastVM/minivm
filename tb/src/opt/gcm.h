// Scheduling: "Global Code Motion Global Value Numbering", Cliff Click 1995
// https://courses.cs.washington.edu/courses/cse501/06wi/reading/click-pldi95.pdf

////////////////////////////////
// Early scheduling
////////////////////////////////
static void schedule_early(TB_Passes* passes, TB_Node* n) {
    // already visited
    if (worklist_test_n_set(&passes->worklist, n)) {
        return;
    }

    // track leaf nodes
    if (n->input_count <= 2) {
        dyn_array_put(passes->worklist.items, n);
    }

    // schedule inputs first
    FOREACH_N(i, 0, n->input_count) if (n->inputs[i]) {
        schedule_early(passes, n->inputs[i]);
    }

    if (!is_pinned(n)) {
        TB_Node* best = passes->f->start_node;
        int best_depth = 0;

        // choose deepest block
        FOREACH_N(i, 0, n->input_count) if (n->inputs[i] && n->inputs[i]->inputs[0]) {
            TB_Node* bb = unsafe_get_region(n->inputs[i]);

            int bb_depth = TB_NODE_GET_EXTRA_T(bb, TB_NodeRegion)->dom_depth;
            if (best_depth < bb_depth) {
                best = bb;
                best_depth = bb_depth;
            }
        }

        if (passes->f->start_node == best) {
            best = passes->f->params[0];
        }

        set_input(passes, n, best, 0);
    }
}

////////////////////////////////
// Late scheduling
////////////////////////////////
// schedule nodes such that they appear the least common
// ancestor to all their users
static TB_Node* find_lca(TB_Node* a, TB_Node* b) {
    if (a == NULL) return b;

    // line both up
    while (dom_depth(a) > dom_depth(b)) a = idom(a);
    while (dom_depth(b) > dom_depth(a)) b = idom(b);

    while (a != b) {
        b = idom(b);
        a = idom(a);
    }

    return a;
}

static void schedule_late(TB_Passes* passes, TB_Node* n) {
    // already visited
    if (worklist_test_n_set(&passes->worklist, n)) {
        return;
    }

    // schedule all users first
    for (User* use = find_users(passes, n); use; use = use->next) {
        schedule_late(passes, use->n);
    }

    // pinned nodes can't be rescheduled
    if (is_pinned(n)) {
        return;
    }

    // we're gonna find the least common ancestor
    TB_Node* lca = NULL;
    for (User* use = find_users(passes, n); use; use = use->next) {
        TB_Node* y = use->n;
        if (y->inputs[0] == NULL) continue; // dead

        TB_Node* use_block = tb_get_parent_region(y->inputs[0]);
        if (y->type == TB_PHI) {
            if (y->input_count != use_block->input_count + 1) {
                tb_panic("phi has parent with mismatched predecessors");
            }

            ptrdiff_t j = 1;
            for (; j < y->input_count; j++) {
                if (y->inputs[j] == n) {
                    break;
                }
            }
            assert(j >= 0);

            use_block = get_block_begin(use_block->inputs[j - 1]);
        }

        lca = find_lca(lca, use_block);
    }

    if (passes->f->start_node == lca) {
        lca = passes->f->params[0];
    }

    // tb_assert(lca, "missing least common ancestor");
    set_input(passes, n, lca, 0);
}

void tb_pass_schedule(TB_Passes* p) {
    if (p->scheduled) {
        return;
    }

    CUIK_TIMED_BLOCK("schedule") {
        Worklist* restrict ws = &p->worklist;
        p->scheduled = true;

        size_t block_count;
        CUIK_TIMED_BLOCK("dominators") {
            worklist_clear(ws);

            block_count = tb_push_postorder(p->f, ws);
            tb_compute_dominators(p->f, block_count, &ws->items[0]);
        }

        CUIK_TIMED_BLOCK("early schedule") {
            worklist_clear_visited(ws);
            FOREACH_N(i, 0, block_count) {
                TB_Node* bb = ws->items[i];
                assert(bb->type == TB_START || bb->type == TB_REGION);

                TB_NodeRegion* r = TB_NODE_GET_EXTRA(bb);
                schedule_early(p, r->end);
            }
        }

        // move nodes closer to their usage site
        CUIK_TIMED_BLOCK("late schedule") {
            worklist_clear_visited(ws);

            // schedule late on leaves
            FOREACH_N(i, 0, dyn_array_length(ws->items)) {
                schedule_late(p, ws->items[i]);
            }

            schedule_late(p, p->f->start_node);
        }
    }
}
