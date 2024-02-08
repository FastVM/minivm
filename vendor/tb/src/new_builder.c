// I'm trying to transition most people over since it's a bit nicer.

typedef struct TB_GraphCtrl TB_GraphCtrl;
struct TB_GraphCtrl {
    TB_GraphCtrl* prev;

    int pos; // where the stack head was before doing the code
    bool has_else;
    bool is_loop;

    // LOOP: [0] body, [1] break
    // IF:   [0] true, [1] false
    TB_Node* paths[2];

    // loop header
    TB_Node* header;
    TB_Node* join;

    // preserved names
    TB_ArenaSavepoint sp;
    int val_cnt;
    TB_Node* vals[];
};

struct TB_GraphBuilder {
    TB_Function* f;
    TB_Arena* arena;

    // active control flow
    TB_Node *top_ctrl, *bot_ctrl;

    // ctrl stack
    TB_GraphCtrl* top;

    // value stack
    int val_cap, val_cnt;
    TB_Node** vals;
};

static TB_Node* xfer_ctrl(TB_GraphBuilder* g, TB_Node* n) {
    TB_Node* prev = g->bot_ctrl;
    g->bot_ctrl = n;
    return prev;
}

static void push(TB_GraphBuilder* g, TB_Node* n) {
    if (g->val_cnt == g->val_cap) {
        g->val_cap *= 2;
        g->vals = tb_platform_heap_realloc(g->vals, g->val_cap * sizeof(TB_Node*));
    }

    // we don't wanna GVN phis too early, they might be incomplete
    if (n->type != TB_PHI) {
        n = tb_pass_gvn_node(g->f, n);
    }

    g->vals[g->val_cnt++] = n;
}

static TB_Node* pop(TB_GraphBuilder* g) {
    assert(g->val_cnt > 0 && "nothing on the value stack!!!");
    return g->vals[--g->val_cnt];
}

TB_GraphBuilder* tb_builder_enter(TB_Function* f, TB_Arena* arena) {
    TB_GraphBuilder* g = tb_arena_alloc(arena, sizeof(TB_GraphBuilder));
    *g = (TB_GraphBuilder){ .f = f, .arena = arena, .val_cap = 32 };
    g->vals = tb_platform_heap_alloc(g->val_cap * sizeof(TB_Node*));

    // both RPC and memory are mutable vars
    assert(g->val_cap >= 2 + f->param_count);
    g->val_cnt = 2 + f->param_count;
    FOREACH_N(i, 0, 2 + f->param_count) {
        g->vals[i] = f->params[1 + i];
    }

    g->top_ctrl = g->bot_ctrl = f->params[0];
    return g;
}

void tb_builder_exit(TB_GraphBuilder* g) {
}

int tb_builder_save(TB_GraphBuilder* g) {
    return g->val_cnt;
}

void tb_builder_restore(TB_GraphBuilder* g, int v) {
    assert(g->val_cnt >= v);
    g->val_cnt = v;
}

void tb_builder_push(TB_GraphBuilder* g, TB_Node* n) {
    push(g, n);
}

TB_Node* tb_builder_pop(TB_GraphBuilder* g) {
    return pop(g);
}

void tb_builder_uint(TB_GraphBuilder* g, TB_DataType dt, uint64_t x) {
    assert(TB_IS_POINTER_TYPE(dt) || TB_IS_INTEGER_TYPE(dt));
    if (dt.type == TB_INT && dt.data < 64) {
        uint64_t mask = ~UINT64_C(0) >> (64 - dt.data);
        x &= mask;
    }

    TB_Node* n = tb_alloc_node(g->f, TB_INTEGER_CONST, dt, 1, sizeof(TB_NodeInt));
    set_input(g->f, n, g->f->root_node, 0);
    TB_NODE_SET_EXTRA(n, TB_NodeInt, .value = x);
    push(g, n);
}

void tb_builder_sint(TB_GraphBuilder* g, TB_DataType dt, int64_t x) {
    assert(TB_IS_POINTER_TYPE(dt) || TB_IS_INTEGER_TYPE(dt));

    TB_Node* n = tb_alloc_node(g->f, TB_INTEGER_CONST, dt, 1, sizeof(TB_NodeInt));
    set_input(g->f, n, g->f->root_node, 0);
    TB_NODE_SET_EXTRA(n, TB_NodeInt, .value = x);
    push(g, n);
}

