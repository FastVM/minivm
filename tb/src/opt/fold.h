
#define MASK_UPTO(pos) (~UINT64_C(0) >> (64 - pos))
#define BEXTR(src,pos) (((src) >> (pos)) & 1)
uint64_t tb__sxt(uint64_t src, uint64_t src_bits, uint64_t dst_bits) {
    uint64_t sign_bit = BEXTR(src, src_bits-1);
    uint64_t mask = MASK_UPTO(dst_bits) & ~MASK_UPTO(src_bits);

    uint64_t dst = src & ~mask;
    return dst | (sign_bit ? mask : 0);
}

uint64_t tb__mask(uint64_t bits) {
    return ~UINT64_C(0) >> (64 - bits);
}

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
        return true;

        default:
        return false;
    }
}

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
static TB_Node* ideal_truncate(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    TB_Node* src = n->inputs[1];
    if (src->type != TB_INTEGER_CONST || n->dt.type != TB_INT) {
        return NULL;
    }

    TB_NodeInt* src_i = TB_NODE_GET_EXTRA(src);

    uint64_t mask = n->dt.data == 64 ? UINT64_MAX : (1ull << n->dt.data) - 1;
    return make_int_node(f, opt, n->dt, src_i->value & mask);
}

static TB_Node* ideal_int2ptr(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    TB_Node* src = n->inputs[1];
    if (src->type != TB_INTEGER_CONST) {
        return NULL;
    }

    TB_NodeInt* src_i = TB_NODE_GET_EXTRA(src);
    return make_int_node(f, opt, n->dt, src_i->value);
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

static TB_Node* ideal_select(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    TB_Node* src = n->inputs[1];

    // select(y <= x, a, b) => select(x < y, b, a) flipped conditions
    if (src->type == TB_CMP_SLE || src->type == TB_CMP_ULE) {
        TB_Node* new_cmp = tb_alloc_node(f, src->type == TB_CMP_SLE ? TB_CMP_SLT : TB_CMP_ULT, TB_TYPE_BOOL, 3, sizeof(TB_NodeCompare));
        set_input(opt, new_cmp, src->inputs[2], 1);
        set_input(opt, new_cmp, src->inputs[1], 2);
        TB_NODE_SET_EXTRA(new_cmp, TB_NodeCompare, .cmp_dt = TB_NODE_GET_EXTRA_T(src, TB_NodeCompare)->cmp_dt);

        set_input(opt, n, new_cmp, 1);
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
            set_input(opt, ext_node, src, 1);
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
            set_input(opt, new_node, a, 1);
            set_input(opt, new_node, b, 2);
            return new_node;
        }

        // (select (lt A B) B A) => (max A B)
        if (n->inputs[2] == b && n->inputs[3] == a) {
            TB_Node* new_node = tb_alloc_node(f, TB_FMAX, n->dt, 3, 0);
            set_input(opt, new_node, a, 1);
            set_input(opt, new_node, b, 2);
            return new_node;
        }
    }

    return NULL;
}

static TB_Node* ideal_extension(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    TB_Node* src = n->inputs[1];

    // Ext(phi(a: con, b: con)) => phi(Ext(a: con), Ext(b: con))
    if (src->type == TB_PHI) {
        FOREACH_N(i, 1, src->input_count) {
            if (src->inputs[i]->type != TB_INTEGER_CONST) return NULL;
        }

        // generate extension nodes
        TB_NodeTypeEnum ext_type = n->type;
        TB_DataType dt = n->dt;
        FOREACH_N(i, 1, src->input_count) {
            assert(src->inputs[i]->type == TB_INTEGER_CONST);

            TB_Node* ext_node = tb_alloc_node(f, ext_type, dt, 2, 0);
            set_input(opt, ext_node, src->inputs[i], 1);
            set_input(opt, src, ext_node, i);
            tb_pass_mark(opt, ext_node);
        }

        src->dt = dt;
        return src;
    }

    return NULL;
}

static TB_Node* identity_extension(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    TB_Node* src = n->inputs[1];
    if (src->type == TB_INTEGER_CONST) {
        uint64_t src_i = TB_NODE_GET_EXTRA_T(src, TB_NodeInt)->value;

        size_t src_bits = src->dt.data;
        size_t dst_bits = n->dt.data;

        uint64_t mask = ~UINT64_C(0) >> (64 - src_bits);
        uint64_t val;
        if (n->type == TB_SIGN_EXT) {
            val = tb__sxt(src_i, src_bits, dst_bits);
        } else {
            val = src_i & mask;
        }

        return make_int_node(f, opt, n->dt, val);
    } else {
        return n;
    }
}

static TB_Node* ideal_int_unary(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    assert(n->type == TB_NOT || n->type == TB_NEG);
    TB_Node* src = n->inputs[1];
    if (src->type == TB_INTEGER_CONST) {
        assert(src->dt.type == TB_INT && src->dt.data > 0);
        uint64_t src_i = ~TB_NODE_GET_EXTRA_T(src, TB_NodeInt)->value;

        if (n->type == TB_NEG) {
            // -x => ~x + 1
            src_i += 1;
        }

        return make_int_node(f, opt, n->dt, src_i);
    } else {
        return NULL;
    }
}

