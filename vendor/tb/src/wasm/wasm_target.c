#ifdef TB_HAS_WASM
#include "../tb_internal.h"
#include "../emitter.h"
#include "../opt/passes.h"

typedef struct {
    int id; // where in the locals it went, -1 for unallocated
    int uses;
} ValueDesc;

// control flow element
typedef struct WasmElem WasmElem;
struct WasmElem {
    enum {
        WASM_BLOCK,
        WASM_BLOCK2, // body goes after _then case
        WASM_BR,
        WASM_IF,
        WASM_LOOP
    } tag;

    bool refs;
    uint16_t depth;

    TB_Node* body;
    WasmElem* _then;
    WasmElem* _else; // only relevant for WASM_IF
};

typedef struct DomTree DomTree;
struct DomTree {
    int id;
    WasmElem* elem;
    TB_Node *start, *end;
    DynArray(DomTree*) kids;
};

typedef struct Ctx {
    TB_Passes* p;
    TB_CGEmitter emit;

    TB_Module* module;
    TB_Function* f;
    TB_FeatureSet features;

    TB_CFG cfg;
    size_t block_count;

    DomTree* doms;

    DynArray(DomTree*) filtered;
    DynArray(PhiVal) phi_vals;

    int stack_n;
    TB_Node* stack[64];

    DynArray(uint8_t) local_desc;
    ValueDesc* locals;
} Ctx;

static void indent(int depth) { while (depth--) printf("  "); }

// uleb128 encode
static void emit_uint(Ctx* ctx, uint64_t x) {
    do {
        uint32_t lo = x & 0x7F;
        x >>= 7;
        if (x) {
            lo |= 0x80;
        }
        EMIT1(&ctx->emit, lo);
    } while (x);
}

WasmElem* new_elem(int tag, TB_Node* body) {
    WasmElem* e = tb_arena_alloc(tmp_arena, sizeof(WasmElem));
    *e = (WasmElem){ .tag = tag, .body = body };
    return e;
}

static uint8_t get_wasm_type(TB_DataType dt) {
    switch (dt.type) {
        case TB_INT: {
            if (dt.data <= 8)  return 0x7F;
            if (dt.data <= 16) return 0x7F;
            if (dt.data <= 32) return 0x7F;
            if (dt.data <= 64) return 0x7E;
            break;
        }
        case TB_FLOAT: {
            if (dt.data == TB_FLT_32) return 0x7D;
            if (dt.data == TB_FLT_64) return 0x7C;
            break;
        }
        case TB_PTR: return 0x7F;
    }

    assert(0 && "TODO");
    return 0;
}

// uleb128 encode
static void patch_uint(uint8_t* p, uint64_t x) {
    for (int i = 0; i < 4; i++) {
        uint32_t lo = x & 0x7F;
        x >>= 7;
        if (i < 3) {
            lo |= 0x80;
        }
        *p++ = lo;
    }
}

static int use_count(TB_Node* n) {
    int c = 0;
    FOR_USERS(u, n) { c += 1; }
    return c;
}

static int spill_tos(Ctx* ctx, TB_Node* n) {
    // if there was something on top, we probably wanna spill it later
    ValueDesc* spilled = &ctx->locals[n->gvn];
    if (spilled->id < 0) {
        dyn_array_put(ctx->local_desc, get_wasm_type(n->dt));
        spilled->id = dyn_array_length(ctx->local_desc) - 1;
    }
    return spilled->id;
}

static void push_val(Ctx* ctx, TB_Node* n) {
    spill_tos(ctx, n);
    ctx->stack[ctx->stack_n++] = n;
}

