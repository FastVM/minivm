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

static bool is_effect_tuple(TB_Node* n) {
    switch (n->type) {
        case TB_CALL:
        case TB_SYSCALL:
        case TB_TAILCALL:
        case TB_READ:
        case TB_WRITE:
        case TB_MACHINE_OP:
        return true;

        default:
        return false;
    }
}

// terminator without successors
static bool cfg_is_endpoint(TB_Node* n) {
    switch (n->type) {
        case TB_UNREACHABLE:
        case TB_TRAP:
        case TB_END:
        case TB_TAILCALL:
        return true;

        default:
        return false;
    }
}

static bool cfg_is_terminator(TB_Node* n) {
    switch (n->type) {
        case TB_BRANCH:
        case TB_UNREACHABLE:
        case TB_TRAP:
        case TB_END:
        case TB_TAILCALL:
        return true;

        default:
        return false;
    }
}
