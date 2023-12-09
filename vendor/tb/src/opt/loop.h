
// clone anything except control edges and phis to region
static TB_Node* loop_clone_node(TB_Passes* restrict p, TB_Function* f, TB_Node* region, TB_Node* n, int phi_index) {
    TB_Node* cloned = n;
    if (n->type == TB_PHI && n->inputs[0] == region) {
        // replace OG with phi's edge
        cloned = n->inputs[phi_index];
    } else if (n->type == TB_REGION || n->type == TB_START || (n->type == TB_PROJ && n->inputs[0]->type == TB_START)) {
        // doesn't clone
    } else {
        size_t extra = extra_bytes(n);
        cloned = tb_alloc_node(f, n->type, n->dt, n->input_count, extra);

        // clone extra data (i hope it's that easy lol)
        memcpy(cloned->extra, n->extra, extra);

        // mark new node
        tb_pass_mark(p, cloned);

        // fill cloned edges
        FOREACH_N(i, 1, n->input_count) {
            TB_Node* in = loop_clone_node(p, f, region, n->inputs[i], phi_index);

            cloned->inputs[i] = in;
            add_user(p, cloned, in, i, NULL);
        }

        cloned = gvn(p, cloned, extra);
    }

    #if TB_OPTDEBUG_LOOP
    printf("CLONE: ");
    print_node_sexpr(n, 0);
    printf(" => ");
    print_node_sexpr(cloned, 0);
    printf("\n");
    #endif

    return cloned;
}

