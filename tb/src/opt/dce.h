
bool tb_uses_effects(TB_Node* n) {
    switch (n->type) {
        // memory effects
        case TB_LOAD:
        case TB_READ:
        case TB_WRITE:
        case TB_STORE:
        case TB_MEMCPY:
        case TB_MEMSET:
        case TB_ATOMIC_LOAD:
        case TB_ATOMIC_XCHG:
        case TB_ATOMIC_ADD:
        case TB_ATOMIC_SUB:
        case TB_ATOMIC_AND:
        case TB_ATOMIC_XOR:
        case TB_ATOMIC_OR:
        return true;

        case TB_PROJ:
        return n->dt.type == TB_CONTROL;

        // control flow
        case TB_PHI:
        case TB_START:
        case TB_REGION:
        case TB_BRANCH:
        case TB_END:
        case TB_UNREACHABLE:
        case TB_DEBUGBREAK:
        case TB_TRAP:
        case TB_SYSCALL:
        case TB_CALL:
        return true;

        default:
        return false;
    }
}

bool tb_has_effects(TB_Node* n) {
    switch (n->type) {
        // memory effects
        case TB_READ:
        case TB_WRITE:
        case TB_STORE:
        case TB_MEMCPY:
        case TB_MEMSET:
        case TB_ATOMIC_LOAD:
        case TB_ATOMIC_XCHG:
        case TB_ATOMIC_ADD:
        case TB_ATOMIC_SUB:
        case TB_ATOMIC_AND:
        case TB_ATOMIC_XOR:
        case TB_ATOMIC_OR:
        return true;

        case TB_PROJ:
        return n->dt.type == TB_CONTROL;

        // control flow
        case TB_START:
        case TB_REGION:
        case TB_BRANCH:
        case TB_END:
        case TB_UNREACHABLE:
        case TB_DEBUGBREAK:
        case TB_TRAP:
        case TB_SYSCALL:
        case TB_CALL:
        return true;

        default:
        return false;
    }
}
