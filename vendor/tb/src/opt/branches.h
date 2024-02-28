
static TB_Node* ideal_region(TB_Passes* restrict p, TB_Function* f, TB_Node* n) {
    TB_NodeRegion* r = TB_NODE_GET_EXTRA(n);

    // if a region is dead, start a violent death chain
    if (n->input_count == 0) {
        return NULL;
    } else if (n->input_count == 1) {
        // single entry regions are useless...
        // check for any phi nodes, because we're single entry they're all degens
        User* use = n->users;
        while (use != NULL) {
            User* next = use->next;
            if (use->n->type == TB_PHI) {
                assert(use->n->input_count == 2);
                subsume_node(f, use->n, use->n->inputs[1]);
            }
            use = next;
        }

        // we might want this as an identity
        return n->inputs[0];
    } else {
        bool changes = false;

        size_t i = 0, extra_edges = 0;
        while (i < n->input_count) {
            Lattice* ty = lattice_universe_get(p, n->inputs[i]);
            if (n->inputs[i]->type == TB_DEAD || ty == &XCTRL_IN_THE_SKY) {
                remove_input(f, n, i);

                // update PHIs
                FOR_USERS(use, n) {
                    if (use->n->type == TB_PHI && use->slot == 0) {
                        remove_input(f, use->n, i + 1);
                    }
                }
            } else if (cfg_is_region(n->inputs[i])) {
                #if 1
                // pure regions can be collapsed into direct edges
                if (n->inputs[i]->users->next == NULL && n->inputs[i]->input_count > 0) {
                    assert(n->inputs[i]->users->n == n);
                    changes = true;

                    TB_Node* pred = n->inputs[i];
                    {
                        size_t old_count = n->input_count;
                        size_t new_count = old_count + (pred->input_count - 1);

                        // convert pred-of-pred into direct pred
                        set_input(f, n, pred->inputs[0], i);

                        // append rest to the end (order really doesn't matter)
                        //
                        // NOTE(NeGate): we might waste quite a bit of space because of the arena
                        // alloc and realloc
                        TB_Node** new_inputs = alloc_from_node_arena(f, new_count * sizeof(TB_Node*));
                        memcpy(new_inputs, n->inputs, old_count * sizeof(TB_Node*));
                        n->inputs = new_inputs;
                        n->input_count = new_count;
                        n->input_cap = new_count;

                        FOREACH_N(j, 0, pred->input_count - 1) {
                            new_inputs[old_count + j] = pred->inputs[j + 1];
                            add_user(f, n, pred->inputs[j + 1], old_count + j, NULL);
                        }
                    }

                    // update PHIs
                    FOR_USERS(use, n) {
                        if (use->n->type == TB_PHI && use->slot == 0) {
                            // we don't replace the initial, just the rest
                            TB_Node* phi = use->n;
                            TB_Node* phi_val = phi->inputs[i + 1];

                            // append more phi vals... lovely allocs
                            size_t phi_ins = phi->input_count;
                            size_t new_phi_ins = phi_ins + (pred->input_count - 1);

                            TB_Node** new_inputs = alloc_from_node_arena(f, new_phi_ins * sizeof(TB_Node*));
                            memcpy(new_inputs, phi->inputs, phi_ins * sizeof(TB_Node*));
                            phi->inputs = new_inputs;
                            phi->input_count = new_phi_ins;
                            phi->input_cap = new_phi_ins;

                            FOREACH_N(j, 0, pred->input_count - 1) {
                                new_inputs[phi_ins + j] = phi_val;
                                add_user(f, phi, phi_val, phi_ins + j, NULL);
                            }
                        }
                    }

                    extra_edges += 1;
                    continue;
                }
                #endif
            }

            i += 1;
        }

        if (changes) {
            return n->input_count == 1 ? n->inputs[0] : n;
        }
    }

    return NULL;
}

