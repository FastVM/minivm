#if 0
TB_Attrib* get_debug_var(TB_Node* n) {
    dyn_array_for(i, n->attribs) {
        if (n->attribs[i].tag == TB_ATTRIB_VARIABLE) return &n->attribs[i];
    }

    return NULL;
}

static void add_region_pred_tracked(TB_Passes* opt, TB_Function* f, TB_Node* n, TB_Node* pred) {
    // detach old predecessor list, make bigger one
    assert(n->type == TB_REGION);

    size_t old_count = n->input_count;
    TB_Node** new_inputs = alloc_from_node_arena(f, (old_count + 1) * sizeof(TB_Node*));
    memcpy(new_inputs, n->inputs, old_count * sizeof(TB_Node*));
    new_inputs[old_count] = pred;

    n->inputs = new_inputs;
    n->input_count = old_count + 1;

    add_user(opt, n, pred, old_count, NULL);
}

bool tb_passes_loop(TB_Passes* p) {
    verify_tmp_arena(p);

    TB_Function* f = p->f;
    TB_PostorderWalk order = p->order;

    // canonicalize loops
    DynArray(ptrdiff_t) backedges = NULL;
    FOREACH_N(i, 0, order.count) {
        TB_Node* header = order.traversal[i];
        if (header->input_count != 2) {
            continue;
        }

        // find backedges, we'll be unifying them soon
        dyn_array_clear(backedges);
        FOREACH_N(j, 0, header->input_count) {
            if (tb_is_dominated_by(header, tb_get_parent_region(header->inputs[j]))) {
                dyn_array_put(backedges, j);
            }
        }

        size_t backedge_count = dyn_array_length(backedges);
        if (backedge_count == 0) {
            continue;
        }

        DO_IF(TB_OPTDEBUG_LOOP)(printf("Loop %p:\n", header));
        tb_assert(backedge_count == 1, "TODO: we only support one backedge rn");

        // the header in uncanonical form has a branch at the top to exit,
        // also this branch should be a trivial IF not anything wacky like
        // a switch... tf?
        TB_Node* br = TB_NODE_GET_EXTRA_T(header, TB_NodeRegion)->end;
        TB_NodeBranch* br_info = TB_NODE_GET_EXTRA(br);
        if (br->type != TB_BRANCH || br->input_count != 2 || br_info->succ_count != 2) {
            continue;
        }

        // find the loop exit path
        ptrdiff_t exit_i = -1;
        TB_Node* backedge_proj = header->inputs[backedges[0]];
        TB_Node* backedge_bb = tb_get_parent_region(backedge_proj);
        FOREACH_N(j, 0, br_info->succ_count) {
            if (!tb_is_dominated_by(br_info->succ[j], backedge_bb)) {
                DO_IF(TB_OPTDEBUG_LOOP)(printf("  exit %p\n", br_info->succ[j]));
                exit_i = j;
                break;
            }
        }

        // we didn't find the loop exit here... it's not a valid candidate
        // for loop rotation
        if (i < 0) {
            continue;
        }

        // Loop rotation is not fun:
        //
        //       header         header:                    header:
        //         |              ...                        ...
        //         v              if (A) body else exit      if (A) body else exit
        //     +->body->exit    body:                      body:
        //     |   |              ...                        ...
        //     |   v              jmp body                   if (A) body else exit
        //     +-latch          exit:                      exit:
        //
        TB_Node* cond = br->inputs[1];
        TB_Node* exit = br_info->succ[exit_i];
        TB_Node* body = br_info->succ[1 - exit_i];

        // move phi nodes from the header to the body
        User* use = find_users(p, header);
        while (use != NULL) {
            User* next = use->next;
            if (use->n->type == TB_PHI) {
                set_input(p, use->n, body, 0);
            }
            use = next;
        }

        DO_IF(TB_OPTDEBUG_LOOP)(r->tag = lil_name(f, "loop.header.%d", i));
        br_info->succ_count = 1;
        br_info->succ[0] = body;

        // duplicate condition for backedge
        bool n;
        TB_Node* dupped_cond = clone_node(p, f, header, cond, &n);
        set_input(p, br, dupped_cond, 0);

        // backedge needs to refer to the body not the header
        TB_Node* backedge_br = backedge_proj->inputs[0];
        TB_NodeBranch* backedge_br_info = TB_NODE_GET_EXTRA(backedge_br);
        DO_IF(TB_OPTDEBUG_LOOP)(backedge->tag = lil_name(f, "loop.back.%d", i));

        backedge_br_info->succ_count = 2;
        backedge_br_info->succ = alloc_from_node_arena(f, 2 * sizeof(TB_Node*));
        backedge_br_info->succ[0] = body;
        backedge_br_info->succ[1] = exit;

        // backedge should now point into the body
        add_region_pred_tracked(p, f, body, backedge_proj);
        add_region_pred_tracked(p, f, exit, backedge_proj);

        {
            TB_Node* old = backedge_br->inputs[0];

            backedge_br->input_count = 2;
            backedge_br->inputs = alloc_from_node_arena(f, 2 * sizeof(TB_Node*));
            backedge_br->inputs[0] = old;

            add_user(p, backedge_br, cond, 1, NULL);
            backedge_br->inputs[1] = cond;
        }

        // the other path leaves the loop
        {
            TB_Node* proj = tb_alloc_node(f, TB_PROJ, TB_TYPE_CONTROL, 1, sizeof(TB_NodeProj));
            set_input(p, proj, backedge_proj->inputs[0], 0);
            TB_NODE_SET_EXTRA(proj, TB_NodeProj, .index = 1);
            add_region_pred_tracked(p, f, exit, proj);
        }

        if (backedges[0] == 1) {
            set_input(p, header, header->inputs[1 - backedges[0]], 0);
        }
        header->input_count = 1;
    }

    tb_function_print(f, tb_default_print_callback, stdout);
    return false;
}
#endif
