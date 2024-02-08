
static bool get_int_const(TB_Node* n, uint64_t* imm) {
    if (n->type == TB_INTEGER_CONST) {
        TB_NodeInt* i = TB_NODE_GET_EXTRA(n);
        *imm = i->value;
        return true;
    } else {
        return false;
    }
}

////////////////////////////////
// Integer idealizations
////////////////////////////////
static TB_Node* ideal_bitcast(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    TB_Node* src = n->inputs[1];

    if (src->type == TB_BITCAST) {
        set_input(f, n, src->inputs[1], 1);
        return n;
    }

    // int -> smaller int means truncate
    if (src->dt.type == TB_INT && n->dt.type == TB_INT && src->dt.data > n->dt.data) {
        n->type = TB_TRUNCATE;
        return n;
    } else if (src->type == TB_INTEGER_CONST) {
        return make_int_node(f, opt, n->dt, TB_NODE_GET_EXTRA_T(src, TB_NodeInt)->value);
    }

    return NULL;
}

// cmp.slt(a, 0) => is_sign(a)
static bool sign_check(TB_Node* n) {
    uint64_t x;
    return n->type == TB_CMP_SLT && get_int_const(n->inputs[2], &x) && x == 0;
}

static bool is_non_zero(TB_Node* n) {
    TB_NodeInt* i = TB_NODE_GET_EXTRA(n);
    return n->type == TB_INTEGER_CONST && i->value != 0;
}

static bool is_zero(TB_Node* n) {
    TB_NodeInt* i = TB_NODE_GET_EXTRA(n);
    return n->type == TB_INTEGER_CONST && i->value == 0;
}

static bool is_one(TB_Node* n) {
    TB_NodeInt* i = TB_NODE_GET_EXTRA(n);
    return n->type == TB_INTEGER_CONST && i->value == 1;
}

static bool inverted_cmp(TB_Node* n, TB_Node* n2) {
    switch (n->type) {
        case TB_CMP_EQ: return n2->type == TB_CMP_NE && n2->inputs[1] == n->inputs[1] && n2->inputs[2] == n->inputs[2];
        case TB_CMP_NE: return n2->type == TB_CMP_EQ && n2->inputs[1] == n->inputs[1] && n2->inputs[2] == n->inputs[2];
        // flipped inputs
        case TB_CMP_SLE: return n2->type == TB_CMP_SLT && n2->inputs[2] == n->inputs[1] && n2->inputs[1] == n->inputs[2];
        case TB_CMP_ULE: return n2->type == TB_CMP_ULT && n2->inputs[2] == n->inputs[1] && n2->inputs[1] == n->inputs[2];
        case TB_CMP_SLT: return n2->type == TB_CMP_SLE && n2->inputs[2] == n->inputs[1] && n2->inputs[1] == n->inputs[2];
        case TB_CMP_ULT: return n2->type == TB_CMP_ULE && n2->inputs[2] == n->inputs[1] && n2->inputs[1] == n->inputs[2];
        default: return false;
    }
}

static Lattice* sccp_sext(TB_Passes* restrict opt, TB_Node* n) {
    Lattice* a = lattice_universe_get(opt, n->inputs[1]);
    if (a == &TOP_IN_THE_SKY) {
        return &TOP_IN_THE_SKY;
    }

    int old_bits = n->inputs[1]->dt.data;

    uint64_t sign_range = (1 << (old_bits - 1)) - 1;
    uint64_t mask = tb__mask(n->dt.data) & ~tb__mask(old_bits);

    int64_t min, max;
    if (a->_int.min >= a->_int.max || a->_int.max < sign_range) {
        min = tb__sxt(a->_int.min, old_bits, n->dt.data);
        max = tb__sxt(a->_int.max, old_bits, n->dt.data);
    } else {
        min = lattice_int_min(old_bits) | mask;
        max = lattice_int_max(old_bits);
    }

    uint64_t zeros = a->_int.known_zeros;
    uint64_t ones  = a->_int.known_ones;

    // if we know the sign bit then we can know what the extended bits look like
    if (zeros >> (old_bits - 1)) {
        zeros |= mask;
    } else if (ones >> (old_bits - 1)) {
        ones |= mask;
    }

    return lattice_intern(opt, (Lattice){ LATTICE_INT, ._int = { min, max, zeros, ones } });
}

static Lattice* sccp_zext(TB_Passes* restrict opt, TB_Node* n) {
    Lattice* a = lattice_universe_get(opt, n->inputs[1]);
    if (a == &TOP_IN_THE_SKY) {
        return &TOP_IN_THE_SKY;
    }

    uint64_t mask = tb__mask(n->dt.data) & ~tb__mask(n->inputs[1]->dt.data);

    int64_t min = a->_int.min;
    int64_t max = a->_int.max;
    uint64_t zeros = a->_int.known_zeros | mask; // we know the top bits must be zero
    uint64_t ones  = a->_int.known_ones;

    return lattice_intern(opt, (Lattice){ LATTICE_INT, ._int = { min, max, zeros, ones } });
}