static TB_Node* ideal_phi(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    // degenerate PHI, poison it
    if (n->input_count == 1) {
        log_warn("%s: ir: generated poison due to PHI with no edges", f->super.name);
        return make_poison(f, n->dt);
    }

    // if branch, both paths are empty => select(cond, t, f)
    //
    // TODO(NeGate): we can make this diamond trick work for bigger
    // branches, we should support a lookup instruction similar to
    // "switch" logic for data.
    TB_DataType dt = n->dt;
    TB_Node* region = n->inputs[0];
    if (n->dt.type != TB_MEMORY) {
        if (region->input_count == 2) {
            // for now we'll leave multi-phi scenarios alone, we need
            // to come up with a cost-model around this stuff.
            FOR_USERS(use, region) {
                if (use->n->type == TB_PHI) {
                    if (use->n != n) return NULL;
                }
            }

            // guarentee paths are effectless (there's only one data phi and no control nodes)
            //
            //        If
            //       /  \
            // CProjT    CProjF          Region[0][0] == Region[1][0]
            //       \  /
            //      Region
            //
            TB_Node* left = region->inputs[0];
            TB_Node* right = region->inputs[1];
            if (left->type == TB_PROJ && right->type == TB_PROJ &&
                left->inputs[0]->type == TB_BRANCH && left->inputs[0] == right->inputs[0]) {
                TB_Node* branch = left->inputs[0];
                TB_NodeBranch* header_br = TB_NODE_GET_EXTRA(branch);

                if (header_br->succ_count == 2) {
                    assert(branch->input_count == 2);

                    TB_Node *values[2];
                    FOR_USERS(u, branch) {
                        TB_Node* proj = u->n;
                        if (proj->type == TB_PROJ) {
                            int index = TB_NODE_GET_EXTRA_T(proj, TB_NodeProj)->index;
                            // the projection needs to exclusively refer to the region,
                            // if not we can't elide those effects here.
                            if (proj->users->next != NULL || proj->users->n != region) {
                                return NULL;
                            }

                            int phi_i = proj->users->slot;
                            assert(phi_i + 1 < n->input_count);
                            values[index] = n->inputs[1 + phi_i];
                        }
                    }

                    uint64_t falsey = TB_NODE_GET_EXTRA_T(branch, TB_NodeBranch)->keys[0].key;
                    TB_Node* cond = branch->inputs[1];

                    // TODO(NeGate): handle non-zero falseys
                    if (falsey == 0) {
                        // header -> merge
                        {
                            TB_Node* parent = branch->inputs[0];
                            tb_pass_kill_node(f, branch);
                            tb_pass_kill_node(f, left);
                            tb_pass_kill_node(f, right);

                            // attach the header and merge to each other
                            tb_pass_mark(opt, parent);
                            tb_pass_mark_users(opt, region);
                            subsume_node(f, region, parent);
                        }

                        TB_Node* selector = tb_alloc_node(f, TB_SELECT, dt, 4, 0);
                        set_input(f, selector, cond, 1);
                        set_input(f, selector, values[0], 2);
                        set_input(f, selector, values[1], 3);
                        return selector;
                    }
                }
            }
        }

        if (region->input_count > 2 && n->dt.type == TB_INT) {
            if (region->inputs[0]->type != TB_PROJ || region->inputs[0]->inputs[0]->type != TB_BRANCH) {
                return NULL;
            }

            // try to make a multi-way lookup:
            //
            //      Branch
            //       / | \
            //    ... ... ...         each of these is a CProj
            //       \ | /
            //       Region
            //            \
            //             \ ... ...  each of these is a trivial value (int consts only for now)
            //              \ | /
            //               Phi
            TB_Node* parent = region->inputs[0]->inputs[0];
            TB_NodeBranch* br = TB_NODE_GET_EXTRA(parent);
            if (parent->type != TB_BRANCH || br->succ_count != n->input_count - 1) {
                return NULL;
            }

            // verify we have a really clean looking diamond shape
            FOREACH_N(i, 0, n->input_count - 1) {
                if (region->inputs[i]->type != TB_PROJ || region->inputs[i]->inputs[0] != parent) return NULL;
                if (n->inputs[1 + i]->type != TB_INTEGER_CONST) return NULL;
            }

            // convert to lookup node
            TB_Node* lookup = tb_alloc_node(f, TB_LOOKUP, n->dt, 2, sizeof(TB_NodeLookup) + (br->succ_count * sizeof(TB_LookupEntry)));
            set_input(f, lookup, parent->inputs[1], 1);

            TB_NodeLookup* l = TB_NODE_GET_EXTRA(lookup);
            l->entry_count = br->succ_count;
            FOREACH_N(i, 0, n->input_count - 1) {
                TB_Node* k = region->inputs[i];
                int index = TB_NODE_GET_EXTRA_T(k, TB_NodeProj)->index;
                assert(index < br->succ_count);

                if (index == 0) {
                    l->entries[index].key = 0; // default value, doesn't matter
                } else {
                    l->entries[index].key = br->keys[index - 1].key;
                }

                TB_NodeInt* v = TB_NODE_GET_EXTRA(n->inputs[1 + i]);
                l->entries[index].val = v->value;
            }

            // kill branch, we don't really need it anymore
            TB_Node* before = parent->inputs[0];
            tb_pass_kill_node(f, parent);
            subsume_node(f, region, before);

            return lookup;
        }
    }

    return NULL;
}

