enum { MAX_DOM_WALK = 10 };

static TB_Node* ideal_region(TB_Passes* restrict p, TB_Function* f, TB_Node* n) {
    TB_NodeRegion* r = TB_NODE_GET_EXTRA(n);

    // if a region is dead, start a violent death chain
    if (n->input_count == 0) {
        n->type = TB_DEAD;
        return n;
    } else if (n->input_count == 1) {
        // single entry regions are useless...
        // check for any phi nodes, because we're single entry they're all degens
        User* use = n->users;
        while (use != NULL) {
            User* next = use->next;
            if (use->n->type == TB_PHI) {
                assert(use->n->input_count == 2);
                subsume_node(p, f, use->n, use->n->inputs[1]);
            }
            use = next;
        }

        // we might want this as an identity
        return n->inputs[0];
    } else {
        // remove dead predeccessors
        bool changes = false;

        size_t i = 0, extra_edges = 0;
        while (i < n->input_count) {
            if (n->inputs[i]->type == TB_DEAD) {
                changes = true;
                remove_input(p, f, n, i);

                // update PHIs
                for (User* use = n->users; use; use = use->next) {
                    if (use->n->type == TB_PHI && use->slot == 0) {
                        remove_input(p, f, use->n, i + 1);
                    }
                }
                continue;
            } else if (n->inputs[i]->type == TB_REGION) {
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
                        set_input(p, n, pred->inputs[0], i);

                        // append rest to the end (order really doesn't matter)
                        //
                        // NOTE(NeGate): we might waste quite a bit of space because of the arena
                        // alloc and realloc
                        TB_Node** new_inputs = alloc_from_node_arena(f, new_count * sizeof(TB_Node*));
                        memcpy(new_inputs, n->inputs, old_count * sizeof(TB_Node*));
                        n->inputs = new_inputs;
                        n->input_count = new_count;

                        FOREACH_N(j, 0, pred->input_count - 1) {
                            new_inputs[old_count + j] = pred->inputs[j + 1];
                            add_user(p, n, pred->inputs[j + 1], old_count + j, NULL);
                        }
                    }

                    // update PHIs
                    for (User* use = n->users; use; use = use->next) {
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

                            FOREACH_N(j, 0, pred->input_count - 1) {
                                new_inputs[phi_ins + j] = phi_val;
                                add_user(p, phi, phi_val, phi_ins + j, NULL);
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

        return changes ? n : NULL;
    }

    return NULL;
}

static TB_Node* ideal_phi(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    // degenerate PHI, poison it
    if (n->input_count == 1) {
        log_warn("%s: ir: generated poison due to PHI with no edges", f->super.name);
        return make_poison(f, opt, n->dt);
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
            for (User* use = find_users(opt, region); use; use = use->next) {
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
                    for (User* u = branch->users; u; u = u->next) {
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

                    uint64_t falsey = TB_NODE_GET_EXTRA_T(branch, TB_NodeBranch)->keys[0];
                    TB_Node* cond = branch->inputs[1];

                    // TODO(NeGate): handle non-zero falseys
                    if (falsey == 0) {
                        // header -> merge
                        {
                            TB_Node* parent = branch->inputs[0];
                            tb_pass_kill_node(opt, branch);
                            tb_pass_kill_node(opt, left);
                            tb_pass_kill_node(opt, right);

                            // attach the header and merge to each other
                            tb_pass_mark(opt, parent);
                            tb_pass_mark_users(opt, region);
                            subsume_node(opt, f, region, parent);
                        }

                        TB_Node* selector = tb_alloc_node(f, TB_SELECT, dt, 4, 0);
                        set_input(opt, selector, cond, 1);
                        set_input(opt, selector, values[0], 2);
                        set_input(opt, selector, values[1], 3);
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
            set_input(opt, lookup, parent->inputs[1], 1);

            TB_NodeLookup* l = TB_NODE_GET_EXTRA(lookup);
            l->entry_count = br->succ_count;
            FOREACH_N(i, 0, n->input_count - 1) {
                TB_Node* k = region->inputs[i];
                int index = TB_NODE_GET_EXTRA_T(k, TB_NodeProj)->index;
                assert(index < br->succ_count);

                if (index == 0) {
                    l->entries[index].key = 0; // default value, doesn't matter
                } else {
                    l->entries[index].key = br->keys[index - 1];
                }

                TB_NodeInt* v = TB_NODE_GET_EXTRA(n->inputs[1 + i]);
                l->entries[index].val = v->value;
            }

            // kill branch, we don't really need it anymore
            TB_Node* before = parent->inputs[0];
            tb_pass_kill_node(opt, parent);
            subsume_node(opt, f, region, before);

            return lookup;
        }
    }

    return NULL;
}

static TB_Node* ideal_branch(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    TB_NodeBranch* br = TB_NODE_GET_EXTRA(n);

    if (br->succ_count == 2) {
        if (n->input_count == 2 && br->keys[0] == 0) {
            TB_Node* cmp_node = n->inputs[1];
            TB_NodeTypeEnum cmp_type = cmp_node->type;

            // empty BB, just does if branch but the condition is effect-less
            // if (a && b) A else B => if (a ? b : 0) A else B
            //
            // TODO(NeGate): implement form which works on an arbitrary falsey
            /*if (n->inputs[0]->type == TB_REGION && n->inputs[0]->input_count == 2 && is_empty_bb(opt, n)) {
                TB_Node* bb = n->inputs[0];

                uint64_t falsey = br->keys[0];
                TB_Node* pred_branch = bb->inputs[0]->inputs[0];

                // needs one pred
                uint64_t pred_falsey;
                if (bb->input_count == 1 && is_if_branch(pred_branch, &pred_falsey)) {
                    TB_NodeBranch* pred_br_info = TB_NODE_GET_EXTRA(pred_branch);

                    bool bb_on_false = pred_br_info->succ[0] == bb;
                    TB_Node* shared_edge = pred_br_info->succ[!bb_on_false];

                    int shared_i = -1;
                    if (shared_edge == br->succ[0]) shared_i = 0;
                    if (shared_edge == br->succ[1]) shared_i = 1;

                    if (shared_i >= 0) {
                        TB_Node* pred_cmp = pred_branch->inputs[1];

                        // convert first branch into an unconditional into bb
                        transmute_goto(opt, f, pred_branch, bb);

                        // we wanna normalize into a comparison (not a boolean -> boolean)
                        if (!(pred_cmp->dt.type == TB_INT && pred_cmp->dt.data == 1)) {
                            assert(pred_cmp->dt.type != TB_FLOAT && "TODO");
                            TB_Node* imm = make_int_node(f, opt, pred_cmp->dt, pred_falsey);
                            tb_pass_mark(opt, imm);

                            TB_Node* new_node = tb_alloc_node(f, TB_CMP_NE, TB_TYPE_BOOL, 3, sizeof(TB_NodeCompare));
                            set_input(opt, new_node, pred_cmp, 1);
                            set_input(opt, new_node, imm, 2);
                            TB_NODE_SET_EXTRA(new_node, TB_NodeCompare, .cmp_dt = pred_cmp->dt);

                            tb_pass_mark(opt, new_node);
                            pred_cmp = new_node;
                        }

                        TB_Node* false_node = make_int_node(f, opt, n->inputs[1]->dt, falsey);
                        tb_pass_mark(opt, false_node);

                        // a ? b : 0
                        TB_Node* selector = tb_alloc_node(f, TB_SELECT, n->inputs[1]->dt, 4, 0);
                        set_input(opt, selector, pred_cmp, 1);
                        set_input(opt, selector, n->inputs[1], 2 + bb_on_false);
                        set_input(opt, selector, false_node, 2 + !bb_on_false);

                        set_input(opt, n, selector, 1);
                        tb_pass_mark(opt, selector);
                        return n;
                    }
                }
            }*/

            // br ((y <= x)) => br (x < y) flipped conditions
            if (cmp_type == TB_CMP_SLE || cmp_type == TB_CMP_ULE) {
                TB_Node* new_cmp = tb_alloc_node(f, cmp_type == TB_CMP_SLE ? TB_CMP_SLT : TB_CMP_ULT, TB_TYPE_BOOL, 3, sizeof(TB_NodeCompare));
                set_input(opt, new_cmp, cmp_node->inputs[2], 1);
                set_input(opt, new_cmp, cmp_node->inputs[1], 2);
                TB_NODE_SET_EXTRA(new_cmp, TB_NodeCompare, .cmp_dt = TB_NODE_GET_EXTRA_T(cmp_node, TB_NodeCompare)->cmp_dt);

                // flip
                for (User* u = n->users; u; u = u->next) {
                    TB_NodeProj* p = TB_NODE_GET_EXTRA(u->n);
                    p->index = !p->index;
                }

                set_input(opt, n, new_cmp, 1);
                tb_pass_mark(opt, new_cmp);
                return n;
            }

            // br ((x != y) != 0) => br (x != y)
            if ((cmp_type == TB_CMP_NE || cmp_type == TB_CMP_EQ) && cmp_node->inputs[2]->type == TB_INTEGER_CONST) {
                uint64_t imm = TB_NODE_GET_EXTRA_T(cmp_node->inputs[2], TB_NodeInt)->value;
                set_input(opt, n, cmp_node->inputs[1], 1);
                br->keys[0] = imm;

                // flip successors
                if (cmp_type == TB_CMP_EQ) {
                    for (User* u = n->users; u; u = u->next) {
                        TB_NodeProj* p = TB_NODE_GET_EXTRA(u->n);
                        p->index = !p->index;
                    }
                }

                return n;
            }
        }
    }

    // constant fold branch
    if (n->input_count == 2) {
        Lattice* key = lattice_universe_get(&opt->universe, n->inputs[1]);

        ptrdiff_t taken = -1;
        if (key->tag == LATTICE_INT && key->_int.min == key->_int.max) {
            int64_t key_const = key->_int.max;
            taken = 0;

            FOREACH_N(i, 0, br->succ_count - 1) {
                int64_t case_key = br->keys[i];
                if (key_const == case_key) {
                    taken = i + 1;
                    break;
                }
            }
        } else if (br->succ_count == 2) {
            // TODO(NeGate): extend this to JOIN redundant checks to ideally
            // narrow on more complex checks.
            int64_t* primary_keys = br->keys;

            // check for redundant conditions in the doms.
            TB_Node* initial_bb = get_block_begin(n->inputs[0]);
            for (User* u = n->inputs[1]->users; u; u = u->next) {
                if (u->n->type != TB_BRANCH || u->slot != 1 || u->n == n) {
                    continue;
                }

                TB_Node* end = u->n;
                int64_t* keys = TB_NODE_GET_EXTRA_T(end, TB_NodeBranch)->keys;
                if (TB_NODE_GET_EXTRA_T(end, TB_NodeBranch)->succ_count != 2 && keys[0] != primary_keys[0]) {
                    continue;
                }

                for (User* succ_user = end->users; succ_user; succ_user = succ_user->next) {
                    assert(succ_user->n->type == TB_PROJ);
                    int index = TB_NODE_GET_EXTRA_T(succ_user->n, TB_NodeProj)->index;
                    TB_Node* succ = cfg_next_bb_after_cproj(succ_user->n);

                    // we must be dominating for this to work
                    if (!lattice_dommy(&opt->universe, succ, initial_bb)) {
                        continue;
                    }

                    taken = index;
                    goto match;
                }
            }
        }

        if (taken >= 0) match: {
            TB_Node* dead = make_dead_node(f, opt);

            // convert dead projections into DEAD and convert live projection into index 0
            for (User* u = n->users; u; u = u->next) {
                TB_Node* proj = u->n;
                if (proj->type == TB_PROJ) {
                    int index = TB_NODE_GET_EXTRA_T(proj, TB_NodeProj)->index;
                    if (index != taken) {
                        subsume_node(opt, f, proj, dead);
                    } else {
                        TB_NODE_GET_EXTRA_T(proj, TB_NodeProj)->index = 0;

                        // if we folded away from a region, then we should subsume
                        // the degen phis.
                        subsume_node(opt, f, proj, n->inputs[0]);
                    }
                }
            }

            // remove condition (shouldn't have any users at thi point)
            assert(n->users == NULL);
            tb_pass_mark_users(opt, n->inputs[0]);
            tb_pass_mark_users(opt, dead);

            return dead;
        }
    }

    return NULL;
}