static Lattice* sccp_trunc(TB_Passes* restrict opt, TB_Node* n) {
    Lattice* a = lattice_universe_get(opt, n->inputs[1]);
    if (a == &TOP_IN_THE_SKY) {
        return &TOP_IN_THE_SKY;
    }

    int64_t mask = tb__mask(n->dt.data);
    int64_t min = a->_int.min & mask;
    int64_t max = a->_int.max & mask;
    if (min > max) {
        min = 0;
        max = mask;
    }

    uint64_t zeros = a->_int.known_zeros | ~mask;
    uint64_t ones  = a->_int.known_ones  & mask;
    return lattice_intern(opt, (Lattice){ LATTICE_INT, ._int = { min, max, zeros, ones } });
}

static Lattice* sccp_arith(TB_Passes* restrict opt, TB_Node* n) {
    Lattice* a = lattice_universe_get(opt, n->inputs[1]);
    Lattice* b = lattice_universe_get(opt, n->inputs[2]);
    if (a == &TOP_IN_THE_SKY || b == &TOP_IN_THE_SKY) {
        return &TOP_IN_THE_SKY;
    }

    assert(a->tag == LATTICE_INT && b->tag == LATTICE_INT);
    uint64_t mask = tb__mask(n->dt.data);
    uint64_t min, max;
    bool overflow = false;
    switch (n->type) {
        case TB_ADD:
        overflow |= l_add_overflow(a->_int.min, b->_int.min, mask, &min);
        overflow |= l_add_overflow(a->_int.max, b->_int.max, mask, &max);
        if (min > max) { overflow = true; }
        break;

        case TB_SUB:
        overflow |= l_sub_overflow(a->_int.min, b->_int.min, mask, &min);
        overflow |= l_sub_overflow(a->_int.max, b->_int.max, mask, &max);
        if (min > max) { overflow = true; }
        break;

        case TB_MUL:
        overflow |= l_mul_overflow(a->_int.min, b->_int.min, mask, &min);
        overflow |= l_mul_overflow(a->_int.max, b->_int.max, mask, &max);
        if (min > max) { overflow = true; }
        break;
    }

    // truncate to the size of the raw DataType
    min &= mask, max &= mask;

    if (min == max) {
        return lattice_intern(opt, (Lattice){ LATTICE_INT, ._int = { min, min, ~min, min } });
    } else if (overflow) {
        return lattice_intern(opt, (Lattice){ LATTICE_INT, ._int = { 0, mask } });
    } else {
        return lattice_intern(opt, (Lattice){ LATTICE_INT, ._int = { min, max } });
    }
}

static Lattice* sccp_bitcast(TB_Passes* restrict opt, TB_Node* n) {
    Lattice* a = lattice_universe_get(opt, n->inputs[1]);
    if (a == &TOP_IN_THE_SKY) {
        return &TOP_IN_THE_SKY;
    }

    if (a->tag == LATTICE_INT && a->_int.min == a->_int.max && n->dt.type == TB_PTR) {
        // bitcast with a constant leads to fun cool stuff (usually we get constant zeros for NULL)
        return a->_int.min ? &XNULL_IN_THE_SKY : &NULL_IN_THE_SKY;
    }

    return NULL;
}

static Lattice* sccp_unary(TB_Passes* restrict opt, TB_Node* n) {
    Lattice* a = lattice_universe_get(opt, n->inputs[1]);
    if (a == &TOP_IN_THE_SKY) {
        return &TOP_IN_THE_SKY;
    }

    if (a->tag == LATTICE_INT) {
        uint64_t mask = tb__mask(n->dt.data);
        uint64_t min = ~a->_int.min & mask;
        uint64_t max = ~a->_int.max & mask;

        if (wrapped_int_lt(max, min, n->dt.data)) {
            SWAP(int64_t, min, max);
        }

        uint64_t zeros = 0, ones = 0;
        if (n->type == TB_NEG) {
            // -x => ~x + 1
            //   because of this addition we can technically
            //   overflow... umm? glhf?
            uint64_t min_inc = (min+1) & mask;
            uint64_t max_inc = (max+1) & mask;

            if (min_inc < min || max_inc < min) {
                min = lattice_int_min(n->dt.data);
                max = lattice_int_max(n->dt.data);
            } else {
                min = min_inc;
                max = max_inc;
            }
        } else {
            zeros = a->_int.known_ones;
            ones  = a->_int.known_zeros;
        }

        return lattice_intern(opt, (Lattice){ LATTICE_INT, ._int = { min, max, zeros, ones } });
    } else {
        return NULL;
    }
}

