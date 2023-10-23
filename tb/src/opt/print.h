
typedef struct {
    TB_Passes* opt;
    TB_Function* f;
    TB_CFG cfg;
} PrinterCtx;

static void print_type(TB_DataType dt) {
    switch (dt.type) {
        case TB_INT: {
            if (dt.data == 0) printf("void");
            else printf("i%d", dt.data);
            break;
        }
        case TB_PTR: {
            if (dt.data == 0) printf("ptr");
            else printf("ptr%d", dt.data);
            break;
        }
        case TB_FLOAT: {
            if (dt.data == TB_FLT_32) printf("f32");
            if (dt.data == TB_FLT_64) printf("f64");
            break;
        }
        case TB_TUPLE: {
            printf("tuple");
            break;
        }
        case TB_CONTROL: {
            printf("control");
            break;
        }
        case TB_MEMORY: {
            printf("memory");
            break;
        }
        case TB_CONT: {
            printf("cont");
            break;
        }
        default: tb_todo();
    }
}

static void print_type2(TB_DataType dt) {
    printf("\x1b[96m");
    print_type(dt);
    printf("\x1b[0m");
}

static void print_ref_to_node(PrinterCtx* ctx, TB_Node* n, bool def) {
    if (n == NULL) {
        printf("_");
    } else if (n->type == TB_START) {
        printf("%s", ctx->f->super.name);

        if (def) {
            printf("(");
            TB_Node** params = ctx->f->params;
            FOREACH_N(i, 1, 3 + ctx->f->param_count) {
                if (i > 1) printf(", ");

                if (params[i] == NULL) {
                    printf("_");
                } else {
                    printf("v%u: ", params[i]->gvn);
                    print_type2(params[i]->dt);
                }
            }
            printf(")");
        }
    } else if (n->type == TB_REGION) {
        TB_NodeRegion* r = TB_NODE_GET_EXTRA(n);
        if (r->tag != NULL) {
            printf(".%s", r->tag);
        } else {
            ptrdiff_t i = try_find_traversal_index(&ctx->cfg, n);
            if (i >= 0) {
                printf(".bb%zu", i);
            } else {
                printf("*DEAD*");
            }
        }

        if (def) {
            bool first = true;
            printf("(");
            for (User* u = n->users; u; u = u->next) {
                if (u->n->type == TB_PHI) {
                    if (first) {
                        first = false;
                    } else {
                        printf(", ");
                    }

                    printf("v%u: ", u->n->gvn);
                    print_type2(u->n->dt);
                }
            }
            printf(") // v%u", n->gvn);
        }
    } else if (n->type == TB_FLOAT32_CONST) {
        TB_NodeFloat32* f = TB_NODE_GET_EXTRA(n);
        printf("%f", f->value);
    } else if (n->type == TB_FLOAT64_CONST) {
        TB_NodeFloat64* f = TB_NODE_GET_EXTRA(n);
        printf("%f", f->value);
    } else if (n->type == TB_SYMBOL) {
        TB_Symbol* sym = TB_NODE_GET_EXTRA_T(n, TB_NodeSymbol)->sym;
        if (sym->name[0]) {
            printf("%s", sym->name);
        } else {
            printf("sym%p", sym);
        }
    } else if (n->type == TB_PROJ && n->dt.type == TB_CONTROL) {
        if (n->inputs[0]->type == TB_START) {
            print_ref_to_node(ctx, n->inputs[0], def);
        } else {
            ptrdiff_t i = try_find_traversal_index(&ctx->cfg, n);
            if (i >= 0) {
                printf(".bb%zu", i);
                if (def) {
                    printf("() // v%u", n->gvn);
                }
            } else {
                printf("*DEAD*");
            }
        }
    } else if (n->type == TB_ZERO_EXT) {
        printf("(zxt.");
        print_type2(n->dt);
        printf(" ");
        print_ref_to_node(ctx, n->inputs[1], false);
        printf(")");
    } else if (n->type == TB_SIGN_EXT) {
        printf("(sxt.");
        print_type2(n->dt);
        printf(" ");
        print_ref_to_node(ctx, n->inputs[1], false);
        printf(")");
    } else if (n->type == TB_INTEGER_CONST) {
        TB_NodeInt* num = TB_NODE_GET_EXTRA(n);

        if (num->value < 0xFFFF) {
            printf("%"PRId64, num->value);
        } else {
            printf("%#0"PRIx64, num->value);
        }
    } else {
        printf("v%u", n->gvn);
    }
}

