// Certain aliasing optimizations technically count as peepholes lmao, these can get fancy
// so the sliding window notion starts to break down but there's no global analysis and
// i can make them incremental technically so we'll go wit it.
typedef struct {
    TB_Node* base;
    int64_t offset;
} KnownPointer;

static bool is_local_ptr(TB_Node* n) {
    // skip past ptr arith
    while (n->type == TB_MEMBER_ACCESS || n->type == TB_ARRAY_ACCESS) {
        n = n->inputs[1];
    }

    return n->type == TB_LOCAL;
}

static KnownPointer known_pointer(TB_Node* n) {
    if (n->type == TB_MEMBER_ACCESS) {
        return (KnownPointer){ n->inputs[1], TB_NODE_GET_EXTRA_T(n, TB_NodeMember)->offset };
    } else {
        return (KnownPointer){ n, 0 };
    }
}

typedef struct FoundPhis {
    struct FoundPhis* prev;
    TB_Node* old;
    TB_Node* new;
} FoundPhis;

static TB_Node* data_phi_from_memory_phi(TB_Passes* restrict p, TB_Function* f, TB_Node* n, TB_Node* addr, TB_Node* mem, FoundPhis* prev, int steps) {
    // convert memory phis into data phis
    assert(mem->type == TB_PHI);
    assert(mem->dt.type == TB_MEMORY && "memory input should be memory");

    TB_DataType dt = n->dt;

    TB_ArenaSavepoint sp = tb_arena_save(tmp_arena);
    size_t path_count = mem->input_count - 1;
    TB_Node** paths = tb_arena_alloc(tmp_arena, path_count * sizeof(TB_Node*));

    TB_Node* phi = tb_alloc_node(f, TB_PHI, dt, 1 + path_count, 0);

    bool fail = false;
    FOREACH_N(i, 0, path_count) {
        TB_Node* st = mem->inputs[1+i];
        if (st->type == TB_PHI && steps > 0) {
            TB_Node* k = NULL;

            // if we've already seen this phi before, let's just reference the OG
            for (FoundPhis* p = prev; p; p = p->prev) {
                if (p->old == st) k = p->new;
            }

            if (k == NULL) {
                FoundPhis next = { prev, mem, phi };
                k = data_phi_from_memory_phi(p, f, n, addr, st, &next, steps - 1);
                if (k == NULL) {
                    fail = true;
                    break;
                }
            }

            paths[i] = k;
        } else if (st->type != TB_STORE || st->inputs[2] != addr || st->inputs[3]->dt.raw != dt.raw) {
            fail = true;
            break;
        } else {
            paths[i] = st->inputs[3];
        }
    }

    if (fail) {
        violent_kill(f, phi);
        return NULL;
    }

    set_input(f, phi, mem->inputs[0], 0);
    FOREACH_N(i, 0, path_count) {
        set_input(f, phi, paths[i], 1+i);
    }
    tb_arena_restore(tmp_arena, sp);

    return phi;
}

static TB_Node* ideal_load(TB_Passes* restrict p, TB_Function* f, TB_Node* n) {
    TB_Node* ctrl = n->inputs[0];
    TB_Node* mem = n->inputs[1];
    TB_Node* addr = n->inputs[2];

    if (mem->type == TB_PHI) {
        TB_Node* k = data_phi_from_memory_phi(p, f, n, addr, mem, NULL, 2);
        if (k) return k;
    }

    if (ctrl != NULL) {
        // we've dependent on code which must always be run (ROOT.mem)
        if (n->inputs[0]->type == TB_PROJ && n->inputs[0]->inputs[0]->type == TB_ROOT) {
            set_input(f, n, NULL, 0);
            return n;
        } else {
            TB_Node* base = addr;
            while (base->type == TB_MEMBER_ACCESS || base->type == TB_ARRAY_ACCESS) {
                base = base->inputs[1];
            }

            // loads based on LOCALs don't need control-dependence, it's actually kinda annoying
            if (base->type == TB_LOCAL) {
                set_input(f, n, NULL, 0);
                return n;
            }
        }

        // if all paths are dominated by a load of some address then it's safe
        // to relax ctrl deps.
        ICodeGen* cg = f->super.module->codegen;
        int bits_read = bits_in_data_type(cg->pointer_size, n->dt);

        for (User* u = addr->users; u; u = u->next) {
            // find other users of the address which read the same size (or more)
            TB_NodeTypeEnum type = 0;
            if (u->n != n && u->slot == 2 && u->n->type == TB_LOAD) {
                TB_DataType mem_dt = n->type == TB_LOAD ? n->dt : n->inputs[3]->dt;
                int other_bits_read = bits_in_data_type(cg->pointer_size, mem_dt);
                if (bits_read <= other_bits_read) {
                    TB_Node* other_ctrl = u->n->inputs[0];
                    if (other_ctrl == NULL || (fast_dommy(other_ctrl, ctrl) && other_ctrl != ctrl)) {
                        set_input(f, n, other_ctrl, 0);
                        return n;
                    }
                }
            }
        }
    }

    return NULL;
}

static TB_Node* identity_load(TB_Passes* restrict p, TB_Function* f, TB_Node* n) {
    // god i need a pattern matcher
    //   (load (store X A Y) A) => Y
    TB_Node *mem = n->inputs[1], *addr = n->inputs[2];
    if (mem->type == TB_STORE && mem->inputs[2] == addr &&
        n->dt.raw == mem->inputs[3]->dt.raw) {
        return mem->inputs[3];
    }

    return n;
}

