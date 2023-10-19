
typedef struct {
    TB_Passes* opt;
    TB_Function* f;
    size_t block_count;
} PrinterCtx;

static void print_type(TB_DataType dt) {
    assert(dt.width < 8 && "Vector width too big!");

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

static void print_ref_to_node(PrinterCtx* ctx, TB_Node* n, bool def) {
    if (n == NULL) {
        printf("_");
    } else if (n->type == TB_START) {
        printf("%s", ctx->f->super.name);

        if (def) {
            printf("(");
            TB_Node** params = ctx->f->params;
            FOREACH_N(i, 1, 1 + ctx->f->param_count) {
                if (i > 1) printf(", ");

                if (params[i] == NULL) {
                    printf("_");
                } else {
                    printf("v%zu: ", params[i]->gvn);
                    print_type(params[i]->dt);
                }
            }
            printf(")");
        }
    } else if (n->type == TB_REGION) {
        TB_NodeRegion* r = TB_NODE_GET_EXTRA(n);
        if (r->tag != NULL) {
            printf(".%s", r->tag);
        } else {
            ptrdiff_t i = try_find_traversal_index(n);
            if (i >= 0) {
                printf(".bb%zu", i);
            } else {
                printf("*DEAD*");
            }
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
    } else if (n->type == TB_ZERO_EXT) {
        printf("(zxt.");
        print_type(n->dt);
        printf(" ");
        print_ref_to_node(ctx, n->inputs[1], false);
        printf(")");
    } else if (n->type == TB_SIGN_EXT) {
        printf("(sxt.");
        print_type(n->dt);
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
        printf("v%llu", (long long unsigned) n->gvn);
    }
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

static void print_bb(PrinterCtx* ctx, TB_Node* bb) {
    assert(bb->type == TB_START || bb->type == TB_REGION);
    print_ref_to_node(ctx, bb, true);
    printf(":");

    // print predecessors
    if (bb->input_count > 0) {
        printf(" # preds: ");
        FOREACH_N(j, 0, bb->input_count) {
            print_ref_to_node(ctx, tb_get_parent_region(bb->inputs[j]), false);
            printf(" ");
        }
    }

    if (ctx->opt->error_n == bb) {
        printf("\x1b[31m  <-- ERROR\x1b[0m");
    }
    printf("\n");

    TB_Node* end = TB_NODE_GET_EXTRA_T(bb, TB_NodeRegion)->end;
    Worklist* ws = &ctx->opt->worklist;

    sched_walk(ctx->opt, ws, NULL, bb, end);
    assert(ws->items[ctx->block_count] == bb);

    TB_Node* prev_effect = NULL;
    FOREACH_N(i, ctx->block_count + 1, dyn_array_length(ws->items)) {
        TB_Node* n = ws->items[i];

        // skip these
        if (n->type == TB_INTEGER_CONST || n->type == TB_FLOAT32_CONST ||
            n->type == TB_FLOAT64_CONST || n->type == TB_SYMBOL ||
            n->type == TB_SIGN_EXT || n->type == TB_ZERO_EXT ||
            n->type == TB_PROJ) {
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
                if (br->succ_count == 1) {
                    printf("  goto ");
                    print_ref_to_node(ctx, br->succ[0], false);
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
                    print_ref_to_node(ctx, br->succ[0], false);
                    printf(" else ");
                    print_ref_to_node(ctx, br->succ[1], false);
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

                        print_ref_to_node(ctx, br->succ[i], false);
                        printf("\n");
                    }
                    printf("  }");
                }
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

                    for (User* use = find_users(ctx->opt, n); use; use = use->next) {
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
                        printf("v%zu", projs[i]->gvn);
                    }
                    printf(" = %s.(", tb_node_get_name(n));
                    FOREACH_N(i, first, 4) {
                        if (projs[i] == NULL) break;
                        if (i > first) printf(", ");
                        print_type(projs[i]->dt);
                    }
                    printf(")");
                } else {
                    // print as normal instruction
                    if (n->dt.type == TB_INT && n->dt.data == 0) {
                        printf("  %s.", tb_node_get_name(n));
                    } else {
                        printf("  v%zu = %s.", n->gvn, tb_node_get_name(n));
                    }

                    TB_DataType dt = n->dt;
                    if (n->type >= TB_CMP_EQ && n->type <= TB_CMP_FLE) {
                        dt = TB_NODE_GET_EXTRA_T(n, TB_NodeCompare)->cmp_dt;
                    } else if (n->type == TB_STORE) {
                        dt = n->inputs[3]->dt;
                    }
                    print_type(dt);
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

    dyn_array_set_length(ws->items, ctx->block_count);
}

bool tb_pass_print(TB_Passes* opt) {
    TB_Function* f = opt->f;

    // schedule nodes
    tb_pass_schedule(opt);

    PrinterCtx ctx = { opt, f };
    worklist_clear(&opt->worklist);

    ctx.block_count = tb_push_postorder(f, &opt->worklist);
    TB_Node* stop_bb = get_block_begin(f->stop_node);

    worklist_clear_visited(&opt->worklist);

    bool has_stop = false;
    FOREACH_REVERSE_N(i, 0, ctx.block_count) {
        TB_Node* bb = opt->worklist.items[i];
        if (bb != stop_bb) {
            print_bb(&ctx, bb);
        } else {
            has_stop = true;
        }
    }

    if (has_stop) {
        print_bb(&ctx, stop_bb);
    }

    ctx.opt->error_n = NULL;
    return false;
}