static TB_Node* ideal_branch(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    TB_NodeBranch* br = TB_NODE_GET_EXTRA(n);

    if (br->succ_count == 2) {
        if (n->input_count == 2 && br->keys[0].key == 0) {
            TB_Node* cmp_node = n->inputs[1];
            TB_NodeTypeEnum cmp_type = cmp_node->type;

            // empty BB, just does if branch but the condition is effect-less
            // if (a && b) A else B => if (a ? b : 0) A else B
            //
            // TODO(NeGate): implement form which works on an arbitrary falsey
            if (n->inputs[0]->type == TB_PROJ && n->inputs[0]->inputs[0]->type == TB_BRANCH && is_empty_bb(opt, n)) {
                uint64_t falsey = br->keys[0].key;
                TB_Node* pred_branch = n->inputs[0]->inputs[0];

                int index = TB_NODE_GET_EXTRA_T(n->inputs[0], TB_NodeProj)->index;

                // needs one pred
                uint64_t pred_falsey;
                if (is_if_branch(pred_branch, &pred_falsey) && pred_falsey == 0) {
                    TB_NodeBranch* pred_br_info = TB_NODE_GET_EXTRA(pred_branch);

                    // check our parent's aux path
                    User* other_proj      = proj_with_index(pred_branch, 1 - index);
                    TB_Node* shared_edge  = cfg_next_bb_after_cproj(other_proj->n);

                    // check our aux path
                    User* other_proj2     = proj_with_index(n, 1 - index);
                    TB_Node* shared_edge2 = cfg_next_bb_after_cproj(other_proj2->n);

                    // if they're the same then we've got a shortcircuit eval setup
                    if (shared_edge == shared_edge2) {
                        assert(cfg_is_region(shared_edge));
                        int shared_i  = other_proj->n->users->slot;
                        int shared_i2 = other_proj2->n->users->slot;

                        bool match = true;
                        FOR_USERS(phis, shared_edge) if (phis->n->type == TB_PHI) {
                            if (phis->n->inputs[1+shared_i] != phis->n->inputs[1+shared_i2]) {
                                match = false;
                                break;
                            }
                        }

                        if (match) {
                            // remove pred from shared edge
                            remove_input(f, shared_edge, shared_i);
                            FOR_USERS(use, shared_edge) {
                                if (use->n->type == TB_PHI && use->slot == 0) {
                                    remove_input(f, use->n, shared_i + 1);
                                    tb_pass_mark(opt, use->n);
                                }
                            }

                            TB_Node* before = pred_branch->inputs[0];
                            TB_Node* cmp = pred_branch->inputs[1];

                            // remove first branch
                            tb_pass_kill_node(f, pred_branch);
                            set_input(f, n, before, 0);

                            // we wanna normalize into a comparison (not a boolean -> boolean)
                            if (!(cmp->dt.type == TB_INT && cmp->dt.data == 1)) {
                                assert(cmp->dt.type != TB_FLOAT && "TODO");
                                TB_Node* imm = make_int_node(f, opt, cmp->dt, pred_falsey);

                                TB_Node* new_node = tb_alloc_node(f, TB_CMP_NE, TB_TYPE_BOOL, 3, sizeof(TB_NodeCompare));
                                set_input(f, new_node, cmp, 1);
                                set_input(f, new_node, imm, 2);
                                TB_NODE_SET_EXTRA(new_node, TB_NodeCompare, .cmp_dt = cmp->dt);

                                tb_pass_mark(opt, new_node);
                                cmp = new_node;
                            }

                            // construct branchless merge
                            TB_Node* false_node = make_int_node(f, opt, n->inputs[1]->dt, 0);

                            // a ? b : 0
                            TB_Node* selector = tb_alloc_node(f, TB_SELECT, n->inputs[1]->dt, 4, 0);
                            set_input(f, selector, cmp,          1);
                            set_input(f, selector, n->inputs[1], 2);
                            set_input(f, selector, false_node,   3);

                            set_input(f, n, selector, 1);
                            tb_pass_mark(opt, selector);
                            return n;
                        }
                    }
                }
            }

            // br ((y <= x)) => br (x < y) flipped conditions
            if (cmp_type == TB_CMP_SLE || cmp_type == TB_CMP_ULE) {
                TB_Node* new_cmp = tb_alloc_node(f, cmp_type == TB_CMP_SLE ? TB_CMP_SLT : TB_CMP_ULT, TB_TYPE_BOOL, 3, sizeof(TB_NodeCompare));
                set_input(f, new_cmp, cmp_node->inputs[2], 1);
                set_input(f, new_cmp, cmp_node->inputs[1], 2);
                TB_NODE_SET_EXTRA(new_cmp, TB_NodeCompare, .cmp_dt = TB_NODE_GET_EXTRA_T(cmp_node, TB_NodeCompare)->cmp_dt);

                // flip
                FOR_USERS(u, n) {
                    TB_NodeProj* p = TB_NODE_GET_EXTRA(u->n);
                    p->index = !p->index;
                }

                set_input(f, n, new_cmp, 1);
                tb_pass_mark(opt, new_cmp);
                return n;
            }

            // br ((x != y) != 0) => br (x != y)
            if ((cmp_type == TB_CMP_NE || cmp_type == TB_CMP_EQ) && cmp_node->inputs[2]->type == TB_INTEGER_CONST) {
                uint64_t imm = TB_NODE_GET_EXTRA_T(cmp_node->inputs[2], TB_NodeInt)->value;
                set_input(f, n, cmp_node->inputs[1], 1);
                br->keys[0].key = imm;

                // flip successors
                if (cmp_type == TB_CMP_EQ) {
                    br->keys[0].taken = br->total_hits - br->keys[0].taken;
                    FOR_USERS(u, n) {
                        TB_NodeProj* p = TB_NODE_GET_EXTRA(u->n);
                        p->index = !p->index;
                    }
                }

                return n;
            }
        }
    }

    return NULL;
}