// deals with printing BB params
static void print_branch_edge(PrinterCtx* ctx, TB_Node* n, bool fallthru) {
    TB_Node* target = fallthru ? cfg_next_control(n) : cfg_next_bb_after_cproj(n);
    print_ref_to_node(ctx, target, false);

    // print phi args
    printf("(");
    if (target->type == TB_REGION) {
        int phi_i = -1;
        for (User* u = n->users; u; u = u->next) {
            if (u->n->type == TB_REGION) {
                phi_i = 1 + u->slot;
                break;
            }
        }

        bool first = true;
        for (User* u = target->users; u; u = u->next) {
            if (u->n->type == TB_PHI) {
                if (first) {
                    first = false;
                } else {
                    printf(", ");
                }

                assert(phi_i >= 0);
                printf("v%u", u->n->inputs[phi_i]->gvn);
            }
        }
    }
    printf(")");
}

static void print_location(TB_Function* f, TB_Node* n) {
    ptrdiff_t search = nl_map_get(f->attribs, n);
    if (search >= 0) {
        DynArray(TB_Attrib) attribs = f->attribs[search].v;
        dyn_array_for(i, attribs) {
            TB_Attrib* a = &attribs[i];
            if (a->tag == TB_ATTRIB_LOCATION) {
                printf("  # location %s:%d\n", a->loc.file->path, a->loc.line);
                return;
            }
        }
    }
}

