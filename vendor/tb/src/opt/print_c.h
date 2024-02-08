
#include <new_hash_map.h>
#include <hash_map.h>
#include <dyn_array.h>

typedef struct nl_buffer_t nl_buffer_t;

nl_buffer_t *nl_buffer_new(void *arena);

void nl_buffer_format(nl_buffer_t *buf, const char *fmt, ...);
char *nl_buffer_get(nl_buffer_t *buf);
char *nl_buffer_copy(nl_buffer_t *buf);

typedef struct {
    size_t gvn;
    size_t label;
    bool used: 1;
} CFmtFrame;

typedef struct {
    size_t gvn;
    size_t label;
    bool used: 1;
} CFmtFallNext;

typedef struct {
    size_t low;
    size_t high;
} CFmtBlockRange;

typedef struct {
    const char *name;
    TB_Passes* opt;
    TB_Function* f;
    TB_Module *module;
    size_t a;
    size_t num_labels;
    DynArray(CFmtFrame *) visited_blocks;
    NL_Table block_ranges;
    TB_CFG cfg;
    TB_Scheduler sched;
    NL_HashSet completed_blocks;
    NL_HashSet needed_blocks;
    DynArray(size_t) declared_vars;
    nl_buffer_t *globals;
    nl_buffer_t *pre;
    nl_buffer_t *buf;
    TB_Node *return_block;
    void *arena;
    ptrdiff_t loop_goes_to;
    int depth;
} CFmtState;

static void c_fmt_ref_to_node(CFmtState* ctx, TB_Node* n);
static void c_fmt_typed_ref_to_node(CFmtState* ctx, TB_DataType dt, TB_Node* n);
static void c_fmt_bb(CFmtState* ctx, TB_Node* bb_start);

static void c_fmt_spaces(CFmtState *ctx) {
    for (unsigned char i = 0; i < ctx->depth; i++) {
        nl_buffer_format(ctx->buf, "  ");
    }
}

static const char *c_fmt_type_name(TB_DataType dt) {
    switch (dt.type) {
        case TB_INT: {
            if (dt.data == 0) return  "void";
            if (dt.data == 1) return  "char";
            if (dt.data <= 8) return  "uint8_t";
            if (dt.data <= 16) return  "uint16_t";
            if (dt.data <= 32) return  "uint32_t";
            if (dt.data <= 64) return  "uint64_t";
            else __builtin_trap();
            break;
        }
        case TB_PTR: {
            if (dt.data == 0) return  "void*";
            else tb_todo();
            break;
        }
        case TB_FLOAT: {
            if (dt.data == TB_FLT_32) return  "float";
            if (dt.data == TB_FLT_64) return  "double";
            break;
        }
        case TB_TUPLE: {
            tb_todo();
            break;
        }
        case TB_CONTROL: {
            tb_todo();
            break;
        }
        case TB_MEMORY: {
            tb_todo();
            break;
        }
        default: {
            tb_todo();
            break;
        }
    }
    return "void *";
}

static const char *c_fmt_type_name_signed(TB_DataType dt) {
    switch (dt.type) {
        case TB_INT: {
            if (dt.data == 0) return  "void";
            if (dt.data == 1) return  "char";
            if (dt.data <= 8) return  "int8_t";
            if (dt.data <= 16) return  "int16_t";
            if (dt.data <= 32) return  "int32_t";
            if (dt.data <= 64) return  "int64_t";
            else __builtin_trap();
            break;
        }
        case TB_PTR: {
            if (dt.data == 0) return  "void*";
            else tb_todo();
            break;
        }
        case TB_FLOAT: {
            if (dt.data == TB_FLT_32) return  "float";
            if (dt.data == TB_FLT_64) return  "double";
            break;
        }
        case TB_TUPLE: {
            tb_todo();
            break;
        }
        case TB_CONTROL: {
            tb_todo();
            break;
        }
        case TB_MEMORY: {
            tb_todo();
            break;
        }
        default: {
            tb_todo();
            break;
        }
    }
    return "void *";
}

static void c_fmt_output(CFmtState* ctx, TB_Node* n) {
    c_fmt_spaces(ctx);
    if (n->users != NULL) {
        FOREACH_N(i, 0, dyn_array_length(ctx->declared_vars)) {
            if (ctx->declared_vars[i] == n->gvn) {
                nl_buffer_format(ctx->buf, "v%u = ", n->gvn);
                return;
            }
        }
        nl_buffer_format(ctx->buf, "%s v%u = ", c_fmt_type_name(n->dt), n->gvn);
        dyn_array_put(ctx->declared_vars, n->gvn);
    }
}

static bool c_fmt_will_inline(TB_Node *n) {
    // if (n->type == TB_INTEGER_CONST || n->type == TB_FLOAT32_CONST ||
    //     n->type == TB_FLOAT64_CONST || n->type == TB_SYMBOL ||
    //     n->type == TB_SIGN_EXT || n->type == TB_ZERO_EXT ||
    //     n->type == TB_PROJ || n->type == TB_REGION ||
    //     n->type == TB_NULL ||
    //     n->type == TB_PHI) {
    //     return true;
    // }
    if (n->type == TB_ROOT) {
        return true;
    }
    if (n->type == TB_PROJ && n->dt.type == TB_CONTROL) {
        return true;
    }
    if (n->type == TB_REGION) {
        return true;
    }
    if (n->type == TB_FLOAT32_CONST) {
        return true;
    }
    if (n->type == TB_FLOAT64_CONST) {
        return true;
    }
    if (n->type == TB_SYMBOL) {
        return true;
    }
    if (n->type == TB_ZERO_EXT) {
        return true;
    }
    if (n->type == TB_SIGN_EXT) {
        return true;
    }
    if (n->type == TB_INTEGER_CONST) {
        return true;
    }
    size_t len = 0;
    for (User *head = n->users; head != NULL; head = head->next) {
        len += 1;
    }
    if (len <= 1) {
        if (n->type == TB_SELECT) {
            return true;
        }
        if (n->type == TB_SHL || n->type == TB_SHR || n->type == TB_SAR) {
            return true;
        }
        if (n->type == TB_AND || n->type == TB_OR || n->type == TB_XOR) {
            return true;
        }
        if (n->type == TB_ADD || n->type == TB_FADD) {
            return true;
        }
        if (n->type == TB_SUB || n->type == TB_FSUB) {
            return true;
        }
        if (n->type == TB_MUL || n->type == TB_FMUL) {
            return true;
        }
        if (n->type == TB_SDIV || n->type == TB_UDIV || n->type == TB_FDIV) {
            return true;
        }
        if (n->type == TB_CMP_EQ) {
            return true;
        }
        if (n->type == TB_CMP_NE) {
            return true;
        }
        if (n->type == TB_CMP_SLT || n->type == TB_CMP_ULT || n->type == TB_CMP_FLT) {
            return true;
        }
        if (n->type == TB_CMP_SLE || n->type == TB_CMP_ULE || n->type == TB_CMP_FLE) {
            return true;
        }
    }
    return false;
}

