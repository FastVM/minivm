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

typedef struct Promotion Promotion;
struct Promotion {
    Promotion* next;
    TB_Node* n;
};

typedef struct Mem2Reg_Ctx {
    TB_TemporaryStorage* tls;
    TB_Function* f;
    TB_Passes* p;

    TB_CFG cfg;
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
        if (c->to_promote[i] == r) return (int)i;
    }

    return -1;
}

// This doesn't really generate a PHI node, it just produces a NULL node which will
// be mutated into a PHI node by the rest of the code.
static TB_Node* new_phi(Mem2Reg_Ctx* restrict c, TB_Function* f, int var, TB_Node* block, TB_DataType dt) {
    TB_Node* n = tb_alloc_node(f, TB_PHI, dt, 1 + block->input_count, 0);
    FOREACH_N(i, 0, 1 + block->input_count) n->inputs[i] = NULL;

    set_input(c->p, n, block, 0);

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
        TB_Node* pred = get_pred(phi_region, i);
        if (pred == bb) {
            set_input(c->p, phi_node, node, i+1);
            break;
        }
    }

    tb_pass_mark_users(c->p, phi_node);
    // tb_unreachable();
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
            log_warn("%s: ir: generated poison due to read of uninitialized local", f->super.name);
            top = make_poison(f, c->p, TB_TYPE_VOID);
        } else {
            top = stack[var][dyn_array_length(stack[var]) - 1];
        }

        bool found = false;
        FOREACH_N(j, 0, dst->input_count) {
            TB_Node* pred = get_pred(dst, j);
            if (pred == bb) {
                // try to replace
                set_input(c->p, phi_reg, top, j + 1);
                found = true;
                break;
            }
        }

        if (!found) {
            add_phi_operand(c, f, phi_reg, bb, top);
        }
    }
}

static bool is_effect_tuple(TB_Node* n) {
    return n->type == TB_CALL ||
        n->type == TB_SYSCALL ||
        n->type == TB_READ    ||
        n->type == TB_WRITE   ||
        n->type == TB_MACHINE_OP;
}

static void ssa_rename(Mem2Reg_Ctx* c, TB_Function* f, TB_Node* bb, DynArray(TB_Node*)* stack) {
    assert(bb);
    TB_Passes* p = c->p;

    // push phi nodes
    size_t* old_len = tb_tls_push(c->tls, sizeof(size_t) * c->to_promote_count);
    FOREACH_N(var, 0, c->to_promote_count) {
        ptrdiff_t search = nl_map_get(c->defs[var], bb);
        if (search >= 0 && c->defs[var][search].v->type == TB_PHI) {
            dyn_array_put(stack[var], c->defs[var][search].v);
        }

        old_len[var] = dyn_array_length(stack[var]);
    }

    // rewrite operations
    TB_BasicBlock* bb_info = &nl_map_get_checked(c->cfg.node_to_block, bb);
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
                    // push new store value onto the stack
                    dyn_array_put(stack[var], n->inputs[3]);
                    kill = true;
                }
            }

            // check for any loads and replace them
            for (User* u = n->users; u; u = u->next) {
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
                            tb_pass_mark(c->p, cast);
                            set_input(c->p, cast, val, 1);

                            val = cast;
                        }

                        tb_pass_mark_users(p, use);
                        set_input(p, use, NULL, 1); // unlink first
                        subsume_node(p, f, use, val);
                    }
                }
            }

            // next memory has to be decided before we kill the node since
            // murder will dettach the users.
            TB_Node* next = mem_user(p, n, 1);

            // we can remove the effect now
            if (kill) {
                TB_Node* into = n->inputs[1];
                tb_pass_mark(c->p, into);
                tb_pass_mark_users(c->p, into);
                set_input(p, n, NULL, 1);
                subsume_node(p, c->f, n, into);
            }

            n = next;
        } while (n != NULL && get_block_begin(n) == bb);
    }

    // replace phi arguments on successor
    if (end != NULL) {
        // fill successors
        for (User* u = end->users; u; u = u->next) {
            if (cfg_is_control(u->n)) {
                TB_Node* succ = cfg_get_fallthru(u->n);
                ssa_replace_phi_arg(c, f, bb, succ, stack);
            }
        }
    }

    // for each successor s of the BB in the dominator
    //    rename(s)
    //
    // TODO(NeGate): maybe we want a data structure for this because it'll
    // be "kinda" slow.
    FOREACH_N(i, 0, c->cfg.block_count) {
        TB_Node* k = c->blocks[i];
        TB_Node* v = idom(&c->cfg, k);

        if (v == bb && k != bb) {
            ssa_rename(c, f, k, stack);
        }
    }

    FOREACH_N(var, 0, c->to_promote_count) {
        dyn_array_set_length(stack[var], old_len[var]);
    }
    tb_tls_restore(c->tls, old_len);
}

static void insert_phis(Mem2Reg_Ctx* restrict ctx, TB_Node* bb, TB_Node* n) {
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
    } while (n != NULL && get_block_begin(n) == bb);
}