static Lattice* sccp_bits(TB_Passes* restrict opt, TB_Node* n) {
    Lattice* a = lattice_universe_get(opt, n->inputs[1]);
    Lattice* b = lattice_universe_get(opt, n->inputs[2]);
    if (a == &TOP_IN_THE_SKY || b == &TOP_IN_THE_SKY) {
        return &TOP_IN_THE_SKY;
    }

    uint64_t zeros, ones;
    switch (n->type) {
        case TB_AND:
        // 0 if either is zero, 1 if both are 1
        zeros = a->_int.known_zeros | b->_int.known_zeros;
        ones  = a->_int.known_ones  & b->_int.known_ones;
        break;

        case TB_OR:
        // 0 if both are 0, 1 if either is 1
        zeros = a->_int.known_zeros & b->_int.known_zeros;
        ones  = a->_int.known_ones  | b->_int.known_ones;
        break;

        case TB_XOR:
        // 0 if both bits are 0 or 1
        // 1 if both bits aren't the same
        zeros = (a->_int.known_zeros & b->_int.known_zeros) | (a->_int.known_ones & b->_int.known_ones);
        ones  = (a->_int.known_zeros & b->_int.known_ones)  | (a->_int.known_ones & b->_int.known_zeros);
        break;

        default: tb_todo();
    }

    uint64_t mask = tb__mask(n->dt.data);
    zeros &= mask, ones &= mask;

    // we can deduce a min and max by assuming the unknown bits are either zeros or ones
    int64_t min = ones, max = ~zeros;
    if (wrapped_int_lt(max, min, n->dt.data)) {
        min = lattice_int_min(n->dt.data);
        max = lattice_int_max(n->dt.data);
    }
    min &= mask, max &= mask;

    return lattice_intern(opt, (Lattice){ LATTICE_INT, ._int = { min, max, zeros, ones } });
}

static Lattice* sccp_shift(TB_Passes* restrict opt, TB_Node* n) {
    Lattice* a = lattice_universe_get(opt, n->inputs[1]);
    Lattice* b = lattice_universe_get(opt, n->inputs[2]);
    if (a == &TOP_IN_THE_SKY || b == &TOP_IN_THE_SKY) {
        return &TOP_IN_THE_SKY;
    }

    uint64_t bits = n->dt.data;
    uint64_t mask = tb__mask(n->dt.data);

    // shift that's in-bounds can tell us quite a few nice details
    if (b->_int.max <= bits) {
        uint64_t min, max, zeros, ones = 0;
        switch (n->type) {
            case TB_SHL:
            min = a->_int.min << b->_int.min;
            max = a->_int.max << b->_int.max;
            min &= mask, max &= mask;

            if (((a->_int.min & b->_int.min) < 0 && min >= 0) ||
                (~(a->_int.max | b->_int.max) < 0 && max < 0) ||
                wrapped_int_lt(max, min, n->dt.data)
            ) {
                min = lattice_int_min(n->dt.data);
                max = lattice_int_max(n->dt.data);
            }

            // we at least shifted this many bits therefore we
            // at least have this many zeros at the bottom
            zeros = (1ull << b->_int.min) - 1ull;
            // if we know how many bits we shifted then we know where
            // our known ones ones went
            if (b->_int.min == b->_int.max) {
                ones <<= b->_int.min;
            }
            break;

            case TB_SAR:
            case TB_SHR:
            // perform shift logic as unsigned
            min = a->_int.min;
            max = a->_int.max;
            if (min > max) {
                min = 0, max = mask;
            }

            // the largest value is caused by the lowest shift amount
            min >>= b->_int.max;
            max >>= b->_int.min;

            // convert range back into signed
            if (wrapped_int_lt(max, min, n->dt.data)) {
                min = lattice_int_min(n->dt.data);
                max = lattice_int_max(n->dt.data);
            }

            // TODO(NeGate): we can technically guarentee the top bits are zero
            zeros = 0;
            // if we know how many bits we shifted then we know where
            // our known ones ones went
            if (b->_int.min == b->_int.max) {
                ones >>= b->_int.min;
            }
            break;

            default: tb_todo();
        }

        return lattice_intern(opt, (Lattice){ LATTICE_INT, ._int = { min, max, zeros, ones } });
    } else {
        return NULL;
    }
}

