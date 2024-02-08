
typedef struct Block {
    struct Block* parent;
    TB_ArenaSavepoint sp;
    TB_Node* bb;
    TB_Node* end;
    int succ_i;
    TB_Node* succ[];
} Block;

void tb_free_cfg(TB_CFG* cfg) {
    nl_map_for(i, cfg->node_to_block) {
        nl_hashset_free(cfg->node_to_block[i].v.items);
    }
    nl_map_free(cfg->node_to_block);
}

TB_CFG tb_compute_rpo(TB_Function* f, TB_Passes* p) {
    return tb_compute_rpo2(f, &p->worklist);
}

// walks until the terminator or other critical edge
static TB_Node* end_of_bb(TB_Node* n) {
    while (!cfg_is_terminator(n)) {
        TB_Node* next = cfg_next_control0(n);
        if (next == NULL || next->type == TB_REGION) {
            break;
        }
        n = next;
    }

    return n;
}

static Block* create_block(TB_Arena* arena, TB_Node* bb) {
    TB_ArenaSavepoint sp = tb_arena_save(tmp_arena);

    TB_Node* end = end_of_bb(bb);
    size_t succ_count = end->type == TB_BRANCH ? TB_NODE_GET_EXTRA_T(end, TB_NodeBranch)->succ_count : 1;
    if (cfg_is_endpoint(end)) {
        succ_count = 0;
    }

    Block* top = tb_arena_alloc(arena, sizeof(Block) + succ_count*sizeof(TB_Node*));
    *top = (Block){
        .sp  = sp,
        .bb  = bb,
        .end = end,
        .succ_i = succ_count,
    };

    if (end->type == TB_BRANCH) {
        FOR_USERS(u, end) {
            if (u->n->type == TB_PROJ) {
                int index = TB_NODE_GET_EXTRA_T(u->n, TB_NodeProj)->index;
                top->succ[index] = cfg_next_bb_after_cproj(u->n);
            }
        }
    } else if (!cfg_is_endpoint(end)) {
        top->succ[0] = cfg_next_user(end)->n;
    }

    return top;
}

TB_CFG tb_compute_rpo2(TB_Function* f, Worklist* ws) {
    cuikperf_region_start("RPO", NULL);
    assert(dyn_array_length(ws->items) == 0);

    TB_CFG cfg = { 0 };
    nl_map_create(cfg.node_to_block, (f->node_count / 16) + 4);

    // push initial block
    Block* top = create_block(tmp_arena, f->params[0]);
    worklist_test_n_set(ws, f->params[0]);

    while (top != NULL) {
        cuikperf_region_start("rpo_iter", NULL);
        if (top->succ_i > 0) {
            // push next unvisited succ
            TB_Node* succ = top->succ[--top->succ_i];
            if (!worklist_test_n_set(ws, succ)) {
                Block* new_top = create_block(tmp_arena, succ);
                new_top->parent = top;
                top = new_top;
            }
        } else {
            Block b = *top;

            TB_BasicBlock bb = { .start = b.bb, .end = b.end, .dom_depth = -1 };
            if (b.bb->type == TB_REGION) {
                bb.freq = TB_NODE_GET_EXTRA_T(b.bb, TB_NodeRegion)->freq;
                assert(bb.freq >= BB_LOW_FREQ);
            } else {
                bb.freq = 1.0;
            }

            dyn_array_put(ws->items, b.bb);
            nl_map_put(cfg.node_to_block, b.bb, bb);
            cfg.block_count += 1;

            tb_arena_restore(tmp_arena, top->sp);
            top = b.parent; // off to wherever we left off
        }
        cuikperf_region_end();
    }

    // just reverse the items here... im too lazy to flip all my uses
    CUIK_TIMED_BLOCK("reversing") {
        size_t last = cfg.block_count - 1;
        FOREACH_N(i, 0, cfg.block_count / 2) {
            SWAP(TB_Node*, ws->items[i], ws->items[last - i]);
        }
    }

    CUIK_TIMED_BLOCK("dom depths") {
        FOREACH_N(i, 0, cfg.block_count) {
            TB_BasicBlock* bb = &nl_map_get_checked(cfg.node_to_block, ws->items[i]);
            if (i == 0) {
                bb->dom_depth = 0;
            }
            bb->id = i;
        }
    }

    cuikperf_region_end();
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

static TB_BasicBlock* get_pred_bb(TB_CFG* cfg, TB_Node* n, int i) {
    n = get_pred(n, i);
    return &nl_map_get_checked(cfg->node_to_block, n);
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
                TB_Node* runner = get_pred_cfg(&cfg, bb, k);

                while (!(runner->type == TB_PROJ && runner->inputs[0]->type == TB_ROOT) && runner != idom(&cfg, bb)) {
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

    TB_BasicBlock* entry = &nl_map_get_checked(cfg.node_to_block, blocks[0]);
    entry->dom = entry;

    bool changed = true;
    while (changed) {
        changed = false;

        // for all nodes, b, in reverse postorder (except start node)
        FOREACH_N(i, 1, cfg.block_count) {
            TB_Node* b = blocks[i];
            TB_Node* new_idom = NULL;

            // pick first "processed" pred
            size_t j = 0, pred_count = b->input_count;
            for (; j < pred_count; j++) {
                TB_Node* p = get_pred(b, j);
                if (idom(&cfg, p) != NULL) {
                    new_idom = p;
                    break;
                }
            }

            // for all other predecessors, p, of b
            for (; j < pred_count; j++) {
                TB_Node* p = get_pred(b, j);

                // if doms[p] already calculated
                TB_Node* idom_p = idom(&cfg, p);
                if (idom_p != NULL) {
                    assert(p->input_count > 0);

                    int a = try_find_traversal_index(&cfg, p);
                    if (a >= 0) {
                        int b = find_traversal_index(&cfg, new_idom);
                        while (a != b) {
                            // while (finger1 < finger2)
                            //   finger1 = doms[finger1]
                            while (a > b) {
                                TB_Node* d = idom(&cfg, blocks[a]);
                                a = d ? find_traversal_index(&cfg, d) : 0;
                            }

                            // while (finger2 < finger1)
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

            assert(new_idom != NULL);
            TB_BasicBlock* b_bb = &nl_map_get_checked(cfg.node_to_block, b);
            if (b_bb->dom == NULL || b_bb->dom->start != new_idom) {
                b_bb->dom = &nl_map_get_checked(cfg.node_to_block, new_idom);
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

bool tb_is_dominated_by(TB_CFG cfg, TB_Node* expected_dom, TB_Node* n) {
    TB_BasicBlock* expected = &nl_map_get_checked(cfg.node_to_block, expected_dom);
    TB_BasicBlock* bb = &nl_map_get_checked(cfg.node_to_block, n);

    while (bb != expected) {
        if (bb->dom == bb) {
            return false;
        }
        bb = bb->dom;
    }

    return true;
}