static void compile_bb(Ctx* ctx, TB_Node* bb_start, int depth) {
    TB_BasicBlock* bb = ctx->p->scheduled[bb_start->gvn];
    Worklist* ws = &ctx->p->worklist;

    #ifndef NDEBUG
    TB_BasicBlock* expected = &nl_map_get_checked(ctx->cfg.node_to_block, bb_start);
    assert(expected == bb);
    #endif

    dyn_array_clear(ctx->phi_vals);
    greedy_scheduler(ctx->p, &ctx->cfg, ws, &ctx->phi_vals, bb, bb->end);

    FOREACH_N(i, ctx->cfg.block_count, dyn_array_length(ws->items)) {
        TB_Node* bot = ws->items[i];
        if (bot->type == TB_SYMBOL || bot->type == TB_PROJ || cfg_is_region(bot) || bot->type == TB_PHI || bot->type == TB_BRANCH) {
            continue;
        }

        // begin building up a tree (right now they'll all be tiny)
        ctx->stack_n  = 1;
        ctx->stack[0] = bot;

        switch (bot->type) {
            case TB_INTEGER_CONST:
            case TB_FLOAT32_CONST:
            case TB_FLOAT64_CONST:
            break;

            // integer ops
            case TB_AND:
            case TB_OR:
            case TB_XOR:
            case TB_ADD:
            case TB_SUB:
            case TB_MUL:
            case TB_SHL:
            case TB_SHR:
            case TB_SAR:
            case TB_CMP_EQ:
            case TB_CMP_NE:
            case TB_CMP_SLT:
            case TB_CMP_SLE:
            case TB_CMP_ULT:
            case TB_CMP_ULE:
            case TB_CMP_FLT:
            case TB_CMP_FLE:
            case TB_FADD:
            case TB_FSUB:
            case TB_FMUL:
            case TB_FDIV:
            push_val(ctx, bot->inputs[2]);
            push_val(ctx, bot->inputs[1]);
            break;

            case TB_ARRAY_ACCESS:
            push_val(ctx, bot->inputs[2]);
            push_val(ctx, bot->inputs[1]);
            break;

            case TB_MEMBER_ACCESS:
            push_val(ctx, bot->inputs[1]);
            break;

            case TB_CALL:
            FOREACH_REVERSE_N(i, 3, bot->input_count) {
                push_val(ctx, bot->inputs[i]);
            }
            break;

            case TB_LOAD:
            push_val(ctx, bot->inputs[2]);
            break;

            case TB_STORE:
            push_val(ctx, bot->inputs[3]);
            push_val(ctx, bot->inputs[2]);
            break;

            case TB_RETURN: {
                if (bot->input_count > 3) {
                    push_val(ctx, bot->inputs[3]);
                }
                break;
            }

            default: tb_todo();
        }

        // ok emit ops now
        FOREACH_REVERSE_N(i, 0, ctx->stack_n) {
            TB_Node* n = ctx->stack[i];
            ValueDesc* val = &ctx->locals[n->gvn];

            if (i != 0 && val->id >= 0) {
                EMIT1(&ctx->emit, 0x20);
                emit_uint(ctx, val->id);
                continue;
            }

            switch (n->type) {
                case TB_INTEGER_CONST: {
                    TB_NodeInt* i = TB_NODE_GET_EXTRA(n);
                    EMIT1(&ctx->emit, 0x41);
                    emit_uint(ctx, i->value);
                    break;
                }

                case TB_FLOAT32_CONST: {
                    TB_NodeFloat32* i = TB_NODE_GET_EXTRA(n);

                    uint32_t x;
                    memcpy(&x, &i->value, sizeof(x));

                    EMIT1(&ctx->emit, 0x43);
                    EMIT4(&ctx->emit, x);
                    break;
                }

                case TB_FLOAT64_CONST: {
                    TB_NodeFloat64* i = TB_NODE_GET_EXTRA(n);

                    uint64_t x;
                    memcpy(&x, &i->value, sizeof(x));

                    EMIT1(&ctx->emit, 0x44);
                    EMIT8(&ctx->emit, x);
                    break;
                }

                // integer ops
                case TB_AND:
                case TB_OR:
                case TB_XOR:
                case TB_ADD:
                case TB_SUB:
                case TB_MUL: {
                    static const uint8_t ops[] = { 0x71, 0x72, 0x73, 0x6A, 0x6B, 0x6C };

                    assert(n->dt.type == TB_INT);
                    int base = n->dt.data > 32 ? 0x7C - 0x6A : 0;
                    EMIT1(&ctx->emit, base + ops[n->type - TB_AND]);
                    break;
                }

                // float ops
                case TB_FADD:
                case TB_FSUB:
                case TB_FMUL:
                case TB_FDIV:
                case TB_FMIN:
                case TB_FMAX: {
                    assert(n->dt.type == TB_FLOAT);
                    int base = n->dt.data == TB_FLT_64 ? 0xA0 : 0x92;
                    EMIT1(&ctx->emit, base + (n->type - TB_FADD));
                    break;
                }

                case TB_SHL:
                case TB_SHR:
                case TB_SAR: {
                    static const uint8_t ops[] = { 0 /* shl */, 2 /* shr */, 1 /* sar */ };

                    int base = n->dt.data > 32 ? 0x86 : 0x74;
                    EMIT1(&ctx->emit, base + ops[n->type - TB_SHL]);
                    break;
                }

                case TB_LOAD: {
                    TB_CharUnits align = TB_NODE_GET_EXTRA_T(n, TB_NodeMemAccess)->align;
                    if (n->dt.type == TB_INT) {
                        if (0) {}
                        else if (n->dt.data <= 8)  { EMIT1(&ctx->emit, 0x2D); } // i32.load8_u
                        else if (n->dt.data <= 16) { EMIT1(&ctx->emit, 0x2E); } // i32.load16_u
                        else if (n->dt.data <= 32) { EMIT1(&ctx->emit, 0x28); } // i32.load
                        else if (n->dt.data <= 64) { EMIT1(&ctx->emit, 0x29); } // i64.load
                        else tb_todo();
                    } else {
                        if (0) {}
                        else if (n->dt.data == TB_FLT_32) { EMIT1(&ctx->emit, 0x2A); } // f32.load
                        else if (n->dt.data == TB_FLT_64) { EMIT1(&ctx->emit, 0x2B); } // f64.load
                        else tb_todo();
                    }

                    // memarg
                    EMIT1(&ctx->emit, align); // align
                    EMIT1(&ctx->emit, 0x00);  // offset
                    break;
                }

                case TB_STORE: {
                    TB_CharUnits align = TB_NODE_GET_EXTRA_T(n, TB_NodeMemAccess)->align;
                    TB_DataType dt = n->dt;
                    if (dt.type == TB_INT) {
                        if (0) {}
                        else if (dt.data <= 8)  { EMIT1(&ctx->emit, 0x3A); } // i32.store8_u
                        else if (dt.data <= 16) { EMIT1(&ctx->emit, 0x3B); } // i32.store16_u
                        else if (dt.data <= 32) { EMIT1(&ctx->emit, 0x36); } // i32.store
                        else if (dt.data <= 64) { EMIT1(&ctx->emit, 0x37); } // i64.store
                        else tb_todo();
                    } else {
                        if (0) {}
                        else if (dt.data == TB_FLT_32) { EMIT1(&ctx->emit, 0x38); } // f32.store
                        else if (dt.data == TB_FLT_64) { EMIT1(&ctx->emit, 0x39); } // f64.store
                        else tb_todo();
                    }

                    // memarg
                    EMIT1(&ctx->emit, align); // align
                    EMIT1(&ctx->emit, 0x00);
                    break;
                }

                case TB_MEMBER_ACCESS: {
                    int32_t offset = TB_NODE_GET_EXTRA_T(n, TB_NodeMember)->offset;

                    // i32.const stride
                    EMIT1(&ctx->emit, 0x41);
                    emit_uint(ctx, offset);
                    // i32.add
                    EMIT1(&ctx->emit, 0x6A);
                    break;
                }

                case TB_ARRAY_ACCESS: {
                    int32_t stride = TB_NODE_GET_EXTRA_T(n, TB_NodeArray)->stride;

                    // i32.const stride
                    EMIT1(&ctx->emit, 0x41);
                    emit_uint(ctx, stride);
                    // i32.mul
                    EMIT1(&ctx->emit, 0x6C);
                    // i32.add
                    EMIT1(&ctx->emit, 0x6A);
                    break;
                }

                case TB_CALL: {
                    assert(n->inputs[2]->type == TB_SYMBOL);
                    TB_Symbol* sym = TB_NODE_GET_EXTRA_T(n->inputs[2], TB_NodeSymbol)->sym;

                    // call
                    EMIT1(&ctx->emit, 0x10);
                    EMIT4(&ctx->emit, 0x00808080);
                    tb_emit_symbol_patch(ctx->emit.output, sym, ctx->emit.count - 4);
                    break;
                }

                case TB_CMP_EQ:
                case TB_CMP_NE:
                case TB_CMP_SLT:
                case TB_CMP_SLE:
                case TB_CMP_ULT:
                case TB_CMP_ULE:
                {
                    // position of the 32bit ops, we'll relocate for 64bit ones (same offsets)
                    static const uint8_t ops[] = { 0x46, 0x47, 0x48, 0x4C, 0x49, 0x4D };

                    if (n->dt.type == TB_INT || n->dt.type == TB_PTR) {
                        int bits = n->dt.type == TB_INT ? n->dt.data : 32;
                        int base = bits > 32 ? 0x51 - 0x46 : 0;

                        EMIT1(&ctx->emit, base + ops[n->type - TB_CMP_EQ]);
                    } else {
                        tb_todo();
                    }
                    break;
                }

                case TB_RETURN: {
                    EMIT1(&ctx->emit, 0x0F);
                    break;
                }

                default: tb_todo();
            }
        }

        if (bot->dt.type == TB_INT || bot->dt.type == TB_PTR || bot->dt.type == TB_FLOAT) {
            int dst = spill_tos(ctx, bot);
            EMIT1(&ctx->emit, 0x21);
            emit_uint(ctx, dst);
        }
    }

    // if the endpoint is a not a terminator, we've hit some implicit GOTO edge
    TB_Node* end = bb->end;
    if (!cfg_is_terminator(end)) {
        // writeback phis
        FOREACH_N(i, 0, dyn_array_length(ctx->phi_vals)) {
            PhiVal* v = &ctx->phi_vals[i];

            // get_local src
            int src = ctx->locals[v->n->gvn].id;
            EMIT1(&ctx->emit, 0x20);
            emit_uint(ctx, src);
            // set_local dst
            int dst = spill_tos(ctx, v->phi);
            EMIT1(&ctx->emit, 0x21);
            emit_uint(ctx, dst);
        }
    }
}

