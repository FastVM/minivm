
typedef struct {
    TB_DataType dt;
    uint16_t size;
    uint16_t splits;
} LegalType;

typedef struct LegalizeCtx LegalizeCtx;
typedef bool (*LegalizeFn)(LegalizeCtx* ctx, TB_DataType dt, LegalType* out);

struct LegalizeCtx {
    TB_Arch arch;

    TB_Function* f;
    TB_Passes* p;

    Worklist* ws;
    LegalizeFn fn;
};

static bool legalize_64bit_machine(LegalizeCtx* ctx, TB_DataType dt, LegalType* out) {
    // if it's not 1,8,16,32 or 64 bit we need to legalize
    if (dt.type == TB_INT && dt.data != 1 && dt.data != 8 && dt.data != 16 && dt.data != 32 && dt.data != 64) {
        out->dt = TB_TYPE_I64;
        out->size = 8;
        out->splits = (dt.data + 63) / 64;
        return true;
    }

    return false;
}

static bool legalize_32bit_machine(LegalizeCtx* ctx, TB_DataType dt, LegalType* out) {
    // if it's not 1,8,16 or 32 bit we need to legalize
    if (dt.type == TB_INT && dt.data != 1 && dt.data != 8 && dt.data != 16 && dt.data != 32) {
        out->dt = TB_TYPE_I32;
        out->size = 4;
        out->splits = (dt.data + 31) / 32;
        return true;
    }

    return false;
}

static TB_Node* make_member(TB_Function* f, TB_Node* src, int32_t offset) {
    if (offset == 0) { return src; }

    TB_Node* n = tb_alloc_node(f, TB_MEMBER_ACCESS, TB_TYPE_PTR, 2, sizeof(TB_NodeMember));
    set_input(f, n, src, 1);
    TB_NODE_SET_EXTRA(n, TB_NodeMember, .offset = offset);
    return tb__gvn(f, n, 0);
}

static TB_Node** legalize_node(LegalizeCtx* ctx, TB_Node* n, LegalType valid) {
    TB_ArenaSavepoint sp = tb_arena_save(tmp_arena);
    TB_Node** pieces = tb_arena_alloc(tmp_arena, valid.splits * sizeof(TB_Node*));

    TB_Function* f = ctx->f;
    if (n->type == TB_INTEGER_CONST) {
        uint64_t imm = TB_NODE_GET_EXTRA_T(n, TB_NodeInt)->value;
        uint64_t mask = tb__mask(valid.size*8);

        FOREACH_N(i, 0, valid.splits) {
            pieces[i] = make_int_node(f, ctx->p, valid.dt, imm & mask);
            imm >>= valid.size*8;
        }
    } else if (n->type == TB_LOAD) {
        int32_t align = TB_NODE_GET_EXTRA_T(n, TB_NodeMemAccess)->align;
        if (align < valid.size) { align = valid.size; }

        int32_t off  = 0;
        FOREACH_N(i, 0, valid.splits) {
            TB_Node* addr = make_member(f, n->inputs[2], off);
            off += valid.size;

            // make split load
            TB_Node* ld = tb_alloc_node(f, TB_LOAD, valid.dt, 3, sizeof(TB_NodeMemAccess));
            set_input(f, ld, n->inputs[0], 0);
            set_input(f, ld, n->inputs[1], 1);
            set_input(f, ld, addr,         2);
            TB_NODE_SET_EXTRA(ld, TB_NodeMemAccess, .align = align);

            pieces[i] = ld;
        }
    } else if (n->type == TB_ADD) {
        TB_Node** a = legalize_node(ctx, n->inputs[1], valid);
        TB_Node** b = legalize_node(ctx, n->inputs[2], valid);

        TB_Node* carry = NULL;
        FOREACH_N(i, 0, valid.splits) {
            TB_Node* n = tb_alloc_node(f, TB_ADC, TB_TYPE_TUPLE, 4, 0);
            set_input(f, n, a[i],  1);
            set_input(f, n, b[i],  2);
            set_input(f, n, carry, 3);

            pieces[i] = make_proj_node(f, valid.dt,     n, 0);
            carry     = make_proj_node(f, TB_TYPE_BOOL, n, 1);
        }
    } else {
        tb_todo();
    }
    return pieces;
}

static TB_Node* walk_node(LegalizeCtx* ctx, TB_Node* n) {
    if (worklist_test_n_set(ctx->ws, n)) {
        return n;
    }

    TB_Function* f = ctx->f;

    // let's find ops we need to scale up
    LegalType valid;
    if (n->type == TB_STORE && ctx->fn(ctx, n->inputs[3]->dt, &valid)) {
        // split store
        TB_Node** op = legalize_node(ctx, n->inputs[3], valid);

        int32_t align = TB_NODE_GET_EXTRA_T(n, TB_NodeMemAccess)->align;
        if (align < valid.size) { align = valid.size; }

        TB_Node* prev_st = n->inputs[1];
        int32_t off  = 0;
        FOREACH_N(i, 0, valid.splits - 1) {
            TB_Node* addr = make_member(f, n->inputs[2], off);
            off += valid.size;

            // make split stores
            TB_Node* st = tb_alloc_node(f, TB_STORE, TB_TYPE_MEMORY, 4, sizeof(TB_NodeMemAccess));
            set_input(f, st, n->inputs[0], 0);
            set_input(f, st, prev_st,      1);
            set_input(f, st, addr,         2);
            set_input(f, st, op[i],        3);
            TB_NODE_SET_EXTRA(st, TB_NodeMemAccess, .align = align);
            prev_st = st;
        }

        nl_hashset_remove2(&f->gvn_nodes, n, gvn_hash, gvn_compare);

        // we'll replace our original store with the first load (so all the
        // stores are in order)
        TB_Node* addr = make_member(f, n->inputs[2], off);
        set_input(f, n, prev_st,              1);
        set_input(f, n, addr,                 2);
        set_input(f, n, op[valid.splits - 1], 3);
    }

    // replace all input edges
    FOREACH_N(i, 0, n->input_count) {
        TB_Node* in = n->inputs[i];
        if (in) {
            TB_Node* k  = walk_node(ctx, in);
            if (in != k) { set_input(ctx->f, n, k, i); }
        }
    }

    return n;
}

// This is the post-optimize pass which gets rid of weird integer types
void tb_pass_legalize(TB_Passes* p, TB_Arch arch) {
    CUIK_TIMED_BLOCK("global sched") {
        TB_Function* f = p->f;

        LegalizeCtx ctx = { arch, f, p, &p->worklist };
        ctx.fn = legalize_64bit_machine;

        // bottom-up rewrite
        f->root_node = walk_node(&ctx, f->root_node);
    }
}

