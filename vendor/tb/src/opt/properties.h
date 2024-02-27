// Keeping track of all kinds of TB node properties
static bool is_associative(TB_NodeTypeEnum type) {
    switch (type) {
        case TB_ADD: case TB_MUL:
        case TB_AND: case TB_XOR: case TB_OR:
        return true;

        default:
        return false;
    }
}

static bool is_commutative(TB_NodeTypeEnum type) {
    switch (type) {
        case TB_ADD: case TB_MUL:
        case TB_AND: case TB_XOR: case TB_OR:
        case TB_CMP_NE: case TB_CMP_EQ:
        case TB_FADD: case TB_FMUL:
        return true;

        default:
        return false;
    }
}

static bool is_mem_access(TB_Node* n) {
    switch (n->type) {
        case TB_LOAD:
        case TB_STORE:
        case TB_MEMCPY:
        case TB_MEMSET:
        return true;

        default:
        return false;
    }
}

static bool is_effect_tuple(TB_Node* n) {
    switch (n->type) {
        case TB_CALL:
        case TB_SYSCALL:
        case TB_TAILCALL:
        case TB_READ:
        case TB_WRITE:
        case TB_INLINE_ASM:
        return true;

        default:
        return false;
    }
}

static bool cfg_is_region(TB_Node* n) {
    return n->type >= TB_REGION && n->type <= TB_AFFINE_LOOP;
}

static bool cfg_is_natural_loop(TB_Node* n) {
    return n->type >= TB_NATURAL_LOOP && n->type <= TB_AFFINE_LOOP;
}

static bool cfg_is_terminator(TB_Node* n) {
    switch (n->type) {
        case TB_BRANCH:
        case TB_UNREACHABLE:
        case TB_TRAP:
        case TB_RETURN:
        case TB_TAILCALL:
        case TB_ROOT:
        return true;

        default:
        return false;
    }
}

static bool cfg_is_endpoint(TB_Node* n) {
    switch (n->type) {
        case TB_UNREACHABLE:
        case TB_TRAP:
        case TB_RETURN:
        case TB_TAILCALL:
        case TB_ROOT:
        return true;

        default:
        return false;
    }
}