void tb_builder_float32(TB_GraphBuilder* g, float imm) {
    TB_Node* n = tb_alloc_node(g->f, TB_FLOAT32_CONST, TB_TYPE_F32, 1, sizeof(TB_NodeFloat32));
    set_input(g->f, n, g->f->root_node, 0);
    TB_NODE_SET_EXTRA(n, TB_NodeFloat32, .value = imm);
    push(g, n);
}

void tb_builder_float64(TB_GraphBuilder* g, double imm) {
    TB_Node* n = tb_alloc_node(g->f, TB_FLOAT64_CONST, TB_TYPE_F64, 1, sizeof(TB_NodeFloat64));
    set_input(g->f, n, g->f->root_node, 0);
    TB_NODE_SET_EXTRA(n, TB_NodeFloat64, .value = imm);
    push(g, n);
}

void tb_builder_string(TB_GraphBuilder* g, ptrdiff_t len, const char* str) {
    if (len < 0) len = strlen(str) + 1;

    TB_Function* f = g->f;
    TB_Global* dummy = tb_global_create(f->super.module, 0, NULL, NULL, TB_LINKAGE_PRIVATE);
    tb_global_set_storage(f->super.module, tb_module_get_rdata(f->super.module), dummy, len, 1, 1);

    char* dst = tb_global_add_region(f->super.module, dummy, 0, len);
    memcpy(dst, str, len);

    push(g, tb_inst_get_symbol_address(f, (TB_Symbol*) dummy));
}

void tb_builder_cast(TB_GraphBuilder* g, TB_DataType dt, int type) {
    assert(type >= TB_TRUNCATE && type <= TB_BITCAST);
    TB_Node* a = pop(g);

    TB_Function* f = g->f;
    TB_Node* n = tb_alloc_node(f, type, dt, 2, 0);
    set_input(f, n, a, 1);
    push(g, n);
}

void tb_builder_binop_int(TB_GraphBuilder* g, int type, TB_ArithmeticBehavior ab) {
    assert(type >= TB_AND && type <= TB_SMOD);
    TB_Node* b = pop(g);
    TB_Node* a = pop(g);

    TB_Function* f = g->f;
    TB_Node* n = tb_alloc_node(f, type, a->dt, 3, sizeof(TB_NodeBinopInt));
    set_input(f, n, a, 1);
    set_input(f, n, b, 2);
    TB_NODE_SET_EXTRA(n, TB_NodeBinopInt, .ab = ab);
    push(g, n);
}

void tb_builder_cmp(TB_GraphBuilder* g, int type, bool flip, TB_DataType dt) {
    TB_Node* b = pop(g);
    TB_Node* a = pop(g);
    tb_assert(type >= TB_CMP_EQ && type <= TB_CMP_FLE, "'type' wasn't a comparison node type (see TB_NodeTypeEnum)");
    tb_assert(TB_DATA_TYPE_EQUALS(a->dt, b->dt), "datatype mismatch");

    TB_Function* f = g->f;
    TB_Node* n = tb_alloc_node(f, type, TB_TYPE_BOOL, 3, sizeof(TB_NodeCompare));
    if (flip) {
        set_input(f, n, b, 1);
        set_input(f, n, a, 2);
    } else {
        set_input(f, n, a, 1);
        set_input(f, n, b, 2);
    }
    TB_NODE_SET_EXTRA(n, TB_NodeCompare, .cmp_dt = a->dt);
    push(g, n);
}

void tb_builder_array(TB_GraphBuilder* g, int64_t stride) {
    TB_Node* index = pop(g);
    TB_Node* base  = pop(g);
    tb_assert(base->dt.type == TB_PTR,  "base on ARRAY must be an integer");
    tb_assert(index->dt.type == TB_INT, "index on ARRAY must be an integer");

    TB_Function* f = g->f;
    TB_Node* n = tb_alloc_node(f, TB_ARRAY_ACCESS, TB_TYPE_PTR, 3, sizeof(TB_NodeArray));
    set_input(f, n, base,  1);
    set_input(f, n, index, 2);
    TB_NODE_SET_EXTRA(n, TB_NodeArray, .stride = stride);
    push(g, n);
}