static Lattice* sccp_cmp(TB_Passes* restrict opt, TB_Node* n) {
    Lattice* a = lattice_universe_get(opt, n->inputs[1]);
    Lattice* b = lattice_universe_get(opt, n->inputs[2]);
    if (a == &TOP_IN_THE_SKY || b == &TOP_IN_THE_SKY) {
        return &TOP_IN_THE_SKY;
    }

    if (a == &BOT_IN_THE_SKY || b == &BOT_IN_THE_SKY) {
        return &BOT_IN_THE_SKY;
    }

    TB_DataType dt = n->inputs[1]->dt;
    if (dt.type == TB_INT) {
        uint64_t mask = tb__mask(dt.data);
        uint64_t sign_range = (1 << (dt.data - 1)) - 1;

        bool a_cst = a->_int.min == a->_int.max;
        bool b_cst = b->_int.min == b->_int.max;

        int cmp = 2; // 0 or 1 (2 for BOT)
        switch (n->type) {
            case TB_CMP_EQ:
            if (a_cst && b_cst) cmp = a->_int.min == b->_int.min;
            break;

            case TB_CMP_NE:
            if (a_cst && b_cst) cmp = a->_int.min == b->_int.min;
            break;

            case TB_CMP_SLE:
            case TB_CMP_SLT:
            if (a->_int.max < sign_range && b->_int.max < sign_range) {
                if (wrapped_int_lt(a->_int.max, b->_int.min, dt.data)) cmp = 1;
                if (wrapped_int_lt(b->_int.max, a->_int.min, dt.data)) cmp = 0;
            }
            break;

            case TB_CMP_ULT:
            case TB_CMP_ULE:
            if (a->_int.min <= a->_int.max && b->_int.min <= b->_int.max) {
                if ((a->_int.max & mask) < (b->_int.min & mask)) cmp = 1;
                if ((b->_int.max & mask) < (a->_int.min & mask)) cmp = 0;
            }
            break;
        }

        if (cmp != 2) {
            return lattice_intern(opt, (Lattice){ LATTICE_INT, ._int = { cmp, cmp, ~cmp, cmp } });
        }
    } else if (dt.type == TB_PTR && (n->type == TB_CMP_EQ || n->type == TB_CMP_NE)) {
        a = lattice_meet(opt, a, &XNULL_IN_THE_SKY, TB_TYPE_PTR);
        b = lattice_meet(opt, b, &XNULL_IN_THE_SKY, TB_TYPE_PTR);

        if (n->type == TB_CMP_EQ) {
            return a == b ? &TRUE_IN_THE_SKY : &FALSE_IN_THE_SKY;
        } else {
            return a != b ? &TRUE_IN_THE_SKY : &FALSE_IN_THE_SKY;
        }
    }

    return NULL;
}

static TB_Node* ideal_select(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    TB_Node* src = n->inputs[1];

    LatticeTrifecta key_truthy = lattice_truthy(lattice_universe_get(opt, src));
    if (key_truthy == LATTICE_KNOWN_TRUE) {
        return n->inputs[2];
    } else if (key_truthy == LATTICE_KNOWN_FALSE) {
        return n->inputs[3];
    }

    // select(y <= x, a, b) => select(x < y, b, a) flipped conditions
    if (src->type == TB_CMP_SLE || src->type == TB_CMP_ULE) {
        TB_Node* new_cmp = tb_alloc_node(f, src->type == TB_CMP_SLE ? TB_CMP_SLT : TB_CMP_ULT, TB_TYPE_BOOL, 3, sizeof(TB_NodeCompare));
        set_input(f, new_cmp, src->inputs[2], 1);
        set_input(f, new_cmp, src->inputs[1], 2);
        TB_NODE_SET_EXTRA(new_cmp, TB_NodeCompare, .cmp_dt = TB_NODE_GET_EXTRA_T(src, TB_NodeCompare)->cmp_dt);

        set_input(f, n, new_cmp, 1);
        tb_pass_mark(opt, new_cmp);
        return n;
    }

    // T(some_bool ? 1 : 0) => movzx(T, some_bool)
    if (src->dt.type == TB_INT && src->dt.data == 1) {
        uint64_t on_true, on_false;
        bool true_imm = get_int_const(n->inputs[2], &on_true);
        bool false_imm = get_int_const(n->inputs[3], &on_false);

        // A ? A : 0 => A (booleans)
        if (src == n->inputs[2] && false_imm && on_false == 0) {
            return src;
        }

        // A ? 0 : !A => A (booleans)
        if (inverted_cmp(src, n->inputs[3]) && true_imm && on_true == 0) {
            return src;
        }

        if (true_imm && false_imm && on_true == 1 && on_false == 0) {
            TB_Node* ext_node = tb_alloc_node(f, TB_ZERO_EXT, n->dt, 2, 0);
            set_input(f, ext_node, src, 1);
            tb_pass_mark(opt, ext_node);
            return ext_node;
        }
    }

    // (select.f32 (v43: cmp.lt.f32 ...) (v41: load.f32 ...) (v42: load.f32 ...))
    if (n->dt.type == TB_FLOAT && src->type == TB_CMP_FLT) {
        TB_Node* a = src->inputs[1];
        TB_Node* b = src->inputs[2];

        // (select (lt A B) A B) => (min A B)
        if (n->inputs[2] == a && n->inputs[3] == b) {
            TB_Node* new_node = tb_alloc_node(f, TB_FMIN, n->dt, 3, 0);
            set_input(f, new_node, a, 1);
            set_input(f, new_node, b, 2);
            return new_node;
        }

        // (select (lt A B) B A) => (max A B)
        if (n->inputs[2] == b && n->inputs[3] == a) {
            TB_Node* new_node = tb_alloc_node(f, TB_FMAX, n->dt, 3, 0);
            set_input(f, new_node, a, 1);
            set_input(f, new_node, b, 2);
            return new_node;
        }
    }

    return NULL;
}

