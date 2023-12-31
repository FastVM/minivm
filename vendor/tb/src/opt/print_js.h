
struct vm_io_buffer_t;

struct vm_io_buffer_t *vm_io_buffer_new(void);
void vm_io_buffer_format(struct vm_io_buffer_t *buf, const char *fmt, ...);
char *vm_io_buffer_get(struct vm_io_buffer_t *buf);

typedef struct {
    TB_Passes* opt;
    TB_Function* f;
    TB_CFG cfg;
    TB_Scheduler sched;
    struct vm_io_buffer_t *buf;
} JsPrinterCtx;

static void js_print_type_name(JsPrinterCtx* ctx, TB_DataType dt) {
    switch (dt.type) {
        case TB_INT: {
            if (dt.data == 0) vm_io_buffer_format(ctx->buf, "tb.types.void");
            else vm_io_buffer_format(ctx->buf, "tb.types.int(%d)", dt.data);
            break;
        }
        case TB_PTR: {
            if (dt.data == 0) vm_io_buffer_format(ctx->buf, "tb.types.ptr");
            else vm_io_buffer_format(ctx->buf, "tb.types.ptr%d", dt.data);
            break;
        }
        case TB_FLOAT: {
            if (dt.data == TB_FLT_32) vm_io_buffer_format(ctx->buf, "tb.types.f32");
            if (dt.data == TB_FLT_64) vm_io_buffer_format(ctx->buf, "tb.types.f64");
            break;
        }
        case TB_TUPLE: {
            vm_io_buffer_format(ctx->buf, "tb.types.tuple");
            break;
        }
        case TB_CONTROL: {
            vm_io_buffer_format(ctx->buf, "tb.types.control");
            break;
        }
        case TB_MEMORY: {
            vm_io_buffer_format(ctx->buf, "tb.types.memory");
            break;
        }
        default: tb_todo();
    }
}

static void js_print_ref_to_node(JsPrinterCtx* ctx, TB_Node* n, bool def) {
    if (n == NULL) {
        vm_io_buffer_format(ctx->buf, "_");
    } else if (n->type == TB_ROOT) {
        if (def) {
            vm_io_buffer_format(ctx->buf, "const ");
        }

        vm_io_buffer_format(ctx->buf, "v%zu", n->gvn);

        if (def) {
            vm_io_buffer_format(ctx->buf, " = (");
            TB_Node** params = ctx->f->params;
            bool any = false;
            FOREACH_N(i, 1, 3 + ctx->f->param_count) {
                if (params[i] == NULL && params[i]->dt.type != TB_MEMORY && params[i]->dt.type != TB_CONTROL) {
                    if (any) {
                        vm_io_buffer_format(ctx->buf, ", ");
                    }

                    vm_io_buffer_format(ctx->buf, "v%u", params[i]->gvn);
                    any = true;
                }
            }
            vm_io_buffer_format(ctx->buf, ") => ");
        }
    } else if (n->type == TB_PROJ && n->dt.type == TB_CONTROL) {
        if (def) {
            vm_io_buffer_format(ctx->buf, "const v%zu = (", n->gvn);
            bool first = true;
            size_t num_users = 0;
            FOR_USERS(u, n) {
                if (u->n->gvn != 0 && u->n->dt.type != TB_CONTROL && u->n->dt.type != TB_MEMORY) {
                    if (!first) {
                        vm_io_buffer_format(ctx->buf, ", ");
                    }

                    vm_io_buffer_format(ctx->buf, "v%u", u->n->gvn);
                    first = false;
                    num_users += 1;
                }
            }
            // vm_io_buffer_format(ctx->buf, "/* #args = %zu */ ", num_users);
            vm_io_buffer_format(ctx->buf, ") => ");
        } else {
            vm_io_buffer_format(ctx->buf, "v%zu", n->gvn);
        }
    } else if (n->type == TB_REGION || (n->type == TB_PROJ && n->dt.type == TB_CONTROL)) {
        TB_NodeRegion* r = TB_NODE_GET_EXTRA(n);
        if (def) {
            vm_io_buffer_format(ctx->buf, "const ");
        }

        vm_io_buffer_format(ctx->buf, "v%zu", n->gvn);
        
        if (def) {
            bool first = true;
            vm_io_buffer_format(ctx->buf, " = (");
            FOR_USERS(u, n) {
                // if (u->n->type == TB_PHI) {
                    if (u->n->gvn != 0 && u->n->dt.type != TB_CONTROL && u->n->dt.type != TB_MEMORY) {
                        if (!first) {
                            vm_io_buffer_format(ctx->buf, ", ");
                        }

                        vm_io_buffer_format(ctx->buf, "v%u", u->n->gvn);
                        first = false;
                    }
                // }
            }
            // vm_io_buffer_format(ctx->buf, ")");
            vm_io_buffer_format(ctx->buf, ") => ");
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
            vm_io_buffer_format(ctx->buf, "%s", sym->name);
        } else {
            vm_io_buffer_format(ctx->buf, "sym%p", sym);
        }
    } else if (n->type == TB_ZERO_EXT) {
        vm_io_buffer_format(ctx->buf, "(zxt.???");
        // js_print_type2(ctx, n->dt);
        vm_io_buffer_format(ctx->buf, " ");
        js_print_ref_to_node(ctx, n->inputs[1], false);
        vm_io_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_SIGN_EXT) {
        vm_io_buffer_format(ctx->buf, "(sxt.???");
        // js_print_type2(ctx, n->dt);
        vm_io_buffer_format(ctx->buf, " ");
        js_print_ref_to_node(ctx, n->inputs[1], false);
        vm_io_buffer_format(ctx->buf, ")");
    } else if (n->type == TB_INTEGER_CONST) {
        TB_NodeInt* num = TB_NODE_GET_EXTRA(n);

        if (num->value < 0xFFFF) {
            vm_io_buffer_format(ctx->buf, "%"PRId64, num->value);
        } else {
            vm_io_buffer_format(ctx->buf, "%#0"PRIx64, num->value);
        }
    } else {
        vm_io_buffer_format(ctx->buf, "v%u", n->gvn);
    }
}

