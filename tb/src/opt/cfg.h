
void tb_free_cfg(TB_CFG* cfg) {
    nl_map_for(i, cfg->node_to_block) {
        nl_hashset_free(cfg->node_to_block[i].v.items);
    }
    nl_map_free(cfg->node_to_block);
}

TB_CFG tb_compute_rpo(TB_Function* f, TB_Passes* p) {
    return tb_compute_rpo2(f, &p->worklist, &p->stack);
}

static TB_Node* next_control(Worklist* ws, TB_Node* n) {
    // unless it's a branch (aka a terminator), it'll have one successor
    TB_Node* next = NULL;
    for (User* u = n->users; u; u = u->next) {
        TB_Node* succ = u->n;

        // we can't treat regions in the chain
        if (succ->type == TB_REGION) break;

        // we've found the next step in control flow
        if (cfg_is_control(succ) && !worklist_test_n_set(ws, succ)) {
            return succ;
        }
    }

    return NULL;
}

TB_CFG tb_compute_rpo2(TB_Function* f, Worklist* ws, DynArray(TB_Node*)* tmp_stack) {
    assert(dyn_array_length(ws->items) == 0);

    TB_CFG cfg = { 0 };
    DynArray(TB_Node*) stack = *tmp_stack;
    if (stack == NULL) {
        stack = dyn_array_create(TB_Node*, 1024);
    }

    dyn_array_put(stack, f->start_node);
    worklist_test_n_set(ws, f->start_node);

    // depth-first search
    int order = 0;
    while (dyn_array_length(stack)) {
        TB_Node* n = dyn_array_pop(stack);

        // we've spotted a BB entry
        if (cfg_is_bb_entry(n)) {
            // proj BB's will prefer to be REGION BB's
            if (n->inputs[0]->type != TB_START && n->type == TB_PROJ && n->users->n->type == TB_REGION) {
                // we've already seen this BB, let's skip it
                if (worklist_test_n_set(ws, n->users->n)) {
                    continue;
                }

                n = n->users->n;
            }

            // walk until terminator
            TB_Node* entry = n;
            TB_BasicBlock bb = { .id = cfg.block_count++ };
            while (!cfg_is_terminator(n)) {
                TB_Node* next = next_control(ws, n);
                if (next == NULL) {
                    break;
                }
                n = next;
            }

            // the start node always has it's dom depth filled
            if (bb.id == 0) {
                bb.dom = entry;
                bb.dom_depth = 0;
            } else {
                bb.dom_depth = -1;
            }

            bb.end = n;
            dyn_array_put(ws->items, entry);
            nl_map_put(cfg.node_to_block, entry, bb);
        }

        // add successors (could be multi-way like a branch)
        if (n->type == TB_BRANCH) {
            size_t succ_count = TB_NODE_GET_EXTRA_T(n, TB_NodeBranch)->succ_count;

            dyn_array_put_uninit(stack, succ_count);
            TB_Node** top = &stack[dyn_array_length(stack) - 1];

            for (User* u = n->users; u; u = u->next) {
                TB_Node* succ = u->n;
                if (cfg_is_control(succ) && !worklist_test_n_set(ws, succ)) {
                    assert(succ->type == TB_PROJ);
                    int index = TB_NODE_GET_EXTRA_T(succ, TB_NodeProj)->index;
                    top[-index] = succ;
                }
            }
        } else {
            for (User* u = n->users; u; u = u->next) {
                TB_Node* succ = u->n;
                if (cfg_is_control(succ) && !worklist_test_n_set(ws, succ)) {
                    dyn_array_put(stack, succ);
                }
            }
        }
    }

    *tmp_stack = stack;
    return cfg;
}

static int find_traversal_index(TB_CFG* cfg, TB_Node* n) {
    return nl_map_get_checked(cfg->node_to_block, n).id;
}

static int try_find_traversal_index(TB_CFG* cfg, TB_Node* n) {
    ptrdiff_t search = nl_map_get(cfg->node_to_block, n);
    return search >= 0 ? cfg->node_to_block[search].v.id : -1;
}

