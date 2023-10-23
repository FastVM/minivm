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
        case TB_DEAD: return "dead";
        case TB_UNREACHABLE: return "unreachable";

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

        case TB_CLZ: return "clz";
        case TB_CTZ: return "ctz";
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
        case TB_TAILCALL: return "tailcall";

        default: tb_todo();return "(unknown)";
    }
}

#define P(...) callback(user_data, __VA_ARGS__)
static void tb_print_type(TB_DataType dt, TB_PrintCallback callback, void* user_data) {
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

static bool print_graph_node(TB_Function* f, NL_HashSet* visited, TB_PrintCallback callback, void* user_data, TB_Node* restrict n) {
    do {
        if (!nl_hashset_put(visited, n)) {
            return false;
        }

        /*bool is_effect = tb_has_effects(n);
        const char* fillcolor = is_effect ? "lightgrey" : "antiquewhite1";
        if (n->dt.type == TB_MEMORY) {
            fillcolor = "lightblue";
        }*/

        P("  r%u [ordering=in; shape=plaintext; label=\"", n->gvn);
        switch (n->type) {
            case TB_START: P("start"); break;
            case TB_REGION: P("region"); break;

            case TB_LOAD: {
                P("ld.");
                tb_print_type(n->dt, callback, user_data);
                break;
            }
            case TB_STORE: {
                P("st.");
                tb_print_type(n->inputs[2]->dt, callback, user_data);
                break;
            }

            case TB_SYMBOL: {
                TB_Symbol* sym = TB_NODE_GET_EXTRA_T(n, TB_NodeSymbol)->sym;
                if (sym->name[0]) {
                    P("%s", sym->name);
                } else {
                    P("sym%p", sym);
                }
                break;
            }

            case TB_BITCAST: {
                P("bitcast ");
                tb_print_type(n->inputs[1]->dt, callback, user_data);
                P(" -> ");
                tb_print_type(n->dt, callback, user_data);
                break;
            }

            case TB_INTEGER_CONST: {
                TB_NodeInt* num = TB_NODE_GET_EXTRA(n);
                if (num->value < 0xFFFF) {
                    P("%"PRId64, num->value);
                } else {
                    P("%#0"PRIx64, num->value);
                }
                break;
            }

            case TB_ARRAY_ACCESS: {
                int64_t stride = TB_NODE_GET_EXTRA_T(n, TB_NodeArray)->stride;
                P("*%td", stride);
                break;
            }

            case TB_MEMBER_ACCESS: {
                int64_t offset = TB_NODE_GET_EXTRA_T(n, TB_NodeMember)->offset;
                P("+%td", offset);
                break;
            }

            case TB_PROJ: {
                int index = TB_NODE_GET_EXTRA_T(n, TB_NodeProj)->index;
                if (n->inputs[0]->type == TB_START) {
                    if (index == 0) {
                        P("ctrl");
                    } else if (index == 1) {
                        P("mem");
                    } else if (index == 2) {
                        P("rpc");
                    } else {
                        P("%c", 'a'+(index - 3));
                    }
                } else {
                    P("%d", index);
                }
                break;
            }

            default:
            P("%s", tb_node_get_name(n));
            break;
        }
        P("\"]");

        FOREACH_N(i, 0, n->input_count) if (n->inputs[i]) {
            TB_Node* in = n->inputs[i];

            const char* color = "black";
            TB_DataType dt = n->type == TB_PROJ ? n->dt : in->dt;

            if (dt.type == TB_CONTROL) {
                color = "red";
            } else if (dt.type == TB_CONT) {
                color = "purple";
            } else if (dt.type == TB_MEMORY) {
                color = "blue";
            }

            P("; r%u -> r%u [color=%s]", in->gvn, n->gvn, color);
        }
        P("\n");

        // print all the inputs
        FOREACH_N(i, 1, n->input_count) if (n->inputs[i]) {
            print_graph_node(f, visited, callback, user_data, n->inputs[i]);
        }

        if (n->input_count == 0) break;
        n = n->inputs[0];
    } while (n != NULL);

    return true;
}

TB_API void tb_pass_print_dot(TB_Passes* opt, TB_PrintCallback callback, void* user_data) {
    TB_Function* f = opt->f;
    P("digraph %s {\n", f->super.name ? f->super.name : "unnamed");

    Worklist tmp_ws = { 0 };
    worklist_alloc(&tmp_ws, f->node_count);

    TB_CFG cfg = tb_compute_rpo2(f, &tmp_ws, &opt->stack);

    NL_HashSet visited = nl_hashset_alloc(f->node_count);
    FOREACH_N(i, 0, cfg.block_count) {
        TB_BasicBlock* bb = &nl_map_get_checked(cfg.node_to_block, tmp_ws.items[i]);
        print_graph_node(f, &visited, callback, user_data, bb->end);
    }
    nl_hashset_free(visited);
    worklist_free(&tmp_ws);
    tb_free_cfg(&cfg);

    P("}\n");
}
