// Local instruction scheduling is handled here, the idea is just to do a topological
// sort which is anti-dependency aware, a future TB could implement multiple schedulers.
//
// Once the worklist is filled, you can walk backwards and generate instructions accordingly.
static void sched_walk_phi(TB_Passes* passes, Worklist* ws, DynArray(PhiVal)* phi_vals, TB_BasicBlock* bb, TB_Node* phi, size_t phi_index) {
    TB_Node* val = phi->inputs[1 + phi_index];

    // reserve PHI space
    if (phi_vals && phi->dt.type != TB_MEMORY) {
        PhiVal p;
        p.phi = phi;
        p.n   = val;
        p.dst = -1;
        p.src = -1;
        dyn_array_put(*phi_vals, p);
    }

    sched_walk(passes, ws, phi_vals, bb, val, false);
}

void sched_walk(TB_Passes* passes, Worklist* ws, DynArray(PhiVal)* phi_vals, TB_BasicBlock* bb, TB_Node* n, bool is_end) {
    ptrdiff_t search = nl_map_get(passes->scheduled, n);
    if (search < 0 || passes->scheduled[search].v != bb || worklist_test_n_set(ws, n)) {
        return;
    }

    // if we're a branch, push our PHI nodes
    if (is_end) {
        for (User* u = n->users; u; u = u->next) {
            if (!cfg_is_control(u->n)) continue;
            TB_Node* dst = cfg_next_region_control(u->n);

            // find predecessor index and do that edge
            ptrdiff_t phi_index = -1;
            FOREACH_N(j, 0, dst->input_count) {
                TB_BasicBlock* pred = nl_map_get_checked(passes->scheduled, dst->inputs[j]);

                if (pred == bb) {
                    phi_index = j;
                    break;
                }
            }
            if (phi_index < 0) continue;

            // schedule memory PHIs
            for (User* use = dst->users; use; use = use->next) {
                TB_Node* phi = use->n;
                if (phi->type == TB_PHI && phi->dt.type == TB_MEMORY) {
                    sched_walk_phi(passes, ws, phi_vals, bb, phi, phi_index);
                }
            }

            // schedule data PHIs, we schedule these afterwards because it's "generally" better
            for (User* use = dst->users; use; use = use->next) {
                TB_Node* phi = use->n;
                if (phi->type == TB_PHI && phi->dt.type != TB_MEMORY) {
                    sched_walk_phi(passes, ws, phi_vals, bb, phi, phi_index);
                }
            }
        }
    }

    // push inputs
    FOREACH_REVERSE_N(i, 0, n->input_count) if (n->inputs[i]) {
        sched_walk(passes, ws, phi_vals, bb, n->inputs[i], false);
    }

    // before the terminator we should eval leftovers that GCM linked here
    if (is_end) {
        nl_hashset_for(entry, &bb->items) {
            sched_walk(passes, ws, phi_vals, bb, *entry, false);
        }
    }

    dyn_array_put(ws->items, n);

    if (is_mem_out_op(n) && n->type != TB_PHI && n->type != TB_PROJ) {
        // memory effects have anti-dependencies, the previous loads
        // must finish before the next memory effect is applied.
        for (User* use = find_users(passes, n->inputs[1]); use; use = use->next) {
            if (use->slot == 1 && use->n != n) {
                sched_walk(passes, ws, phi_vals, bb, use->n, false);
            }
        }
    }

    // push outputs (projections, if they apply)
    if (n->dt.type == TB_TUPLE && n->type != TB_BRANCH) {
        for (User* use = find_users(passes, n); use; use = use->next) {
            TB_Node* use_n = use->n;
            if (use_n->type == TB_PROJ) {
                sched_walk(passes, ws, phi_vals, bb, use_n, false);
            }
        }
    }
}
