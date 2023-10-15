#include "tb_internal.h"
#include <stdarg.h>

TB_API void tb_default_print_callback(void* user_data, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf((FILE*)user_data, fmt, ap);
    va_end(ap);
}

TB_API const char* tb_node_get_name(TB_Node* n) {
    switch (n->type) {
        case TB_NULL: return "BAD";

        case TB_START:  return "start";
        case TB_END:    return "end";
        case TB_PROJ:   return "proj";
        case TB_REGION: return "region";

        case TB_LOCAL: return "local";

        case TB_VA_START: return "vastart";
        case TB_DEBUGBREAK: return "dbgbrk";

        case TB_POISON: return "poison";
        case TB_INTEGER_CONST: return "int";
        case TB_FLOAT32_CONST: return "float32";
        case TB_FLOAT64_CONST: return "float64";

        case TB_PHI: return "phi";
        case TB_SELECT: return "select";

        case TB_ARRAY_ACCESS: return "array";
        case TB_MEMBER_ACCESS: return "member";

        case TB_PTR2INT: return "ptr2int";
        case TB_INT2PTR: return "int2ptr";

        case TB_SAFEPOINT_POLL: return "safepoint";

        case TB_MEMSET: return "memset";
        case TB_MEMCPY: return "memcpy";

        case TB_ZERO_EXT: return "zxt";
        case TB_SIGN_EXT: return "sxt";
        case TB_FLOAT_EXT: return "fpxt";
        case TB_TRUNCATE: return "trunc";
        case TB_BITCAST: return "bitcast";
        case TB_UINT2FLOAT: return "uint2float";
        case TB_INT2FLOAT: return "int2float";
        case TB_FLOAT2UINT: return "float2uint";
        case TB_FLOAT2INT: return "float2int";
        case TB_SYMBOL: return "symbol";

        case TB_CMP_NE: return "cmp.ne";
        case TB_CMP_EQ: return "cmp.eq";
        case TB_CMP_ULT: return "cmp.ult";
        case TB_CMP_ULE: return "cmp.ule";
        case TB_CMP_SLT: return "cmp.slt";
        case TB_CMP_SLE: return "cmp.sle";
        case TB_CMP_FLT: return "cmp.lt";
        case TB_CMP_FLE: return "cmp.le";

        case TB_ATOMIC_LOAD: return "atomic.load";
        case TB_ATOMIC_XCHG: return "atomic.xchg";
        case TB_ATOMIC_ADD: return "atomic.add";
        case TB_ATOMIC_SUB: return "atomic.sub";
        case TB_ATOMIC_AND: return "atomic.and";
        case TB_ATOMIC_XOR: return "atomic.xor";
        case TB_ATOMIC_OR: return "atomic.or";
        case TB_ATOMIC_CAS: return "atomic.cas";

        case TB_NEG: return "neg";
        case TB_NOT: return "not";
        case TB_AND: return "and";
        case TB_OR: return "or";
        case TB_XOR: return "xor";
        case TB_ADD: return "add";
        case TB_SUB: return "sub";
        case TB_MUL: return "mul";
        case TB_UDIV: return "udiv";
        case TB_SDIV: return "sdiv";
        case TB_UMOD: return "umod";
        case TB_SMOD: return "smod";
        case TB_SHL: return "shl";
        case TB_SHR: return "shr";
        case TB_ROL: return "rol";
        case TB_ROR: return "ror";
        case TB_SAR: return "sar";

        case TB_FADD: return "fadd";
        case TB_FSUB: return "fsub";
        case TB_FMUL: return "fmul";
        case TB_FDIV: return "fdiv";
        case TB_FMAX: return "fmax";
        case TB_FMIN: return "fmin";

        case TB_MULPAIR: return "mulpair";
        case TB_LOAD: return "load";
        case TB_STORE: return "store";
        case TB_MERGEMEM: return "merge";

        case TB_CALL: return "call";
        case TB_SYSCALL: return "syscall";
        case TB_BRANCH: return "branch";

        default: tb_todo();return "(unknown)";
    }
}

#define P(...) callback(user_data, __VA_ARGS__)
static void tb_print_type(TB_DataType dt, TB_PrintCallback callback, void* user_data) {
    assert(dt.width < 8 && "Vector width too big!");

    switch (dt.type) {
        case TB_INT: {
            if (dt.data == 0) P("void");
            else P("i%d", dt.data);
            break;
        }
        case TB_PTR: {
            if (dt.data == 0) P("ptr");
            else P("ptr%d", dt.data);
            break;
        }
        case TB_FLOAT: {
            if (dt.data == TB_FLT_32) P("f32");
            if (dt.data == TB_FLT_64) P("f64");
            break;
        }
        case TB_TUPLE: {
            P("tuple");
            break;
        }
        case TB_MEMORY: {
            P("memory");
            break;
        }
        case TB_CONTROL: {
            P("control");
            break;
        }
        case TB_CONT: {
            P("cont");
            break;
        }
        default: tb_todo();
    }
}