static TB_Node* ideal_int_binop(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    TB_NodeTypeEnum type = n->type;
    if (is_commutative(type)) {
        // if it's commutative: we wanna have a canonical form.
        // lower types to the right (constants are basically the lowest things)
        if (n->inputs[1]->type < n->inputs[2]->type) {
            TB_Node* tmp = n->inputs[1];
            set_input(opt, n, n->inputs[2], 1);
            set_input(opt, n, tmp, 2);
            return n;
        }
    }

    TB_Node* a = n->inputs[1];
    TB_Node* b = n->inputs[2];
    if (type == TB_OR) {
        assert(n->dt.type == TB_INT);
        int bits = n->dt.data;

        // (or (shr a 40) (shl a 24)) => (rol a 24)
        if (a->type == TB_SHR && b->type == TB_SHL) {
            uint64_t shl_amt, shr_amt;
            if (a->inputs[1] == b->inputs[1] &&
                get_int_const(a->inputs[2], &shr_amt) &&
                get_int_const(b->inputs[2], &shl_amt) &&
                shl_amt == bits - shr_amt) {
                // convert to rotate left
                n->type = TB_ROL;
                set_input(opt, n, b->inputs[1], 1);
                set_input(opt, n, b->inputs[2], 2);
                return n;
            }
        }
    } else if (type == TB_MUL) {
        uint64_t rhs;
        if (get_int_const(b, &rhs)) {
            uint64_t log2 = tb_ffs(rhs) - 1;
            if (rhs == (UINT64_C(1) << log2)) {
                TB_Node* shl_node = tb_alloc_node(f, TB_SHL, n->dt, 3, sizeof(TB_NodeBinopInt));
                set_input(opt, shl_node, a, 1);
                set_input(opt, shl_node, make_int_node(f, opt, n->dt, log2), 2);

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
                case TB_CMP_SLT: n->type = TB_CMP_SLE; break;
                case TB_CMP_SLE: n->type = TB_CMP_SLT; break;
                case TB_CMP_ULT: n->type = TB_CMP_ULE; break;
                case TB_CMP_ULE: n->type = TB_CMP_ULT; break;
                default: return NULL;
            }

            TB_DataType cmp_dt = TB_NODE_GET_EXTRA_T(cmp, TB_NodeCompare)->cmp_dt;
            TB_NODE_SET_EXTRA(n, TB_NodeCompare, .cmp_dt = cmp_dt);

            set_input(opt, n, cmp->inputs[2], 1);
            set_input(opt, n, cmp->inputs[1], 2);
            return n;
        }
    } else if (type == TB_SHL || type == TB_SHR) {
        // (a << b) >> c = a << (b - c)
        // (a >> b) << c = a >> (b - c)
        TB_NodeTypeEnum flipped = type == TB_SHL ? TB_SHR : TB_SHL;

        uint64_t a, b;
        if (n->inputs[1]->type == flipped &&
            get_int_const(n->inputs[1]->inputs[2], &a) &&
            get_int_const(n->inputs[2], &b)) {
            TB_Node* imm = make_int_node(f, opt, n->dt, a - b);
            tb_pass_mark(opt, imm);

            set_input(opt, n, n->inputs[1]->inputs[1], 1);
            set_input(opt, n, imm, 2);
            return n;
        }
    }

    if (a->type != TB_INTEGER_CONST || b->type != TB_INTEGER_CONST) {
        return NULL;
    }

    // fully fold
    uint64_t ai = TB_NODE_GET_EXTRA_T(a, TB_NodeInt)->value;
    uint64_t bi = TB_NODE_GET_EXTRA_T(b, TB_NodeInt)->value;
    if (type >= TB_CMP_EQ && type <= TB_CMP_ULE) {
        bool result = false;
        switch (type) {
            case TB_CMP_EQ:  result = ai == bi; break;
            case TB_CMP_NE:  result = ai != bi; break;
            case TB_CMP_ULT: result = ai <  bi; break;
            case TB_CMP_ULE: result = ai <= bi; break;
            default: tb_unreachable();
        }

        return make_int_node(f, opt, n->dt, result);
    } else if (type >= TB_AND && type <= TB_MUL) {
        uint64_t dst;
        switch (type) {
            case TB_AND: dst = ai & bi; break;
            case TB_OR:  dst = ai | bi; break;
            case TB_XOR: dst = ai ^ bi; break;
            case TB_ADD: dst = ai + bi; break;
            case TB_SUB: dst = ai - bi; break;
            case TB_MUL: dst = ai * bi; break;
            default: tb_unreachable();
        }

        // truncate
        return make_int_node(f, opt, n->dt, dst & tb__mask(n->dt.data));
    } else {
        return NULL;
    }
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
            set_input(opt, shr_node, x, 1);
            set_input(opt, shr_node, make_int_node(f, opt, dt, log2), 2);
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

    // 128bit division here can overflow so we need to handle that case
    uint64_t a = tb_div128(1ull << sh, y - 1, y);

    // now we can take a and sh and do:
    //   x / y  => mulhi(x, a) >> sh
    int bits = dt.data;
    if (bits > 32) {
        TB_Node* mul_node = tb_alloc_node(f, TB_MULPAIR, TB_TYPE_TUPLE, 3, sizeof(TB_NodeArithPair));
        set_input(opt, mul_node, x, 1);
        set_input(opt, mul_node, make_int_node(f, opt, dt, a), 2);

        TB_Node* lo = make_proj_node(f, opt, dt, mul_node, 1);
        TB_Node* hi = make_proj_node(f, opt, dt, mul_node, 2);
        TB_NODE_SET_EXTRA(mul_node, TB_NodeArithPair, .lo = lo, .hi = hi);

        TB_Node* sh_node = tb_alloc_node(f, TB_SHR, dt, 3, sizeof(TB_NodeBinopInt));
        set_input(opt, sh_node, hi, 1);
        set_input(opt, sh_node, make_int_node(f, opt, dt, sh), 2);
        TB_NODE_SET_EXTRA(sh_node, TB_NodeBinopInt, .ab = 0);

        return sh_node;
    } else {
        TB_DataType big_dt = TB_TYPE_INTN(bits * 2);
        sh += bits; // chopping the low half

        a &= (1ull << bits) - 1;

        // extend x
        TB_Node* ext_node = tb_alloc_node(f, TB_ZERO_EXT, big_dt, 2, 0);
        set_input(opt, ext_node, x, 1);

        TB_Node* mul_node = tb_alloc_node(f, TB_MUL, big_dt, 3, sizeof(TB_NodeBinopInt));
        set_input(opt, mul_node, ext_node, 1);
        set_input(opt, mul_node, make_int_node(f, opt, big_dt, a), 2);
        TB_NODE_SET_EXTRA(mul_node, TB_NodeBinopInt, .ab = 0);

        TB_Node* sh_node = tb_alloc_node(f, TB_SHR, big_dt, 3, sizeof(TB_NodeBinopInt));
        set_input(opt, sh_node, mul_node, 1);
        set_input(opt, sh_node, make_int_node(f, opt, big_dt, sh), 2);
        TB_NODE_SET_EXTRA(sh_node, TB_NodeBinopInt, .ab = 0);

        TB_Node* trunc_node = tb_alloc_node(f, TB_TRUNCATE, dt, 2, 0);
        set_input(opt, trunc_node, sh_node, 1);
        return trunc_node;
    }
}