static TB_Node** successors(Ctx* ctx, Worklist* ws, TB_Node* end, size_t* out_succ_count) {
    size_t succ_count = !cfg_is_endpoint(end);
    if (end->type == TB_BRANCH) {
        succ_count = TB_NODE_GET_EXTRA_T(end, TB_NodeBranch)->succ_count;
    }
    *out_succ_count = succ_count;

    size_t base = dyn_array_length(ws->items);
    dyn_array_put_uninit(ws->items, succ_count);
    TB_Node** succ_blocks = &ws->items[base];

    if (end->type == TB_BRANCH) {
        FOR_USERS(u, end) {
            if (u->n->type == TB_PROJ) {
                TB_Node* succ = cfg_next_bb_after_cproj(u->n);
                int index = TB_NODE_GET_EXTRA_T(u->n, TB_NodeProj)->index;
                succ_blocks[index] = succ;
            }
        }
    } else if (!cfg_is_endpoint(end)) {
        TB_Node* succ = cfg_next_control(end);
        if (succ) { succ_blocks[0] = succ; }
    }

    return succ_blocks;
}

static bool wasm_is_natural_loop(Ctx* ctx, TB_Node* header) {
    if (cfg_is_region(header)) {
        FOREACH_N(i, 0, header->input_count) {
            TB_Node* pred = cfg_get_pred(&ctx->cfg, header, i);
            if (slow_dommy(&ctx->cfg, header, pred)) {
                return true;
            }
        }
    }

    return false;
}