static Lattice* value_split_mem(TB_Passes* restrict p, TB_Node* n) {
    TB_NodeMemSplit* s = TB_NODE_GET_EXTRA(n);

    size_t size = sizeof(Lattice) + s->alias_cnt*sizeof(Lattice*);
    Lattice* l = tb_arena_alloc(tmp_arena, size);
    *l = (Lattice){ LATTICE_TUPLE, ._tuple = { s->alias_cnt } };
    FOREACH_N(i, 0, s->alias_cnt) {
        l->elems[i] = lattice_alias(p, s->alias_idx[i]);
    }

    Lattice* k = nl_hashset_put2(&p->type_interner, l, lattice_hash, lattice_cmp);
    if (k) {
        tb_arena_free(tmp_arena, l, size);
        return k;
    } else {
        return l;
    }
}

static TB_Node* ideal_merge_mem(TB_Passes* restrict p, TB_Function* f, TB_Node* n) {
    bool progress = false;
    TB_Node* split_node = n->inputs[1];
    TB_NodeMemSplit* split = TB_NODE_GET_EXTRA(split_node);

    // remove useless split edges
    size_t i = 2;
    while (i < n->input_count) {
        if (n->inputs[i]->type == TB_PROJ && n->inputs[i]->inputs[0] == split_node && single_use(n->inputs[i])) {
            int j = i - 2;
            progress = true;

            assert(split->alias_cnt > 0);
            assert(split->alias_cnt + 2 == n->input_count);

            split->alias_cnt -= 1;
            split->alias_idx[j] = split->alias_idx[split->alias_cnt];

            User* proj = proj_with_index(split_node, split->alias_cnt);
            assert(proj->n);
            TB_NODE_SET_EXTRA(proj->n, TB_NodeProj, .index = j);

            tb_pass_kill_node(f, n->inputs[i]);

            // simple remove-swap
            set_input(f, n, n->inputs[n->input_count - 1], i);
            set_input(f, n, NULL, n->input_count - 1);
            n->input_count -= 1;

            // we didn't *really* change the memory type, just reordered it (all the live projs
            // are the same so we don't need to push them onto the worklist)
            Lattice* new_split_type = value_split_mem(p, split_node);
            lattice_universe_map(p, split_node, new_split_type);
        } else {
            i += 1;
        }
    }

    return progress ? n : NULL;
}

static TB_Node* ideal_store(TB_Passes* restrict p, TB_Function* f, TB_Node* n) {
    TB_Node *mem = n->inputs[1], *addr = n->inputs[2], *val = n->inputs[3];
    TB_DataType dt = val->dt;

    // if a store has only one user in this chain it means it's only job was
    // to facilitate the creation of that user store... if we can detect that
    // user store is itself dead, everything in the middle is too.
    if (mem->type == TB_STORE && single_use(mem) && mem->inputs[2] == addr && mem->inputs[3]->dt.raw == dt.raw) {
        // choose the bigger alignment (we wanna keep this sort of info)
        TB_NodeMemAccess* a = TB_NODE_GET_EXTRA(mem);
        TB_NodeMemAccess* b = TB_NODE_GET_EXTRA(n);
        if (a->align > b->align) b->align = a->align;

        // make sure to kill the stores to avoid problems
        TB_Node* parent = mem->inputs[1];
        tb_pass_kill_node(f, mem);

        set_input(f, n, parent, 1);
        return n;
    }

    return NULL;
}

static bool is_cool(uint64_t x) { return x == 1 || x == 2 || x == 4 || x == 8; }
static TB_Node* ideal_memset(TB_Passes* restrict p, TB_Function* f, TB_Node* n) {
    // convert small memsets into stores
    uint64_t count, val;
    if (get_int_const(n->inputs[4], &count) && get_int_const(n->inputs[3], &val) && is_cool(count)) {
        // fill rest of the bytes
        FOREACH_N(i, 1, count) {
            val |= (val & 0xFF) << (i*8);
        }

        TB_DataType dt = TB_TYPE_INTN(count*8);
        set_input(f, n, make_int_node(f, p, dt, val), 3);
        set_input(f, n, NULL, 4);
        n->input_count = 4;
        n->type = TB_STORE;
        return n;
    }

    return NULL;
}

static TB_Node* ideal_return(TB_Passes* restrict p, TB_Function* f, TB_Node* n) {
    return NULL;
}

static TB_Node* ideal_memcpy(TB_Passes* restrict p, TB_Function* f, TB_Node* n) {
    // convert small memsets into ld+st pairs
    uint64_t count, val;
    if (get_int_const(n->inputs[4], &count) && is_cool(count)) {
        TB_Node* ctrl = n->inputs[0];
        TB_Node* mem  = n->inputs[1];
        TB_Node* src  = n->inputs[3];

        TB_DataType dt = TB_TYPE_INTN(count*8);
        TB_Node* ld = tb_alloc_node(f, TB_LOAD, dt, 3, sizeof(TB_NodeBinopInt));
        set_input(f, ld, ctrl, 0);
        set_input(f, ld, mem, 1);
        set_input(f, ld, src, 2);
        tb_pass_mark(p, ld);

        // convert to store, remove extra input
        n->type = TB_STORE;
        set_input(f, n, ld, 3);
        set_input(f, n, NULL, 4);
        n->input_count = 4;
        return n;
    }

    return NULL;
}

static Lattice* value_merge_mem(TB_Passes* restrict p, TB_Node* n) {
    return &BOT_IN_THE_SKY;
}

static Lattice* value_mem(TB_Passes* restrict p, TB_Node* n) {
    // just inherit memory from parent
    return lattice_universe_get(p, n->inputs[1]);
}