void tb_builder_member(TB_GraphBuilder* g, int64_t offset) {
    if (offset == 0) { return; }
    TB_Node* base = pop(g);

    TB_Function* f = g->f;
    TB_Node* n = tb_alloc_node(f, TB_MEMBER_ACCESS, TB_TYPE_PTR, 2, sizeof(TB_NodeMember));
    set_input(f, n, base, 1);
    TB_NODE_SET_EXTRA(n, TB_NodeMember, .offset = offset);
    push(g, n);
}

void tb_builder_load(TB_GraphBuilder* g, int mem_var, bool ctrl_dep, TB_DataType dt, int32_t offset, TB_CharUnits alignment) {
    TB_Function* f = g->f;

    TB_Node* addr = pop(g);
    if (offset) {
        TB_Node* n = tb_alloc_node(f, TB_MEMBER_ACCESS, TB_TYPE_PTR, 2, sizeof(TB_NodeMember));
        set_input(f, n, addr, 1);
        TB_NODE_SET_EXTRA(n, TB_NodeMember, .offset = offset);
        addr = n;
    }

    TB_Node* n = tb_alloc_node(f, TB_LOAD, dt, 3, sizeof(TB_NodeMemAccess));
    if (ctrl_dep) {
        set_input(f, n, g->bot_ctrl, 0);
    }
    set_input(f, n, g->vals[mem_var], 1);
    set_input(f, n, addr, 2);
    TB_NODE_SET_EXTRA(n, TB_NodeMemAccess, .align = alignment);

    push(g, n);
}

void tb_builder_store(TB_GraphBuilder* g, int mem_var, int32_t offset, TB_CharUnits alignment) {
    TB_Function* f = g->f;

    TB_Node* val  = pop(g);
    TB_Node* addr = pop(g);
    assert(g->vals[mem_var]->dt.type == TB_MEMORY);

    if (offset) {
        TB_Node* n = tb_alloc_node(f, TB_MEMBER_ACCESS, TB_TYPE_PTR, 2, sizeof(TB_NodeMember));
        set_input(f, n, addr, 1);
        TB_NODE_SET_EXTRA(n, TB_NodeMember, .offset = offset);
        addr = n;
    }

    TB_Node* n = tb_alloc_node(f, TB_STORE, TB_TYPE_MEMORY, 4, sizeof(TB_NodeMemAccess));
    set_input(f, n, f->trace.bot_ctrl, 0);
    set_input(f, n, g->vals[mem_var], 1);
    set_input(f, n, addr, 2);
    set_input(f, n, val, 3);
    TB_NODE_SET_EXTRA(n, TB_NodeMemAccess, .align = alignment);

    // assign ROOT memory
    g->vals[mem_var] = n;
}

void tb_builder_debugbreak(TB_GraphBuilder* g) {
    TB_Node* n = tb_alloc_node(g->f, TB_DEBUGBREAK, TB_TYPE_CONTROL, 1, 0);
    set_input(g->f, n, xfer_ctrl(g, n), 0);
}

int tb_builder_var(TB_GraphBuilder* g) {
    return g->val_cnt - 1;
}

void tb_builder_get_var(TB_GraphBuilder* g, int id) {
    assert(id < g->val_cnt);
    push(g, g->vals[id]);
}

void tb_builder_set_var(TB_GraphBuilder* g, int id) {
    assert(id < g->val_cnt - 1);
    g->vals[id] = pop(g);
}

void tb_builder_if(TB_GraphBuilder* g) {
    TB_Function* f = g->f;

    TB_ArenaSavepoint sp = tb_arena_save(g->arena);
    TB_GraphCtrl* ctrl = tb_arena_alloc(g->arena, sizeof(TB_GraphCtrl) + g->val_cnt*sizeof(TB_Node*));
    *ctrl = (TB_GraphCtrl){ 0 };

    TB_Node* cond = pop(g);

    // clone incoming state so we can diff & insert phis later
    ctrl->sp = sp;
    ctrl->val_cnt = g->val_cnt;
    FOREACH_N(i, 0, g->val_cnt) {
        ctrl->vals[i] = g->vals[i];
    }

    ctrl->prev = g->top;
    g->top = ctrl;

    // generate branch op
    {
        TB_Node* n = tb_alloc_node(f, TB_BRANCH, TB_TYPE_TUPLE, 2, sizeof(TB_NodeBranch) + sizeof(int64_t));
        set_input(f, n, xfer_ctrl(g, NULL), 0);
        set_input(f, n, cond, 1);

        ctrl->paths[0] = tb__make_proj(f, TB_TYPE_CONTROL, n, 0);
        ctrl->paths[1] = tb__make_proj(f, TB_TYPE_CONTROL, n, 1);

        TB_NodeBranch* br = TB_NODE_GET_EXTRA(n);
        br->succ_count = 2;
        br->keys[0] = 0;
    }

    // add merge region (no phis yet, they'll be back)
    ctrl->header = NULL;
    ctrl->join = tb_alloc_node_dyn(f, TB_REGION, TB_TYPE_CONTROL, 0, 2, sizeof(TB_NodeRegion));
    TB_NODE_SET_EXTRA(ctrl->join, TB_NodeRegion, .freq = 1.0f);

    // goto the true path
    g->top_ctrl = g->bot_ctrl = ctrl->paths[0];
}