static bool has_merge_root(Ctx* ctx, TB_Node* n, int id) {
    if (!cfg_is_region(n)) {
        return false;
    }

    FOREACH_N(i, 0, n->input_count) {
        TB_Node* pred = cfg_get_pred(&ctx->cfg, n, i);
        TB_BasicBlock* pred_bb = ctx->p->scheduled[pred->gvn];

        if (pred_bb->id >= id) {
            return false;
        }
    }

    return true;
}

static WasmElem* do_dom_tree(Ctx* ctx, DomTree* node);

static WasmElem* do_branch(Ctx* ctx, DomTree* src, TB_Node* bb_start) {
    TB_BasicBlock* bb = ctx->p->scheduled[bb_start->gvn];
    DomTree* dst = &ctx->doms[bb->id];

    if (dst->id < src->id || has_merge_root(ctx, bb_start, dst->id)) {
        // forward or backwards edge (continue/break)
        WasmElem* e = new_elem(WASM_BR, NULL);
        assert(dst->elem);
        e->_then = dst->elem;
        dst->elem->refs = true;
        return e;
    } else {
        return do_dom_tree(ctx, dst);
    }
}

static WasmElem* do_dom_tree(Ctx* ctx, DomTree* node) {
    bool loop = wasm_is_natural_loop(ctx, node->start);
    WasmElem* e = node->elem = new_elem(loop ? WASM_LOOP : WASM_BLOCK, node->start);

    // find merges (embed within each other)
    size_t base = dyn_array_length(ctx->filtered);
    WasmElem* last = NULL;
    FOREACH_N(i, 0, dyn_array_length(node->kids)) {
        TB_Node* start = node->kids[i]->start;
        TB_BasicBlock* start_bb = ctx->p->scheduled[start->gvn];

        DomTree* kid = node->kids[i];
        if (has_merge_root(ctx, start, start_bb->id)) {
            dyn_array_put(ctx->filtered, kid);

            if (last) {
                WasmElem* new_e = new_elem(WASM_BLOCK2, start);
                new_e->_then = last;
                last = kid->elem = new_e;
            } else {
                last = kid->elem;
            }
        }
    }

    // exit path of node
    size_t succ_count;
    TB_Node** succ_blocks = successors(ctx, &ctx->p->worklist, node->end, &succ_count);
    if (succ_count == 1) {
        e->_then = do_branch(ctx, node, succ_blocks[0]);
    } else if (succ_count == 2) {
        WasmElem* if_block = new_elem(WASM_IF, NULL);
        e->_then = if_block;
        if_block->_then = do_branch(ctx, node, succ_blocks[0]);
        if_block->_else = do_branch(ctx, node, succ_blocks[1]);
    } else if (succ_count != 0) {
        tb_todo();
    }

    // compile merges
    FOREACH_N(i, base, dyn_array_length(ctx->filtered)) {
        do_dom_tree(ctx, ctx->filtered[i]);
    }
    if (ctx->filtered) {
        dyn_array_set_length(ctx->filtered, base);
    }
    return e;
}