static bool nice_ass_trunc(TB_NodeTypeEnum t) { return t == TB_ADD || t == TB_AND || t == TB_XOR || t == TB_OR || t == TB_MUL || t == TB_SHL || t == TB_SHR || t == TB_SAR || t == TB_SMOD || t == TB_UMOD; }
static TB_Node* ideal_truncate(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    TB_Node* src = n->inputs[1];

    if (src->type == TB_ZERO_EXT && src->inputs[1]->dt.type == TB_INT && n->dt.type == TB_INT) {
        int now = n->dt.data;
        int before = src->inputs[1]->dt.data;

        if (now != before) {
            // we're extending the original value
            TB_Node* ext = tb_alloc_node(f, now < before ? TB_TRUNCATE : src->type, n->dt, 2, 0);
            set_input(f, ext, src->inputs[1], 1);
            return ext;
        } else {
            return src->inputs[1];
        }
    }

    // Trunc(NiceAssBinop(a, b)) => NiceAssBinop(Trunc(a), Trunc(b))
    if (nice_ass_trunc(src->type)) {
        TB_Node* left = tb_alloc_node(f, TB_TRUNCATE, n->dt, 2, 0);
        set_input(f, left, src->inputs[1], 1);
        tb_pass_mark(opt, left);

        TB_Node* right = tb_alloc_node(f, TB_TRUNCATE, n->dt, 2, 0);
        set_input(f, right, src->inputs[2], 1);
        tb_pass_mark(opt, right);

        TB_Node* new_binop = tb_alloc_node(f, src->type, n->dt, 3, 0);
        set_input(f, new_binop, left, 1);
        set_input(f, new_binop, right, 2);
        return new_binop;
    }

    return NULL;
}

static TB_Node* ideal_extension(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    TB_NodeTypeEnum ext_type = n->type;
    TB_Node* src = n->inputs[1];

    if (src->type == ext_type) {
        do {
            src = src->inputs[1];
        } while (src->type == ext_type);
        set_input(f, n, src, 1);
        return n;
    }

    // Ext(phi(a: con, b: con)) => phi(Ext(a: con), Ext(b: con))
    if (src->type == TB_PHI) {
        FOREACH_N(i, 1, src->input_count) {
            if (src->inputs[i]->type != TB_INTEGER_CONST) return NULL;
        }

        // generate extension nodes
        TB_DataType dt = n->dt;
        FOREACH_N(i, 1, src->input_count) {
            assert(src->inputs[i]->type == TB_INTEGER_CONST);

            TB_Node* ext_node = tb_alloc_node(f, ext_type, dt, 2, 0);
            set_input(f, ext_node, src->inputs[i], 1);
            set_input(f, src, ext_node, i);
            tb_pass_mark(opt, ext_node);
        }

        src->dt = dt;
        return src;
    }

    // Cast(NiceAssBinop(a, b)) => NiceAssBinop(Cast(a), Cast(b))
    if (nice_ass_trunc(src->type)) {
        TB_Node* left = tb_alloc_node(f, ext_type, n->dt, 2, 0);
        set_input(f, left, src->inputs[1], 1);
        tb_pass_mark(opt, left);

        TB_Node* right = tb_alloc_node(f, ext_type, n->dt, 2, 0);
        set_input(f, right, src->inputs[2], 1);
        tb_pass_mark(opt, right);

        TB_Node* new_binop = tb_alloc_node(f, src->type, n->dt, 3, 0);
        set_input(f, new_binop, left, 1);
        set_input(f, new_binop, right, 2);
        return new_binop;
    }

    return NULL;
}

static int node_pos(TB_Node* n) {
    switch (n->type) {
        case TB_INTEGER_CONST:
        case TB_FLOAT32_CONST:
        case TB_FLOAT64_CONST:
        return 1;

        default:
        return 2;

        case TB_PHI:
        return 3;
    }
}

