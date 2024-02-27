
typedef enum {
    // we had some escape so we can't rewrite
    RENAME_NONE,

    // the value has no escapes but requires pointer
    // arithmetic, we can at least make equivalence
    // classes for them & split edges.
    RENAME_MEMORY,

    // best stuff, simple names + phis
    RENAME_VALUE,
} RenameMode;

typedef struct {
    TB_Node* addr;
    int alias_idx;
} Rename;

typedef struct {
    TB_Node* n;
    int slot;
    int reason;
} MemOp;

typedef struct {
    int local_count;
    Rename* renames;
    NL_Table phi2local;
} LocalSplitter;

// used by phi2local whenever a memory phi is marked as split (it's not bound to any
// new name but shouldn't count as NULL)
static Rename RENAME_DUMMY = { 0 };

static int bits_in_data_type(int pointer_size, TB_DataType dt);

static bool good_mem_op(TB_Passes* p, TB_Node* n) { // ld, st, memcpy, memset
    if (n->type == TB_LOAD) {
        return true;
    } else if (n->type >= TB_STORE && n->type <= TB_MEMSET) {
        Lattice* l = lattice_universe_get(p, n);
        return l == &TOP_IN_THE_SKY || l == p->root_mem;
    } else {
        return false;
    }
}

static bool same_base(TB_Node* a, TB_Node* b) {
    while (b->type == TB_MEMBER_ACCESS || b->type == TB_ARRAY_ACCESS) {
        if (a == b) return true;
        b = b->inputs[1];
    }

    return a == b;
}

static TB_Node* next_mem_user(TB_Node* n) {
    FOR_USERS(u, n) {
        if (is_mem_out_op(u->n)) {
            return u->n;
        }
    }

    return NULL;
}

static int categorize_alias_idx(LocalSplitter* restrict ctx, TB_Node* n) {
    // skip any member or array accesses
    while (n->type == TB_ARRAY_ACCESS || n->type == TB_MEMBER_ACCESS) {
        n = n->inputs[1];
    }

    FOREACH_N(i, 0, ctx->local_count) {
        if (ctx->renames[i].addr == n) return i;
    }

    return -1;
}

enum {
    MEM_FORK, MEM_JOIN, MEM_END, MEM_USE
};

static bool all_stores_dead(TB_Node* n) {
    FOR_USERS(u, n) {
        if (u->slot != 2 || u->n->type != TB_STORE) {
            return false;
        }
    }

    return true;
}

static void expunge(TB_Function* f, TB_Node* n) {
    // delete users without weird iteration invalidation issues
    while (n->users) {
        TB_Node* use_n = n->users->n;
        int use_i      = n->users->slot;

        assert(use_i == 2);
        n->users = n->users->next;
        use_n->inputs[2] = NULL;

        FOR_USERS(u, use_n) {
            // kill connected phis
            if (u->n->type == TB_PHI) {
                set_input(f, u->n, NULL, 0);
            }
        }

        tb_pass_kill_node(f, use_n);
    }
    tb_pass_kill_node(f, n);
}

static TB_Node* node_or_poison(TB_Function* f, TB_Node* n, TB_DataType dt) {
    return n ? n : make_poison(f, dt);
}