static const char* elem_name(WasmElem* elem) {
    switch (elem->tag) {
        case WASM_BLOCK:  return "block";
        case WASM_BLOCK2: return "block";
        case WASM_LOOP:   return "loop";
        case WASM_BR:     return "br";
        case WASM_IF:     return "if";
        default:          return NULL;
    }
}

static uint8_t elem_byte(WasmElem* elem) {
    switch (elem->tag) {
        case WASM_BLOCK:  return 0x02;
        case WASM_BLOCK2: return 0x02;
        case WASM_LOOP:   return 0x03;
        case WASM_IF:     return 0x04;
        default:          return 0;
    }
}

static void render_block(Ctx* ctx, WasmElem* elem, int depth) {
    elem->depth = depth;

    // no block necessary if no one jumps to it
    bool is_block = elem->tag == WASM_IF || elem->refs > 0;
    if (is_block) {
        indent(depth), printf("%s", elem_name(elem));
        if (elem->tag != WASM_BR) {
            printf(" (depth=%d)", depth);
        }
        printf("\n");
    }

    int inner_depth = depth + is_block;
    switch (elem->tag) {
        case WASM_BLOCK:
        case WASM_LOOP:
        if (elem->body) { indent(inner_depth), printf("v%u\n", elem->body->gvn); }
        if (elem->_then) { render_block(ctx, elem->_then, inner_depth); }
        break;

        case WASM_BLOCK2:
        if (elem->_then) { render_block(ctx, elem->_then, inner_depth); }
        if (elem->body) { indent(inner_depth), printf("v%u\n", elem->body->gvn); }
        break;

        case WASM_IF:
        render_block(ctx, elem->_then, inner_depth);
        indent(depth), printf("else\n");
        render_block(ctx, elem->_else, inner_depth);
        break;

        case WASM_BR:
        indent(depth), printf("br %d\n", elem->_then->depth);
        break;
    }

    if (is_block) {
        indent(depth), printf("end\n");
    }
}