// deals with printing BB params
static void js_print_branch_edge(JsPrinterCtx* ctx, TB_Node* n, bool fallthru) {
    TB_Node* target = fallthru ? cfg_next_control(n) : cfg_next_bb_after_cproj(n);
    js_print_ref_to_node(ctx, target, false);

    // print phi args
    vm_io_buffer_format(ctx->buf, "(");
    if (target->type == TB_REGION) {
        int phi_i = -1;
        FOR_USERS(u, n) {
            if (u->n->type == TB_REGION) {
                phi_i = 1 + u->slot;
                break;
            }
        }

        bool first = true;
        FOR_USERS(u, target) {
            // if (u->n->type == TB_PHI) {
                if (u->n->inputs[phi_i] != NULL && u->n->inputs[phi_i]->dt.type != TB_CONTROL && u->n->inputs[phi_i]->dt.type != TB_MEMORY) {
                    if (!first) {
                        vm_io_buffer_format(ctx->buf, ", ");
                    }

                    assert(phi_i >= 0);
                    js_print_ref_to_node(ctx, u->n->inputs[phi_i], false);
                    first = false;
                }
            // }
        }
    }
    vm_io_buffer_format(ctx->buf, ")");
}

static void js_print_bb(JsPrinterCtx* ctx, TB_Node* bb_start) {
    js_print_ref_to_node(ctx, bb_start, true);
    vm_io_buffer_format(ctx->buf, "{\n");

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
                    vm_io_buffer_format(ctx->buf, "  b1 ");
                //     js_print_branch_edge(ctx, succ[0], false);
                } else if (br->succ_count == 2) {
                    vm_io_buffer_format(ctx->buf, "  if (");
                    FOREACH_N(i, 1, n->input_count) {
                        if (i != 1) vm_io_buffer_format(ctx->buf, ", ");
                        js_print_ref_to_node(ctx, n->inputs[i], false);
                    }
                    if (br->keys[0] == 0) {
                        vm_io_buffer_format(ctx->buf, " !== 0");
                    } else {
                        vm_io_buffer_format(ctx->buf, " !== %"PRId64, br->keys[0]);
                    }
                    vm_io_buffer_format(ctx->buf, ") {\n    return () => ");
                    js_print_branch_edge(ctx, succ[0], false);
                    vm_io_buffer_format(ctx->buf, ";\n  } else {\n    return () => ");
                    js_print_branch_edge(ctx, succ[1], false);
                    vm_io_buffer_format(ctx->buf, ";\n  }");
                //     vm_io_buffer_format(ctx->buf, " else ");
                } else {
                    vm_io_buffer_format(ctx->buf, "  b%zu ", (size_t) br->succ_count);
                //     vm_io_buffer_format(ctx->buf, "  br ");
                //     FOREACH_N(i, 1, n->input_count) {
                //         if (i != 1) vm_io_buffer_format(ctx->buf, ", ");
                //         js_print_ref_to_node(ctx, n->inputs[i], false);
                //     }
                //     vm_io_buffer_format(ctx->buf, "%s=> {\n", n->input_count > 1 ? " " : "");

                //     FOREACH_N(i, 0, br->succ_count) {
                //         if (i != 0) vm_io_buffer_format(ctx->buf, "    %"PRId64": ", br->keys[i - 1]);
                //         else vm_io_buffer_format(ctx->buf, "    default: ");

                //         js_print_branch_edge(ctx, succ[i], false);
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
                vm_io_buffer_format(ctx->buf, "  return [");
                bool any = false;
                FOREACH_N(i, 1, n->input_count) {
                    if (i >= 4) {
                        if (any) vm_io_buffer_format(ctx->buf, ", ");
                        js_print_ref_to_node(ctx, n->inputs[i], false);
                        any = true;
                    }
                }
                vm_io_buffer_format(ctx->buf, "];\n");
                break;
            }

            case TB_CALLGRAPH: {
                break;
            }

            default: {
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
                //         // js_print_type2(ctx, projs[i]->dt);
                //     }
                //     vm_io_buffer_format(ctx->buf, ")");
                // } else {
                    // print as normal instruction
                    vm_io_buffer_format(ctx->buf, "  const v%u = tb.%s(", n->gvn, tb_node_get_name(n));

                    TB_DataType dt = n->dt;
                    if (n->type >= TB_CMP_EQ && n->type <= TB_CMP_FLE) {
                        dt = TB_NODE_GET_EXTRA_T(n, TB_NodeCompare)->cmp_dt;
                    } else if (n->type == TB_STORE) {
                        dt = n->inputs[3]->dt;
                    }
                    // js_print_type2(ctx, dt);
                // }

                js_print_type_name(ctx, dt);
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
                        vm_io_buffer_format(ctx->buf, ", ");
                        js_print_type_name(ctx, n->inputs[1]->dt);
                        break;

                    case TB_MEMBER_ACCESS: {
                        // vm_io_buffer_format(ctx->buf, ", %"PRId64, TB_NODE_GET_EXTRA_T(n, TB_NodeMember)->offset);
                        break;
                    }

                    case TB_ARRAY_ACCESS: {
                        // vm_io_buffer_format(ctx->buf, ", %"PRId64, TB_NODE_GET_EXTRA_T(n, TB_NodeArray)->stride);
                        break;
                    }

                    case TB_PROJ: {
                        // vm_io_buffer_format(ctx->buf, ", %d", TB_NODE_GET_EXTRA_T(n, TB_NodeProj)->index);
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
                        // TB_NodeBinopInt* b = TB_NODE_GET_EXTRA(n);
                        // if (b->ab & TB_ARITHMATIC_NSW) vm_io_buffer_format(ctx->buf, " !nsw");
                        // if (b->ab & TB_ARITHMATIC_NUW) vm_io_buffer_format(ctx->buf, " !nuw");
                        break;
                    }

                    case TB_LOAD:
                    case TB_STORE:
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

                    case TB_LOCAL: {
                        TB_NodeLocal* l = TB_NODE_GET_EXTRA(n);
                        vm_io_buffer_format(ctx->buf, ", 0x%x", l->size);
                        // if (l->type) {
                        //     vm_io_buffer_format(ctx->buf, " !var(%s)", l->name);
                        // }
                        break;
                    }

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
                        js_print_ref_to_node(ctx, n->inputs[i], false);
                    }
                }
                vm_io_buffer_format(ctx->buf, ");\n");
                break;
            }
        }
    }

    dyn_array_set_length(ws->items, ctx->cfg.block_count);

    if (!cfg_is_terminator(bb->end)) {
        vm_io_buffer_format(ctx->buf, "  return () => ");
        js_print_branch_edge(ctx, bb->end, true);
        vm_io_buffer_format(ctx->buf, "\n");
    }

    vm_io_buffer_format(ctx->buf, "};\n");
}