bool tb_pass_loop(TB_Passes* p) {
    cuikperf_region_start("loop rotate", NULL);

    bool progress = false;
    verify_tmp_arena(p);

    size_t block_count = tb_pass_update_cfg(p, &p->worklist, true);
    TB_Node** blocks = &p->worklist.items[0];

    TB_Function* f = p->f;

    // find & canonicalize loops
    DynArray(ptrdiff_t) backedges = NULL;
    FOREACH_N(i, 0, block_count) {
        TB_Node* header = blocks[i];
        if (header->type != TB_REGION || header->input_count < 2) {
            continue;
        }

        // find all backedges
        dyn_array_clear(backedges);
        FOREACH_N(j, 0, header->input_count) {
            TB_Node* pred = get_pred_cfg(&p->cfg, header, j);
            if (lattice_dommy(&p->universe, header, pred)) {
                dyn_array_put(backedges, j);
            }
        }

        // found a loop :)
        if (dyn_array_length(backedges) > 0) {
            TB_OPTDEBUG(LOOP)(printf("found loop on .bb%zu with %zu backedges\n", i, dyn_array_length(backedges)));
            TB_NODE_GET_EXTRA_T(header, TB_NodeRegion)->freq = 10.0f;
        }

        if (0) {
            ptrdiff_t single_backedge = backedges[0];

            // as part of loop simplification we convert backedges into one, this
            // makes it easier to analyze the exit condition.
            if (dyn_array_length(backedges) > 1) {
                tb_todo();
            }

            // somehow we couldn't simplify the loop? welp
            if (single_backedge < 0 || header->input_count != 2) {
                continue;
            }

            // if it's already rotated don't do it again
            if (header->inputs[single_backedge]->type == TB_PROJ && header->inputs[single_backedge]->inputs[0]->type == TB_BRANCH) {
                continue;
            }

            // if we don't have the latch in the header BB... ngmi
            TB_BasicBlock* header_info = &nl_map_get_checked(p->cfg.node_to_block, header);
            TB_Node* latch = header_info->end;
            if (latch->type != TB_BRANCH && TB_NODE_GET_EXTRA_T(latch, TB_NodeBranch)->succ_count != 2) {
                break;
            }

            TB_Node* backedge_bb = get_pred_cfg(&p->cfg, header, single_backedge);

            // check which paths lead to exitting the loop (don't dominate the single backedge)
            int exit_proj_i = -1;
            TB_Node* projs[2];
            for (User* u = latch->users; u; u = u->next) {
                if (u->n->type == TB_PROJ) {
                    TB_Node* succ = cfg_next_bb_after_cproj(u->n);

                    int proj_i = TB_NODE_GET_EXTRA_T(u->n, TB_NodeProj)->index;
                    projs[proj_i] = u->n;

                    if (!lattice_dommy(&p->universe, succ, backedge_bb)) {
                        // if we have multiple exit latches... just leave
                        if (exit_proj_i >= 0) {
                            exit_proj_i = -1;
                            break;
                        }

                        exit_proj_i = proj_i;
                    } else {
                        // body must not have any PHIs
                        if (cfg_has_phis(succ)) {
                            exit_proj_i = -1;
                            break;
                        }
                    }
                }
            }

            // infinite loop? yikes
            if (exit_proj_i < 0) {
                continue;
            }

            // update latch before killing it (we want it's current users to know shit's
            // in motion)
            tb_pass_mark_users(p, latch);

            // convert to rotated loops
            //
            //     header:                    latch:
            //       ...                        ...
            //       if (A) body else exit      if (A) body else exit
            //     body:                      body:
            //       ...                        ...
            //       jmp body                   if (A) body else exit
            //     exit:                      exit:
            //
            // clone latch and intercept the backedge (making sure to replace all phis with
            // the value that's not in the body)
            TB_Node* exit_region;
            {
                int init_edge = 1 - single_backedge;
                TB_Node *bot_cloned = NULL, *top_cloned = NULL;
                for (TB_Node* curr = latch; curr != header; curr = curr->inputs[0]) {
                    TB_Node* cloned = loop_clone_node(p, f, header, curr, 1 + init_edge);
                    tb_pass_mark_users(p, cloned);

                    // attach control edge
                    if (top_cloned) {
                        set_input(p, top_cloned, cloned, 0);
                    } else {
                        bot_cloned = cloned;
                    }
                    top_cloned = cloned;
                }

                // create cprojs
                assert(bot_cloned->type == TB_BRANCH);
                TB_Node* proj0 = make_proj_node(f, p, TB_TYPE_CONTROL, bot_cloned, 0);
                TB_Node* proj1 = make_proj_node(f, p, TB_TYPE_CONTROL, bot_cloned, 1);

                tb_pass_mark(p, proj0);
                tb_pass_mark(p, proj1);

                // add zero trip count check
                TB_Node* ztc_check = tb_alloc_node(f, TB_REGION, TB_TYPE_CONTROL, 1, sizeof(TB_NodeRegion));
                set_input(p, ztc_check, header->inputs[init_edge], 0);
                TB_NODE_GET_EXTRA_T(ztc_check, TB_NodeRegion)->freq = 1.0f;
                tb_pass_mark(p, ztc_check);

                // intercept the init path on the header
                set_input(p, top_cloned, ztc_check, 0);
                set_input(p, header, exit_proj_i ? proj0 : proj1, init_edge);

                exit_region = tb_alloc_node(f, TB_REGION, TB_TYPE_CONTROL, 2, sizeof(TB_NodeRegion));
                TB_NODE_GET_EXTRA_T(exit_region, TB_NodeRegion)->freq = 1.0f;
                tb_pass_mark(p, exit_region);
                set_input(p, exit_region, exit_proj_i ? proj1 : proj0, 0);

                // connect exit projection to exit region, then connect the exit region to
                // what the exit successor wanted
                User* after_exit = cfg_next_user(projs[exit_proj_i]);
                assert(after_exit != NULL && "no successors after exit?");

                set_input(p, exit_region, projs[exit_proj_i], 1);
                set_input(p, after_exit->n, exit_region, after_exit->slot);

                DO_IF(TB_OPTDEBUG_LOOP)(TB_NODE_GET_EXTRA_T(ztc_check, TB_NodeRegion)->tag = lil_name(f, "loop.ztc.%d", i));
                DO_IF(TB_OPTDEBUG_LOOP)(TB_NODE_GET_EXTRA_T(exit_region, TB_NodeRegion)->tag = lil_name(f, "loop.exit.%d", i));
                DO_IF(TB_OPTDEBUG_LOOP)(TB_NODE_GET_EXTRA_T(header, TB_NodeRegion)->tag = lil_name(f, "loop.body.%d", i));
            }

            // insert duplicated branch for backedge
            {
                TB_Node *bot_cloned = NULL, *top_cloned = NULL;
                for (TB_Node* curr = latch; curr != header; curr = curr->inputs[0]) {
                    TB_Node* cloned = loop_clone_node(p, f, header, curr, 1 + single_backedge);

                    // attach control edge
                    if (top_cloned) {
                        set_input(p, top_cloned, cloned, 0);
                    } else {
                        bot_cloned = cloned;
                    }
                    top_cloned = cloned;
                }

                // create cprojs
                assert(bot_cloned->type == TB_BRANCH);
                TB_Node* proj0 = make_proj_node(f, p, TB_TYPE_CONTROL, bot_cloned, 0);
                TB_Node* proj1 = make_proj_node(f, p, TB_TYPE_CONTROL, bot_cloned, 1);
                tb_pass_mark(p, proj0);
                tb_pass_mark(p, proj1);

                set_input(p, top_cloned, header->inputs[single_backedge], 0);
                set_input(p, header, exit_proj_i ? proj0 : proj1, single_backedge);

                // remove initial latch now
                User* after_next = cfg_next_user(projs[1 - exit_proj_i]);
                assert(after_next != NULL && "no successors after next?");
                assert(after_next->slot == 0 && "pretty sure it should've been passed through in[0]");

                set_input(p, exit_region, exit_proj_i ? proj1 : proj0, 1);
                set_input(p, after_next->n, latch->inputs[0], after_next->slot);

                tb_pass_kill_node(p, projs[exit_proj_i]);
                tb_pass_kill_node(p, projs[1 - exit_proj_i]);
                tb_pass_kill_node(p, latch);
            }

            progress = true;
        }

        skip:;
    }

    dyn_array_destroy(backedges);
    cuikperf_region_end();
    return progress;
}