#if 0
static void tb_print_node(TB_Function* f, NL_HashSet* visited, TB_PrintCallback callback, void* user_data, TB_Node* restrict n) {
    if (!nl_hashset_put(visited, n)) {
        return;
    }

    bool is_effect = tb_has_effects(n);

    const char* fillcolor = is_effect ? "lightgrey" : "antiquewhite1";
    if (n->type == TB_PROJ) {
        fillcolor = "lightblue";
    }

    P("  r%p [style=\"filled\"; ordering=in; shape=box; fillcolor=%s; label=\"", n, fillcolor);
    switch (n->type) {
        case TB_FLOAT32_CONST: {
            TB_NodeFloat32* f = TB_NODE_GET_EXTRA(n);
            P("f32 %f", f->value);
            break;
        }

        case TB_FLOAT64_CONST: {
            TB_NodeFloat64* f = TB_NODE_GET_EXTRA(n);
            P("f64 %f", f->value);
            break;
        }

        case TB_INTEGER_CONST: {
            P("%s ", tb_node_get_name(n));

            TB_NodeInt* num = TB_NODE_GET_EXTRA(n);
            tb_print_type(n->dt, callback, user_data);

            if (num->value < 0xFFFF) {
                int bits = n->dt.type == TB_PTR ? 64 : n->dt.data;
                int64_t x = tb__sxt(num->value, bits, 64);

                P(" %"PRId64, x);
            } else {
                P("%#0"PRIx64, num->value);
            }
            break;
        }

        case TB_MEMBER_ACCESS: {
            TB_NodeMember* m = TB_NODE_GET_EXTRA(n);
            P("member %"PRId64, m->offset);
            break;
        }

        case TB_SYMBOL: {
            TB_NodeSymbol* s = TB_NODE_GET_EXTRA(n);
            P("symbol %s", s->sym->name ? s->sym->name : "???");
            break;
        }

        case TB_END: {
            P("stop ");
            FOREACH_N(i, 1, n->input_count) {
                if (i != 1) P(", ");
                tb_print_type(n->inputs[i]->dt, callback, user_data);
            }
            break;
        }

        case TB_STORE: {
            P("store ");
            tb_print_type(n->inputs[3]->dt, callback, user_data);
            break;
        }

        case TB_START:
        case TB_REGION:
        case TB_BRANCH:
        P("%s", tb_node_get_name(n));
        break;

        case TB_PROJ: {
            int index = TB_NODE_GET_EXTRA_T(n, TB_NodeProj)->index;

            P("proj.");
            tb_print_type(n->dt, callback, user_data);
            P(" %zu", index);
            break;
        }

        case TB_CMP_EQ:
        case TB_CMP_NE:
        case TB_CMP_ULT:
        case TB_CMP_ULE:
        case TB_CMP_SLT:
        case TB_CMP_SLE:
        case TB_CMP_FLT:
        case TB_CMP_FLE:
        P("%s ", tb_node_get_name(n));
        tb_print_type(n->inputs[1]->dt, callback, user_data);
        break;

        default:
        P("%s ", tb_node_get_name(n));
        tb_print_type(n->dt, callback, user_data);
        break;
    }
    P("\"];\n");

    FOREACH_N(i, 0, n->input_count) if (n->inputs[i]) {
        TB_Node* in = n->inputs[i];

        if (in->type == TB_PROJ && (in->inputs[0]->type != TB_START || in->dt.type == TB_CONTROL)) {
            // projections get treated as edges
            TB_Node* src = in->inputs[0];
            int index = TB_NODE_GET_EXTRA_T(in, TB_NodeProj)->index;

            tb_print_node(f, visited, callback, user_data, src);

            P("  r%p -> r%p [label=\"", src, n);
            if (src->type == TB_BRANCH) {
                // branch projections can get nicer looking
                TB_NodeBranch* br = TB_NODE_GET_EXTRA(src);

                TB_Node* key = src->input_count > 1 ? src->inputs[1] : NULL;
                if (br->keys[0] == 0 && br->succ_count == 2 && key && key->dt.type == TB_INT) {
                    // boolean branch, we can use true and false
                    P(index ? "is false?" : "is true?");
                } else if (br->succ_count == 1) {
                    P("");
                } else if (index == 0) {
                    P("is default?");
                } else {
                    P("is %d?", br->keys[index - 1]);
                }
            } else if (in->dt.type == TB_CONTROL) {
                P("cproj");
            } else {
                P("%zu", index);
            }

            if (in->dt.type == TB_CONTROL) {
                P("\"] [color=\"red\"]\n");
            } else {
                P("\"]\n");
            }
        } else {
            tb_print_node(f, visited, callback, user_data, in);
            P("  r%p -> r%p", in, n);
            if (i == 0 || n->type == TB_REGION) {
                P(" [color=\"red\"]");
            } else if (in->dt.type == TB_MEMORY) {
                P(" [color=\"blue\" style=\"dashed\"]");
            }

            if (n->type == TB_CALL && i > 1) {
                P(" [label=\"%zu\"];\n", i - 2);
            } else if (n->type == TB_PHI && i > 0) {
                P(" [label=\"%zu\"];\n", i - 1);
            } else {
                P("\n");
            }
        }
    }
}
#endif