void js_print_begin(JsPrinterCtx *ctx) {
    vm_io_buffer_format(ctx->buf, "import tb from './tb.js';\n");
}

void js_print_end(JsPrinterCtx *ctx) {
    vm_io_buffer_format(ctx->buf, "tb.main(v1);\n");
}

char *tb_pass_js_print(TB_Passes* opt) {
    TB_Function* f = opt->f;
    cuikperf_region_start("print", NULL);

    Worklist old = opt->worklist;
    Worklist tmp_ws = { 0 };
    worklist_alloc(&tmp_ws, f->node_count);

    JsPrinterCtx ctx = { opt, f };
    ctx.buf = vm_io_buffer_new();

    opt->worklist = tmp_ws;
    ctx.cfg = tb_compute_rpo(f, opt);

    // does the IR printing need smart scheduling lol (yes... when we're testing things)
    ctx.sched = greedy_scheduler;

    TB_ArenaSavepoint sp = tb_arena_save(tmp_arena);

    // schedule nodes
    tb_pass_schedule(opt, ctx.cfg, false);
    worklist_clear_visited(&opt->worklist);

    js_print_begin(&ctx);
    
    TB_Node* end_bb = NULL;
    FOREACH_N(i, 0, ctx.cfg.block_count) {
        TB_Node* end = nl_map_get_checked(ctx.cfg.node_to_block, opt->worklist.items[i]).end;
        if (end == f->root_node) {
            end_bb = opt->worklist.items[i];
            continue;
        }

        js_print_bb(&ctx, opt->worklist.items[i]);
    }

    if (end_bb != NULL) {
        js_print_bb(&ctx, end_bb);
    }

    js_print_end(&ctx);

    tb_arena_restore(tmp_arena, sp);
    worklist_free(&opt->worklist);
    tb_free_cfg(&ctx.cfg);
    opt->worklist = old;
    opt->scheduled = NULL;
    opt->error_n = NULL;
    cuikperf_region_end();

    return vm_io_buffer_get(ctx.buf);
}
