typedef enum {
    COHERENCY_DEAD,
    COHERENCY_GOOD,

    // failure states
    COHERENCY_USES_ADDRESS,
    COHERENCY_BAD_DATA_TYPE,
    COHERENCY_UNINITIALIZED,
    COHERENCY_VOLATILE
} Coherency;

// Region -> Value
typedef NL_Map(TB_Node*, TB_Node*) Mem2Reg_Def;

typedef struct Mem2Reg_Ctx {
    TB_Function* f;
    TB_Passes* p;
    TB_Node** blocks;

    // Stack slots we're going to convert into
    // SSA form
    size_t to_promote_count;
    TB_Node** to_promote;

    // [to_promote_count]
    Mem2Reg_Def* defs;
} Mem2Reg_Ctx;

static int bits_in_data_type(int pointer_size, TB_DataType dt);
static Coherency tb_get_stack_slot_coherency(TB_Passes* p, TB_Function* f, TB_Node* address, TB_DataType* dt);

static int get_variable_id(Mem2Reg_Ctx* restrict c, TB_Node* r) {
    // TODO(NeGate): Maybe we speed this up... maybe it doesn't matter :P
    FOREACH_N(i, 0, c->to_promote_count) {
        if (c->to_promote[i] == r) return i;
    }

    return -1;
}

// This doesn't really generate a PHI node, it just produces a NULL node which will
// be mutated into a PHI node by the rest of the code.
static TB_Node* new_phi(Mem2Reg_Ctx* restrict c, TB_Function* f, int var, TB_Node* block, TB_DataType dt) {
    TB_Node* n = tb_alloc_node(f, TB_PHI, dt, 1 + block->input_count, 0);
    set_input(f, n, block, 0);

    // append variable attrib
    /*for (TB_Attrib* a = c->to_promote[var]->first_attrib; a; a = a->next) if (a->type == TB_ATTRIB_VARIABLE) {
        tb_node_append_attrib(n, a);
        break;
    }*/

    DO_IF(TB_OPTDEBUG_MEM2REG)(log_debug("v%u: insert new PHI node (in v%u)", n->gvn, block->gvn));
    tb_pass_mark(c->p, n);
    return n;
}

static void add_phi_operand(Mem2Reg_Ctx* restrict c, TB_Function* f, TB_Node* phi_node, TB_Node* bb, TB_Node* node) {
    // we're using NULL nodes as the baseline PHI0
    if (phi_node == node) {
        return;
    }

    phi_node->dt = node->dt;
    if (phi_node->type == TB_POISON) {
        return;
    }

    assert(phi_node->type == TB_PHI);
    TB_Node* phi_region = phi_node->inputs[0];
    DO_IF(TB_OPTDEBUG_MEM2REG)(printf("v%u: adding v%u to PHI\n", phi_node->gvn, node->gvn));

    // the slot to fill is based on the predecessor list of the region
    FOREACH_N(i, 0, phi_region->input_count) {
        TB_Node* pred = get_pred_cfg(&c->p->cfg, phi_region, i);
        if (pred == bb) {
            set_input(f, phi_node, node, i+1);
            break;
        }
    }

    tb_pass_mark_users(c->p, phi_node);
}

static void write_variable(Mem2Reg_Ctx* c, int var, TB_Node* block, TB_Node* value) {
    if (c->defs[var] == NULL) {
        nl_map_create(c->defs[var], 16);
    }

    nl_map_put(c->defs[var], block, value);
}