static Lattice* value_call(TB_Passes* restrict opt, TB_Node* n) {
    TB_NodeCall* c = TB_NODE_GET_EXTRA(n);

    size_t size = sizeof(Lattice) + c->proj_count*sizeof(Lattice*);
    Lattice* l = tb_arena_alloc(tmp_arena, size);
    *l = (Lattice){ LATTICE_TUPLE, ._tuple = { c->proj_count } };

    FOREACH_N(i, 1, c->proj_count) {
        l->elems[i] = &BOT_IN_THE_SKY;
    }

    FOR_USERS(u, n) {
        if (u->n->type == TB_PROJ) {
            int index = TB_NODE_GET_EXTRA_T(u->n, TB_NodeProj)->index;
            if (index > 0) {
                l->elems[index] = lattice_from_dt(opt, u->n->dt);
            }
        }
    }

    // control just flows through
    l->elems[0] = lattice_universe_get(opt, n->inputs[0]);

    Lattice* k = nl_hashset_put2(&opt->type_interner, l, lattice_hash, lattice_cmp);
    if (k) {
        tb_arena_free(tmp_arena, l, size);
        return k;
    } else {
        return l;
    }
}

static Lattice* value_branch(TB_Passes* restrict opt, TB_Node* n) {
    Lattice* before = lattice_universe_get(opt, n->inputs[0]);
    if (before == &TOP_IN_THE_SKY) {
        return &TOP_IN_THE_SKY;
    }

    TB_NodeBranch* br = TB_NODE_GET_EXTRA(n);

    // constant fold branch
    assert(n->input_count == 2);
    Lattice* key = lattice_universe_get(opt, n->inputs[1]);
    if (key == &TOP_IN_THE_SKY) {
        return &TOP_IN_THE_SKY;
    }

    ptrdiff_t taken = -1;
    if (key->tag == LATTICE_INT && key->_int.min == key->_int.max) {
        int64_t key_const = key->_int.max;
        taken = 0;

        FOREACH_N(i, 0, br->succ_count - 1) {
            int64_t case_key = br->keys[i].key;
            if (key_const == case_key) {
                taken = i + 1;
                break;
            }
        }
    } else if (br->succ_count == 2) {
        TB_BranchKey* primary_keys = br->keys;

        // check for redundant conditions in the doms.
        FOR_USERS(u, n->inputs[1]) {
            if (u->n->type != TB_BRANCH || u->slot != 1 || u->n == n) {
                continue;
            }

            TB_Node* end = u->n;
            TB_BranchKey* keys = TB_NODE_GET_EXTRA_T(end, TB_NodeBranch)->keys;
            if (TB_NODE_GET_EXTRA_T(end, TB_NodeBranch)->succ_count != 2 || keys[0].key != primary_keys[0].key) {
                continue;
            }

            FOR_USERS(succ_user, end) {
                assert(succ_user->n->type == TB_PROJ);
                int index = TB_NODE_GET_EXTRA_T(succ_user->n, TB_NodeProj)->index;
                TB_Node* succ = cfg_next_bb_after_cproj(succ_user->n);

                // we must be dominating for this to work
                if (!fast_dommy(succ, n)) {
                    continue;
                }

                taken = index;
                goto match;
            }
        }
    }

    // construct tuple type
    match:;
    size_t size = sizeof(Lattice) + br->succ_count*sizeof(Lattice*);
    Lattice* l = tb_arena_alloc(tmp_arena, size);
    *l = (Lattice){ LATTICE_TUPLE, ._tuple = { br->succ_count } };
    FOREACH_N(i, 0, br->succ_count) {
        l->elems[i] = taken < 0 || i == taken ? &CTRL_IN_THE_SKY : &XCTRL_IN_THE_SKY;
    }

    Lattice* k = nl_hashset_put2(&opt->type_interner, l, lattice_hash, lattice_cmp);
    if (k) {
        tb_arena_free(tmp_arena, l, size);
        return k;
    } else {
        return l;
    }
}