static void compile_block(Ctx* ctx, WasmElem* elem, int depth) {
    elem->depth = depth;

    // no block necessary if no one jumps to it
    bool is_block = elem->tag == WASM_IF || elem->refs > 0;
    if (is_block) {
        EMIT1(&ctx->emit, elem_byte(elem));
        EMIT1(&ctx->emit, 0x40);
    }

    int inner_depth = depth + is_block;
    switch (elem->tag) {
        case WASM_BLOCK:
        case WASM_LOOP:
        if (elem->body) { compile_bb(ctx, elem->body, inner_depth); }
        if (elem->_then) { compile_block(ctx, elem->_then, inner_depth); }
        break;

        case WASM_BLOCK2:
        if (elem->_then) { compile_block(ctx, elem->_then, inner_depth); }
        if (elem->body) { compile_bb(ctx, elem->body, inner_depth); }
        break;

        case WASM_IF:
        compile_block(ctx, elem->_then, inner_depth);
        EMIT1(&ctx->emit, 0x05);
        compile_block(ctx, elem->_else, inner_depth);
        break;

        case WASM_BR:
        EMIT1(&ctx->emit, 0x0C);
        emit_uint(ctx, depth - elem->_then->depth);
        break;
    }

    if (is_block) {
        EMIT1(&ctx->emit, 0x0B);
    }
}

static int dom_sort_cmp(const void* a, const void* b) {
    DomTree* const* aa = a;
    DomTree* const* bb = b;
    return aa[0]->id - bb[0]->id;
}

static void compile_function(TB_Passes* restrict p, TB_FunctionOutput* restrict func_out, const TB_FeatureSet* features, TB_Arena* code_arena, bool emit_asm) {
    verify_tmp_arena(p);

    TB_Arena* arena = tmp_arena;
    TB_ArenaSavepoint sp = tb_arena_save(arena);

    TB_Function* restrict f = p->f;
    TB_OPTDEBUG(CODEGEN)(tb_pass_print(p));

    Ctx ctx = {
        .module = f->super.module,
        .f = f,
        .p = p,
        .emit = {
            .output = func_out,
            .arena = arena,
            .has_comments = false,
        }
    };

    // allocate entire top of the code arena (we'll trim it later if possible)
    ctx.emit.capacity = code_arena->high_point - code_arena->watermark;
    ctx.emit.data = tb_arena_alloc(code_arena, ctx.emit.capacity);
    ctx.local_desc = dyn_array_create(uint8_t, 32);

    // patch place for the code size of the function
    EMIT4(&ctx.emit, 0x00808080);
    EMIT4(&ctx.emit, 0x00808080);

    ctx.locals = tb_arena_alloc(arena, f->node_count * sizeof(ValueDesc));
    FOREACH_N(i, 0, f->node_count) {
        ctx.locals[i].id = -1;
    }

    FOR_USERS(u, f->root_node) if (u->n->type == TB_PROJ) {
        int i = TB_NODE_GET_EXTRA_T(u->n, TB_NodeProj)->index;
        if (i >= 3) {
            // params fit into the first few locals
            ctx.locals[u->n->gvn].id = i - 3;
            ctx.locals[u->n->gvn].uses = use_count(u->n);
            dyn_array_put(ctx.local_desc, get_wasm_type(u->n->dt));
        }
    }

    if (features == NULL) {
        ctx.features = (TB_FeatureSet){ 0 };
    } else {
        ctx.features = *features;
    }

    Worklist* restrict ws = &p->worklist;

    // legalize step takes out any of our 16bit and 8bit math ops
    tb_pass_prep(p);
    tb_pass_legalize(p, f->super.module->target_arch);

    worklist_clear(ws);

    CUIK_TIMED_BLOCK("global sched") {
        // We need to generate a CFG
        ctx.cfg = tb_compute_rpo(f, p);
        ctx.block_count = dyn_array_length(ws->items);
        // And perform global scheduling
        tb_pass_schedule(p, ctx.cfg, false);
    }

    DomTree* doms = ctx.doms = tb_arena_alloc(tmp_arena, ctx.cfg.block_count * sizeof(DomTree));
    FOREACH_N(i, 0, ctx.cfg.block_count) {
        doms[i].id    = i;
        doms[i].elem  = NULL;
        doms[i].start = NULL;
        doms[i].end   = NULL;
        doms[i].kids  = NULL;
    }

    FOREACH_N(i, 0, ctx.cfg.block_count) {
        TB_BasicBlock* bb = ctx.p->scheduled[ws->items[i]->gvn];
        doms[i].start = ws->items[i];
        doms[i].end   = bb->end;

        dyn_array_put(doms[bb->dom->id].kids, &doms[bb->id]);
    }

    FOREACH_N(i, 0, ctx.cfg.block_count) {
        DomTree* node = &ctx.doms[i];
        qsort(node->kids, dyn_array_length(node->kids), sizeof(DomTree*), dom_sort_cmp);
    }

    CUIK_TIMED_BLOCK("emit") {
        worklist_clear_visited(ws);

        WasmElem* root = do_dom_tree(&ctx, &doms[0]);
        #if TB_OPTDEBUG_CODEGEN
        render_block(&ctx, root, 0);
        #endif
        compile_block(&ctx, root, 0);
    }

    tb_free_cfg(&ctx.cfg);
    dyn_array_destroy(ctx.phi_vals);

    // uleb code size patch
    patch_uint(ctx.emit.data,     ctx.emit.count - 4);
    patch_uint(&ctx.emit.data[4], 0);

    // trim code arena (it fits in a single chunk so just arena free the top)
    code_arena->watermark = (char*) &ctx.emit.data[ctx.emit.count];
    tb_arena_realign(code_arena);

    // TODO(NeGate): move the assembly output to code arena
    if (emit_asm) CUIK_TIMED_BLOCK("dissassembly") {
        // __debugbreak();
    }

    log_debug("%s: code_arena=%.1f KiB", f->super.name, tb_arena_current_size(code_arena) / 1024.0f);
    tb_arena_restore(arena, sp);
    p->scheduled = NULL;

    // we're done, clean up
    func_out->asm_out = ctx.emit.head_asm;
    func_out->code = ctx.emit.data;
    func_out->code_size = ctx.emit.count;
    // func_out->locations = ctx.locations;
    // func_out->stack_slots = ctx.debug_stack_slots;
    // func_out->stack_usage = ctx.stack_usage;
    // func_out->prologue_length = ctx.prologue_length;
    // func_out->epilogue_length = ctx.epilogue_length;
}