static void ssa_replace_phi_arg(Mem2Reg_Ctx* c, TB_Function* f, TB_Node* bb, TB_Node* dst, DynArray(TB_Node*)* stack) {
    FOREACH_N(var, 0, c->to_promote_count) {
        ptrdiff_t search = nl_map_get(c->defs[var], dst);
        if (search < 0) continue;

        TB_Node* phi_reg = c->defs[var][search].v;
        if (phi_reg->type != TB_PHI) continue;

        TB_Node* top;
        if (dyn_array_length(stack[var]) == 0) {
            // this is UB land, insert poison
            top = make_poison(f, c->p, phi_reg->dt);
            log_warn("%s: v%u: generated poison due to read of uninitialized local", f->super.name, top->gvn);
        } else {
            top = stack[var][dyn_array_length(stack[var]) - 1];
        }

        DO_IF(TB_OPTDEBUG_MEM2REG)(printf("v%u: replace v%u to PHI\n", phi_reg->gvn, top->gvn));

        bool found = false;
        FOREACH_N(j, 0, dst->input_count) {
            TB_Node* pred = get_pred_cfg(&c->p->cfg, dst, j);
            if (pred == bb) {
                // try to replace
                set_input(f, phi_reg, top, j + 1);
                found = true;
                break;
            }
        }

        if (!found) {
            add_phi_operand(c, f, phi_reg, bb, top);
        }
    }
}

static void ssa_rename(Mem2Reg_Ctx* c, TB_Function* f, TB_Node* bb, DynArray(TB_Node*)* stack) {
    assert(bb);
    TB_Passes* p = c->p;

    // push phi nodes
    TB_ArenaSavepoint sp = tb_arena_save(tmp_arena);
    size_t* old_len = tb_arena_alloc(tmp_arena, sizeof(size_t) * c->to_promote_count);
    FOREACH_N(var, 0, c->to_promote_count) {
        old_len[var] = dyn_array_length(stack[var]);

        ptrdiff_t search = nl_map_get(c->defs[var], bb);
        if (search >= 0 && c->defs[var][search].v->type == TB_PHI) {
            dyn_array_put(stack[var], c->defs[var][search].v);
        }
    }

    // rewrite operations
    TB_BasicBlock* bb_info = &nl_map_get_checked(c->p->cfg.node_to_block, bb);
    TB_Node* end = bb_info->end;

    DO_IF(TB_OPTDEBUG_MEM2REG)(
        printf("  FORST %u: ", bb->gvn),
        print_node_sexpr(bb, 0),
        printf("\n")
    );

    // go through all uses and replace their accessors
    TB_Node* n = bb_info->mem_in;
    if (n != NULL) {
        do {
            DO_IF(TB_OPTDEBUG_MEM2REG)(
                printf("  SIGMA %u: ", n->gvn),
                print_node_sexpr(n, 0),
                printf("\n")
            );

            // if we spot a store, we push to the stack
            bool kill = false;
            if (n->type == TB_STORE) {
                int var = get_variable_id(c, n->inputs[2]);
                if (var >= 0) {
                    DO_IF(TB_OPTDEBUG_MEM2REG)(printf("    ASSIGN %d -> %u\n", var, n->inputs[3]->gvn));

                    // push new store value onto the stack
                    dyn_array_put(stack[var], n->inputs[3]);
                    kill = true;
                }
            }

            // check for any loads and replace them
            FOR_USERS(u, n) {
                TB_Node* use = u->n;

                if (u->slot == 1 && use->type == TB_LOAD) {
                    int var = get_variable_id(c, use->inputs[2]);
                    if (var >= 0) {
                        TB_Node* val;
                        if (dyn_array_length(stack[var]) == 0) {
                            // this is UB since it implies we've read before initializing the
                            // stack slot.
                            val = make_poison(f, p, use->dt);
                            log_warn("v%u: found load-before-init in mem2reg, this is UB", use->gvn);
                        } else {
                            val = stack[var][dyn_array_length(stack[var]) - 1];
                        }

                        // make sure it's the right type
                        if (use->dt.raw != val->dt.raw) {
                            TB_Node* cast = tb_alloc_node(c->f, TB_BITCAST, use->dt, 2, 0);
                            tb_pass_mark(p, cast);
                            set_input(f, cast, val, 1);

                            val = cast;
                        }

                        set_input(f, use, NULL, 1); // unlink first
                        subsume_node(p, f, use, val);

                        tb_pass_mark(p, val);
                        tb_pass_mark_users(p, val);
                    }
                }
            }

            // next memory has to be decided before we kill the node since
            // murder will dettach the users.
            TB_Node* next = mem_user(p, n, 1);

            // we can remove the effect now
            if (kill) {
                TB_Node* into = n->inputs[1];
                subsume_node(p, c->f, n, into);

                tb_pass_mark(c->p, into);
                tb_pass_mark_users(p, into);
            }

            n = next;
        } while (n != NULL && n->type != TB_PHI && cfg_underneath(&c->p->cfg, n, bb_info));
    }

    // replace phi arguments on successor
    if (end->type == TB_BRANCH) {
        // fill successors
        FOR_USERS(u, end) {
            if (!cfg_is_control(u->n)) continue;

            TB_Node* succ = cfg_next_bb_after_cproj(u->n);
            if (succ->type != TB_REGION) continue;

            // for p in succ's phis:
            //   if p is a var v, replace edge with stack[v]
            ssa_replace_phi_arg(c, f, bb, succ, stack);
        }
    } else if (end->type != TB_ROOT && end->type != TB_UNREACHABLE) {
        // fallthrough case
        ssa_replace_phi_arg(c, f, bb, cfg_next_control(end), stack);
    }

    // for each successor s of the BB in the dominator
    //    rename(s)
    //
    // TODO(NeGate): maybe we want a data structure for this because it'll
    // be "kinda" slow.
    FOREACH_N(i, 0, c->p->cfg.block_count) {
        TB_Node* k = c->blocks[i];
        TB_Node* v = idom(&c->p->cfg, k);

        if (v == bb && k != bb) {
            ssa_rename(c, f, k, stack);
        }
    }

    FOREACH_N(var, 0, c->to_promote_count) {
        dyn_array_set_length(stack[var], old_len[var]);
    }
    tb_arena_restore(tmp_arena, sp);
}

