// Local instruction scheduling is handled here, the idea is just to do a topological
// sort which is anti-dependency aware, a future TB could implement multiple schedulers.
//
// Once the worklist is filled, you can walk backwards and generate instructions accordingly.
static bool is_same_bb(TB_Node* bb, TB_Node* n) {
    if (n->type != TB_START && n->inputs[0] == NULL) {
        return false;
    }

    while (n->type != TB_START && n->type != TB_REGION) {
        n = n->inputs[0];
    }

    return n == bb;
}

static void sched_walk_phi(TB_Passes* passes, Worklist* ws, DynArray(PhiVal)* phi_vals, TB_Node* bb, TB_Node* phi, size_t phi_index) {
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

    sched_walk(passes, ws, phi_vals, bb, val);
}

void sched_walk(TB_Passes* passes, Worklist* ws, DynArray(PhiVal)* phi_vals, TB_Node* bb, TB_Node* n) {
    if (!is_same_bb(bb, n) || worklist_test_n_set(ws, n)) {
        return;
    }

    // if we're a branch, push our PHI nodes
    if (n->type == TB_BRANCH) {
        TB_NodeBranch* br = TB_NODE_GET_EXTRA(n);
        TB_Node** succ = br->succ;

        FOREACH_N(i, 0, br->succ_count) {
            TB_Node* dst = br->succ[i];

            // find predecessor index and do that edge
            ptrdiff_t phi_index = -1;
            FOREACH_N(j, 0, dst->input_count) {
                TB_Node* pred = unsafe_get_region(dst->inputs[j]);

                if (pred == bb) {
                    phi_index = j;
                    break;
                }
            }
            if (phi_index < 0) continue;

            // schedule memory PHIs
            for (User* use = find_users(passes, dst); use; use = use->next) {
                TB_Node* phi = use->n;
                if (phi->type == TB_PHI && phi->dt.type == TB_MEMORY) {
                    sched_walk_phi(passes, ws, phi_vals, bb, phi, phi_index);
                }
            }

            // schedule data PHIs, we schedule these afterwards because it's "generally" better
            for (User* use = find_users(passes, dst); use; use = use->next) {
                TB_Node* phi = use->n;
                if (phi->type == TB_PHI && phi->dt.type != TB_MEMORY) {
                    sched_walk_phi(passes, ws, phi_vals, bb, phi, phi_index);
                }
            }
        }
    }

    // push inputs
    FOREACH_REVERSE_N(i, 0, n->input_count) if (n->inputs[i]) {
        sched_walk(passes, ws, phi_vals, bb, n->inputs[i]);
    }

    // before the terminator we should eval leftovers that GCM linked here
    if (is_block_end(n)) {
        TB_Node* parent = get_block_begin(n);
        for (User* use = find_users(passes, parent); use; use = use->next) {
            sched_walk(passes, ws, phi_vals, bb, use->n);
        }
    }

    dyn_array_put(ws->items, n);

    if (is_mem_out_op(n)) {
        // memory effects have anti-dependencies, the previous loads
        // must finish before the next memory effect is applied.
        for (User* use = find_users(passes, n->inputs[1]); use; use = use->next) {
            if (use->slot == 1 && use->n != n) {
                sched_walk(passes, ws, phi_vals, bb, use->n);
            }
        }
    }

    // push outputs (projections, if they apply)
    if (n->dt.type == TB_TUPLE && n->type != TB_BRANCH) {
        for (User* use = find_users(passes, n); use; use = use->next) {
            TB_Node* use_n = use->n;
            if (use_n->type == TB_PROJ) {
                sched_walk(passes, ws, phi_vals, bb, use_n);
            }
        }
    }
}