static void fixup_mem_node(TB_Function* f, TB_Passes* restrict p, LocalSplitter* restrict ctx, TB_Node* curr, TB_Node** latest) {
    int user_cnt = 0;
    MemOp* users = NULL;
    TB_ArenaSavepoint sp = tb_arena_save(tmp_arena);

    // walk past each memory effect and categorize it
    while (curr) {
        // printf("WALK v%u\n", curr->gvn);

        int user_cap = 0;
        FOR_USERS(u, curr) { user_cap++; }

        sp = tb_arena_save(tmp_arena);
        user_cnt = 0;
        users = tb_arena_alloc(tmp_arena, user_cap * sizeof(MemOp));
        FOR_USERS(u, curr) {
            TB_Node* use_n = u->n;
            int use_i = u->slot;

            // we can either be followed by:
            // * merge: we're done now, stitch in the latest ops
            //
            // * phi: whoever makes it here first follows it through and generates the
            //        parallel memory phis, everyone else just fills in the existing nodes.
            //
            // * normie memory op: just advances (maybe forking)
            int reason = -1;

            if (0) {}
            else if (use_n->type == TB_RETURN)   { reason = MEM_END;  }
            else if (use_n->type == TB_PHI)      { reason = MEM_JOIN; }
            else if (is_mem_only_in_op(u->n))    { reason = MEM_USE;  }
            else if (cfg_is_mproj(use_n) || (use_i == 1 && is_mem_out_op(use_n))) {
                reason = MEM_FORK;
            }

            if (reason >= 0) {
                users[user_cnt].n    = u->n;
                users[user_cnt].slot = u->slot;
                users[user_cnt].reason = reason;
                user_cnt += 1;
            }
        }

        // skip past projections
        TB_Node* st_val = NULL;
        if (curr->type >= TB_STORE && curr->type <= TB_MEMSET) {
            int cat = categorize_alias_idx(ctx, curr->inputs[2]);
            if (cat < 0) {
                set_input(f, curr, latest[0], 1);
                latest[0] = curr;
            } else {
                int alias_idx = ctx->renames[cat].alias_idx;
                if (alias_idx >= 0) {
                    // rewrite memory edge
                    set_input(f, curr, latest[1 + cat], 1);
                    latest[1 + cat] = curr;

                    // invalidate old memory type
                    tb_pass_mark(p, curr);
                    lattice_universe_map(p, curr, NULL);
                } else {
                    // get rid of the store, we don't need it
                    latest[1 + cat] = curr->inputs[3];

                    // printf("* KILL %%%u (latest[%d] = %%%u)\n", curr->gvn, 1 + cat, curr->inputs[3]->gvn);
                    tb_pass_kill_node(f, curr);
                }
            }
        } else if (curr->dt.type == TB_TUPLE) {
            // skip to mproj
            assert(curr->type != TB_SPLITMEM);
            curr = next_mem_user(curr);
        }

        // fixup any connected loads
        FOREACH_N(i, 0, user_cnt) {
            TB_Node* use_n = users[i].n;
            int use_i = users[i].slot;
            int reason = users[i].reason;

            if (reason == MEM_USE) {
                int cat = categorize_alias_idx(ctx, use_n->inputs[2]);
                if (cat < 0) {
                    set_input(f, use_n, latest[0], 1);
                } else {
                    int alias_idx = ctx->renames[cat].alias_idx;
                    if (alias_idx >= 0) {
                        // rewrite edge
                        set_input(f, use_n, latest[1 + cat], 1);
                        tb_pass_mark(p, use_n);
                    } else {
                        assert(use_n->type == TB_LOAD);

                        TB_Node* val = node_or_poison(f, latest[1 + cat], use_n->dt);
                        subsume_node(f, use_n, val);
                    }
                }
            }
        }

        // "tail" such that we don't make another stack frame and more
        // importantly another "latest" array
        if (user_cnt == 1 && users[0].reason == MEM_FORK) {
            curr = users[0].n;
            tb_arena_restore(tmp_arena, sp);
        } else {
            break;
        }
    }

    FOREACH_N(i, 0, user_cnt) {
        TB_Node* use_n = users[i].n;
        int use_i = users[i].slot;
        int reason = users[i].reason;

        switch (reason) {
            case MEM_FORK: {
                TB_ArenaSavepoint sp = tb_arena_save(tmp_arena);
                TB_Node** new_latest = tb_arena_alloc(tmp_arena, (1 + ctx->local_count) * sizeof(TB_Node*));
                FOREACH_N(i, 0, 1 + ctx->local_count) {
                    new_latest[i] = latest[i];
                }

                fixup_mem_node(f, p, ctx, use_n, new_latest);
                tb_arena_restore(tmp_arena, sp);
                break;
            }

            case MEM_JOIN: {
                // stitch latest state to phis
                TB_Node* region = use_n->inputs[0];

                Rename* v = nl_table_get(&ctx->phi2local, use_n);
                if (v == NULL) {
                    nl_table_put(&ctx->phi2local, use_n, &RENAME_DUMMY);

                    // convert single phi into parallel phis (use_n will become the leftovers mem)
                    TB_Node** new_latest = tb_arena_alloc(tmp_arena, (1 + ctx->local_count) * sizeof(TB_Node*));

                    // convert single phi into multiple parallel phis (first one will be replaced
                    // with the root mem)
                    set_input(f, use_n, latest[0], use_i);
                    lattice_universe_map(p, use_n, NULL);

                    assert(use_n->dt.type == TB_MEMORY);
                    new_latest[0] = use_n;

                    // make extra alias phis
                    FOREACH_N(i, 0, ctx->local_count) {
                        // let's hope the first datatype we get from the phi is decent, if not the
                        // peepholes will ideally fix it.
                        TB_Node* val = latest[1 + i];
                        TB_DataType dt = TB_TYPE_MEMORY;

                        if (val != NULL) {
                            if (ctx->renames[i].alias_idx < 0) {
                                dt = val->dt;
                            }

                            TB_Node* new_phi = tb_alloc_node(f, TB_PHI, dt, use_n->input_count, 0);
                            set_input(f, new_phi, region, 0);
                            set_input(f, new_phi, val, use_i);
                            new_latest[1 + i] = new_phi;
                            tb_pass_mark(p, new_phi);

                            nl_table_put(&ctx->phi2local, new_phi, &ctx->renames[i]);
                            lattice_universe_map(p, new_phi, NULL);
                        } else {
                            new_latest[1 + i] = NULL;
                        }
                    }

                    // first entry, every other time we'll just be stitching phis
                    tb_pass_mark(p, use_n);
                    fixup_mem_node(f, p, ctx, use_n, new_latest);
                } else if (v == &RENAME_DUMMY) {
                    FOR_USERS(phi, region) if (phi->n->type == TB_PHI) {
                        Rename* name = nl_table_get(&ctx->phi2local, phi->n);
                        if (name == &RENAME_DUMMY) {
                            set_input(f, phi->n, latest[0], use_i);
                        } else if (name) {
                            if (name->alias_idx < 0) {
                                TB_Node* val = latest[1 + (name - ctx->renames)];
                                if (val->dt.raw != phi->n->dt.raw) {
                                    // insert bitcast
                                    TB_Node* cast = tb_alloc_node(f, TB_BITCAST, phi->n->dt, 2, 0);
                                    set_input(f, cast, val, 1);
                                    val = cast;
                                }

                                set_input(f, phi->n, val, use_i);
                            } else {
                                set_input(f, phi->n, latest[1 + (name - ctx->renames)], use_i);
                            }
                        }
                    }
                }
                break;
            }

            case MEM_END: {
                // stitch the latest nodes to the merge or return
                assert(use_n->type == TB_RETURN);
                set_input(f, use_n, latest[0], 1);

                /* size_t j = 0;
                FOREACH_N(i, 0, ctx->local_count) if (ctx->renames[i].alias_idx > 0) {
                    assert(latest[i] != NULL && "TODO we should place a poison?");
                    set_input(f, use_n, latest[i], 2+j);
                    j += 1;
                } */
                break;
            }
        }
    }

    tb_arena_restore(tmp_arena, sp);
}