static void insert_phis(Mem2Reg_Ctx* restrict ctx, TB_Node* bb, TB_Node* n, TB_BasicBlock* bb_info) {
    DO_IF(TB_OPTDEBUG_MEM2REG)(
        printf("  FORST %u: ", bb->gvn),
        print_node_sexpr(bb, 0),
        printf("\n")
    );

    do {
        DO_IF(TB_OPTDEBUG_MEM2REG)(
            printf("  OMEGA %u: ", n->gvn),
            print_node_sexpr(n, 0),
            printf("\n")
        );

        if (n->type == TB_STORE) {
            int var = get_variable_id(ctx, n->inputs[2]);
            if (var >= 0) {
                write_variable(ctx, var, bb, n->inputs[3]);
            }
        }

        // next memory
        n = mem_user(ctx->p, n, 1);
    } while (n != NULL && n->type != TB_PHI && cfg_underneath(&ctx->p->cfg, n, bb_info));
}

void tb_pass_mem2reg(TB_Passes* p) {
    cuikperf_region_start("mem2reg", NULL);

    TB_Function* f = p->f;

    ////////////////////////////////
    // Decide which stack slots to promote
    ////////////////////////////////
    FOR_USERS(u, f->root_node) {
        if (u->n->type != TB_LOCAL) continue;
        TB_Node* n = u->n;

        TB_DataType dt;
        Coherency coherence = tb_get_stack_slot_coherency(p, f, n, &dt);

        switch (coherence) {
            case COHERENCY_GOOD: {
                dyn_array_put(p->worklist.items, n);
                n->dt = dt;

                DO_IF(TB_OPTDEBUG_MEM2REG)(log_debug("%s: v%u promoting to IR register", f->super.name, n->gvn));
                break;
            }
            case COHERENCY_UNINITIALIZED: {
                DO_IF(TB_OPTDEBUG_MEM2REG)(log_debug("%s: v%u could not mem2reg (uninitialized)", f->super.name, n->gvn));
                break;
            }
            case COHERENCY_VOLATILE: {
                DO_IF(TB_OPTDEBUG_MEM2REG)(log_debug("%s: v%u could not mem2reg (volatile load/store)", f->super.name, n->gvn));
                break;
            }
            case COHERENCY_USES_ADDRESS: {
                DO_IF(TB_OPTDEBUG_MEM2REG)(log_debug("%s: v%u could not mem2reg (uses address directly)", f->super.name, n->gvn));
                break;
            }
            case COHERENCY_BAD_DATA_TYPE: {
                DO_IF(TB_OPTDEBUG_MEM2REG)(log_debug("%s: v%u could not mem2reg (data type is too inconsistent)", f->super.name, n->gvn));
                break;
            }
            case COHERENCY_DEAD: {
                DO_IF(TB_OPTDEBUG_MEM2REG)(log_debug("%s: v%u could not mem2reg (dead)", f->super.name, n->gvn));
                break;
            }
            default: tb_todo();
        }
    }

    size_t to_promote_count = dyn_array_length(p->worklist.items);
    if (to_promote_count == 0) {
        // doesn't need to mem2reg
        cuikperf_region_end();
        return;
    }

    TB_ArenaSavepoint sp = tb_arena_save(tmp_arena);

    Mem2Reg_Ctx c = {
        .f = f,
        .p = p,
        .to_promote_count = to_promote_count,
        .to_promote = tb_arena_alloc(tmp_arena, to_promote_count * sizeof(TB_Node*)),
    };

    memcpy(c.to_promote, p->worklist.items, to_promote_count * sizeof(TB_Node*));
    dyn_array_clear(p->worklist.items);

    c.defs = tb_arena_alloc(tmp_arena, to_promote_count * sizeof(Mem2Reg_Def));
    memset(c.defs, 0, to_promote_count * sizeof(Mem2Reg_Def));

    tb_pass_update_cfg(p, &p->worklist, true);
    c.blocks = &p->worklist.items[0];

    worklist_clear_visited(&p->worklist);
    TB_DominanceFrontiers* df = tb_get_dominance_frontiers(f, p, c.p->cfg, c.blocks);

    ////////////////////////////////
    // Phase 1: Insert phi functions
    ////////////////////////////////
    // Identify the final value of all the variables in the function per basic block
    FOREACH_N(i, 0, c.p->cfg.block_count) {
        TB_Node* bb = c.blocks[i];
        TB_BasicBlock* bb_info = &nl_map_get_checked(c.p->cfg.node_to_block, bb);

        // mark into worklist
        worklist_test_n_set(&p->worklist, bb);
        tb_pass_mark_users(p, bb);

        if (i == 0) {
            // start block can use the input memory as the earliest point
            insert_phis(&c, bb, f->params[1], bb_info);
            bb_info->mem_in = f->params[1];
            continue;
        }

        TB_Node* end = bb_info->end;

        // find memory phi
        TB_Node* n = bb;
        TB_Node* mem = NULL;
        while (n != NULL) {
            FOR_USERS(u, n) {
                if (is_mem_out_op(u->n)) {
                    mem = u->n;
                    goto done;
                }
            }

            if (n == end) break;
            n = cfg_next_control(n);
        }

        done:
        // find earliest memory in the BB:
        //   note this doesn't account for multiple memory streams
        //   but that's fine for now...
        if (mem && !(mem->type == TB_PROJ && mem->inputs[0]->type == TB_ROOT)) {
            while (mem->type != TB_PHI && cfg_underneath(&c.p->cfg, mem->inputs[1], bb_info)) {
                mem = mem->inputs[1];
            }

            insert_phis(&c, bb, mem, bb_info);
        }

        bb_info->mem_in = mem;
    }

    // for each global name we'll insert phi nodes
    TB_Node** phi_p = tb_arena_alloc(tmp_arena, c.p->cfg.block_count * sizeof(TB_Node*));

    NL_HashSet ever_worked = nl_hashset_alloc(c.p->cfg.block_count);
    NL_HashSet has_already = nl_hashset_alloc(c.p->cfg.block_count);
    FOREACH_N(var, 0, c.to_promote_count) {
        nl_hashset_clear(&ever_worked);
        nl_hashset_clear(&has_already);

        size_t p_count = 0;
        FOREACH_N(i, 0, c.p->cfg.block_count) {
            TB_Node* bb = c.blocks[i];

            ptrdiff_t search = nl_map_get(c.defs[var], bb);
            if (search >= 0) {
                nl_hashset_put(&ever_worked, bb);
                phi_p[p_count++] = bb;
            }
        }

        // it's a global name
        if (p_count > 1) {
            // insert phi per dominance of the blocks it's defined in
            for (size_t i = 0; i < p_count; i++) {
                TB_Node* bb = phi_p[i];
                TB_Node* value = nl_map_get_checked(c.defs[var], bb);
                TB_DataType dt = value->dt;

                // for all DFs of BB, insert PHI
                int bb_id = nl_map_get_checked(c.p->cfg.node_to_block, bb).id;
                uint64_t* frontier = &df->arr[bb_id * df->stride];
                FOREACH_N(j, 0, df->stride) FOREACH_BIT(k, j*64, frontier[j]) {
                    TB_Node* l = c.blocks[k];
                    if (!nl_hashset_put(&has_already, l)) continue;

                    ptrdiff_t search = nl_map_get(c.defs[var], l);

                    TB_Node* phi_reg = NULL;
                    if (search < 0) {
                        phi_reg = new_phi(&c, f, var, l, dt);
                        nl_map_put(c.defs[var], l, phi_reg);
                    } else {
                        phi_reg = c.defs[var][search].v;

                        if (phi_reg->type != TB_PHI) {
                            TB_Node* old_reg = phi_reg;
                            phi_reg = new_phi(&c, f, var, l, dt);
                            add_phi_operand(&c, f, phi_reg, l, old_reg);

                            nl_map_put(c.defs[var], l, phi_reg);
                        }
                    }

                    add_phi_operand(&c, f, phi_reg, bb, value);

                    if (nl_hashset_put(&ever_worked, l)) {
                        phi_p[p_count++] = l;
                    }
                }
            }
        }
    }
    tb_platform_heap_free(df);

    ////////////////////////////////
    // Phase 2: Rename loads and stores
    ////////////////////////////////
    DynArray(TB_Node*)* stack = tb_arena_alloc(tmp_arena, c.to_promote_count * sizeof(DynArray(TB_Node*)));
    FOREACH_N(var, 0, c.to_promote_count) {
        stack[var] = dyn_array_create(TB_Node*, 16);
    }

    ssa_rename(&c, f, c.blocks[0], stack);

    // don't need these anymore
    FOREACH_N(var, 0, c.to_promote_count) {
        assert(c.to_promote[var]->users == NULL);
        tb_pass_kill_node(c.p, c.to_promote[var]);
    }
    tb_arena_restore(tmp_arena, sp);
    tb_free_cfg(&p->cfg);
    cuikperf_region_end();
}

