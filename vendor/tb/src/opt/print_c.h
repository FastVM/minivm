
struct vm_io_buffer_t;

struct vm_io_buffer_t *vm_io_buffer_new(void);
void vm_io_buffer_format(struct vm_io_buffer_t *buf, const char *fmt, ...);
char *vm_io_buffer_get(struct vm_io_buffer_t *buf);

typedef struct {
    TB_Passes* opt;
    TB_Function* f;
    TB_CFG cfg;
    TB_Scheduler sched;
    struct vm_io_buffer_t *globals;
    struct vm_io_buffer_t *args;
    struct vm_io_buffer_t *pre;
    struct vm_io_buffer_t *buf;
    bool has_ret: 1;
} CFmtState;

static size_t c_fmt_type_size(CFmtState* ctx, TB_DataType dt) {
    switch (dt.type) {
        case TB_INT: {
            if (dt.data == 0) return 0;
            if (dt.data == 1) return 1;
            if (dt.data == 8) return 1;
            if (dt.data == 16) return 2;
            if (dt.data == 32) return 4;
            if (dt.data == 64) return 8;
            else __builtin_trap();
            break;
        }
        case TB_PTR: {
            if (dt.data == 0) return sizeof(void *);
            else tb_todo();
            break;
        }
        case TB_FLOAT: {
            if (dt.data == TB_FLT_32) return sizeof(float);
            if (dt.data == TB_FLT_64) return sizeof(double);
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
}

static const char *c_fmt_type_name(CFmtState* ctx, TB_DataType dt) {
    switch (dt.type) {
        case TB_INT: {
            if (dt.data == 0) return  "void";
            if (dt.data == 1) return  "char";
            if (dt.data == 8) return  "uint8_t";
            if (dt.data == 16) return  "uint16_t";
            if (dt.data == 32) return  "uint32_t";
            if (dt.data == 64) return  "uint64_t";
            else __builtin_trap();
            break;
        }
        case TB_PTR: {
            if (dt.data == 0) return  "void *";
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
}

static void c_fmt_ref_to_node(CFmtState* ctx, TB_Node* n, bool def) {
    if (n == NULL) {
        vm_io_buffer_format(ctx->buf, "_");
    } else if (n->type == TB_ROOT) {
        if (def) {
            vm_io_buffer_format(ctx->buf, "\nbb%u:;\n", n->gvn);
            // vm_io_buffer_format(ctx->buf, "}\n", n->gvn);
        } else {
            vm_io_buffer_format(ctx->buf, "bb%u", n->gvn);
        }
    } else if (n->type == TB_PROJ && n->dt.type == TB_CONTROL) {
        if (def) {
            vm_io_buffer_format(ctx->buf, "\nbb%u:;\n", n->gvn);
            size_t count = 0;
            FOR_USERS(u, n) {
                if (u->n != NULL && u->n->type == TB_PHI && u->n->dt.type != TB_MEMORY && u->n->dt.type != TB_CONTROL && u->n->dt.type != TB_TUPLE) {
                    vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, u->n->dt), u->n->gvn);
                    // vm_io_buffer_format(ctx->pre, "  %s v%ua%zu;\n", c_fmt_type_name(ctx, u->n->dt), n->gvn, count);
                    // vm_io_buffer_format(ctx->buf, "  v%u = v%ua%zu;\n", u->n->gvn, n->gvn, count);
                    count += 1;
                }
            }
            // vm_io_buffer_format(ctx->buf, "}\n", n->gvn);
        } else {
            vm_io_buffer_format(ctx->buf, "bb%u", n->gvn);
        }
    } else if (n->type == TB_REGION || (n->type == TB_PROJ && n->dt.type == TB_CONTROL)) {
        TB_NodeRegion* r = TB_NODE_GET_EXTRA(n);
        if (def) {
            vm_io_buffer_format(ctx->buf, "\nbb%u:;\n", n->gvn);
            size_t count = 0;
            FOR_USERS(u, n) {
                if (u->n != NULL && u->n->type == TB_PHI && u->n->dt.type != TB_MEMORY && u->n->dt.type != TB_CONTROL && u->n->dt.type != TB_TUPLE) {
                    vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, u->n->dt), u->n->gvn);
                    // vm_io_buffer_format(ctx->pre, "  %s v%ua%zu;\n", c_fmt_type_name(ctx, u->n->dt), n->gvn, count);
                    // vm_io_buffer_format(ctx->buf, "  v%u = v%ua%zu;\n", u->n->gvn, n->gvn, count);
                    count += 1;
                }
            }
        } else {
            vm_io_buffer_format(ctx->buf, "bb%zu", n->gvn);
        }
    } else if (n->type == TB_FLOAT32_CONST) {
        TB_NodeFloat32* f = TB_NODE_GET_EXTRA(n);
        vm_io_buffer_format(ctx->buf, "%f", f->value);
    } else if (n->type == TB_FLOAT64_CONST) {
        TB_NodeFloat64* f = TB_NODE_GET_EXTRA(n);
        vm_io_buffer_format(ctx->buf, "%f", f->value);
    } else if (n->type == TB_SYMBOL) {
        TB_Symbol* sym = TB_NODE_GET_EXTRA_T(n, TB_NodeSymbol)->sym;
        if (sym->name[0]) {
            vm_io_buffer_format(ctx->buf, "/* %s */", sym->name);
        }
        vm_io_buffer_format(ctx->buf, "(void *) %p", TB_NODE_GET_EXTRA_T(n, TB_NodeSymbol)->sym->address);
    } else if (n->type == TB_ZERO_EXT) {
        vm_io_buffer_format(ctx->buf, "(zxt.???");
        // c_fmt_type2(ctx, n->dt);
        vm_io_buffer_format(ctx->buf, " ");
        c_fmt_ref_to_node(ctx, n->inputs[1], false);
        vm_io_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_SIGN_EXT) {
        vm_io_buffer_format(ctx->buf, "(sxt.???");
        // c_fmt_type2(ctx, n->dt);
        vm_io_buffer_format(ctx->buf, " ");
        c_fmt_ref_to_node(ctx, n->inputs[1], false);
        vm_io_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_INTEGER_CONST) {
        TB_NodeInt* num = TB_NODE_GET_EXTRA(n);

        vm_io_buffer_format(ctx->buf, "(%s)", c_fmt_type_name(ctx, n->dt));
        if (num->value < 0xFFFF) {
            vm_io_buffer_format(ctx->buf, "%"PRId64"llu", num->value);
        } else {
            vm_io_buffer_format(ctx->buf, "%#0"PRIx64"llu", num->value);
        }
    } else {
        vm_io_buffer_format(ctx->buf, "v%u", n->gvn);
    }
}