static bool print_graph_node(TB_Function* f, NL_HashSet* visited, TB_PrintCallback callback, void* user_data, TB_Node* restrict n) {
    if (!nl_hashset_put(visited, n)) {
        return false;
    }

    bool is_effect = tb_has_effects(n);
    if (is_effect) {
        return false;
    }

    const char* fillcolor = is_effect ? "lightgrey" : "antiquewhite1";
    P("  r%p [style=\"filled\"; ordering=in; shape=box; fillcolor=%s; label=\"", n, fillcolor);
    P("%zu: %s", n->gvn, tb_node_get_name(n));
    P("\"];\n");

    FOREACH_N(i, 0, n->input_count) if (n->inputs[i]) {
        TB_Node* in = n->inputs[i];
        P("  r%p -> r%p\n", in, n);
        print_graph_node(f, visited, callback, user_data, in);
    }

    return true;
}

static void print_graph_bb(TB_Function* f, NL_HashSet* visited, TB_PrintCallback callback, void* user_data, TB_Node* restrict bb) {
    if (!nl_hashset_put(visited, bb)) {
        return;
    }

    // walk control edges (aka predecessors)
    TB_NodeRegion* r = TB_NODE_GET_EXTRA(bb);
    if (r->end->type == TB_BRANCH) {
        TB_NodeBranch* br = TB_NODE_GET_EXTRA(r->end);
        FOREACH_REVERSE_N(i, 0, br->succ_count) {
            print_graph_bb(f, visited, callback, user_data, br->succ[i]);
        }
    }

    P("  subgraph {\n", bb->gvn);
    TB_Node* curr = r->end;
    do {
        nl_hashset_put(visited, curr);
        P("    r%p [style=\"filled\"; shape=box; fillcolor=antiquewhite1; label=\"%zu: ", curr, curr->gvn);
        if (curr->type == TB_END) {
            P("END");
        } else {
            P("EFFECT");
        }
        P("\"]\n    r%p -> r%p\n", curr->inputs[0], curr);
        curr = curr->inputs[0];
    } while (curr != bb);

    // basic block header
    P("    r%p [style=\"filled\"; shape=box; fillcolor=antiquewhite1; label=\"%zu: %s\"]\n", bb, bb->gvn, bb->type == TB_START ? "START" : "REGION");
    if (bb->type == TB_START) {
        P("    { rank=min; r%p }\n", bb);
    } else if (r->end->type == TB_END) {
        P("    { rank=max; r%p }\n", r->end);
    }
    P("  }\n");

    // write predecessor edges
    FOREACH_N(i, 0, bb->input_count) {
        TB_Node* pred = bb->inputs[i];
        if (pred->type == TB_PROJ) {
            P("  r%p -> r%p\n", pred->inputs[0], bb);
        } else {
            P("  r%p -> r%p\n", bb->inputs[i], bb);
        }
    }

    // process adjacent nodes
    curr = r->end;
    do {
        FOREACH_N(i, 1, curr->input_count) {
            print_graph_node(f, visited, callback, user_data, curr->inputs[i]);
            P("    r%p -> r%p\n", curr->inputs[i], curr);
        }
        curr = curr->inputs[0];
    } while (curr != bb);
}

TB_API void tb_function_print(TB_Function* f, TB_PrintCallback callback, void* user_data) {
    P("digraph %s {\n  rankdir=TB\n", f->super.name ? f->super.name : "unnamed");

    NL_HashSet visited = nl_hashset_alloc(f->node_count);
    print_graph_bb(f, &visited, callback, user_data, f->start_node);
    nl_hashset_free(visited);

    P("}\n\n");
}