////////////////////////////////
// Integer identities
////////////////////////////////
// a + 0 => a
// a - 0 => a
// a * 0 => 0
// a / 0 => poison
static TB_Node* identity_int_binop(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    if (!is_zero(n->inputs[2])) return n;

    switch (n->type) {
        default: return n;

        case TB_SHL:
        case TB_SHR:
        case TB_ADD:
        case TB_SUB:
        case TB_MUL:
        return n->inputs[1];

        case TB_UDIV:
        case TB_SDIV:
        return tb_inst_poison(f);

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
}

////////////////////////////////
// Pointer idealizations
////////////////////////////////
static TB_Node* ideal_array_ptr(TB_Passes* restrict opt, TB_Function* f, TB_Node* n) {
    int64_t stride = TB_NODE_GET_EXTRA_T(n, TB_NodeArray)->stride;
    TB_Node* base  = n->inputs[1];
    TB_Node* index = n->inputs[2];

    // (array A B 4) => (member A B*4) where B is constant
    if (index->type == TB_INTEGER_CONST) {
        int64_t src_i = TB_NODE_GET_EXTRA_T(index, TB_NodeInt)->value;

        int64_t offset = src_i * stride;
        TB_Node* new_n = tb_alloc_node(f, TB_MEMBER_ACCESS, n->dt, 2, sizeof(TB_NodeMember));
        set_input(opt, new_n, base, 1);
        TB_NODE_SET_EXTRA(new_n, TB_NodeMember, .offset = offset);
        return new_n;
    }

    // (array A (add B C) D) => (member (array A B D) C*D)
    if (index->type == TB_ADD) {
        TB_Node* new_index = index->inputs[1];
        TB_Node* add_rhs   = index->inputs[2];

        uint64_t offset;
        if (get_int_const(add_rhs, &offset)) {
            offset *= stride;

            TB_Node* new_n = tb_alloc_node(f, TB_ARRAY_ACCESS, TB_TYPE_PTR, 3, sizeof(TB_NodeArray));
            set_input(opt, new_n, base, 1);
            set_input(opt, new_n, new_index, 2);
            TB_NODE_SET_EXTRA(new_n, TB_NodeArray, .stride = stride);

            TB_Node* new_member = tb_alloc_node(f, TB_MEMBER_ACCESS, TB_TYPE_PTR, 2, sizeof(TB_NodeMember));
            set_input(opt, new_member, new_n, 1);
            TB_NODE_SET_EXTRA(new_member, TB_NodeMember, .offset = offset);

            tb_pass_mark(opt, new_n);
            tb_pass_mark(opt, new_member);
            return new_member;
        }
    }

    return NULL;
}