static TB_Node* ideal_int_binop(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    TB_NodeTypeEnum type = n->type;
    TB_Node* a = n->inputs[1];
    TB_Node* b = n->inputs[2];
    bool less = node_pos(a) < node_pos(b);

    // if it's commutative: we wanna have a canonical form.
    if (is_commutative(type) && less) {
        set_input(f, n, b, 1);
        set_input(f, n, a, 2);
        return n;
    }

    // (aa + ab) + b => aa + (ab + b)
    if (is_associative(type) && a->type == type && b->type != type && less) {
        TB_Node* abb = tb_alloc_node(f, type, n->dt, 3, sizeof(TB_NodeBinopInt));
        set_input(f, abb, a->inputs[2], 1);
        set_input(f, abb, b, 2);
        tb_pass_mark(opt, abb);

        set_input(f, n, a->inputs[1], 1);
        set_input(f, n, abb,          2);
        return n;
    }

    if (type == TB_OR) {
        assert(n->dt.type == TB_INT);
        int bits = n->dt.data;

        // (or (shl a 24) (shr a 40)) => (rol a 24)
        if (a->type == TB_SHL && (b->type == TB_SHR || b->type == TB_SAR)) {
            uint64_t shl_amt, shr_amt;
            if (a->inputs[1] == b->inputs[1] &&
                get_int_const(a->inputs[2], &shl_amt) &&
                get_int_const(b->inputs[2], &shr_amt) &&
                shl_amt == bits - shr_amt) {
                // convert to rotate left
                n->type = TB_ROL;
                set_input(f, n, a->inputs[1], 1);
                set_input(f, n, a->inputs[2], 2);
                return n;
            }
        }
    } else if (type == TB_MUL) {
        uint64_t rhs;
        if (get_int_const(b, &rhs)) {
            uint64_t log2 = tb_ffs(rhs) - 1;
            if (rhs == (UINT64_C(1) << log2)) {
                TB_Node* shl_node = tb_alloc_node(f, TB_SHL, n->dt, 3, sizeof(TB_NodeBinopInt));
                set_input(f, shl_node, a, 1);
                set_input(f, shl_node, make_int_node(f, opt, n->dt, log2), 2);

                tb_pass_mark(opt, shl_node->inputs[1]);
                tb_pass_mark(opt, shl_node->inputs[2]);
                return shl_node;
            }
        }
    } else if (type == TB_CMP_EQ) {
        // (a == 0) is !a
        TB_Node* cmp = n->inputs[1];

        uint64_t rhs;
        if (get_int_const(n->inputs[2], &rhs) && rhs == 0) {
            // !(a <  b) is (b <= a)
            switch (cmp->type) {
                case TB_CMP_EQ: n->type = TB_CMP_EQ; break;
                case TB_CMP_NE: n->type = TB_CMP_NE; break;
                case TB_CMP_SLT: n->type = TB_CMP_SLE; break;
                case TB_CMP_SLE: n->type = TB_CMP_SLT; break;
                case TB_CMP_ULT: n->type = TB_CMP_ULE; break;
                case TB_CMP_ULE: n->type = TB_CMP_ULT; break;
                default: return NULL;
            }

            TB_DataType cmp_dt = TB_NODE_GET_EXTRA_T(cmp, TB_NodeCompare)->cmp_dt;
            TB_NODE_SET_EXTRA(n, TB_NodeCompare, .cmp_dt = cmp_dt);

            set_input(f, n, cmp->inputs[2], 1);
            set_input(f, n, cmp->inputs[1], 2);
            return n;
        }
    } else if (type == TB_SHL || type == TB_SHR || type == TB_SAR) {
        // (a << b) >> c = (a << (b - c)) & (ALL >> b)
        // (a >> b) << c = (a >> (b - c)) & ((1 << b) - 1)
        uint64_t b, c;
        if ((n->inputs[1]->type == TB_SHL || n->inputs[1]->type == TB_SHR || n->inputs[1]->type == TB_SAR) &&
            get_int_const(n->inputs[2], &c) && get_int_const(n->inputs[1]->inputs[2], &b)) {
            TB_NodeTypeEnum inner_shift = n->inputs[1]->type;

            // track how far we're shifting (and how many bits need chopping)
            int amt       = inner_shift == TB_SHL ? b               : -b;
            uint64_t mask = inner_shift == TB_SHL ? UINT64_MAX << b : UINT64_MAX >> c;

            // apply outer shift
            amt  += type == TB_SHL ? c         : -c;
            mask  = type == TB_SHL ? mask << b :  mask >> b;

            TB_Node* shift = n->inputs[1]->inputs[1];
            if (amt) {
                TB_Node* imm = make_int_node(f, opt, n->dt, b - c);
                tb_pass_mark(opt, imm);

                // if we have a negative shift amount, that's a right shift
                shift = tb_alloc_node(f, amt < 0 ? TB_SHR : TB_SHL, n->dt, 3, sizeof(TB_NodeBinopInt));
                set_input(f, shift, n->inputs[1]->inputs[1], 1);
                set_input(f, shift, imm, 2);

                tb_pass_mark(opt, shift);
            }

            TB_Node* mask_node = make_int_node(f, opt, n->dt, mask);
            TB_Node* and_node = tb_alloc_node(f, TB_AND, n->dt, 3, sizeof(TB_NodeBinopInt));
            set_input(f, and_node, shift,     1);
            set_input(f, and_node, mask_node, 2);
            return and_node;
        }
    }

    if (type >= TB_CMP_EQ && type <= TB_CMP_ULE) {
        // (Cmp Sxt(a) Sxt(b)) => (Cmp a b)
        if (n->inputs[1]->type == TB_SIGN_EXT && n->inputs[2]->type == TB_SIGN_EXT) {
            TB_DataType dt = n->inputs[1]->inputs[1]->dt;
            set_input(f, n, n->inputs[1]->inputs[1], 1);
            set_input(f, n, n->inputs[2]->inputs[1], 2);
            TB_NODE_SET_EXTRA(n, TB_NodeCompare, .cmp_dt = dt);
            return n;
        }
    }

    return NULL;
}

