
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

        size_t i = 0;
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
        if (left->inputs[0]->type == TB_BRANCH && left->inputs[0] == right->inputs[0]) {
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

            // check if we're dominated by a branch that already checked it
            /*TB_Node* bb = get_block_begin(n->inputs[0]);
            for (User* u = find_users(opt, cmp_node); u; u = u->next) {
                if (u->n != n && u->slot == 1 && u->n->type == TB_BRANCH) {
                    TB_NodeBranch* dom_branch = TB_NODE_GET_EXTRA(u->n);
                    if (dom_branch->succ_count == 2 && dom_branch->keys[0] == 0) {
                        // found another branch, check if we're dominated by one of it's successors
                        // if so, then all our paths to 'br' can use info from the branch.
                        ptrdiff_t match = -1;
                        FOREACH_N(i, 0, dom_branch->succ_count) {
                            TB_Node* target = dom_branch->succ[i];
                            if (tb_is_dominated_by(opt->cfg, target, bb)) {
                                match = i;
                                break;
                            }
                        }

                        // we can now look for the condition to match
                        if (match >= 0) {
                            transmute_goto(opt, f, n, br->succ[match]);
                        }
                    }
                }
            }*/
        }
    }

    // constant fold branch
    if (n->input_count == 2) {
        Lattice* key = lattice_universe_get(&opt->universe, n->inputs[1]);

        // we can walk the dominator tree to see if the condition is already
        // been checked.

        if (key->tag == LATTICE_INT && key->_int.min == key->_int.max) {
            int64_t key_const = key->_int.max;

            size_t taken = 0;
            FOREACH_N(i, 0, br->succ_count - 1) {
                int64_t case_key = br->keys[i];
                if (key_const == case_key) {
                    taken = i + 1;
                    break;
                }
            }

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
                        assert(proj->users->next == NULL);
                        TB_Node* succ = proj->users->n;
                        if (succ->type == TB_REGION) {
                            int phi_i = proj->users->slot;

                            User* u = succ->users;
                            while (u != NULL) {
                                User* next = u->next;
                                if (u->n->type == TB_PHI) {
                                    tb_pass_mark_users(opt, u->n);
                                    subsume_node(opt, f, u->n, u->n->inputs[phi_i + 1]);
                                }
                                u = next;
                            }
                        }

                        tb_pass_kill_node(opt, proj);
                        set_input(opt, succ, n->inputs[0], 0);
                    }
                }
            }

            // remove condition
            return dead;
        }
    }

    return NULL;
}
