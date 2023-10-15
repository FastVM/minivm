
typedef struct {
    TB_Function* f;

    size_t block_count;
    TB_Node** blocks;
} DomContext;

// we'll be walking backwards from the end node
static void postorder(Worklist* restrict ws, TB_Node* n) {
    if (!worklist_test_n_set(ws, n)) {
        // walk control edges (aka predecessors)
        TB_NodeRegion* r = TB_NODE_GET_EXTRA(n);
        if (r->end->type == TB_BRANCH) {
            TB_NodeBranch* br = TB_NODE_GET_EXTRA(r->end);
            FOREACH_REVERSE_N(i, 0, br->succ_count) {
                postorder(ws, br->succ[i]);
            }
        }

        dyn_array_put(ws->items, n);
    }
}

size_t tb_push_postorder(TB_Function* f, Worklist* restrict ws) {
    assert(dyn_array_length(ws->items) == 0);
    postorder(ws, f->start_node);
    return dyn_array_length(ws->items);
}

static int find_traversal_index(TB_Node* n) {
    assert(n->type == TB_REGION || n->type == TB_START);
    assert(TB_NODE_GET_EXTRA_T(n, TB_NodeRegion)->postorder_id >= 0);
    return TB_NODE_GET_EXTRA_T(n, TB_NodeRegion)->postorder_id;
}

static int try_find_traversal_index(TB_Node* n) {
    assert(n->type == TB_REGION || n->type == TB_START);
    return TB_NODE_GET_EXTRA_T(n, TB_NodeRegion)->postorder_id;
}

static int resolve_dom_depth(TB_Node* bb) {
    if (dom_depth(bb) >= 0) {
        return dom_depth(bb);
    }

    int parent = resolve_dom_depth(idom(bb));

    // it's one more than it's parent
    TB_NODE_GET_EXTRA_T(bb, TB_NodeRegion)->dom_depth = parent + 1;
    return parent + 1;
}

TB_DominanceFrontiers* tb_get_dominance_frontiers(TB_Function* f, size_t count, TB_Node** blocks) {
    size_t stride = (count + 63) / 64;
    size_t elems = stride * count;
    size_t size = sizeof(TB_DominanceFrontiers) + sizeof(uint64_t)*elems;

    TB_DominanceFrontiers* df = tb_platform_heap_alloc(size);
    memset(df, 0, size);
    df->stride = stride;

    FOREACH_REVERSE_N(i, 0, count) {
        TB_Node* bb = blocks[i];
        assert(TB_NODE_GET_EXTRA_T(bb, TB_NodeRegion)->postorder_id == i);

        if (bb->input_count >= 2) {
            FOREACH_N(k, 0, bb->input_count) {
                TB_Node* runner = unsafe_get_region(bb->inputs[k]);

                while (runner->input_count > 0 && runner != idom(bb)) {
                    // add to frontier set
                    TB_NodeRegion* r = TB_NODE_GET_EXTRA(runner);
                    tb_dommy_fronts_put(df, r->postorder_id, i);

                    runner = idom(runner);
                }
            }
        }
    }

    return df;
}

TB_API void tb_free_dominance_frontiers(TB_DominanceFrontiers* df) {
    tb_platform_heap_free(df);
}

// https://www.cs.rice.edu/~keith/EMBED/dom.pdf
void tb_compute_dominators(TB_Function* f, size_t count, TB_Node** blocks) {
    DomContext ctx = { .f = f, .block_count = count, .blocks = blocks };

    FOREACH_N(i, 0, count) {
        TB_NodeRegion* r = TB_NODE_GET_EXTRA(blocks[i]);
        r->dom_depth = -1; // unresolved
        r->dom = NULL;
        r->postorder_id = i;
    }

    // entry dominates itself
    TB_NodeRegion* r = TB_NODE_GET_EXTRA(f->start_node);
    r->dom_depth = 0;
    r->dom = f->start_node;

    // identify post order traversal order
    int entry_dom = ctx.block_count - 1;

    bool changed = true;
    while (changed) {
        changed = false;

        // for all nodes, b, in reverse postorder (except start node)
        FOREACH_REVERSE_N(i, 0, count - 1) {
            TB_Node* b = blocks[i];
            TB_Node* new_idom = unsafe_get_region(b->inputs[0]);

            // for all other predecessors, p, of b
            FOREACH_N(j, 1, b->input_count) {
                TB_Node* p = unsafe_get_region(b->inputs[j]);

                // if doms[p] already calculated
                TB_Node* idom_p = TB_NODE_GET_EXTRA_T(p, TB_NodeRegion)->dom;
                if (idom_p == NULL && p->input_count > 0) {
                    int a = try_find_traversal_index(p);
                    if (a >= 0) {
                        int b = find_traversal_index(new_idom);
                        while (a != b) {
                            // while (finger1 < finger2)
                            //   finger1 = doms[finger1]
                            while (a < b) {
                                TB_Node* d = idom(blocks[a]);
                                a = d ? find_traversal_index(d) : entry_dom;
                            }

                            // while (finger2 < finger1)
                            //   finger2 = doms[finger2]
                            while (b < a) {
                                TB_Node* d = idom(blocks[b]);
                                b = d ? find_traversal_index(d) : entry_dom;
                            }
                        }

                        new_idom = blocks[a];
                    }
                }
            }

            assert(new_idom != NULL);
            TB_NodeRegion* region_b = TB_NODE_GET_EXTRA_T(b, TB_NodeRegion);
            if (region_b->dom != new_idom) {
                region_b->dom = new_idom;
                changed = true;
            }
        }
    }

    // generate depth values
    CUIK_TIMED_BLOCK("generate dom tree") {
        FOREACH_N(i, 0, count - 1) {
            resolve_dom_depth(blocks[i]);
        }
    }
}

TB_Node* tb_get_parent_region(TB_Node* n) {
    while (n->type != TB_REGION && n->type != TB_START) {
        tb_assert(n->inputs[0], "node has no have a control edge");
        n = n->inputs[0];
    }

    return n;
}

bool tb_is_dominated_by(TB_Node* expected_dom, TB_Node* bb) {
    while (expected_dom != bb) {
        TB_Node* new_bb = idom(bb);
        if (bb == new_bb) {
            return false;
        }

        bb = new_bb;
    }

    return true;
}