void tb_builder_else(TB_GraphBuilder* g) {
    assert(g->top && "we're not inside of an if block");
    TB_GraphCtrl* ctrl = g->top;

    if (g->bot_ctrl) {
        // goto true => join
        add_input_late(g->f, ctrl->join, g->bot_ctrl);
    }

    // goto the false path
    ctrl->has_else = true;
    g->top_ctrl = g->bot_ctrl = ctrl->paths[1];
}

void tb_builder_endif(TB_GraphBuilder* g) {
    assert(g->top_ctrl && "we're not inside of an if block");
    TB_GraphCtrl* ctrl = g->top;
    g->top = ctrl->prev;

    if (!ctrl->has_else) {
        // goto false => join
        add_input_late(g->f, ctrl->join, ctrl->paths[1]);
    }

    if (g->bot_ctrl) {
        // fallthru
        add_input_late(g->f, ctrl->join, g->bot_ctrl);
    }

    // insert phis
    TB_Node* join = ctrl->join;
    assert(join->type == TB_REGION);
    assert(g->val_cnt == ctrl->val_cnt && "paths on branch mismatch in outgoing variables?");

    if (join->input_count > 0) {
        FOREACH_N(i, 0, g->val_cnt) {
            if (g->vals[i] != ctrl->vals[i]) {
                TB_Node* n = tb_alloc_node_dyn(g->f, TB_PHI, g->vals[i]->dt, 3, 3, 0);
                set_input(g->f, n, join, 0);
                set_input(g->f, n, ctrl->vals[i], 1);
                set_input(g->f, n, g->vals[i], 2);
                g->vals[i] = n;
            }
        }

        g->top_ctrl = g->bot_ctrl = join;
    } else {
        tb_pass_kill_node(g->f, join);
        g->top_ctrl = g->bot_ctrl = NULL;
    }
    tb_arena_restore(g->arena, ctrl->sp);
}

static void block_jmp(TB_GraphBuilder* g, TB_Node* bot, int depth) {
    TB_GraphCtrl* ctrl = g->top;
    while (depth--) { ctrl = ctrl->prev; }

    // add exit path
    if (!ctrl->is_loop && ctrl->join == NULL) {
        ctrl->join = tb_alloc_node_dyn(g->f, TB_REGION, TB_TYPE_CONTROL, 0, 2, sizeof(TB_NodeRegion));
        TB_NODE_SET_EXTRA(ctrl->join, TB_NodeRegion, .freq = 1.0f);
    }

    TB_Node* target = ctrl->is_loop ? ctrl->header : ctrl->join;

    assert(ctrl->val_cnt == g->val_cnt);
    add_input_late(g->f, target, bot);

    // add edges to phis
    if (ctrl->is_loop) {
        FOREACH_N(i, 0, ctrl->val_cnt) {
            add_input_late(g->f, ctrl->vals[i], g->vals[i]);
        }
    } else {
        FOREACH_N(i, 0, ctrl->val_cnt) {
            TB_Node* val = ctrl->vals[i];
            if (val == NULL) {
                ctrl->vals[i] = g->vals[i];
            } else if (val->type == TB_PHI && val->inputs[0] == target) {
                add_input_late(g->f, val, g->vals[i]);
            } else {
                // add phi
                TB_Node* n = tb_alloc_node_dyn(g->f, TB_PHI, g->vals[i]->dt, 3, 3, 0);
                set_input(g->f, n, target,     0);
                set_input(g->f, n, val,        1);
                set_input(g->f, n, g->vals[i], 2);
                ctrl->vals[i] = n;
            }
        }
    }
}