bool tb_pass_mem2reg(TB_Passes* p) {
    TB_Function* f = p->f;
    TB_TemporaryStorage* tls = tb_tls_steal();

    ////////////////////////////////
    // Decide which stack slots to promote
    ////////////////////////////////
    size_t to_promote_count = 0;
    TB_Node** to_promote = tb_tls_push(tls, sizeof(TB_Node*) * dyn_array_length(p->locals));
    dyn_array_for(i, p->locals) {
        TB_Node* n = p->locals[i];

        TB_DataType dt;
        Coherency coherence = tb_get_stack_slot_coherency(p, f, n, &dt);

        switch (coherence) {
            case COHERENCY_GOOD: {
                to_promote[to_promote_count++] = n;
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
            default: tb_todo();
        }
    }

    if (to_promote_count == 0) {
        // doesn't need to mem2reg
        goto no_changes;
    }

    Mem2Reg_Ctx c = { 0 };
    c.tls = tls;
    c.f = f;
    c.p = p;

    c.to_promote_count = to_promote_count;
    c.to_promote = to_promote;

    c.defs = tb_tls_push(c.tls, to_promote_count * sizeof(Mem2Reg_Def));
    memset(c.defs, 0, to_promote_count * sizeof(Mem2Reg_Def));

    c.cfg = tb_compute_rpo(f, p);
    c.blocks = &p->worklist.items[0];

    worklist_clear_visited(&p->worklist);
    tb_compute_dominators(f, p, c.cfg);

    TB_DominanceFrontiers* df = tb_get_dominance_frontiers(f, p, c.cfg, c.blocks);

    ////////////////////////////////
    // Phase 1: Insert phi functions
    ////////////////////////////////
    // Identify the final value of all the variables in the function per basic block
    FOREACH_N(i, 0, c.cfg.block_count) {
        TB_Node* bb = c.blocks[i];
        TB_BasicBlock* bb_info = &nl_map_get_checked(c.cfg.node_to_block, bb);

        // mark into worklist
        worklist_test_n_set(&p->worklist, bb);
        tb_pass_mark_users(p, bb);

        if (i == 0) {
            // start block can use the input memory as the earliest point
            insert_phis(&c, bb, f->params[1]);
            bb_info->mem_in = f->params[1];
            continue;
        }

        TB_Node* end = bb_info->end;

        // find memory phi
        TB_Node* n = bb;
        TB_Node* mem = NULL;
        do {
            for (User* u = n->users; u; u = u->next) {
                if (is_mem_out_op(u->n)) {
                    mem = u->n;
                    goto done;
                }
            }

            n = cfg_next_control(n);
        } while (n != NULL && n != end);

        done:
        // find earliest memory in the BB:
        //   note this doesn't account for multiple memory streams
        //   but that's fine for now...
        if (mem) {
            while (mem->inputs[1]->inputs[0]->type != TB_START && get_block_begin(mem->inputs[1]->inputs[0]) == bb) {
                mem = mem->inputs[1];
            }

            insert_phis(&c, bb, mem);
        }

        bb_info->mem_in = mem;
    }

    // for each global name we'll insert phi nodes
    TB_Node** phi_p = tb_tls_push(tls, c.cfg.block_count * sizeof(TB_Node*));

    NL_HashSet ever_worked = nl_hashset_alloc(c.cfg.block_count);
    NL_HashSet has_already = nl_hashset_alloc(c.cfg.block_count);
    FOREACH_N(var, 0, c.to_promote_count) {
        nl_hashset_clear(&ever_worked);
        nl_hashset_clear(&has_already);

        size_t p_count = 0;
        FOREACH_N(i, 0, c.cfg.block_count) {
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
                int bb_id = nl_map_get_checked(c.cfg.node_to_block, bb).id;
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
    tb_tls_restore(tls, phi_p);

    ////////////////////////////////
    // Phase 2: Rename loads and stores
    ////////////////////////////////
    DynArray(TB_Node*)* stack = tb_tls_push(tls, c.to_promote_count * sizeof(DynArray(TB_Node*)));
    FOREACH_N(var, 0, c.to_promote_count) {
        stack[var] = dyn_array_create(TB_Node*, 16);
    }

    ssa_rename(&c, f, c.blocks[0], stack);

    // don't need these anymore
    FOREACH_N(var, 0, c.to_promote_count) {
        assert(c.to_promote[var]->users == NULL);
        tb_pass_kill_node(c.p, c.to_promote[var]);
    }

    tb_tls_restore(tls, to_promote);

    tb_free_cfg(&c.cfg);
    cuikperf_region_end();
    return true;

    no_changes:
    cuikperf_region_end();
    return false;
}

// NOTE(NeGate): a stack slot is coherent when all loads and stores share
// the same type and alignment along with not needing any address usage.
static Coherency tb_get_stack_slot_coherency(TB_Passes* p, TB_Function* f, TB_Node* address, TB_DataType* out_dt) {
    User* use = find_users(p, address);
    if (use == NULL) {
        return COHERENCY_DEAD;
    }

    ICodeGen* cg = tb__find_code_generator(f->super.module);
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