static void c_fmt_inline_node(CFmtState* ctx, TB_Node *n) {
    if (n->type == TB_SELECT) {
        TB_Node *cond = n->inputs[n->input_count-3];
        TB_Node *then = n->inputs[n->input_count-2];
        TB_Node *els = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        nl_buffer_format(ctx->buf, "(char) ");
        c_fmt_ref_to_node(ctx, cond);
        nl_buffer_format(ctx->buf, " ? ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, then);
        nl_buffer_format(ctx->buf, " : ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, els);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_SHL) {
        TB_Node *lhs = n->inputs[n->input_count-2];
        TB_Node *rhs = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, lhs);
        nl_buffer_format(ctx->buf, " << ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, rhs);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_SHR || n->type == TB_SAR) {
        TB_Node *lhs = n->inputs[n->input_count-2];
        TB_Node *rhs = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, lhs);
        nl_buffer_format(ctx->buf, " >> ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, rhs);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_AND) {
        TB_Node *lhs = n->inputs[n->input_count-2];
        TB_Node *rhs = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, lhs);
        nl_buffer_format(ctx->buf, " & ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, rhs);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_OR) {
        TB_Node *lhs = n->inputs[n->input_count-2];
        TB_Node *rhs = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, lhs);
        nl_buffer_format(ctx->buf, " | ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, rhs);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_XOR) {
        TB_Node *lhs = n->inputs[n->input_count-2];
        TB_Node *rhs = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, lhs);
        nl_buffer_format(ctx->buf, " ^ ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, rhs);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_ADD || n->type == TB_FADD) {
        TB_Node *lhs = n->inputs[n->input_count-2];
        TB_Node *rhs = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, lhs);
        nl_buffer_format(ctx->buf, " + ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, rhs);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_SUB || n->type == TB_FSUB) {
        TB_Node *lhs = n->inputs[n->input_count-2];
        TB_Node *rhs = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, lhs);
        nl_buffer_format(ctx->buf, " - ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, rhs);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_MUL || n->type == TB_FMUL) {
        TB_Node *lhs = n->inputs[n->input_count-2];
        TB_Node *rhs = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, lhs);
        nl_buffer_format(ctx->buf, " * ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, rhs);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_SDIV) {
        TB_Node *lhs = n->inputs[n->input_count-2];
        TB_Node *rhs = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name_signed(n->dt));
        c_fmt_ref_to_node(ctx, lhs);
        nl_buffer_format(ctx->buf, " / ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name_signed(n->dt));
        c_fmt_ref_to_node(ctx, rhs);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_UDIV) {
        TB_Node *lhs = n->inputs[n->input_count-2];
        TB_Node *rhs = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, lhs);
        nl_buffer_format(ctx->buf, " / ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        c_fmt_ref_to_node(ctx, rhs);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_FDIV) {
        TB_Node *lhs = n->inputs[n->input_count-2];
        TB_Node *rhs = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name_signed(n->dt));
        c_fmt_ref_to_node(ctx, lhs);
        nl_buffer_format(ctx->buf, " / ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name_signed(n->dt));
        c_fmt_ref_to_node(ctx, rhs);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_CMP_EQ) {
        TB_Node *lhs = n->inputs[n->input_count-2];
        TB_Node *rhs = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        c_fmt_ref_to_node(ctx, lhs);
        nl_buffer_format(ctx->buf, " == ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name_signed(lhs->dt));
        c_fmt_ref_to_node(ctx, rhs);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_CMP_NE) {
        TB_Node *lhs = n->inputs[n->input_count-2];
        TB_Node *rhs = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        c_fmt_ref_to_node(ctx, lhs);
        nl_buffer_format(ctx->buf, " != ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name_signed(lhs->dt));
        c_fmt_ref_to_node(ctx, rhs);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_CMP_SLT) {
        TB_Node *lhs = n->inputs[n->input_count-2];
        TB_Node *rhs = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        c_fmt_ref_to_node(ctx, lhs);
        nl_buffer_format(ctx->buf, " < ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name_signed(lhs->dt));
        c_fmt_ref_to_node(ctx, rhs);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_CMP_SLE) {
        TB_Node *lhs = n->inputs[n->input_count-2];
        TB_Node *rhs = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        c_fmt_ref_to_node(ctx, lhs);
        nl_buffer_format(ctx->buf, " <= ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name_signed(lhs->dt));
        c_fmt_ref_to_node(ctx, rhs);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_CMP_ULT) {
        TB_Node *lhs = n->inputs[n->input_count-2];
        TB_Node *rhs = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        c_fmt_ref_to_node(ctx, lhs);
        nl_buffer_format(ctx->buf, " < ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name_signed(lhs->dt));
        c_fmt_ref_to_node(ctx, rhs);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_CMP_ULE) {
        TB_Node *lhs = n->inputs[n->input_count-2];
        TB_Node *rhs = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        c_fmt_ref_to_node(ctx, lhs);
        nl_buffer_format(ctx->buf, " <= ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name_signed(lhs->dt));
        c_fmt_ref_to_node(ctx, rhs);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_CMP_FLT) {
        TB_Node *lhs = n->inputs[n->input_count-2];
        TB_Node *rhs = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        c_fmt_ref_to_node(ctx, lhs);
        nl_buffer_format(ctx->buf, " < ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(lhs->dt));
        c_fmt_ref_to_node(ctx, rhs);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_CMP_FLE) {
        TB_Node *lhs = n->inputs[n->input_count-2];
        TB_Node *rhs = n->inputs[n->input_count-1];
        nl_buffer_format(ctx->buf, "(");
        c_fmt_ref_to_node(ctx, lhs);
        nl_buffer_format(ctx->buf, " <= ");
        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(lhs->dt));
        c_fmt_ref_to_node(ctx, rhs);
        nl_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_FLOAT32_CONST) {
        TB_NodeFloat32* f = TB_NODE_GET_EXTRA(n);
        nl_buffer_format(ctx->buf, "%f", f->value);
    } else if (n->type == TB_FLOAT64_CONST) {
        TB_NodeFloat64* f = TB_NODE_GET_EXTRA(n);
        nl_buffer_format(ctx->buf, "%f", f->value);
    } else if (n->type == TB_SYMBOL) {
        TB_Symbol* sym = TB_NODE_GET_EXTRA_T(n, TB_NodeSymbol)->sym;
        if (sym->name[0]) {
            if (sym->tag == TB_SYMBOL_GLOBAL) {
                nl_buffer_format(ctx->buf, "tb2c_sym%zu_%s", sym->symbol_id, sym->name);
            } else {
                nl_buffer_format(ctx->buf, "%s", sym->name);
            }
        } else if (ctx->module->is_jit) {
            void *addr = sym->address;
            nl_buffer_format(ctx->buf, "(void*)%p", TB_NODE_GET_EXTRA_T(n, TB_NodeSymbol)->sym->address);
        } else {
            switch((int) sym->tag) {
                case TB_SYMBOL_EXTERNAL: {
                    nl_buffer_format(ctx->buf, "(void*)0");
                    break;
                }
                case TB_SYMBOL_GLOBAL: {
                    TB_Global *g = (void *) sym;

                    uint8_t *data = tb_platform_heap_alloc(g->size);

                    memset(&data[g->pos], 0, g->size);
                    FOREACH_N(k, 0, g->obj_count) {
                        if (g->objects[k].type == TB_INIT_OBJ_REGION) {
                            assert(g->objects[k].offset + g->objects[k].region.size <= g->size);
                            memcpy(&data[g->pos + g->objects[k].offset], g->objects[k].region.ptr, g->objects[k].region.size);
                        }
                    }

                    nl_buffer_format(ctx->buf, "\"");
                    FOREACH_N(k, 0, g->size) {
                        if (data[k] == 0 && k + 1 == g->size) {
                            break;
                        }
                        if (32 <= data[k] && data[k] <= 126) {
                            nl_buffer_format(ctx->buf, "%c", data[k]);
                        } else if (data[k] == '\t') {
                            nl_buffer_format(ctx->buf, "\\t");
                        } else if (data[k] == '\r') {
                            nl_buffer_format(ctx->buf, "\\r");
                        } else if (data[k] == '\n') {
                            nl_buffer_format(ctx->buf, "\\n");
                        } else if (data[k] == ' ') {
                            nl_buffer_format(ctx->buf, " ");
                        } else {
                            nl_buffer_format(ctx->buf, "\\x%02x", data[k]);
                        }                  
                    }
                    nl_buffer_format(ctx->buf, "\"");

                    tb_platform_heap_free(data);
                    break;
                }
                case TB_SYMBOL_FUNCTION: {
                    nl_buffer_format(ctx->buf, "(void *) 0");
                    break;
                }
            }
        }
    } else if (n->type == TB_ZERO_EXT) {
        c_fmt_ref_to_node(ctx, n->inputs[1]);
    } else if (n->type == TB_SIGN_EXT) {
        c_fmt_ref_to_node(ctx, n->inputs[1]);
    } else if (n->type == TB_INTEGER_CONST) {
        TB_NodeInt* num = TB_NODE_GET_EXTRA(n);

        nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
        nl_buffer_format(ctx->buf, "%"PRIi64"llu", num->value);
    } else {
        assert(false && "wrong node type");
    }
}

static void c_fmt_ref_to_node(CFmtState* ctx, TB_Node* n) {
    if (c_fmt_will_inline(n)) {
        c_fmt_inline_node(ctx, n);
    } else {
        nl_buffer_format(ctx->buf, "v%u", n->gvn);
        return;
    }
}

static void c_fmt_typed_ref_to_node(CFmtState* ctx, TB_DataType dt, TB_Node *n) {
    nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(dt));
    c_fmt_ref_to_node(ctx, n);
}

CFmtBlockRange *c_fmt_get_block_range(CFmtState* ctx, TB_Node* n) {
    if (nl_table_get(&ctx->block_ranges, n) == NULL) {
        TB_BasicBlock* bb = ctx->opt->scheduled[n->gvn];
        Worklist* ws = &ctx->opt->worklist;
        
        size_t foreach_start = dyn_array_length(ws->items);
        ctx->sched(ctx->opt, &ctx->cfg, ws, NULL, bb, bb->end);
        size_t foreach_end = dyn_array_length(ws->items);

        CFmtBlockRange *range = tb_platform_heap_alloc(sizeof(CFmtBlockRange));

        range->low = foreach_start;
        range->high = foreach_end;

        nl_table_put(&ctx->block_ranges, n, range);
    }
    return nl_table_get(&ctx->block_ranges, n);
}

TB_Node *c_fmt_only_return(CFmtState *ctx, TB_Node *target) {
    Worklist* ws = &ctx->opt->worklist;
    CFmtBlockRange *range = c_fmt_get_block_range(ctx, target);

    FOREACH_N(i, range->low, range->high) {
        TB_Node* n = ws->items[i];

        // skip these
        if (c_fmt_will_inline(n) && n->type != TB_RETURN) {
            continue;
        }

        if (n->type == TB_NULL || n->type == TB_PHI || n->type == TB_PROJ || n->type == TB_CALLGRAPH) {
            continue;
        }

        if (n->type == TB_RETURN) {
            return n;
        } else {
            return NULL;
        }
    }
    return NULL;
}

// deals with printing BB params
static void c_fmt_branch_edge(CFmtState* ctx, TB_Node* n, bool fallthru) {
    TB_Node* target = fallthru ? cfg_next_control(n) : cfg_next_bb_after_cproj(n);

    // TB_Node *node = c_fmt_only_return(ctx, target);
    // if (node != NULL) {
    //     int phi_i = -1;
    //     FOR_USERS(u, n) {
    //         if (u->n->type == TB_REGION) {
    //             phi_i = 1 + u->slot;
    //             break;
    //         }
    //     }

    //     NL_Table table = nl_table_alloc(4);
    //     FOR_USERS(u, target) {
    //         if (phi_i >= u->n->input_count) {
    //             continue;
    //         }
    //         if (u->n->inputs[phi_i]->dt.type != TB_CONTROL && u->n->inputs[phi_i]->dt.type != TB_MEMORY) {
    //             nl_table_put(&table, u->n, u->n->inputs[phi_i]);
    //         }
    //     }
    //     size_t count = 0;
    //     FOREACH_N(i, 3, node->input_count) {
    //         count += 1;
    //     }
    //     if (count == 0) {
    //         c_fmt_spaces(ctx);
    //         nl_buffer_format(ctx->buf, "return;\n");
    //         return;
    //     } else if (count == 1) {
    //         c_fmt_spaces(ctx);
    //         nl_buffer_format(ctx->buf, "return ");
    //         FOREACH_N(i, 3, node->input_count) {
    //             TB_Node *ent = nl_table_get(&table, node->inputs[i]);
    //             if (ent == NULL) {
    //                 ent = node->inputs[i];
    //             }
    //             c_fmt_ref_to_node(ctx, ent);
    //         }
    //         nl_buffer_format(ctx->buf, ";\n");
    //         return;
    //     } else {
    //         c_fmt_spaces(ctx);
    //         nl_buffer_format(ctx->buf, "{\n");
    //         c_fmt_spaces(ctx);
    //         nl_buffer_format(ctx->buf, "  tb2c_%s_ret_t ret;\n", ctx->name);
            
    //         bool index = 0;
    //         FOREACH_N(i, 3, n->input_count) {
    //             c_fmt_spaces(ctx);
    //             nl_buffer_format(ctx->buf, "  ret.v%zu = ", index);
    //             TB_Node *ent = nl_table_get(&table, node->inputs[i]);
    //             if (ent == NULL) {
    //                 ent = node->inputs[i];
    //             }
    //             c_fmt_ref_to_node(ctx, ent);
    //             nl_buffer_format(ctx->buf, ";\n");
    //             index += 1;
    //         }
    //         c_fmt_spaces(ctx);
    //         nl_buffer_format(ctx->buf, "  return ret;\n");
    //         c_fmt_spaces(ctx);
    //         nl_buffer_format(ctx->buf, "}\n");
    //         return;
    //     }
    // }

    // print phi args
    if (target->type == TB_REGION) {
        int phi_i = -1;
        FOR_USERS(u, n) {
            if (u->n->type == TB_REGION) {
                phi_i = 1 + u->slot;
                break;
            }
        }

        if (target->users != NULL) {
            size_t has_phi = 0;
            FOR_USERS(u, target) {
                if (u->n->type == TB_PHI) {
                    if (u->n->inputs[phi_i] != NULL) {
                        if (u->n->inputs[phi_i]->dt.type != TB_CONTROL && u->n->inputs[phi_i]->dt.type != TB_MEMORY) {
                            has_phi += 1;
                        }
                    }
                }
            }
            ctx->a += has_phi;
            size_t pos = 0;
            FOR_USERS(u, target) {
                if (u->n->type == TB_PHI) {
                    if (u->n->inputs[phi_i] != NULL) {
                        if (u->n->inputs[phi_i]->dt.type != TB_CONTROL && u->n->inputs[phi_i]->dt.type != TB_MEMORY) {
                            assert(phi_i >= 0);
                            if (pos != 0) {
                                c_fmt_spaces(ctx);
                                nl_buffer_format(ctx->buf, "%s a%zu = ", c_fmt_type_name(u->n->dt), ctx->a + pos);
                                c_fmt_ref_to_node(ctx, u->n->inputs[phi_i]);
                                nl_buffer_format(ctx->buf, ";\n");
                            }
                            pos += 1;
                        }
                    }
                }
            }
            pos = 0;
            FOR_USERS(u, target) {
                if (u->n->type == TB_PHI) {
                    if (u->n->inputs[phi_i] != NULL) {
                        if (u->n->inputs[phi_i]->dt.type != TB_CONTROL && u->n->inputs[phi_i]->dt.type != TB_MEMORY) {
                            assert(phi_i >= 0);
                            if (pos == 0) {
                                c_fmt_output(ctx, u->n);
                                c_fmt_ref_to_node(ctx, u->n->inputs[phi_i]);
                                nl_buffer_format(ctx->buf, ";\n");
                            } else {
                                c_fmt_output(ctx, u->n);
                                nl_buffer_format(ctx->buf, "a%zu;\n", ctx->a + pos);
                            }
                            pos += 1;
                        }
                    }
                }
            }
        }
    }

    FOREACH_N(i, 0, dyn_array_length(ctx->visited_blocks)) {
        CFmtFrame *next = ctx->visited_blocks[i];
        if (next->gvn == target->gvn) {
            next->used = true;
            c_fmt_spaces(ctx);
            nl_buffer_format(ctx->buf, "goto label%zu;\n", next->label);
            return;
        }
    }
    c_fmt_bb(ctx, target);
}

static void c_fmt_bb(CFmtState* ctx, TB_Node* bb_start) {
    CFmtFrame frame = (CFmtFrame) {
        .gvn = bb_start->gvn,
        .label = ctx->num_labels++,
        .used = false,
    };

    size_t declared_vars_length = dyn_array_length(ctx->declared_vars);

    nl_buffer_t *old_buf = ctx->buf;
    ctx->buf = nl_buffer_new(ctx->arena);

    dyn_array_put(ctx->visited_blocks, &frame);

    // c_fmt_spaces(ctx);
    // nl_buffer_format(ctx->buf, "/* bb%u */\n", bb_start->gvn);

    TB_BasicBlock* bb = ctx->opt->scheduled[bb_start->gvn];
    Worklist* ws = &ctx->opt->worklist;

    #ifndef NDEBUG
    TB_BasicBlock* expected = &nl_map_get_checked(ctx->cfg.node_to_block, bb_start);
    assert(expected == bb);
    #endif

    CFmtBlockRange *range = c_fmt_get_block_range(ctx, bb_start);

    TB_Node* prev_effect = NULL;
    FOREACH_N(i, range->low, range->high) {
        TB_Node* n = ws->items[i];

        // skip these
        if (c_fmt_will_inline(n) && n->type != TB_ROOT) {
            continue;
        }

        if (n->type == TB_MERGEMEM || n->type == TB_SPLITMEM || n->type == TB_NULL || n->type == TB_PHI || n->type == TB_PROJ) {
            continue;
        }

        switch (n->type) {
            case TB_DEAD: {
                c_fmt_spaces(ctx);
                nl_buffer_format(ctx->buf, "// dead\n");
                break;
            }

            case TB_DEBUGBREAK: {
                c_fmt_spaces(ctx);
                nl_buffer_format(ctx->buf, "// debugbreak\n");
                break;
            }
            case TB_UNREACHABLE: {
                c_fmt_spaces(ctx);
                nl_buffer_format(ctx->buf, "// unreachable\n");
                break;
            }

            case TB_BRANCH: {
                TB_NodeBranch* br = TB_NODE_GET_EXTRA(n);
                TB_Node** restrict succ = tb_arena_alloc(ctx->arena, br->succ_count * sizeof(TB_Node**));

                size_t succ_count = 0;

                // fill successors
                FOR_USERS(u, n) {
                    if (u->n->type == TB_PROJ) {
                        int index = TB_NODE_GET_EXTRA_T(u->n, TB_NodeProj)->index;
                        succ[index] = u->n;
                        succ_count += 1;
                    }
                }

                if (succ_count == 1) {
                    c_fmt_branch_edge(ctx, succ[0], false);
                } else if (succ_count == 2) {
                    if (br->keys[0] == 0) {
                        c_fmt_spaces(ctx);
                        nl_buffer_format(ctx->buf, "if (");
                        FOREACH_N(i, 1, n->input_count) {
                            if (i != 1) nl_buffer_format(ctx->buf, ", ");
                            c_fmt_ref_to_node(ctx, n->inputs[i]);
                        }
                        nl_buffer_format(ctx->buf, ") {\n");
                        ctx->depth += 1;
                        c_fmt_branch_edge(ctx, succ[0], false);
                        ctx->depth -= 1;
                        c_fmt_spaces(ctx);
                        nl_buffer_format(ctx->buf, "}\n");
                        c_fmt_branch_edge(ctx, succ[1], false);
                    } else {
                        c_fmt_spaces(ctx);
                        nl_buffer_format(ctx->buf, "if ((uint64_t) ");
                        FOREACH_N(i, 1, n->input_count) {
                            if (i != 1) nl_buffer_format(ctx->buf, ", ");
                            c_fmt_ref_to_node(ctx, n->inputs[i]);
                        }
                        nl_buffer_format(ctx->buf, " == (uint64_t) %"PRIi64, br->keys[0]);
                        nl_buffer_format(ctx->buf, ") {\n");
                        ctx->depth += 1;
                        c_fmt_branch_edge(ctx, succ[1], false);
                        ctx->depth -= 1;
                        c_fmt_spaces(ctx);
                        nl_buffer_format(ctx->buf, "}\n");
                        c_fmt_branch_edge(ctx, succ[0], false);
                    }
                } else {
                    c_fmt_spaces(ctx);
                    nl_buffer_format(ctx->buf, "switch ((uint64_t) ");
                    FOREACH_N(i, 1, n->input_count) {
                        if (i != 1) nl_buffer_format(ctx->buf, ", ");
                        c_fmt_ref_to_node(ctx, n->inputs[i]);
                    }
                    nl_buffer_format(ctx->buf, ") {\n");
                    FOREACH_N(i, 1, succ_count) {
                        c_fmt_spaces(ctx);
                        nl_buffer_format(ctx->buf, "case %"PRIi64"llu:\n", br->keys[i-1]);
                        ctx->depth += 1;
                        c_fmt_branch_edge(ctx, succ[i], false);
                        ctx->depth -= 1;
                    }
                    c_fmt_spaces(ctx);
                    nl_buffer_format(ctx->buf, "default:\n");
                    ctx->depth += 1;
                    c_fmt_branch_edge(ctx, succ[0], false);
                    ctx->depth -= 1;
                    c_fmt_spaces(ctx);
                    nl_buffer_format(ctx->buf, "}\n");
                }
                break;
            }

            case TB_TRAP: {
                c_fmt_spaces(ctx);
                nl_buffer_format(ctx->buf, "throw new Error(\"trap\");\n");
                break;
            }

            case TB_RETURN: {
                size_t count = 0;
                FOREACH_N(i, 3, n->input_count) {
                    count += 1;
                }
                if (count == 0) {
                    c_fmt_spaces(ctx);
                    nl_buffer_format(ctx->buf, "return;\n");
                } else if (count == 1) {
                    c_fmt_spaces(ctx);
                    nl_buffer_format(ctx->buf, "return ");
                    FOREACH_N(i, 3, n->input_count) {
                        c_fmt_ref_to_node(ctx, n->inputs[i]);
                    }
                    nl_buffer_format(ctx->buf, ";\n");
                } else {
                    c_fmt_spaces(ctx);
                    nl_buffer_format(ctx->buf, "{\n");
                    c_fmt_spaces(ctx);
                    nl_buffer_format(ctx->buf, "  tb2c_%s_ret_t ret;\n", ctx->name);
                    
                    bool index = 0;
                    FOREACH_N(i, 3, n->input_count) {
                        c_fmt_spaces(ctx);
                        nl_buffer_format(ctx->buf, "  ret.v%zu = ", index);
                        c_fmt_ref_to_node(ctx, n->inputs[i]);
                        nl_buffer_format(ctx->buf, ";\n");
                        index += 1;
                    }
                    c_fmt_spaces(ctx);
                    nl_buffer_format(ctx->buf, "  return ret;\n");
                    c_fmt_spaces(ctx);
                    nl_buffer_format(ctx->buf, "}\n");
                }
                break;
            }

            case TB_CALLGRAPH: {
                break;
            }

            case TB_STORE: {
                TB_Node *dest = n->inputs[n->input_count-2];
                TB_Node *src = n->inputs[n->input_count-1];
                c_fmt_spaces(ctx);
                nl_buffer_format(ctx->buf, "*(%s*) ", c_fmt_type_name(src->dt));
                c_fmt_ref_to_node(ctx, dest);
                nl_buffer_format(ctx->buf, " = ", c_fmt_type_name(src->dt));
                c_fmt_ref_to_node(ctx, src);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_NOT: {
                TB_Node *src = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                nl_buffer_format(ctx->buf, "~");
                c_fmt_ref_to_node(ctx, src);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_NEG: {
                TB_Node *src = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                nl_buffer_format(ctx->buf, "-");
                c_fmt_ref_to_node(ctx, src);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_LOAD: {
                TB_Node *src = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                nl_buffer_format(ctx->buf, "*(%s*) ", c_fmt_type_name(n->dt));
                c_fmt_ref_to_node(ctx, src);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }
            
            case TB_LOCAL: {
                dyn_array_put(ctx->declared_vars, n->gvn);
                TB_NodeLocal* l = TB_NODE_GET_EXTRA(n);
                c_fmt_spaces(ctx);
                nl_buffer_format(ctx->buf, "uint8_t v%u[0x%x];\n", n->gvn, l->size);
                break;
            }
            
            case TB_BITCAST: {
                TB_Node *src = n->inputs[n->input_count-1];
                if (src->dt.type == TB_FLOAT && n->dt.type == TB_FLOAT) {
                    c_fmt_spaces(ctx);
                    nl_buffer_format(ctx->buf, "{\n");
                    ctx->depth += 1;
                    c_fmt_spaces(ctx);
                    nl_buffer_format(ctx->buf, "union {%s src; %s dest;} tmp;\n", c_fmt_type_name(src->dt), c_fmt_type_name(n->dt));
                    c_fmt_spaces(ctx);
                    nl_buffer_format(ctx->buf, "tmp.src = ");
                    c_fmt_ref_to_node(ctx, src);
                    nl_buffer_format(ctx->buf, ";\n");
                    c_fmt_output(ctx, n);
                    nl_buffer_format(ctx->buf, "tmp.dest;\n");
                    ctx->depth -= 1;
                    c_fmt_spaces(ctx);
                    nl_buffer_format(ctx->buf, "}\n");
                } else {
                    c_fmt_output(ctx, n);
                    nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
                    c_fmt_ref_to_node(ctx, src);
                    nl_buffer_format(ctx->buf, ";\n");
                }
                break;
            }

            case TB_OR: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
                c_fmt_ref_to_node(ctx, lhs);
                nl_buffer_format(ctx->buf, " | ");
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
                c_fmt_ref_to_node(ctx, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_XOR: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
                c_fmt_ref_to_node(ctx, lhs);
                nl_buffer_format(ctx->buf, " ^ ");
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
                c_fmt_ref_to_node(ctx, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }
            case TB_SHR: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
                c_fmt_ref_to_node(ctx, lhs);
                nl_buffer_format(ctx->buf, " >> ");
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
                c_fmt_ref_to_node(ctx, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }
            case TB_SAR: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
                c_fmt_ref_to_node(ctx, lhs);
                nl_buffer_format(ctx->buf, " >> ");
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
                c_fmt_ref_to_node(ctx, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }
            case TB_SHL: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
                c_fmt_ref_to_node(ctx, lhs);
                nl_buffer_format(ctx->buf, " << ");
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
                c_fmt_ref_to_node(ctx, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_AND: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
                c_fmt_ref_to_node(ctx, lhs);
                nl_buffer_format(ctx->buf, " & ");
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
                c_fmt_ref_to_node(ctx, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_FADD:
            case TB_ADD: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                c_fmt_typed_ref_to_node(ctx, lhs->dt, lhs);
                nl_buffer_format(ctx->buf, " + ");
                c_fmt_typed_ref_to_node(ctx, rhs->dt, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }
            case TB_FSUB:
            case TB_SUB: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                c_fmt_typed_ref_to_node(ctx, lhs->dt, lhs);
                nl_buffer_format(ctx->buf, " - ");
                c_fmt_typed_ref_to_node(ctx, rhs->dt, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }
            case TB_FMUL:
            case TB_MUL: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                c_fmt_typed_ref_to_node(ctx, lhs->dt, lhs);
                nl_buffer_format(ctx->buf, " * ");
                c_fmt_typed_ref_to_node(ctx, rhs->dt, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }
            case TB_FDIV: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                c_fmt_typed_ref_to_node(ctx, lhs->dt, lhs);
                nl_buffer_format(ctx->buf, " / ");
                c_fmt_typed_ref_to_node(ctx, rhs->dt, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }
            case TB_SDIV: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name_signed(n->dt));
                c_fmt_typed_ref_to_node(ctx, lhs->dt, lhs);
                nl_buffer_format(ctx->buf, " / ");
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name_signed(n->dt));
                c_fmt_typed_ref_to_node(ctx, rhs->dt, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }
            case TB_UDIV: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                c_fmt_typed_ref_to_node(ctx, lhs->dt, lhs);
                nl_buffer_format(ctx->buf, " / ");
                c_fmt_typed_ref_to_node(ctx, rhs->dt, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }
            case TB_SMOD: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name_signed(n->dt));
                c_fmt_typed_ref_to_node(ctx, lhs->dt, lhs);
                nl_buffer_format(ctx->buf, " %% ");
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name_signed(n->dt));
                c_fmt_typed_ref_to_node(ctx, rhs->dt, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }
            case TB_UMOD: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                c_fmt_typed_ref_to_node(ctx, lhs->dt, lhs);
                nl_buffer_format(ctx->buf, " %% ");
                c_fmt_typed_ref_to_node(ctx, rhs->dt, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_CMP_EQ: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                c_fmt_typed_ref_to_node(ctx, lhs->dt, lhs);
                nl_buffer_format(ctx->buf, " == ");
                c_fmt_typed_ref_to_node(ctx, rhs->dt, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_CMP_NE: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                c_fmt_typed_ref_to_node(ctx, lhs->dt, lhs);
                nl_buffer_format(ctx->buf, " != ");
                c_fmt_typed_ref_to_node(ctx, rhs->dt, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_POISON: {
                break;
            }

            case TB_CMP_FLT: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                c_fmt_typed_ref_to_node(ctx, lhs->dt, lhs);
                nl_buffer_format(ctx->buf, " < ");
                c_fmt_typed_ref_to_node(ctx, rhs->dt, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_CMP_FLE: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                c_fmt_typed_ref_to_node(ctx, lhs->dt, lhs);
                nl_buffer_format(ctx->buf, " <= ");
                c_fmt_typed_ref_to_node(ctx, rhs->dt, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }
            

            case TB_CMP_SLT: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name_signed(lhs->dt));
                c_fmt_typed_ref_to_node(ctx, lhs->dt, lhs);
                nl_buffer_format(ctx->buf, " < ");
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name_signed(lhs->dt));
                c_fmt_typed_ref_to_node(ctx, rhs->dt, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_CMP_SLE: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name_signed(lhs->dt));
                c_fmt_typed_ref_to_node(ctx, lhs->dt, lhs);
                nl_buffer_format(ctx->buf, " <= ");
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name_signed(lhs->dt));
                c_fmt_typed_ref_to_node(ctx, rhs->dt, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }
            

            case TB_CMP_ULT: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                c_fmt_typed_ref_to_node(ctx, lhs->dt, lhs);
                nl_buffer_format(ctx->buf, " < ");
                c_fmt_typed_ref_to_node(ctx, rhs->dt, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_CMP_ULE: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                c_fmt_ref_to_node(ctx, lhs);
                nl_buffer_format(ctx->buf, " <= ");
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(lhs->dt));
                c_fmt_ref_to_node(ctx, rhs);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }
            
            case TB_MEMBER_ACCESS: {
                TB_Node *ptr = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                nl_buffer_format(ctx->buf, "(void*) ((char *) ");
                c_fmt_ref_to_node(ctx, ptr);
                nl_buffer_format(ctx->buf, " + %"PRIi64, TB_NODE_GET_EXTRA_T(n, TB_NodeMember)->offset);
                nl_buffer_format(ctx->buf, ");\n");
                break;
            }

            
            case TB_SELECT: {
                TB_Node *cond = n->inputs[n->input_count-3];
                TB_Node *then = n->inputs[n->input_count-2];
                TB_Node *els = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                c_fmt_ref_to_node(ctx, cond);
                nl_buffer_format(ctx->buf, " ? ");
                c_fmt_ref_to_node(ctx, then);
                nl_buffer_format(ctx->buf, " : ");
                c_fmt_ref_to_node(ctx, els);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_FLOAT2UINT:
            case TB_FLOAT2INT:
            case TB_UINT2FLOAT:
            case TB_INT2FLOAT:
            case TB_TRUNCATE: {
                TB_Node *input = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                nl_buffer_format(ctx->buf, "(%s) ", c_fmt_type_name(n->dt));
                c_fmt_ref_to_node(ctx, input);
                nl_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_ARRAY_ACCESS: {
                TB_Node *ptr = n->inputs[n->input_count-2];
                TB_Node *index = n->inputs[n->input_count-1];
                c_fmt_output(ctx, n);
                nl_buffer_format(ctx->buf, "(void*) ((char *) ");
                c_fmt_ref_to_node(ctx, ptr);
                nl_buffer_format(ctx->buf, " + ");
                c_fmt_ref_to_node(ctx, index);
                nl_buffer_format(ctx->buf, " * %"PRIi64, TB_NODE_GET_EXTRA_T(n, TB_NodeArray)->stride);
                nl_buffer_format(ctx->buf, ");\n");
                break;
            }
            case TB_CALL: {
                TB_Node *func = n->inputs[2];
                
                TB_Node* projs[4] = { 0 };
                FOR_USERS(use, n) {
                    if (use->n->type == TB_PROJ) {
                        int index = TB_NODE_GET_EXTRA_T(use->n, TB_NodeProj)->index;
                        projs[index] = use->n;
                    }
                }

                if (projs[2] == NULL) {
                    nl_buffer_format(ctx->globals, "typedef void(*tb2c_%s_v%u_t)(", ctx->name, n->gvn);
                } else if (projs[3] == NULL) {
                    nl_buffer_format(ctx->globals, "typedef %s(*tb2c_%s_v%u_t)(", c_fmt_type_name(projs[2]->dt), ctx->name, n->gvn);
                } else {
                    nl_buffer_format(ctx->globals, "typedef struct {\n");
                    FOREACH_N(i, 2, 4) {
                        if (projs[i] == NULL) break;
                        nl_buffer_format(ctx->globals, "  %s v%u;\n", c_fmt_type_name(projs[i]->dt), projs[i]->gvn);
                    }
                    nl_buffer_format(ctx->globals, "} tb2c_%s_v%u_ret_t;\n", ctx->name, n->gvn);
                    nl_buffer_format(ctx->globals, "typedef tb2c_%s_v%u_ret_t(*tb2c_%s_v%u_t)(", ctx->name, n->gvn, ctx->name, n->gvn);
                }
                {
                    bool first = true;
                    FOREACH_N(i, 3, n->input_count) {
                        if (n->inputs[i]->dt.type != TB_CONTROL && n->inputs[i]->dt.type != TB_MEMORY) {
                            if (!first) {
                                nl_buffer_format(ctx->globals, ", ");
                            }
                            nl_buffer_format(ctx->globals, "%s", c_fmt_type_name(n->inputs[i]->dt));
                            first = false;
                        }
                    }
                    if (first) {
                            nl_buffer_format(ctx->globals, "void");
                    }
                }
                nl_buffer_format(ctx->globals, ");\n");
                if (projs[2] == NULL || projs[3] == NULL) {
                    if (projs[2] != NULL) {
                        c_fmt_output(ctx, projs[2]);
                    } else {
                        c_fmt_spaces(ctx);
                    }
                    nl_buffer_format(ctx->buf, "((tb2c_%s_v%u_t) ", ctx->name, n->gvn);
                    c_fmt_ref_to_node(ctx, func);
                    nl_buffer_format(ctx->buf, ")");
                    nl_buffer_format(ctx->buf, "(");
                    {
                        bool first = true;
                        FOREACH_N(i, 3, n->input_count) {
                            if (n->inputs[i]->dt.type != TB_CONTROL && n->inputs[i]->dt.type != TB_MEMORY) {
                                if (!first) {
                                    nl_buffer_format(ctx->buf, ", ");
                                }
                                c_fmt_ref_to_node(ctx, n->inputs[i]);
                                first = false;
                            }
                        }
                    }
                    nl_buffer_format(ctx->buf, ");\n");
                } else {
                    c_fmt_spaces(ctx);
                    nl_buffer_format(ctx->buf, "tb2c_%s_v%u_ret_t v%u_ret = ", ctx->name, n->gvn, n->gvn);
                    nl_buffer_format(ctx->buf, "((tb2c_%s_v%u_t) ", ctx->name, n->gvn);
                    c_fmt_ref_to_node(ctx, func);
                    nl_buffer_format(ctx->buf, ")(");
                    {
                        bool first = true;
                        FOREACH_N(i, 3, n->input_count) {
                            if (n->inputs[i]->dt.type != TB_CONTROL && n->inputs[i]->dt.type != TB_MEMORY) {
                                if (!first) {
                                    nl_buffer_format(ctx->buf, ", ");
                                }
                                c_fmt_ref_to_node(ctx, n->inputs[i]);
                                first = false;
                            }
                        }
                    }
                    nl_buffer_format(ctx->buf, ");\n");
                    if (projs[2] != NULL) {
                        FOREACH_N(i, 2, 4) {
                            if (projs[i] == NULL) break;
                            c_fmt_output(ctx, projs[i]);
                            nl_buffer_format(ctx->buf, "v%u_ret.v%u;\n", n->gvn, projs[i]->gvn);
                        }
                    }
                }
                break;
            }

            case TB_MEMCPY: {
                TB_Node *dest = n->inputs[n->input_count-3];
                TB_Node *src = n->inputs[n->input_count-2];
                TB_Node *len = n->inputs[n->input_count-1];
                c_fmt_spaces(ctx);
                nl_buffer_format(ctx->buf, "memcpy(");
                c_fmt_ref_to_node(ctx, dest);
                nl_buffer_format(ctx->buf, ", ");
                c_fmt_ref_to_node(ctx, src);
                nl_buffer_format(ctx->buf, ", ");
                c_fmt_ref_to_node(ctx, len);
                nl_buffer_format(ctx->buf, ");\n");
                break;
            }

            case TB_MEMSET: {
                TB_Node *a = n->inputs[n->input_count-3];
                TB_Node *b = n->inputs[n->input_count-2];
                TB_Node *c = n->inputs[n->input_count-1];
                c_fmt_spaces(ctx);
                nl_buffer_format(ctx->buf, "memset(");
                c_fmt_ref_to_node(ctx, a);
                nl_buffer_format(ctx->buf, ", ");
                c_fmt_ref_to_node(ctx, b);
                nl_buffer_format(ctx->buf, ", ");
                c_fmt_ref_to_node(ctx, c);
                nl_buffer_format(ctx->buf, ");\n");
                break;
            }

            default: {
                fprintf(stderr, "internal unimplemented node type: %s\n", tb_node_get_name(n));
                fflush(stderr);
                __builtin_trap();
                break;
            }
        }
    }

    // dyn_array_set_length(ws->items, ctx->cfg.block_count);

    if (!cfg_is_terminator(bb->end)) {
        c_fmt_branch_edge(ctx, bb->end, true);
    }

    dyn_array_pop(ctx->visited_blocks);

    while (dyn_array_length(ctx->declared_vars) > declared_vars_length) {
        dyn_array_pop(ctx->declared_vars);
    }

    nl_buffer_t *buf = ctx->buf;
    ctx->buf = old_buf;
    if (frame.used) {
        c_fmt_spaces(ctx);
        nl_buffer_format(ctx->buf, "label%zu:;\n", frame.label);
        c_fmt_spaces(ctx);
        nl_buffer_format(ctx->buf, "{\n  ");
        for (const char *c = nl_buffer_get(buf); *c != '\0'; c++) {
            if (*c == '\n') {
                nl_buffer_format(ctx->buf, "\n  ");
            } else {
                nl_buffer_format(ctx->buf, "%c", *c);
            }
        }
        ctx->depth -= 1;
        c_fmt_spaces(ctx);
        nl_buffer_format(ctx->buf, "}\n");
        ctx->depth += 1;
    } else {
        nl_buffer_format(ctx->buf, "%s", nl_buffer_get(buf));
    }
}

TB_API char *tb_pass_c_prelude(TB_Module *mod) {
    size_t n_private_syms = 0;

    void *arena = tb_arena_create(TB_ARENA_MEDIUM_CHUNK_SIZE);

    nl_buffer_t *buf = nl_buffer_new(arena);

    nl_buffer_format(buf, "typedef signed char int8_t;\n");
    nl_buffer_format(buf, "typedef unsigned char uint8_t;\n");
    nl_buffer_format(buf, "typedef signed short int16_t;\n");
    nl_buffer_format(buf, "typedef unsigned short uint16_t;\n");
    nl_buffer_format(buf, "typedef signed int int32_t;\n");
    nl_buffer_format(buf, "typedef unsigned int uint32_t;\n");
    if (sizeof(long) == 8) {
        nl_buffer_format(buf, "typedef signed long int64_t;\n");
        nl_buffer_format(buf, "typedef unsigned long uint64_t;\n");
    } else {
        nl_buffer_format(buf, "typedef signed long long int64_t;\n");
        nl_buffer_format(buf, "typedef unsigned long long uint64_t;\n");
    }
    if (sizeof(size_t) == sizeof(uint64_t)) {
        nl_buffer_format(buf, "typedef uint64_t size_t;\n");
    }
    if (sizeof(size_t) == sizeof(uint32_t)) {
        nl_buffer_format(buf, "typedef uint32_t size_t;\n");
    }
    nl_buffer_format(buf, "void *memcpy(void *dest, const void *src, size_t n);\n");
    nl_buffer_format(buf, "void *memset(void *str, int c, size_t n);\n");
    nl_buffer_format(buf, "\n");
    NL_HashSet* syms = &tb_thread_info(mod)->symbols;
    nl_hashset_for(sym_vp, syms) {
        TB_Symbol *sym = *sym_vp;
        if (sym->name == NULL ||sym->name[0] == '\0') {
            continue;
        }
        switch ((int) sym->tag) {
            case TB_SYMBOL_EXTERNAL: {
                if (mod->is_jit) {
                    nl_buffer_format(buf, "void *%s = (void *) 0x%zx;\n", sym->name, (size_t) sym->address);
                } else {
                    nl_buffer_format(buf, "extern void %s(void);\n", sym->name);
                }
                break;
            }
            case TB_SYMBOL_GLOBAL: {
                TB_Global *g = (void *) sym;
                if (g->linkage == TB_LINKAGE_PRIVATE) {
                    nl_buffer_format(buf, "static ");
                    sym->symbol_id = ++n_private_syms;
                } else {
                    sym->symbol_id = 0;
                }
                if (g->obj_count == 0) {
                    nl_buffer_format(buf, "uint8_t tb2c_sym%zu_%s[%zu];\n", (size_t) sym->symbol_id, sym->name, (size_t) g->size);
                } else {
                    uint8_t *data_buf = tb_platform_heap_alloc(sizeof(uint8_t) * g->size);
                    memset(data_buf, 0, sizeof(uint8_t) * g->size);
                    FOREACH_N(k, 0, g->obj_count) {
                        if (g->objects[k].type == TB_INIT_OBJ_REGION) {
                            assert(g->objects[k].offset + g->objects[k].region.size <= g->size);
                            memcpy(&data_buf[g->pos + g->objects[k].offset], g->objects[k].region.ptr, g->objects[k].region.size);
                        }
                    }
                    nl_buffer_format(buf, "uint8_t tb2c_sym%zu_%s[%zu] = {\n", (size_t) sym->symbol_id, sym->name, (size_t) g->size);
                    for (size_t i = 0; i < g->size; i++) {
                        nl_buffer_format(buf, "  0x%02x,\n", data_buf[i]);
                    }
                    nl_buffer_format(buf, "};\n");
                }
                // nl_buffer_format(buf, "// global: %s\n", sym->name);
                break;
            }
            case TB_SYMBOL_FUNCTION: {
                TB_Function *f = (void *) sym;
                TB_FunctionPrototype* p = f->prototype;
                if (p->return_count == 0) {
                    nl_buffer_format(buf, "typedef void tb2c_%s_ret_t;\n", sym->name);
                } else if (p->return_count == 1) {
                    if (p->params[p->param_count].dt.type == TB_INT && p->params[p->param_count].dt.data == 32 && !strcmp(sym->name, "main")) {
                        nl_buffer_format(buf, "#define tb2c_%s_ret_t int\n", sym->name);
                        nl_buffer_format(buf, "#define main(...) main(int v4, char **v5)\n");
                    } else {
                        nl_buffer_format(buf, "typedef %s tb2c_%s_ret_t;\n", c_fmt_type_name(p->params[p->param_count].dt), sym->name);
                    }
                } else {
                    nl_buffer_format(buf, "typedef struct {\n");
                    
                    bool index = 0;
                    FOREACH_N(i, 0, p->return_count) {
                        nl_buffer_format(buf, "  %s v%zu;\n", c_fmt_type_name(p->params[p->param_count + i].dt), index);
                        index += 1;
                    }
                    nl_buffer_format(buf, "} tb2c_%s_ret_t;\n", sym->name);
                }
                if (f->linkage == TB_LINKAGE_PRIVATE) {
                    nl_buffer_format(buf, "static ");
                }
                nl_buffer_format(buf, "tb2c_%s_ret_t %s(", sym->name, sym->name);
                TB_Node** params = f->params;
                size_t count = 0;
                FOREACH_N(i, 3, 3 + f->param_count) {
                    if (params[i] != NULL && params[i]->dt.type != TB_MEMORY && params[i]->dt.type != TB_CONTROL && params[i]->dt.type != TB_TUPLE) {
                        if (count != 0) {
                            nl_buffer_format(buf, ", ");
                        }
                        nl_buffer_format(buf, "%s v%u", c_fmt_type_name(params[i]->dt), params[i]->gvn);
                        count += 1;
                    }
                }
                if (count == 0) {
                    nl_buffer_format(buf, "void");
                }
                nl_buffer_format(buf, ");\n");
                break;
            }
        }
    }

    char *ret = nl_buffer_get(buf);

    // tb_arena_destroy(arena);

    return ret;
}

TB_API char *tb_pass_c_fmt(TB_Passes* opt) {
    TB_Function* f = opt->f;
    const char *name = f->super.name;
    cuikperf_region_start("print", NULL);

    Worklist old = opt->worklist;
    Worklist tmp_ws = { 0 };
    worklist_alloc(&tmp_ws, f->node_count);

    CFmtState ctx = { name, opt, f, f->super.module };
    
    ctx.arena = tb_arena_create(TB_ARENA_MEDIUM_CHUNK_SIZE);

    ctx.globals = nl_buffer_new(ctx.arena);

    ctx.buf = nl_buffer_new(ctx.arena);

    // ctx.global_funcs = nl_hashset_alloc(ctx.module->compiled_function_count);
    ctx.visited_blocks = dyn_array_create(size_t, 8);
    ctx.declared_vars = dyn_array_create(size_t, 16);
    ctx.needed_blocks = nl_hashset_alloc(ctx.cfg.block_count);
    ctx.completed_blocks = nl_hashset_alloc(ctx.cfg.block_count);
    ctx.block_ranges = nl_table_alloc(ctx.cfg.block_count);

    // nl_hashset_clear(&ctx.visited_blocks);

    opt->worklist = tmp_ws;
    ctx.cfg = tb_compute_rpo(f, opt);

    // does the IR printing need smart scheduling lol (yes... when we're testing things)
    ctx.sched = greedy_scheduler;


    // schedule nodes
    tb_pass_schedule(opt, ctx.cfg, false);
    worklist_clear_visited(&opt->worklist);

    ctx.return_block = opt->worklist.items[ctx.cfg.block_count - 1];

    // TB_Node* end_bb = NULL;
    nl_hashset_put(&ctx.needed_blocks, opt->worklist.items[0]);
    while (true) {
        bool any = false;
        FOREACH_N(i, 0, ctx.cfg.block_count) {
            // TB_Node* end = nl_map_get_checked(ctx.cfg.node_to_block, opt->worklist.items[i]).end;
            // if (end == f->root_node) {
            //     end_bb = opt->worklist.items[i];
            //     continue;
            // }

            if (nl_hashset_lookup(&ctx.needed_blocks, opt->worklist.items[i]) & NL_HASHSET_HIGH_BIT) {
                if (!nl_hashset_put(&ctx.completed_blocks, opt->worklist.items[i])) {
                    continue;
                }
                ctx.depth += 1;
                c_fmt_bb(&ctx, opt->worklist.items[i]);
                ctx.depth -= 1;
                any = true;
            }
        }
        if (!any) {
            break;
        }
    }

    // if (nl_hashset_lookup(&ctx.needed_blocks, end_bb) & NL_HASHSET_HIGH_BIT) {
    //     c_fmt_ref_to_node(&ctx, end_bb, true);
    //     ctx.depth += 1;
    //     c_fmt_bb(&ctx, end_bb);
    //     ctx.depth -= 1;
    // }

    worklist_free(&opt->worklist);
    // tb_free_cfg(&ctx.cfg);
    opt->worklist = old;
    opt->scheduled = NULL;
    opt->error_n = NULL;
    cuikperf_region_end();

    nl_buffer_t *buf = nl_buffer_new(ctx.arena);

    nl_buffer_format(buf, "%s\n", nl_buffer_get(ctx.globals));
    nl_buffer_format(buf, "tb2c_%s_ret_t %s(", name, name);
    TB_Node** params = f->params;
    size_t count = 0;
    FOREACH_N(i, 3, 3 + f->param_count) {
        if (params[i] != NULL && params[i]->dt.type != TB_MEMORY && params[i]->dt.type != TB_CONTROL && params[i]->dt.type != TB_TUPLE) {
            if (count != 0) {
                nl_buffer_format(buf, ", ");
            }
            nl_buffer_format(buf, "%s v%u", c_fmt_type_name(params[i]->dt), params[i]->gvn);
            count += 1;
        }
    }
    if (count == 0) {
        nl_buffer_format(buf, "void");
    }
    nl_buffer_format(buf, ") {\n");
    nl_buffer_format(buf, "%s", nl_buffer_get(ctx.buf));
    nl_buffer_format(buf, "}\n");

    dyn_array_destroy(ctx.visited_blocks);

    char *ret = nl_buffer_get(buf);

    // tb_arena_destroy(ctx.arena);

    return ret;
}
