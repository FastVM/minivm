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

static TB_Node* data_phi_from_memory_phi(TB_Passes* restrict p, TB_Function* f, TB_DataType dt, TB_Node* n, TB_Node* addr, TB_CharUnits* out_align) {
    assert(n->type == TB_PHI);
    assert(n->dt.type == TB_MEMORY && "memory input should be memory");

    TB_Arena* func_arena = p->f->arena;
    TB_ArenaSavepoint sp = tb_arena_save(func_arena);

    size_t path_count = n->input_count - 1;
    TB_Node** paths = tb_arena_alloc(func_arena, path_count * sizeof(TB_Node*));

    // walk each path to find relevant STOREs
    TB_CharUnits align = 0;
    TB_Node** phi_ins = n->inputs;
    FOREACH_N(i, 0, path_count) {
        TB_Node* head = phi_ins[1 + i];
        TB_Node* ctrl = head->inputs[0];
        TB_Node* path = NULL;

        do {
            if (head->type == TB_STORE && addr == head->inputs[2]) {
                TB_CharUnits st_align = TB_NODE_GET_EXTRA_T(head, TB_NodeMemAccess)->align;
                path = head->inputs[3];

                // alignment should be the lowest common alignment
                if (align == 0 || align > st_align) {
                    align = st_align;
                }
                break;
            }

            // previous memory effect
            head = head->inputs[1];
        } while (head->inputs[0] == ctrl);

        if (path == NULL) {
            // we have a path with an unknown value... sadge
            tb_arena_restore(func_arena, sp);
            return NULL;
        }
        paths[i] = path;
    }

    // convert to PHI
    TB_Node* phi = tb_alloc_node(f, TB_PHI, dt, 1 + path_count, 0);
    set_input(p, phi, phi_ins[0], 0);
    FOREACH_N(i, 0, path_count) {
        set_input(p, phi, paths[i], 1+i);
    }

    if (out_align) *out_align = align;
    return phi;
}

static TB_Node* ideal_load(TB_Passes* restrict p, TB_Function* f, TB_Node* n) {
    TB_Node* mem = n->inputs[1];
    TB_Node* addr = n->inputs[2];
    if (n->inputs[0] != NULL) {
        // we've dependent on code which must always be run (START.mem)
        if (n->inputs[0]->type == TB_PROJ && n->inputs[0]->inputs[0]->type == TB_START) {
            set_input(p, n, NULL, 0);
            return n;
        } else {
            TB_Node* base = addr;
            while (base->type == TB_MEMBER_ACCESS || base->type == TB_ARRAY_ACCESS) {
                base = base->inputs[1];
            }

            // loads based on LOCALs don't need control-dependence, it's actually kinda annoying
            if (base->type == TB_LOCAL) {
                set_input(p, n, NULL, 0);
                return n;
            }
        }
    }

    // if LOAD has already been safely accessed we can relax our control dependency
    if (n->inputs[0] != NULL) {
        TB_Node* parent_bb = get_block_begin(n->inputs[0]);
        for (User* u = addr->users; u; u = u->next) {
            TB_Node* use = u->n;
            if (use != n && use->type == TB_LOAD && u->slot == 2) {
                // if the other load has no control deps we don't need any
                // either... if they're the same type (really it just needs
                // to read the same bytes or less)
                if (use->dt.raw == n->dt.raw) {
                    set_input(p, n, NULL, 0);
                    return n;
                }

                // if we're dominated by some previous load then we can inherit
                // it's control dep.
                TB_Node* bb = get_block_begin(use->inputs[0]);
                if (lattice_dommy(&p->universe, bb, parent_bb)) {
                    set_input(p, n, use->inputs[0], 0);
                    return n;
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
        n->dt.raw == mem->inputs[3]->dt.raw && is_same_align(n, mem)) {
        return mem->inputs[3];
    }

    return n;
}

static TB_Node* ideal_store(TB_Passes* restrict p, TB_Function* f, TB_Node* n) {
    TB_Node *mem = n->inputs[1], *addr = n->inputs[2], *val = n->inputs[3];
    TB_DataType dt = val->dt;

    // if a store has only one user in this chain it means it's only job was
    // to facilitate the creation of that user store... if we can detect that
    // user store is itself dead, everything in the middle is too.
    /*while (mem->type == TB_STORE && single_use(p, mem)) {
        if (mem->inputs[2] == addr && mem->inputs[3]->dt.raw == dt.raw && is_same_align(n, mem)) {
            set_input(p, n, mem->inputs[1], 1);
            return n;
        }

        mem = mem->inputs[1];
    }*/

    return NULL;
}

static TB_Node* ideal_end(TB_Passes* restrict p, TB_Function* f, TB_Node* n) {
    // remove dead local store
    if (n->inputs[1]->type == TB_STORE && is_local_ptr(n->inputs[1]->inputs[2])) {
        set_input(p, n, n->inputs[1]->inputs[1], 1);
        return n;
    }

    return NULL;
}

static TB_Node* ideal_memcpy(TB_Passes* restrict p, TB_Function* f, TB_Node* n) {
    return NULL;
}