static TB_Node* ideal_int_mod(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    bool is_signed = n->type == TB_SMOD;

    TB_DataType dt = n->dt;
    TB_Node* x = n->inputs[1];

    uint64_t y = TB_NODE_GET_EXTRA_T(n->inputs[2], TB_NodeInt)->value;
    uint64_t log2 = tb_ffs(y) - 1;
    if (!is_signed && y == (UINT64_C(1) << log2)) {
        TB_Node* and_node = tb_alloc_node(f, TB_AND, dt, 3, sizeof(TB_NodeBinopInt));
        set_input(f, and_node, x, 1);
        set_input(f, and_node, make_int_node(f, opt, dt, y - 1), 2);
        return and_node;
    }

    return NULL;
}

static TB_Node* ideal_int_div(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    bool is_signed = n->type == TB_SDIV;

    // if we have a constant denominator we may be able to reduce the division into a
    // multiply and shift-right
    if (n->inputs[2]->type != TB_INTEGER_CONST) return NULL;

    // https://gist.github.com/B-Y-P/5872dbaaf768c204480109007f64a915
    TB_DataType dt = n->dt;
    TB_Node* x = n->inputs[1];

    uint64_t y = TB_NODE_GET_EXTRA_T(n->inputs[2], TB_NodeInt)->value;
    if (y >= (1ull << 63ull)) {
        // we haven't implemented the large int case
        return NULL;
    } else if (y == 0) {
        return tb_alloc_node(f, TB_POISON, dt, 1, 0);
    } else if (y == 1) {
        return x;
    } else {
        // (udiv a N) => a >> log2(N) where N is a power of two
        uint64_t log2 = tb_ffs(y) - 1;
        if (!is_signed && y == (UINT64_C(1) << log2)) {
            TB_Node* shr_node = tb_alloc_node(f, TB_SHR, dt, 3, sizeof(TB_NodeBinopInt));
            set_input(f, shr_node, x, 1);
            set_input(f, shr_node, make_int_node(f, opt, dt, log2), 2);
            return shr_node;
        }
    }

    // idk how to handle this yet
    if (is_signed) return NULL;

    uint64_t sh = (64 - tb_clz64(y)) - 1; // sh = ceil(log2(y)) + w - 64

    #ifndef NDEBUG
    uint64_t sh2 = 0;
    while(y > (1ull << sh2)){ sh2++; }    // sh' = ceil(log2(y))
    sh2 += 63 - 64;                       // sh  = ceil(log2(y)) + w - 64

    assert(sh == sh2);
    #endif

    // 128bit division here can't overflow
    uint64_t a = tb_div128(1ull << sh, y - 1, y);

    // now we can take a and sh and do:
    //   x / y  => mulhi(x, a) >> sh
    int bits = dt.data;
    if (bits > 32) {
        TB_Node* mul_node = tb_alloc_node(f, TB_MULPAIR, TB_TYPE_TUPLE, 3, 0);
        set_input(f, mul_node, x, 1);
        set_input(f, mul_node, make_int_node(f, opt, dt, a), 2);

        TB_Node* lo = make_proj_node(f, dt, mul_node, 0);
        TB_Node* hi = make_proj_node(f, dt, mul_node, 1);

        tb_pass_mark(opt, mul_node);
        tb_pass_mark(opt, lo);
        tb_pass_mark(opt, hi);

        TB_Node* sh_node = tb_alloc_node(f, TB_SHR, dt, 3, sizeof(TB_NodeBinopInt));
        set_input(f, sh_node, hi, 1);
        set_input(f, sh_node, make_int_node(f, opt, dt, sh), 2);
        TB_NODE_SET_EXTRA(sh_node, TB_NodeBinopInt, .ab = 0);

        return sh_node;
    } else {
        TB_DataType big_dt = TB_TYPE_INTN(bits * 2);
        sh += bits; // chopping the low half

        a &= (1ull << bits) - 1;

        // extend x
        TB_Node* ext_node = tb_alloc_node(f, TB_ZERO_EXT, big_dt, 2, 0);
        set_input(f, ext_node, x, 1);

        TB_Node* mul_node = tb_alloc_node(f, TB_MUL, big_dt, 3, sizeof(TB_NodeBinopInt));
        set_input(f, mul_node, ext_node, 1);
        set_input(f, mul_node, make_int_node(f, opt, big_dt, a), 2);
        TB_NODE_SET_EXTRA(mul_node, TB_NodeBinopInt, .ab = 0);

        TB_Node* sh_node = tb_alloc_node(f, TB_SHR, big_dt, 3, sizeof(TB_NodeBinopInt));
        set_input(f, sh_node, mul_node, 1);
        set_input(f, sh_node, make_int_node(f, opt, big_dt, sh), 2);
        TB_NODE_SET_EXTRA(sh_node, TB_NodeBinopInt, .ab = 0);

        TB_Node* trunc_node = tb_alloc_node(f, TB_TRUNCATE, dt, 2, 0);
        set_input(f, trunc_node, sh_node, 1);

        tb_pass_mark(opt, mul_node);
        tb_pass_mark(opt, sh_node);
        tb_pass_mark(opt, ext_node);
        return trunc_node;
    }
}