static size_t emit_call_patches(TB_Module* restrict m, TB_FunctionOutput* out_f) {
    size_t r = 0;
    uint32_t src_section = out_f->section;

    for (TB_SymbolPatch* patch = out_f->first_patch; patch; patch = patch->next) {
        if (patch->target->tag == TB_SYMBOL_FUNCTION) {
            uint32_t dst_section = ((TB_Function*) patch->target)->output->section;

            // we can relocate across section on WASM, the "object" file isn't really
            // relocatable in the same sense as other objects.
            assert(patch->pos < out_f->code_size);
            size_t actual_pos = out_f->code_pos + patch->pos + 4;

            uint32_t p = ((TB_Function*) patch->target)->output->code_pos - actual_pos;
            patch_uint(&out_f->code[patch->pos], p);

            r += 1;
            patch->internal = true;
        }
    }

    return out_f->patch_count - r;
}

static void get_data_type_size(TB_DataType dt, size_t* out_size, size_t* out_align) {
    switch (dt.type) {
        case TB_INT: {
            // above 64bits we really dont care that much about natural alignment
            bool is_big_int = dt.data > 64;

            // round up bits to a byte
            int bits = is_big_int ? ((dt.data + 7) / 8) : tb_next_pow2(dt.data - 1);

            *out_size  = ((bits+7) / 8);
            *out_align = is_big_int ? 8 : ((dt.data + 7) / 8);
            break;
        }
        case TB_FLOAT: {
            int s = 0;
            if (dt.data == TB_FLT_32) s = 4;
            else if (dt.data == TB_FLT_64) s = 8;
            else tb_unreachable();

            *out_size = s;
            *out_align = s;
            break;
        }
        case TB_PTR: {
            *out_size = 8;
            *out_align = 8;
            break;
        }
        default: tb_unreachable();
    }
}

ICodeGen tb__wasm32_codegen = {
    .minimum_addressable_size = 8,
    .pointer_size = 32,
    .emit_win64eh_unwind_info = NULL,
    .emit_call_patches  = emit_call_patches,
    .get_data_type_size = get_data_type_size,
    .compile_function   = compile_function,
};
#endif
