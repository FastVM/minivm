
// this is a peephole lmao
static TB_Node* ideal_libcall(TB_Passes* restrict passes, TB_Function* f, TB_Node* n) {
    if (n->inputs[2]->type != TB_SYMBOL) {
        return NULL;
    }

    const TB_Symbol* sym = TB_NODE_GET_EXTRA_T(n->inputs[2], TB_NodeSymbol)->sym;

    bool is_memcpy = strcmp(sym->name, "memcpy") == 0;
    bool is_memset = strcmp(sym->name, "memset") == 0;
    if (is_memcpy || is_memset) {
        TB_Node* n2 = tb_alloc_node(f, is_memset ? TB_MEMSET : TB_MEMCPY, TB_TYPE_MEMORY, 5, sizeof(TB_NodeMemAccess));
        set_input(f, n2, n->inputs[0], 0); // ctrl
        set_input(f, n2, n->inputs[1], 1); // mem
        set_input(f, n2, n->inputs[3], 2); // dst
        set_input(f, n2, n->inputs[4], 3); // val
        set_input(f, n2, n->inputs[5], 4); // size
        TB_NODE_SET_EXTRA(n2, TB_NodeMemAccess, .align = 1);

        TB_Node* dst_ptr = n->inputs[2];
        TB_Node* ctrl = n->inputs[0];

        // returns the destination pointer, convert any users of that to dst
        // and CALL has a projection we wanna get rid of
        TB_NodeCall* c = TB_NODE_GET_EXTRA_T(n, TB_NodeCall);
        subsume_node(passes, f, c->projs[0], ctrl);
        subsume_node(passes, f, c->projs[1], n2);
        subsume_node(passes, f, c->projs[2], dst_ptr);
        return dst_ptr;
    }

    return NULL;
}