void tb_builder_loop(TB_GraphBuilder* g) {
    TB_Function* f = g->f;

    TB_ArenaSavepoint sp = tb_arena_save(g->arena);
    TB_GraphCtrl* ctrl = tb_arena_alloc(g->arena, sizeof(TB_GraphCtrl) + g->val_cnt*sizeof(TB_Node*));
    *ctrl = (TB_GraphCtrl){ 0 };

    // add header region
    ctrl->header = tb_alloc_node_dyn(f, TB_REGION, TB_TYPE_CONTROL, 0, 2, sizeof(TB_NodeRegion));
    ctrl->join = NULL;
    ctrl->is_loop = true;
    TB_NODE_SET_EXTRA(ctrl->header, TB_NodeRegion, .freq = 10.0f);

    if (g->bot_ctrl) {
        // fallthru
        add_input_late(g->f, ctrl->header, g->bot_ctrl);
    }

    // since we don't know which values are mutated, we'll just clone all of them
    ctrl->sp = sp;
    ctrl->val_cnt = g->val_cnt;
    FOREACH_N(i, 0, g->val_cnt) {
        TB_Node* n = tb_alloc_node_dyn(g->f, TB_PHI, g->vals[i]->dt, 2, 4, 0);
        set_input(g->f, n, ctrl->header, 0);
        set_input(g->f, n, g->vals[i], 1);
        g->vals[i] = ctrl->vals[i] = n;
    }

    ctrl->prev = g->top;
    g->top = ctrl;

    // goto the loop body
    g->top_ctrl = g->bot_ctrl = ctrl->header;
}

void tb_builder_endloop(TB_GraphBuilder* g) {
    TB_GraphCtrl* ctrl = g->top;
    g->top = ctrl->prev;

    TB_Node* header = ctrl->header;
    FOREACH_N(i, 0, ctrl->val_cnt) {
        // phis are complete now, we can fold them out if unused
        TB_Node* src = ctrl->vals[i];
        /* if (src->type == TB_PHI && src->inputs[0] == header && src->input_count > 1) {
            TB_Node* k = identity_phi(NULL, g->f, src);
            if (k != src) {
                // phi => same
                subsume_node(g->f, src, k);
                g->vals[i] = k;
                continue;
            }
        } */

        g->vals[i] = ctrl->vals[i];
    }

    tb_arena_restore(g->arena, ctrl->sp);
}

void tb_builder_br(TB_GraphBuilder* g, int depth) {
    block_jmp(g, g->bot_ctrl, depth);
    g->top_ctrl = g->bot_ctrl = NULL;
}

void tb_builder_br_if(TB_GraphBuilder* g, int depth) {
    TB_Function* f = g->f;

    TB_Node* cond = pop(g);
    TB_Node* n = tb_alloc_node(f, TB_BRANCH, TB_TYPE_TUPLE, 2, sizeof(TB_NodeBranch) + sizeof(int64_t));
    set_input(f, n, xfer_ctrl(g, NULL), 0);
    set_input(f, n, cond, 1);

    TB_Node* leave    = tb__make_proj(f, TB_TYPE_CONTROL, n, 0);
    TB_Node* fallthru = tb__make_proj(f, TB_TYPE_CONTROL, n, 1);

    TB_NodeBranch* br = TB_NODE_GET_EXTRA(n);
    br->succ_count = 2;
    br->keys[0] = 0;

    block_jmp(g, leave, depth);

    // goto fallthru path
    g->top_ctrl = g->bot_ctrl = fallthru;
}

void tb_builder_block(TB_GraphBuilder* g) {
    TB_Function* f = g->f;

    TB_ArenaSavepoint sp = tb_arena_save(g->arena);
    TB_GraphCtrl* ctrl = tb_arena_alloc(g->arena, sizeof(TB_GraphCtrl) + sizeof(TB_Node*)*g->val_cnt);
    *ctrl = (TB_GraphCtrl){ 0 };
    ctrl->sp = sp;

    ctrl->val_cnt = g->val_cnt;
    memset(ctrl->vals, 0, sizeof(TB_Node*)*g->val_cnt);

    ctrl->prev = g->top;
    g->top = ctrl;
}