// deals with printing BB params
static void c_fmt_branch_edge(CFmtState* ctx, TB_Node* n, bool fallthru) {
    TB_Node* target = fallthru ? cfg_next_control(n) : cfg_next_bb_after_cproj(n);

    // print phi args
    if (target->type == TB_REGION) {
        int phi_i = -1;
        FOR_USERS(u, n) {
            if (u->n->type == TB_REGION) {
                phi_i = 1 + u->slot;
                break;
            }
        }

        size_t pos = 0;
        FOR_USERS(u, target) {
            if (u->n->type == TB_PHI) {
                if (u->n->inputs[phi_i] != NULL) {
                    if (u->n->inputs[phi_i]->dt.type != TB_CONTROL && u->n->inputs[phi_i]->dt.type != TB_MEMORY) {
                        assert(phi_i >= 0);
                        vm_io_buffer_format(ctx->buf, "  v%u = ", u->n->gvn);
                        c_fmt_ref_to_node(ctx, u->n->inputs[phi_i], false);
                        vm_io_buffer_format(ctx->buf, ";\n");
                        pos += 1;
                    }
                }
            }
        }
    }
    vm_io_buffer_format(ctx->buf, "  goto ");
    c_fmt_ref_to_node(ctx, target, false);
    vm_io_buffer_format(ctx->buf, ";\n");
}

