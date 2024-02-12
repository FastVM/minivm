
static void dumb_print_node_internal(TB_Function* f, Lattice** types, TB_Node* n, uint64_t* visited) {
    printf("%%%u: ", n->gvn);
    if (types && types[n->gvn] != NULL && types[n->gvn] != &TOP_IN_THE_SKY) {
        print_lattice(types[n->gvn], n->dt);
    } else {
        if (n->dt.type == TB_TUPLE) {
            // print with multiple returns
            TB_Node* projs[32] = { 0 };
            FOR_USERS(use, n) {
                if (use->n->type == TB_PROJ) {
                    int index = TB_NODE_GET_EXTRA_T(use->n, TB_NodeProj)->index;
                    projs[index] = use->n;
                }
            }

            printf("{ ");
            FOREACH_N(i, 0, 32) {
                if (projs[i] == NULL) break;
                if (i) printf(", ");
                print_type(projs[i]->dt);
            }
            printf(" }");
        } else {
            print_type(n->dt);
        }
    }
    printf(" = %s ", tb_node_get_name(n));
    if (n->type == TB_SYMBOL) {
        TB_Symbol* sym = TB_NODE_GET_EXTRA_T(n, TB_NodeSymbol)->sym;
        if (sym->name[0]) {
            printf("'%s' ", sym->name);
        } else {
            printf("%p ", sym);
        }
    } else if (n->type == TB_INTEGER_CONST) {
        TB_NodeInt* num = TB_NODE_GET_EXTRA(n);

        if (num->value < 0xFFFF) {
            int bits = n->dt.type == TB_PTR ? 64 : n->dt.data;
            printf("%"PRId64" ", tb__sxt(num->value, bits, 64));
        } else {
            printf("%#0"PRIx64" ", num->value);
        }
    } else if (n->type == TB_FLOAT32_CONST) {
        TB_NodeFloat32* f = TB_NODE_GET_EXTRA(n);
        printf("%f ", f->value);
    } else if (n->type == TB_FLOAT64_CONST) {
        TB_NodeFloat64* f = TB_NODE_GET_EXTRA(n);
        printf("%f ", f->value);
    }
    printf("( ");
    FOREACH_N(i, 0, n->input_count) {
        TB_Node* in = n->inputs[i];
        if (in) {
            if (in->type == TB_PROJ) {
                int index = TB_NODE_GET_EXTRA_T(in, TB_NodeProj)->index;
                in = in->inputs[0];

                printf("%%%u.%d ", in->gvn, index);
            } else {
                printf("%%%u ", in->gvn);
            }
        } else {
            printf("___ ");
        }
    }
    printf(")\n");
}

static void dumb_print_node(TB_Function* f, Lattice** types, TB_Node* n, uint64_t* visited) {
    if (visited[n->gvn / 64] & (1ull << (n->gvn % 64))) {
        return;
    }
    visited[n->gvn / 64] |= (1ull << (n->gvn % 64));

    FOREACH_REVERSE_N(i, 0, n->input_count) if (n->inputs[i]) {
        TB_Node* in = n->inputs[i];
        if (in->type == TB_PROJ) { in = in->inputs[0]; }

        dumb_print_node(f, types, in, visited);
    }

    dumb_print_node_internal(f, types, n, visited);
}

void tb_dumb_print(TB_Function* f, TB_Passes* p) {
    printf("=== DUMP %s ===\n", f->super.name);

    uint64_t* visited = tb_platform_heap_alloc(((f->node_count + 63) / 64) * sizeof(uint64_t));
    memset(visited, 0, ((f->node_count + 63) / 64) * sizeof(uint64_t));

    TB_Node* root   = f->root_node;
    Lattice** types = p ? p->types : NULL;

    dumb_print_node_internal(f, types, root, visited);
    visited[root->gvn / 64] |= (1ull << (root->gvn % 64));

    FOREACH_N(i, 0, root->input_count) {
        dumb_print_node(f, types, root->inputs[i], visited);
    }
}