// NOTE(NeGate): a stack slot is coherent when all loads and stores share
// the same type and alignment along with not needing any address usage.
static Coherency tb_get_stack_slot_coherency(TB_Passes* p, TB_Function* f, TB_Node* address, TB_DataType* out_dt) {
    User* use = address->users;
    if (use == NULL) {
        return COHERENCY_DEAD;
    }

    ICodeGen* cg = f->super.module->codegen;
    int pointer_size = cg->pointer_size;
    int char_size = cg->minimum_addressable_size;

    // pick the first load/store and use that as the baseline
    TB_DataType dt = TB_TYPE_VOID;
    bool initialized = false;
    int dt_bits = 0;

    for (; use; use = use->next) {
        TB_Node* n = use->n;
        if (n->type == TB_READ || n->type == TB_WRITE) {
            return COHERENCY_VOLATILE;
        } else if ((n->type == TB_LOAD || n->type == TB_STORE) && n->inputs[2] == address) {
            TB_DataType mem_dt = n->type == TB_LOAD ? n->dt : n->inputs[3]->dt;
            if (!initialized) {
                dt = mem_dt;
                initialized = true;
            } else {
                // we're hoping all data types match in size to continue along
                int bits = bits_in_data_type(pointer_size, mem_dt);
                if (bits == 0 || (dt_bits > 0 && bits != dt_bits)) {
                    return COHERENCY_BAD_DATA_TYPE;
                }
                dt_bits = bits;
            }
        } else {
            DO_IF(TB_OPTDEBUG_MEM2REG)(log_debug("v%u uses pointer arithmatic (%s)", address->gvn, tb_node_get_name(n)));
            return COHERENCY_USES_ADDRESS;
        }
    }

    if (!initialized) {
        return COHERENCY_UNINITIALIZED;
    }

    *out_dt = dt;
    return COHERENCY_GOOD;
}