static void c_fmt_bb(CFmtState* ctx, TB_Node* bb_start) {
    c_fmt_ref_to_node(ctx, bb_start, true);

    TB_BasicBlock* bb = ctx->opt->scheduled[bb_start->gvn];
    Worklist* ws = &ctx->opt->worklist;

    #ifndef NDEBUG
    TB_BasicBlock* expected = &nl_map_get_checked(ctx->cfg.node_to_block, bb_start);
    assert(expected == bb);
    #endif

    ctx->sched(ctx->opt, &ctx->cfg, ws, NULL, bb, bb->end);

    TB_Node* prev_effect = NULL;
    FOREACH_N(i, ctx->cfg.block_count, dyn_array_length(ws->items)) {
        TB_Node* n = ws->items[i];

        // skip these
        if (n->type == TB_INTEGER_CONST || n->type == TB_FLOAT32_CONST ||
            n->type == TB_FLOAT64_CONST || n->type == TB_SYMBOL ||
            n->type == TB_SIGN_EXT || n->type == TB_ZERO_EXT ||
            n->type == TB_PROJ || n->type == TB_REGION ||
            n->type == TB_NULL ||
            n->type == TB_PHI) {
            continue;
        }

        TB_NodeLocation* v;
        if (v = nl_table_get(&ctx->f->locations, n), v) {
            vm_io_buffer_format(ctx->buf, "  # location %s:%d\n", v->file->path, v->line);
        }

        switch (n->type) {
            case TB_DEBUGBREAK: {
                vm_io_buffer_format(ctx->buf, "  throw new Error(\"debug\");\n");
                break;
            }
            case TB_UNREACHABLE: {
                vm_io_buffer_format(ctx->buf, "  throw new Error(\"unreachable\");\n");
                break;
            }

            case TB_BRANCH: {
                TB_NodeBranch* br = TB_NODE_GET_EXTRA(n);
                TB_ArenaSavepoint sp = tb_arena_save(tmp_arena);
                TB_Node** restrict succ = tb_arena_alloc(tmp_arena, br->succ_count * sizeof(TB_Node**));

                // fill successors
                FOR_USERS(u, n) {
                    if (u->n->type == TB_PROJ) {
                        int index = TB_NODE_GET_EXTRA_T(u->n, TB_NodeProj)->index;
                        succ[index] = u->n;
                    }
                }

                if (br->succ_count == 1) {
                    c_fmt_branch_edge(ctx, succ[0], false);
                } else if (br->succ_count == 2) {
                    vm_io_buffer_format(ctx->buf, "  if (");
                    FOREACH_N(i, 1, n->input_count) {
                        if (i != 1) vm_io_buffer_format(ctx->buf, ", ");
                        c_fmt_ref_to_node(ctx, n->inputs[i], false);
                    }
                    if (br->keys[0] == 0) {
                        vm_io_buffer_format(ctx->buf, " != 0");
                    } else {
                        vm_io_buffer_format(ctx->buf, " != %"PRId64, br->keys[0]);
                    }
                    vm_io_buffer_format(ctx->buf, ") {\n");
                    c_fmt_branch_edge(ctx, succ[0], false);
                    vm_io_buffer_format(ctx->buf, "  } else {\n");
                    c_fmt_branch_edge(ctx, succ[1], false);
                    vm_io_buffer_format(ctx->buf, "  }\n");
                } else {
                    vm_io_buffer_format(ctx->buf, "  /* TODO: branch/%zu */ ", (size_t) br->succ_count);
                //     vm_io_buffer_format(ctx->buf, "  br ");
                //     FOREACH_N(i, 1, n->input_count) {
                //         if (i != 1) vm_io_buffer_format(ctx->buf, ", ");
                //         c_fmt_ref_to_node(ctx, n->inputs[i], false);
                //     }
                //     vm_io_buffer_format(ctx->buf, "%s=> {\n", n->input_count > 1 ? " " : "");

                //     FOREACH_N(i, 0, br->succ_count) {
                //         if (i != 0) vm_io_buffer_format(ctx->buf, "    %"PRId64": ", br->keys[i - 1]);
                //         else vm_io_buffer_format(ctx->buf, "    default: ");

                //         c_fmt_branch_edge(ctx, succ[i], false);
                //         vm_io_buffer_format(ctx->buf, "\n");
                //     }
                //     vm_io_buffer_format(ctx->buf, "  }");
                }
                // tb_arena_restore(tmp_arena, sp);
                vm_io_buffer_format(ctx->buf, "\n");
                break;
            }

            case TB_TRAP: {
                vm_io_buffer_format(ctx->buf, "  throw new Error(\"trap\");\n");
                break;
            }

            case TB_ROOT: {
                if (ctx->has_ret) {
                    ctx->globals = vm_io_buffer_new();
                }
                vm_io_buffer_format(ctx->globals, "typedef struct {\n");
                vm_io_buffer_format(ctx->buf, "  {\n");
                vm_io_buffer_format(ctx->buf, "    ret_t ret;\n");
                bool index = 0;
                FOREACH_N(i, 1, n->input_count) {
                    if (i >= 4) {
                        vm_io_buffer_format(ctx->globals, "  %s v%zu;\n", c_fmt_type_name(ctx, n->inputs[i]->dt), index);
                        vm_io_buffer_format(ctx->buf, "    ret.v%zu = ", index);
                        c_fmt_ref_to_node(ctx, n->inputs[i], false);
                        vm_io_buffer_format(ctx->buf, ";\n");
                        index += 1;
                    }
                }
                vm_io_buffer_format(ctx->globals, "} ret_t;\n");
                vm_io_buffer_format(ctx->buf, "    return ret;\n");
                vm_io_buffer_format(ctx->buf, "  }\n");
                break;
            }

            case TB_CALLGRAPH: {
                break;
            }

            case TB_STORE: {
                // vm_io_buffer_format(ctx->buf, "  v%u = (void*)&(char[0x%X]){0}", n->gvn, l->size);
                TB_Node *dest = n->inputs[n->input_count-2];
                TB_Node *src = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->buf, "  *(%s*) ", c_fmt_type_name(ctx, src->dt));
                c_fmt_ref_to_node(ctx, dest, false);
                vm_io_buffer_format(ctx->buf, " = ", c_fmt_type_name(ctx, src->dt));
                c_fmt_ref_to_node(ctx, src, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_LOAD: {
                // vm_io_buffer_format(ctx->buf, "  v%u = (void*)&(char[0x%X]){0}", n->gvn, l->size);
                TB_Node *src = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = *(%s*) ", n->gvn, c_fmt_type_name(ctx, n->dt));
                c_fmt_ref_to_node(ctx, src, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }
            
            case TB_LOCAL: {
                TB_NodeLocal* l = TB_NODE_GET_EXTRA(n);
                vm_io_buffer_format(ctx->pre, "  uint8_t v%u[0x%x];\n", n->gvn, l->size);
                // vm_io_buffer_format(ctx->pre, "  %s v%u = &v%ud[0];\n", c_fmt_type_name(ctx, n->dt), n->gvn, n->gvn);
                // vm_io_buffer_format(ctx->buf, "  v%u = (void *) &v%ud[0];\n", n->gvn, n->gvn);
                break;
            }
            
            case TB_BITCAST: {
                TB_Node *src = n->inputs[n->input_count-1];
                size_t src_size = c_fmt_type_size(ctx, src->dt);
                size_t dest_size = c_fmt_type_size(ctx, n->dt);
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                if (dest_size >= src_size) {
                    vm_io_buffer_format(ctx->buf, "  *(%s *)&v%u = ", c_fmt_type_name(ctx, src->dt), n->gvn, src->gvn);
                    c_fmt_ref_to_node(ctx, src, false);
                    vm_io_buffer_format(ctx->buf, ";\n");
                } else {
                    vm_io_buffer_format(ctx->buf, "  if (1) {\n");
                    vm_io_buffer_format(ctx->buf, "    union {%s src; %s dest;} tmp;\n", c_fmt_type_name(ctx, src->dt), c_fmt_type_name(ctx, n->dt));
                    vm_io_buffer_format(ctx->buf, "    tmp.src = ");
                    c_fmt_ref_to_node(ctx, src, false);
                    vm_io_buffer_format(ctx->buf, ";\n");
                    vm_io_buffer_format(ctx->buf, "    v%u = tmp.dest;\n", n->gvn);
                    vm_io_buffer_format(ctx->buf, "  }\n");
                }
                break;
            }

            case TB_OR: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = ", n->gvn);
                c_fmt_ref_to_node(ctx, lhs, false);
                vm_io_buffer_format(ctx->buf, " | ");
                c_fmt_ref_to_node(ctx, rhs, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_XOR: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = ", n->gvn);
                c_fmt_ref_to_node(ctx, lhs, false);
                vm_io_buffer_format(ctx->buf, " ^ ");
                c_fmt_ref_to_node(ctx, rhs, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_AND: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = ", n->gvn);
                c_fmt_ref_to_node(ctx, lhs, false);
                vm_io_buffer_format(ctx->buf, " & ");
                c_fmt_ref_to_node(ctx, rhs, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_FADD:
            case TB_ADD: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = ", n->gvn);
                c_fmt_ref_to_node(ctx, lhs, false);
                vm_io_buffer_format(ctx->buf, " + ");
                c_fmt_ref_to_node(ctx, rhs, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }
            case TB_FSUB:
            case TB_SUB: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = ", n->gvn);
                c_fmt_ref_to_node(ctx, lhs, false);
                vm_io_buffer_format(ctx->buf, " - ");
                c_fmt_ref_to_node(ctx, rhs, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }
            case TB_FMUL:
            case TB_MUL: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = ", n->gvn);
                c_fmt_ref_to_node(ctx, lhs, false);
                vm_io_buffer_format(ctx->buf, " * ");
                c_fmt_ref_to_node(ctx, rhs, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }
            case TB_FDIV: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = ", n->gvn);
                c_fmt_ref_to_node(ctx, lhs, false);
                vm_io_buffer_format(ctx->buf, " / ");
                c_fmt_ref_to_node(ctx, rhs, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }
            case TB_SDIV: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = (int64_t) ", n->gvn);
                c_fmt_ref_to_node(ctx, lhs, false);
                vm_io_buffer_format(ctx->buf, " / (int64_t) ");
                c_fmt_ref_to_node(ctx, rhs, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }
            case TB_UDIV: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = (uint64_t) ", n->gvn);
                c_fmt_ref_to_node(ctx, lhs, false);
                vm_io_buffer_format(ctx->buf, " / (uint64_t) ");
                c_fmt_ref_to_node(ctx, rhs, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }
            case TB_SMOD: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = (int64_t) ", n->gvn);
                c_fmt_ref_to_node(ctx, lhs, false);
                vm_io_buffer_format(ctx->buf, " %% (int64_t) ");
                c_fmt_ref_to_node(ctx, rhs, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }
            case TB_UMOD: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = (uint64_t) ", n->gvn);
                c_fmt_ref_to_node(ctx, lhs, false);
                vm_io_buffer_format(ctx->buf, " %% (uint64_t) ");
                c_fmt_ref_to_node(ctx, rhs, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_CMP_EQ: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = ", n->gvn);
                c_fmt_ref_to_node(ctx, lhs, false);
                vm_io_buffer_format(ctx->buf, " == ");
                c_fmt_ref_to_node(ctx, rhs, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_POISON: {
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                break;
            }

            case TB_CMP_FLT: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = ", n->gvn);
                c_fmt_ref_to_node(ctx, lhs, false);
                vm_io_buffer_format(ctx->buf, " < ");
                c_fmt_ref_to_node(ctx, rhs, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_CMP_FLE: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = ", n->gvn);
                c_fmt_ref_to_node(ctx, lhs, false);
                vm_io_buffer_format(ctx->buf, " <= ");
                c_fmt_ref_to_node(ctx, rhs, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }
            

            case TB_CMP_SLT: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = (int64_t) ", n->gvn);
                c_fmt_ref_to_node(ctx, lhs, false);
                vm_io_buffer_format(ctx->buf, " < (int64_t) ");
                c_fmt_ref_to_node(ctx, rhs, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_CMP_SLE: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = (int64_t) ", n->gvn);
                c_fmt_ref_to_node(ctx, lhs, false);
                vm_io_buffer_format(ctx->buf, " <= (int64_t) ");
                c_fmt_ref_to_node(ctx, rhs, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }
            

            case TB_CMP_ULT: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = (uint64_t) ", n->gvn);
                c_fmt_ref_to_node(ctx, lhs, false);
                vm_io_buffer_format(ctx->buf, " < (uint64_t) ");
                c_fmt_ref_to_node(ctx, rhs, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_CMP_ULE: {
                TB_Node *lhs = n->inputs[n->input_count-2];
                TB_Node *rhs = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = (uint64_t) ", n->gvn);
                c_fmt_ref_to_node(ctx, lhs, false);
                vm_io_buffer_format(ctx->buf, " <= (uint64_t) ");
                c_fmt_ref_to_node(ctx, rhs, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }
            
            case TB_MEMBER_ACCESS: {
                TB_Node *ptr = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = (void*) ((size_t) ", n->gvn);
                c_fmt_ref_to_node(ctx, ptr, false);
                vm_io_buffer_format(ctx->buf, " + %"PRId64, TB_NODE_GET_EXTRA_T(n, TB_NodeMember)->offset);
                vm_io_buffer_format(ctx->buf, ");\n");
                break;
            }

            
            case TB_SELECT: {
                TB_Node *cond = n->inputs[n->input_count-1];
                TB_Node *then = n->inputs[n->input_count-1];
                TB_Node *els = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = ", n->gvn);
                c_fmt_ref_to_node(ctx, cond, false);
                vm_io_buffer_format(ctx->buf, " ? ");
                c_fmt_ref_to_node(ctx, then, false);
                vm_io_buffer_format(ctx->buf, " : ");
                c_fmt_ref_to_node(ctx, els, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_FLOAT2INT:
            case TB_INT2FLOAT:
            case TB_TRUNCATE: {
                TB_Node *input = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = (%s) ", n->gvn, c_fmt_type_name(ctx, n->dt));
                c_fmt_ref_to_node(ctx, input, false);
                vm_io_buffer_format(ctx->buf, ";\n");
                break;
            }

            case TB_ARRAY_ACCESS: {
                TB_Node *ptr = n->inputs[n->input_count-2];
                TB_Node *index = n->inputs[n->input_count-1];
                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, n->dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = (void*) ((size_t) ", n->gvn);
                c_fmt_ref_to_node(ctx, ptr, false);
                vm_io_buffer_format(ctx->buf, " + ");
                c_fmt_ref_to_node(ctx, index, false);
                vm_io_buffer_format(ctx->buf, " * %"PRId64, TB_NODE_GET_EXTRA_T(n, TB_NodeArray)->stride);
                vm_io_buffer_format(ctx->buf, ");\n");
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
                    vm_io_buffer_format(ctx->globals, "typedef void v%u_ret_t;\n", n->gvn);
                } else {
                    vm_io_buffer_format(ctx->globals, "typedef struct {\n");
                    FOREACH_N(i, 2, 4) {
                        if (projs[i] == NULL) break;
                        vm_io_buffer_format(ctx->globals, "  %s v%u;\n", c_fmt_type_name(ctx, projs[i]->dt), projs[i]->gvn);
                    }
                    vm_io_buffer_format(ctx->globals, "} v%u_ret_t;\n", n->gvn);
                }
                vm_io_buffer_format(ctx->globals, "typedef v%u_ret_t(*v%u_t)(", n->gvn, n->gvn);
                {
                    bool first = true;
                    FOREACH_N(i, 3, n->input_count) {
                        if (n->inputs[i]->dt.type != TB_CONTROL && n->inputs[i]->dt.type != TB_MEMORY) {
                            if (!first) {
                                vm_io_buffer_format(ctx->globals, ", ");
                            }
                            vm_io_buffer_format(ctx->globals, "%s", c_fmt_type_name(ctx, n->inputs[i]->dt));
                            first = false;
                        }
                    }
                    if (first) {
                            vm_io_buffer_format(ctx->globals, "void");
                    }
                }
                vm_io_buffer_format(ctx->globals, ");\n");
                vm_io_buffer_format(ctx->buf, "  {\n");
                if (projs[2] != NULL) {
                    vm_io_buffer_format(ctx->buf, "    v%u_ret_t ret = ", n->gvn);
                }
                vm_io_buffer_format(ctx->buf, "((v%u_t) ", n->gvn);
                c_fmt_ref_to_node(ctx, func, false);
                vm_io_buffer_format(ctx->buf, ")(");
                {
                    bool first = true;
                    FOREACH_N(i, 3, n->input_count) {
                        if (n->inputs[i]->dt.type != TB_CONTROL && n->inputs[i]->dt.type != TB_MEMORY) {
                            if (!first) {
                                vm_io_buffer_format(ctx->buf, ", ");
                            }
                            c_fmt_ref_to_node(ctx, n->inputs[i], false);
                            first = false;
                        }
                    }
                }
                vm_io_buffer_format(ctx->buf, ");\n");
                if (projs[2] != NULL) {
                    FOREACH_N(i, 2, 4) {
                        if (projs[i] == NULL) break;
                        vm_io_buffer_format(ctx->pre, "    %s v%u;\n", c_fmt_type_name(ctx, projs[i]->dt), projs[i]->gvn);
                        vm_io_buffer_format(ctx->buf, "    v%u = ret.v%u;\n", projs[i]->gvn, projs[i]->gvn);
                    }
                }
                vm_io_buffer_format(ctx->buf, "  }\n");
                break;
            }

            default: {
                fprintf(stderr, "internal unimplemented node type: %s\n", tb_node_get_name(n));
                fflush(stderr);
                __builtin_trap();
                // if (n->dt.type == TB_TUPLE) {
                //     // print with multiple returns
                //     TB_Node* projs[4] = { 0 };
                //     FOR_USERS(use, n) {
                //         if (use->n->type == TB_PROJ) {
                //             int index = TB_NODE_GET_EXTRA_T(use->n, TB_NodeProj)->index;
                //             projs[index] = use->n;
                //         }
                //     }

                //     vm_io_buffer_format(ctx->buf, "  ");

                //     size_t first = projs[0] && projs[0]->dt.type == TB_CONTROL ? 1 : 0;
                //     FOREACH_N(i, first, 4) {
                //         if (projs[i] == NULL) break;
                //         if (i > first) vm_io_buffer_format(ctx->buf, ", ");
                //         vm_io_buffer_format(ctx->buf, "v%u", projs[i]->gvn);
                //     }
                //     vm_io_buffer_format(ctx->buf, " = %s.(", tb_node_get_name(n));
                //     FOREACH_N(i, first, 4) {
                //         if (projs[i] == NULL) break;
                //         if (i > first) vm_io_buffer_format(ctx->buf, ", ");
                //         // c_fmt_type2(ctx, projs[i]->dt);
                //     }
                //     vm_io_buffer_format(ctx->buf, ")");
                // } else {
                    // print as normal instruction
                    TB_DataType dt = n->dt;
                    if (n->type >= TB_CMP_EQ && n->type <= TB_CMP_FLE) {
                        dt = TB_NODE_GET_EXTRA_T(n, TB_NodeCompare)->cmp_dt;
                    }
                    // c_fmt_type2(ctx, dt);
                // }

                vm_io_buffer_format(ctx->pre, "  %s v%u;\n", c_fmt_type_name(ctx, dt), n->gvn);
                vm_io_buffer_format(ctx->buf, "  v%u = %s(", n->gvn, tb_node_get_name(n));

                // c_fmt_type_name(ctx, dt);
                // print extra data
                switch (n->type) {
                    case TB_CMP_FLT:
                    case TB_CMP_FLE:
                    case TB_SELECT:
                        // vm_io_buffer_format(ctx->buf, ", ");
                        // c_fmt_type_name(ctx, n->inputs[1]->dt);
                        break;

                    case TB_PROJ: {
                        // vm_io_buffer_format(ctx->buf, ", %d", TB_NODE_GET_EXTRA_T(n, TB_NodeProj)->index);
                        break;
                    }

                    case TB_AND:
                    case TB_OR:
                    case TB_XOR:
                    case TB_SHL:
                    case TB_SHR:
                    case TB_ROL:
                    case TB_ROR:
                    case TB_SAR:
                    {
                        // TB_NodeBinopInt* b = TB_NODE_GET_EXTRA(n);
                        // if (b->ab & TB_ARITHMATIC_NSW) vm_io_buffer_format(ctx->buf, " !nsw");
                        // if (b->ab & TB_ARITHMATIC_NUW) vm_io_buffer_format(ctx->buf, " !nuw");
                        break;
                    }

                    case TB_MEMSET:
                    case TB_MEMCPY: {
                        // TB_NodeMemAccess* mem = TB_NODE_GET_EXTRA(n);
                        // vm_io_buffer_format(ctx->buf, " !align(%d)", mem->align);
                        break;
                    }

                    case TB_ATOMIC_LOAD:
                    case TB_ATOMIC_XCHG:
                    case TB_ATOMIC_ADD:
                    case TB_ATOMIC_SUB:
                    case TB_ATOMIC_AND:
                    case TB_ATOMIC_XOR:
                    case TB_ATOMIC_OR:
                    case TB_ATOMIC_CAS: {
                        // static const char* order_names[] = {
                        //     "relaxed", "consume", "acquire",
                        //     "release", "acqrel", "seqcst"
                        // };

                        // TB_NodeAtomic* atomic = TB_NODE_GET_EXTRA(n);
                        // vm_io_buffer_format(ctx->buf, " !order(%s)", order_names[atomic->order]);
                        // if (n->type == TB_ATOMIC_CAS) {
                        //     vm_io_buffer_format(ctx->buf, " !fail_order(%s)", order_names[atomic->order2]);
                        // }
                        break;
                    }

                    case TB_CALL:
                    case TB_TAILCALL:
                    case TB_SYSCALL:
                    case TB_SAFEPOINT_POLL:
                    break;

                    case TB_LOOKUP: {
                        // TB_NodeLookup* l = TB_NODE_GET_EXTRA(n);

                        // vm_io_buffer_format(ctx->buf, " { default: %"PRId64, l->entries[0].val);
                        // FOREACH_N(i, 1, l->entry_count) {
                        //     vm_io_buffer_format(ctx->buf, ", %"PRId64": %"PRId64, l->entries[i].key, l->entries[i].val);
                        // }
                        // vm_io_buffer_format(ctx->buf, "}");
                        break;
                    }

                    default: tb_assert(extra_bytes(n) == 0, "TODO");
                }

                FOREACH_N(i, 1, n->input_count) {
                    if (n->inputs[i]->dt.type != TB_CONTROL && n->inputs[i]->dt.type != TB_MEMORY) {
                        vm_io_buffer_format(ctx->buf, ", ");
                        c_fmt_ref_to_node(ctx, n->inputs[i], false);
                    }
                }
                vm_io_buffer_format(ctx->buf, ");\n");
                break;
            }
        }
    }

    dyn_array_set_length(ws->items, ctx->cfg.block_count);

    if (!cfg_is_terminator(bb->end)) {
        c_fmt_branch_edge(ctx, bb->end, true);
    }
}

char *tb_pass_c_fmt(TB_Passes* opt) {
    TB_Function* f = opt->f;
    cuikperf_region_start("print", NULL);

    Worklist old = opt->worklist;
    Worklist tmp_ws = { 0 };
    worklist_alloc(&tmp_ws, f->node_count);

    CFmtState ctx = { opt, f };
    ctx.has_ret = false;
    ctx.globals = vm_io_buffer_new();
    ctx.args = vm_io_buffer_new();
    ctx.pre = vm_io_buffer_new();
    ctx.buf = vm_io_buffer_new();

    opt->worklist = tmp_ws;
    ctx.cfg = tb_compute_rpo(f, opt);

    // does the IR printing need smart scheduling lol (yes... when we're testing things)
    ctx.sched = greedy_scheduler;

    TB_ArenaSavepoint sp = tb_arena_save(tmp_arena);

    // schedule nodes
    tb_pass_schedule(opt, ctx.cfg, false);
    worklist_clear_visited(&opt->worklist);

    
    TB_Node* end_bb = NULL;
    FOREACH_N(i, 0, ctx.cfg.block_count) {
        TB_Node* end = nl_map_get_checked(ctx.cfg.node_to_block, opt->worklist.items[i]).end;
        if (end == f->root_node) {
            end_bb = opt->worklist.items[i];
            continue;
        }

        c_fmt_bb(&ctx, opt->worklist.items[i]);
    }

    if (end_bb != NULL) {
        c_fmt_bb(&ctx, end_bb);
    }

    tb_arena_restore(tmp_arena, sp);
    worklist_free(&opt->worklist);
    // tb_free_cfg(&ctx.cfg);
    opt->worklist = old;
    opt->scheduled = NULL;
    opt->error_n = NULL;
    cuikperf_region_end();

    struct vm_io_buffer_t *buf = vm_io_buffer_new();

    vm_io_buffer_format(buf, "typedef unsigned char uint8_t;\n");
    vm_io_buffer_format(buf, "typedef unsigned short uint16_t;\n");
    vm_io_buffer_format(buf, "typedef unsigned int uint32_t;\n");
    vm_io_buffer_format(buf, "typedef unsigned long long uint64_t;\n");
    vm_io_buffer_format(buf, "typedef signed char int8_t;\n");
    vm_io_buffer_format(buf, "typedef signed short int16_t;\n");
    vm_io_buffer_format(buf, "typedef signed int int32_t;\n");
    vm_io_buffer_format(buf, "typedef signed long long int64_t;\n");
    vm_io_buffer_format(buf, "typedef __SIZE_TYPE__ size_t;\n");
    vm_io_buffer_format(buf, "%s\n", vm_io_buffer_get(ctx.globals));
    vm_io_buffer_format(buf, "ret_t entry(");
    TB_Node** params = f->params;
    size_t count = 0;
    FOREACH_N(i, 3, 3 + f->param_count) {
        if (params[i] != NULL && params[i]->dt.type != TB_MEMORY && params[i]->dt.type != TB_CONTROL && params[i]->dt.type != TB_TUPLE) {
            if (count != 0) {
                vm_io_buffer_format(buf, ", ");
            }
            vm_io_buffer_format(buf, "%s v%u", c_fmt_type_name(&ctx, params[i]->dt), params[i]->gvn);
            count += 1;
        }
    }
    if (count == 0) {
        vm_io_buffer_format(buf, "void");
    }
    vm_io_buffer_format(buf, ") {\n");
    vm_io_buffer_format(buf, "%s", vm_io_buffer_get(ctx.pre));
    vm_io_buffer_format(buf, "%s", vm_io_buffer_get(ctx.buf));
    vm_io_buffer_format(buf, "}\n");

    return vm_io_buffer_get(buf);
}