static void print_bb(PrinterCtx* ctx, TB_Node* bb_start) {
    print_ref_to_node(ctx, bb_start, true);
    printf(":");

    // print predecessors
    /*if (!(bb_start->type == TB_PROJ && bb_start->inputs[0]->type == TB_START) && bb_start->input_count > 0) {
        printf(" \x1b[32m# preds: ");
        FOREACH_N(j, 0, bb_start->input_count) {
            print_ref_to_node(ctx, get_pred(bb_start, j), false);
            printf(" ");
        }
        printf("\x1b[0m");
    }*/

    if (ctx->opt->error_n == bb_start) {
        printf("\x1b[31m  <-- ERROR\x1b[0m");
    }
    printf("\n");

    TB_BasicBlock* bb = nl_map_get_checked(ctx->opt->scheduled, bb_start);
    Worklist* ws = &ctx->opt->worklist;

    #ifndef NDEBUG
    TB_BasicBlock* expected = &nl_map_get_checked(ctx->cfg.node_to_block, bb_start);
    assert(expected == bb);
    #endif

    sched_walk(ctx->opt, ws, NULL, bb, bb->end, true);

    TB_Node* prev_effect = NULL;
    FOREACH_N(i, ctx->cfg.block_count, dyn_array_length(ws->items)) {
        TB_Node* n = ws->items[i];

        // skip these
        if (n->type == TB_INTEGER_CONST || n->type == TB_FLOAT32_CONST ||
            n->type == TB_FLOAT64_CONST || n->type == TB_SYMBOL ||
            n->type == TB_SIGN_EXT || n->type == TB_ZERO_EXT ||
            n->type == TB_PROJ || n->type == TB_START ||
            n->type == TB_REGION || n->type == TB_NULL ||
            n->type == TB_PHI) {
            continue;
        }

        if (n->dt.type == TB_TUPLE || n->dt.type == TB_CONTROL || n->dt.type == TB_MEMORY) {
            print_location(ctx->f, n);
        }

        switch (n->type) {
            case TB_DEBUGBREAK: printf("  debugbreak"); break;
            case TB_UNREACHABLE: printf("  unreachable"); break;

            case TB_BRANCH: {
                TB_NodeBranch* br = TB_NODE_GET_EXTRA(n);
                TB_ArenaSavepoint sp = tb_arena_save(tmp_arena);
                TB_Node** restrict succ = tb_arena_alloc(tmp_arena, br->succ_count * sizeof(TB_Node**));

                // fill successors
                for (User* u = n->users; u; u = u->next) {
                    if (u->n->type == TB_PROJ) {
                        int index = TB_NODE_GET_EXTRA_T(u->n, TB_NodeProj)->index;
                        succ[index] = u->n;
                    }
                }

                if (br->succ_count == 1) {
                    printf("  goto ");
                    print_branch_edge(ctx, succ[0], false);
                } else if (br->succ_count == 2) {
                    printf("  if ");
                    FOREACH_N(i, 1, n->input_count) {
                        if (i != 1) printf(", ");
                        print_ref_to_node(ctx, n->inputs[i], false);
                    }
                    if (br->keys[0] == 0) {
                        printf(" then ");
                    } else {
                        printf(" != %"PRId64" then ", br->keys[0]);
                    }
                    print_branch_edge(ctx, succ[0], false);
                    printf(" else ");
                    print_branch_edge(ctx, succ[1], false);
                } else {
                    printf("  br ");
                    FOREACH_N(i, 1, n->input_count) {
                        if (i != 1) printf(", ");
                        print_ref_to_node(ctx, n->inputs[i], false);
                    }
                    printf("%s=> {\n", n->input_count > 1 ? " " : "");

                    FOREACH_N(i, 0, br->succ_count) {
                        if (i != 0) printf("    %"PRId64": ", br->keys[i - 1]);
                        else printf("    default: ");

                        print_branch_edge(ctx, succ[i], false);
                        printf("\n");
                    }
                    printf("  }");
                }
                tb_arena_restore(tmp_arena, sp);
                break;
            }

            case TB_TRAP: {
                printf("  trap");
                break;
            }

            case TB_END: {
                printf("  end ");
                FOREACH_N(i, 1, n->input_count) {
                    if (i != 1) printf(", ");
                    print_ref_to_node(ctx, n->inputs[i], false);
                }
                break;
            }

            default: {
                if (n->dt.type == TB_TUPLE) {
                    // print with multiple returns
                    TB_Node* projs[4];
                    for (size_t i = 0; i < 4; i++) projs[i] = NULL;

                    for (User* use = n->users; use; use = use->next) {
                        if (use->n->type == TB_PROJ) {
                            int index = TB_NODE_GET_EXTRA_T(use->n, TB_NodeProj)->index;
                            projs[index] = use->n;
                        }
                    }

                    printf("  ");

                    size_t first = projs[0]->dt.type == TB_CONTROL ? 1 : 0;
                    FOREACH_N(i, first, 4) {
                        if (projs[i] == NULL) break;
                        if (i > first) printf(", ");
                        printf("v%u", projs[i]->gvn);
                    }
                    printf(" = %s.(", tb_node_get_name(n));
                    FOREACH_N(i, first, 4) {
                        if (projs[i] == NULL) break;
                        if (i > first) printf(", ");
                        print_type2(projs[i]->dt);
                    }
                    printf(")");
                } else {
                    // print as normal instruction
                    if (n->dt.type == TB_INT && n->dt.data == 0) {
                        printf("  %s.", tb_node_get_name(n));
                    } else {
                        printf("  v%u = %s.", n->gvn, tb_node_get_name(n));
                    }

                    TB_DataType dt = n->dt;
                    if (n->type >= TB_CMP_EQ && n->type <= TB_CMP_FLE) {
                        dt = TB_NODE_GET_EXTRA_T(n, TB_NodeCompare)->cmp_dt;
                    } else if (n->type == TB_STORE) {
                        dt = n->inputs[3]->dt;
                    }
                    print_type2(dt);
                }
                printf(" ");

                size_t first = n->type == TB_PROJ ? 0 : 1;
                FOREACH_N(i, first, n->input_count) {
                    if (i != first) printf(", ");
                    print_ref_to_node(ctx, n->inputs[i], false);
                }

                // print extra data
                switch (n->type) {
                    case TB_CMP_EQ:
                    case TB_CMP_NE:
                    case TB_CMP_ULT:
                    case TB_CMP_ULE:
                    case TB_CMP_SLT:
                    case TB_CMP_SLE:
                    case TB_CMP_FLT:
                    case TB_CMP_FLE:
                    case TB_SELECT:
                    case TB_BITCAST:
                    break;

                    case TB_MEMBER_ACCESS: {
                        printf(", %"PRId64, TB_NODE_GET_EXTRA_T(n, TB_NodeMember)->offset);
                        break;
                    }

                    case TB_ARRAY_ACCESS: {
                        printf(", %"PRId64, TB_NODE_GET_EXTRA_T(n, TB_NodeArray)->stride);
                        break;
                    }

                    case TB_PROJ: {
                        printf(", %d", TB_NODE_GET_EXTRA_T(n, TB_NodeProj)->index);
                        break;
                    }

                    case TB_AND:
                    case TB_OR:
                    case TB_XOR:
                    case TB_ADD:
                    case TB_SUB:
                    case TB_MUL:
                    case TB_SHL:
                    case TB_SHR:
                    case TB_ROL:
                    case TB_ROR:
                    case TB_SAR:
                    case TB_UDIV:
                    case TB_SDIV:
                    case TB_UMOD:
                    case TB_SMOD:
                    {
                        TB_NodeBinopInt* b = TB_NODE_GET_EXTRA(n);
                        if (b->ab & TB_ARITHMATIC_NSW) printf(" !nsw");
                        if (b->ab & TB_ARITHMATIC_NUW) printf(" !nuw");
                        break;
                    }

                    case TB_LOAD:
                    case TB_STORE:
                    case TB_MEMSET:
                    case TB_MEMCPY: {
                        TB_NodeMemAccess* mem = TB_NODE_GET_EXTRA(n);
                        printf(" !align(%d)", mem->align);
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
                        static const char* order_names[] = {
                            "relaxed", "consume", "acquire",
                            "release", "acqrel", "seqcst"
                        };

                        TB_NodeAtomic* atomic = TB_NODE_GET_EXTRA(n);
                        printf(" !order(%s)", order_names[atomic->order]);
                        if (n->type == TB_ATOMIC_CAS) {
                            printf(" !fail_order(%s)", order_names[atomic->order2]);
                        }
                        break;
                    }

                    case TB_CALL:
                    case TB_SYSCALL:
                    case TB_SAFEPOINT_POLL:
                    break;

                    case TB_LOCAL: {
                        TB_NodeLocal* l = TB_NODE_GET_EXTRA(n);
                        printf("!size(%u) !align(%u)", l->size, l->align);
                        break;
                    }

                    default: tb_assert(extra_bytes(n) == 0, "TODO");
                }
                break;
            }
        }

        ptrdiff_t search = nl_map_get(ctx->f->attribs, n);
        if (search >= 0) {
            DynArray(TB_Attrib) attribs = ctx->f->attribs[search].v;
            dyn_array_for(i, attribs) {
                if (attribs[i].tag == TB_ATTRIB_VARIABLE) {
                    printf(" !var(%s)", attribs[i].var.name);
                }
            }
        }

        if (ctx->opt->error_n == n) {
            printf("\x1b[31m  <-- ERROR\x1b[0m");
        }

        printf("\n");
    }

    dyn_array_set_length(ws->items, ctx->cfg.block_count);

    if (bb->end->type != TB_END &&
        bb->end->type != TB_TRAP &&
        bb->end->type != TB_BRANCH &&
        bb->end->type != TB_UNREACHABLE) {
        printf("  goto ");
        print_branch_edge(ctx, bb->end, true);
        printf("\n");
    }
}

bool tb_pass_print(TB_Passes* opt) {
    TB_Function* f = opt->f;

    Worklist old = opt->worklist;
    Worklist tmp_ws = { 0 };
    worklist_alloc(&tmp_ws, f->node_count);

    PrinterCtx ctx = { opt, f };
    opt->worklist = tmp_ws;
    ctx.cfg = tb_compute_rpo(f, opt);

    // schedule nodes
    tb_pass_schedule(opt, ctx.cfg);
    worklist_clear_visited(&opt->worklist);

    TB_Node* end_bb = NULL;
    FOREACH_N(i, 0, ctx.cfg.block_count) {
        TB_Node* end = nl_map_get_checked(ctx.cfg.node_to_block, opt->worklist.items[i]).end;
        if (end == f->stop_node) {
            end_bb = opt->worklist.items[i];
            continue;
        }

        print_bb(&ctx, opt->worklist.items[i]);
    }

    if (end_bb != NULL) {
        print_bb(&ctx, end_bb);
    }

    worklist_free(&tmp_ws);
    tb_free_cfg(&ctx.cfg);
    opt->worklist = old;
    opt->error_n = NULL;
    return false;
}