static int resolve_dom_depth(TB_CFG* cfg, TB_Node* bb) {
    if (dom_depth(cfg, bb) >= 0) {
        return dom_depth(cfg, bb);
    }

    int parent = resolve_dom_depth(cfg, idom(cfg, bb));

    // it's one more than it's parent
    nl_map_get_checked(cfg->node_to_block, bb).dom_depth = parent + 1;
    return parent + 1;
}

TB_DominanceFrontiers* tb_get_dominance_frontiers(TB_Function* f, TB_Passes* restrict p, TB_CFG cfg, TB_Node** blocks) {
    size_t stride = (cfg.block_count + 63) / 64;
    size_t elems = stride * cfg.block_count;
    size_t size = sizeof(TB_DominanceFrontiers) + sizeof(uint64_t)*elems;

    TB_DominanceFrontiers* df = tb_platform_heap_alloc(size);
    memset(df, 0, size);
    df->stride = stride;

    FOREACH_N(i, 0, cfg.block_count) {
        TB_Node* bb = blocks[i];
        assert(find_traversal_index(&cfg, bb) == i);

        if (bb->type == TB_REGION && bb->input_count >= 2) {
            FOREACH_N(k, 0, bb->input_count) {
                TB_Node* runner = get_pred(bb, k);

                while (!(runner->type == TB_PROJ && runner->inputs[0]->type == TB_START) && runner != idom(&cfg, bb)) {
                    // add to frontier set
                    int id = nl_map_get_checked(cfg.node_to_block, runner).id;
                    tb_dommy_fronts_put(df, id, i);

                    runner = idom(&cfg, runner);
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
void tb_compute_dominators(TB_Function* f, TB_Passes* restrict p, TB_CFG cfg) {
    tb_compute_dominators2(f, &p->worklist, cfg);
}

void tb_compute_dominators2(TB_Function* f, Worklist* ws, TB_CFG cfg) {
    TB_Node** blocks = ws->items;
    bool changed = true;
    while (changed) {
        changed = false;

        // for all nodes, b, in reverse postorder (except start node)
        FOREACH_REVERSE_N(i, 1, cfg.block_count) {
            TB_Node* b = blocks[i];
            TB_Node* new_idom = get_pred(b, 0);

            if (b->type == TB_REGION) {
                // for all other predecessors, p, of b
                FOREACH_N(j, 1, b->input_count) {
                    TB_Node* p = get_pred(b, j);

                    // if doms[p] already calculated
                    TB_Node* idom_p = idom(&cfg, p);
                    if (idom_p == NULL && p->input_count > 0) {
                        int a = try_find_traversal_index(&cfg, p);
                        if (a >= 0) {
                            int b = find_traversal_index(&cfg, new_idom);
                            while (a != b) {
                                // while (finger1 > finger2)
                                //   finger1 = doms[finger1]
                                while (a > b) {
                                    TB_Node* d = idom(&cfg, blocks[a]);
                                    a = d ? find_traversal_index(&cfg, d) : 0;
                                }

                                // while (finger2 > finger1)
                                //   finger2 = doms[finger2]
                                while (b > a) {
                                    TB_Node* d = idom(&cfg, blocks[b]);
                                    b = d ? find_traversal_index(&cfg, d) : 0;
                                }
                            }

                            new_idom = blocks[a];
                        }
                    }
                }
            }

            assert(new_idom != NULL);
            TB_Node** dom_ptr = &nl_map_get_checked(cfg.node_to_block, b).dom;
            if (*dom_ptr != new_idom) {
                *dom_ptr = new_idom;
                changed = true;
            }
        }
    }

    // generate depth values
    CUIK_TIMED_BLOCK("generate dom tree") {
        FOREACH_REVERSE_N(i, 1, cfg.block_count) {
            resolve_dom_depth(&cfg, blocks[i]);
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

bool tb_is_dominated_by(TB_CFG cfg, TB_Node* expected_dom, TB_Node* bb) {
    while (expected_dom != bb) {
        TB_Node* new_bb = idom(&cfg, bb);
        if (bb == new_bb) {
            return false;
        }

        bb = new_bb;
    }

    return true;
}