void tb_builder_endblock(TB_GraphBuilder* g) {
    if (g->bot_ctrl != NULL) {
        block_jmp(g, g->bot_ctrl, 0);
    }

    TB_GraphCtrl* ctrl = g->top;
    g->top = ctrl->prev;

    FOREACH_N(i, 0, ctrl->val_cnt) {
        g->vals[i] = ctrl->vals[i];
    }

    g->top_ctrl = g->bot_ctrl = ctrl->join;
    tb_arena_restore(g->arena, ctrl->sp);
}

void tb_builder_static_call(TB_GraphBuilder* g, TB_FunctionPrototype* proto, TB_Symbol* target, int mem_var, int nargs) {
    assert(g->val_cnt >= nargs);
    TB_Function* f = g->f;

    TB_Node* target_node;
    {
        target_node = tb_alloc_node(f, TB_SYMBOL, TB_TYPE_PTR, 1, sizeof(TB_NodeSymbol));
        set_input(f, target_node, f->root_node, 0);
        TB_NODE_SET_EXTRA(target_node, TB_NodeSymbol, .sym = target);

        target_node = tb_pass_gvn_node(g->f, target_node);
    }

    // construct call op
    g->val_cnt -= nargs;
    {
        size_t proj_count = 2 + (proto->return_count > 1 ? proto->return_count : 1);

        TB_Node* n = tb_alloc_node(f, TB_CALL, TB_TYPE_TUPLE, 3 + nargs, sizeof(TB_NodeCall) + (sizeof(TB_Node*)*proj_count));
        set_input(f, n, target_node, 2);
        FOREACH_N(i, 0, nargs) {
            set_input(f, n, g->vals[g->val_cnt + i], i + 3);
        }

        TB_NodeCall* c = TB_NODE_GET_EXTRA(n);
        c->proj_count = proj_count;
        c->proto = proto;

        // control proj
        TB_Node* cproj = tb__make_proj(f, TB_TYPE_CONTROL, n, 0);
        set_input(f, n, xfer_ctrl(g, cproj), 0);

        // memory proj
        TB_Node* mproj = tb__make_proj(f, TB_TYPE_MEMORY, n, 1);
        set_input(f, n, g->vals[mem_var], 1);

        // create data projections
        TB_PrototypeParam* rets = TB_PROTOTYPE_RETURNS(proto);
        FOREACH_N(i, 0, proto->return_count) {
            c->projs[i + 2] = tb__make_proj(f, rets[i].dt, n, i + 2);
        }

        // we'll slot a NULL so it's easy to tell when it's empty
        if (proto->return_count == 0) {
            c->projs[2] = NULL;
        }

        c->projs[0] = cproj;
        c->projs[1] = mproj;

        g->vals[mem_var] = mproj;
        add_input_late(f, get_callgraph(f), n);

        // push all returns
        assert(proto->return_count < 2);
        FOREACH_N(i, 0, proto->return_count) {
            push(g, c->projs[2 + i]);
        }
    }
}

void tb_builder_ret(TB_GraphBuilder* g, int count) {
    TB_Function* f = g->f;
    TB_Node* mem_state = g->vals[0];

    assert(g->val_cnt >= count);
    g->val_cnt -= count;
    TB_Node** args = &g->vals[g->val_cnt];

    // allocate return node
    TB_Node* ret = f->ret_node;
    TB_Node* ctrl = ret->inputs[0];
    assert(ctrl->type == TB_REGION);

    // add to PHIs
    assert(ret->input_count >= 3 + count);
    add_input_late(f, ret->inputs[1], mem_state);

    size_t i = 3;
    for (; i < count + 3; i++) {
        TB_Node* v = args[i - 3];
        assert(ret->inputs[i]->dt.raw == v->dt.raw && "datatype mismatch");
        add_input_late(f, ret->inputs[i], v);
    }

    size_t phi_count = ret->input_count;
    for (; i < phi_count; i++) {
        // put poison in the leftovers?
        log_warn("%s: ir: generated poison due to inconsistent number of returned values", f->super.name);

        TB_Node* poison = tb_alloc_node(f, TB_POISON, ret->inputs[i]->dt, 1, 0);
        set_input(f, poison, f->ret_node, 0);

        poison = tb__gvn(f, poison, 0);
        add_input_late(f, ret->inputs[i], poison);
    }

    // basically just tb_inst_goto without the memory PHI (we did it earlier)
    TB_Node* n = g->bot_ctrl;
    g->bot_ctrl = NULL;

    add_input_late(f, ctrl, n);
}