////////////////////////////////
// Integer identities
////////////////////////////////
// a + 0 => a
// a - 0 => a
// a ^ 0 => a
// a * 0 => 0
// a / 0 => poison
static TB_Node* identity_int_binop(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    uint64_t b;
    if (!get_int_const(n->inputs[2], &b)) {
        return n;
    }

    if (n->type == TB_MUL && b == 1) {
        return n->inputs[1];
    } else if (b == 0) {
        switch (n->type) {
            default: return n;

            case TB_SHL:
            case TB_SHR:
            case TB_SAR:
            case TB_ADD:
            case TB_SUB:
            case TB_XOR:
            return n->inputs[1];

            case TB_MUL:
            return n->inputs[0];

            case TB_UDIV:
            case TB_SDIV:
            case TB_UMOD:
            case TB_SMOD:
            return make_poison(f, n->dt);

            // (cmp.ne a 0) => a
            case TB_CMP_NE: {
                // walk up extension
                TB_Node* src = n->inputs[1];
                if (src->type == TB_ZERO_EXT) {
                    src = src->inputs[1];
                }

                if (src->dt.type == TB_INT && src->dt.data == 1) {
                    return src;
                }

                return n;
            }
        }
    } else {
        return n;
    }
}

////////////////////////////////
// Pointer idealizations
////////////////////////////////
static TB_Node* identity_member_ptr(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    if (TB_NODE_GET_EXTRA_T(n, TB_NodeMember)->offset == 0) {
        return n->inputs[1];
    }
    return n;
}

static TB_Node* ideal_member_ptr(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    int64_t offset = TB_NODE_GET_EXTRA_T(n, TB_NodeMember)->offset;
    TB_Node* base  = n->inputs[1];

    if (base->type == TB_MEMBER_ACCESS) {
        offset += TB_NODE_GET_EXTRA_T(base, TB_NodeMember)->offset;
        set_input(f, n, base->inputs[1], 1);

        TB_NODE_SET_EXTRA(n, TB_NodeMember, .offset = offset);
        return n;
    }

    return NULL;
}

static TB_Node* ideal_array_ptr(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    int64_t stride = TB_NODE_GET_EXTRA_T(n, TB_NodeArray)->stride;
    TB_Node* base  = n->inputs[1];
    TB_Node* index = n->inputs[2];

    // (array A B 4) => (member A B*4) where B is constant
    if (index->type == TB_INTEGER_CONST) {
        int64_t src_i = TB_NODE_GET_EXTRA_T(index, TB_NodeInt)->value;

        int64_t offset = src_i * stride;
        TB_Node* new_n = tb_alloc_node(f, TB_MEMBER_ACCESS, n->dt, 2, sizeof(TB_NodeMember));
        set_input(f, new_n, base, 1);
        TB_NODE_SET_EXTRA(new_n, TB_NodeMember, .offset = offset);
        return new_n;
    }

    // (array A (shl B C) D) => (array A B C<<D)
    if (index->type == TB_SHL && index->inputs[2]->type == TB_INTEGER_CONST) {
        uint64_t scale = TB_NODE_GET_EXTRA_T(index->inputs[2], TB_NodeInt)->value;
        set_input(f, n, index->inputs[1], 2);
        TB_NODE_SET_EXTRA(n, TB_NodeArray, .stride = stride << scale);
        return n;
    }

    // (array A (mul B C) D) => (array A B C*D)
    if (index->type == TB_MUL && index->inputs[2]->type == TB_INTEGER_CONST) {
        uint64_t factor = TB_NODE_GET_EXTRA_T(index->inputs[2], TB_NodeInt)->value;
        set_input(f, n, index->inputs[1], 2);
        TB_NODE_SET_EXTRA(n, TB_NodeArray, .stride = stride * factor);
        return n;
    }

    // (array A (add B C) D) => (member (array A B D) C*D)
    if (index->type == TB_ADD) {
        TB_Node* new_index = index->inputs[1];
        TB_Node* add_rhs   = index->inputs[2];

        uint64_t offset;
        if (get_int_const(add_rhs, &offset)) {
            offset *= stride;

            TB_Node* new_n = tb_alloc_node(f, TB_ARRAY_ACCESS, TB_TYPE_PTR, 3, sizeof(TB_NodeArray));
            set_input(f, new_n, base, 1);
            set_input(f, new_n, new_index, 2);
            TB_NODE_SET_EXTRA(new_n, TB_NodeArray, .stride = stride);

            TB_Node* new_member = tb_alloc_node(f, TB_MEMBER_ACCESS, TB_TYPE_PTR, 2, sizeof(TB_NodeMember));
            set_input(f, new_member, new_n, 1);
            TB_NODE_SET_EXTRA(new_member, TB_NodeMember, .offset = offset);

            tb_pass_mark(opt, new_n);
            tb_pass_mark(opt, new_member);
            return new_member;
        }
    }

    return NULL;
}