void tb_pass_locals(TB_Passes* p) {
    TB_Function* f = p->f;

    do {
        cuikperf_region_start("locals", NULL);
        assert(dyn_array_length(p->worklist.items) == 0);

        // find all locals
        LocalSplitter ctx = { 0 };
        NL_ChunkedArr locals = nl_chunked_arr_alloc(tmp_arena);
        FOR_USERS(u, f->root_node) {
            if (u->n->type != TB_LOCAL) continue;
            nl_chunked_arr_put(&locals, u->n);
            ctx.local_count++;
        }

        // find reasons for renaming
        ctx.renames = tb_arena_alloc(tmp_arena, ctx.local_count * sizeof(Rename));

        size_t j = 0;
        bool needs_to_rewrite = false;
        for (NL_ArrChunk* restrict chk = locals.first; chk; chk = chk->next) {
            FOREACH_N(i, 0, chk->count) {
                TB_Node* addr = chk->elems[i];
                RenameMode mode = RENAME_VALUE;

                FOR_USERS(mem, addr) {
                    if (mem->slot == 1 && (mem->n->type == TB_MEMBER_ACCESS || mem->n->type == TB_ARRAY_ACCESS)) {
                        // TODO(NeGate): pointer arith are also fair game, since they'd stay in bounds (given no UB)
                        // mode = RENAME_MEMORY;
                        mode = RENAME_NONE;
                        break;
                    } else if (mem->slot != 2 || !good_mem_op(p, mem->n)) {
                        mode = RENAME_NONE;
                        break;
                    }
                }

                if (mode != RENAME_NONE) {
                    // allocate new alias index
                    if (mode == RENAME_MEMORY) {
                        ctx.renames[j].alias_idx = p->alias_n++;
                        needs_to_rewrite = true;
                    } else if (mode == RENAME_VALUE) {
                        ctx.renames[j].alias_idx = -1;
                        needs_to_rewrite = true;
                    }

                    ctx.renames[j].addr = addr;
                    j += 1;
                }
            }
        }

        if (!needs_to_rewrite) {
            nl_chunked_arr_reset(&locals);
            cuikperf_region_end();
            break;
        }

        ctx.local_count = j;
        ctx.phi2local = nl_table_alloc(200);

        // let's rewrite values & memory
        TB_Node* first_mem = next_mem_user(f->params[1]);
        if (first_mem) {
            TB_Node** latest = tb_arena_alloc(tmp_arena, (1 + ctx.local_count) * sizeof(TB_Node*));
            FOREACH_N(i, 1, 1 + ctx.local_count) { latest[i] = NULL; }
            latest[0] = f->params[1];

            fixup_mem_node(f, p, &ctx, first_mem, latest);
        }

        // ok if they're now value edges we can delete the LOCAL
        FOREACH_N(i, 0, ctx.local_count) if (ctx.renames[i].alias_idx < 0) {
            tb_pass_kill_node(f, ctx.renames[i].addr);
        }

        nl_table_free(ctx.phi2local);
        nl_chunked_arr_reset(&locals);
        cuikperf_region_end();

        // run a round of peepholes, lots of new room to explore :)
        tb_pass_peephole(p);
    } while (true);
}