static TB_Node* identity_safepoint(TB_Passes* restrict p, TB_Function* f, TB_Node* n) {
    if (n->inputs[0]->type == TB_SAFEPOINT_POLL) {
        // (safepoint (safepoint X)) => (safepoint X)
        return n->inputs[0];
    } else {
        return n;
    }
}

static TB_Node* identity_region(TB_Passes* restrict p, TB_Function* f, TB_Node* n) {
    // fold out diamond shaped patterns
    TB_Node* same = n->inputs[0];
    if (same->type == TB_PROJ && same->inputs[0]->type == TB_BRANCH) {
        same = same->inputs[0];

        // if it has phis... quit
        FOR_USERS(u, n) {
            if (u->n->type == TB_PHI) {
                return n;
            }
        }

        FOREACH_N(i, 1, n->input_count) {
            if (n->inputs[i]->type != TB_PROJ || n->inputs[i]->inputs[0] != same) {
                return n;
            }
        }

        TB_Node* before = same->inputs[0];
        tb_pass_kill_node(f, same);
        return before;
    }

    return n;
}

static TB_Node* identity_phi(TB_Passes* restrict p, TB_Function* f, TB_Node* n) {
    TB_Node* same = NULL;
    FOREACH_N(i, 1, n->input_count) {
        if (n->inputs[i] == n) continue;
        if (same && same != n->inputs[i]) return n;
        same = n->inputs[i];
    }

    assert(same);
    if (p) {
        tb_pass_mark_users(p, n->inputs[0]);
    }

    return same;
}
