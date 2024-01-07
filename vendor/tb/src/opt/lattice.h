#include <hashes.h>

static Lattice TOP_IN_THE_SKY   = { LATTICE_TOP   };
static Lattice BOT_IN_THE_SKY   = { LATTICE_BOT   };
static Lattice CTRL_IN_THE_SKY  = { LATTICE_CTRL  };
static Lattice XCTRL_IN_THE_SKY = { LATTICE_XCTRL };
static Lattice XNULL_IN_THE_SKY = { LATTICE_XNULL };
static Lattice NULL_IN_THE_SKY  = { LATTICE_NULL  };
static Lattice FALSE_IN_THE_SKY = { LATTICE_INT, ._int = { 0, 0, 1, 0 } };
static Lattice TRUE_IN_THE_SKY  = { LATTICE_INT, ._int = { 1, 1, 0, 1 } };

static Lattice* lattice_from_dt(TB_Passes* p, TB_DataType dt);

static uint32_t lattice_hash(void* a) {
    size_t s = sizeof(Lattice);
    Lattice* l = a;
    if (l->tag == LATTICE_TUPLE) {
        s += l->_tuple.count*sizeof(Lattice*);
    }

    return tb__murmur3_32(a, s);
}

static bool lattice_cmp(void* a, void* b) {
    Lattice *aa = a, *bb = b;
    if (aa->tag != bb->tag) {
        return false;
    }

    if (aa->tag == LATTICE_TUPLE) {
        if (aa->_tuple.count != bb->_tuple.count) {
            return false;
        }

        return memcmp(aa, bb, sizeof(Lattice) + aa->_tuple.count*sizeof(Lattice*)) == 0;
    } else {
        return memcmp(aa, bb, sizeof(Lattice)) == 0;
    }
}

static bool lattice_is_const_int(Lattice* l) { return l->_int.min == l->_int.max; }
static bool lattice_is_const(Lattice* l) { return l->tag == LATTICE_INT && l->_int.min == l->_int.max; }

static void lattice_universe_grow(TB_Passes* p, size_t top) {
    size_t new_cap = tb_next_pow2(top + 16);
    p->types = tb_platform_heap_realloc(p->types, new_cap * sizeof(Lattice*));

    // clear new space
    FOREACH_N(i, p->type_cap, new_cap) {
        p->types[i] = &TOP_IN_THE_SKY;
    }

    p->type_cap = new_cap;
}

static bool lattice_universe_map_progress(TB_Passes* p, TB_Node* n, Lattice* l) {
    // reserve cap, slow path :p
    if (UNLIKELY(n->gvn >= p->type_cap)) {
        lattice_universe_grow(p, n->gvn);
    }

    Lattice* old = p->types[n->gvn];
    p->types[n->gvn] = l;
    return old != l;
}

static void lattice_universe_map(TB_Passes* p, TB_Node* n, Lattice* l) {
    // reserve cap, slow path :p
    if (UNLIKELY(n->gvn >= p->type_cap)) {
        lattice_universe_grow(p, n->gvn);
    }

    p->types[n->gvn] = l;
}

Lattice* lattice_universe_get(TB_Passes* p, TB_Node* n) {
    // reserve cap, slow path :p
    if (UNLIKELY(n->gvn >= p->type_cap)) {
        lattice_universe_grow(p, n->gvn);
    }

    assert(p->types[n->gvn] != NULL);
    return p->types[n->gvn];
}

static Lattice* lattice_intern(TB_Passes* p, Lattice l) {
    assert(l.tag != LATTICE_TUPLE);
    Lattice* k = nl_hashset_get2(&p->type_interner, &l, lattice_hash, lattice_cmp);
    if (k != NULL) {
        return k;
    }

    // allocate new node
    k = tb_arena_alloc(tmp_arena, sizeof(Lattice));
    memcpy(k, &l, sizeof(l));
    nl_hashset_put2(&p->type_interner, k, lattice_hash, lattice_cmp);
    return k;
}

static bool lattice_top_or_bot(Lattice* l) {
    return l->tag <= LATTICE_TOP;
}

LatticeTrifecta lattice_truthy(Lattice* l) {
    switch (l->tag) {
        case LATTICE_INT:
        if (l->_int.min == l->_int.max) {
            return l->_int.min ? LATTICE_KNOWN_TRUE : LATTICE_KNOWN_FALSE;
        }
        return LATTICE_UNKNOWN;

        case LATTICE_FLOAT32:
        case LATTICE_FLOAT64:
        return LATTICE_UNKNOWN;

        case LATTICE_NULL:  return false;
        case LATTICE_XNULL: return true;

        default:
        return LATTICE_UNKNOWN;
    }
}

static int64_t lattice_int_min(int bits) { return 1ll << (bits - 1); }
static int64_t lattice_int_max(int bits) { return (1ll << (bits - 1)) - 1; }
static uint64_t lattice_uint_max(int bits) { return UINT64_MAX >> (64 - bits); }

static Lattice* lattice_from_dt(TB_Passes* p, TB_DataType dt) {
    switch (dt.type) {
        case TB_INT: {
            assert(dt.data <= 64);
            return lattice_intern(p, (Lattice){ LATTICE_INT, ._int = { 0, lattice_uint_max(dt.data) } });
        }

        case TB_FLOAT: {
            assert(dt.data == TB_FLT_32 || dt.data == TB_FLT_64);
            return lattice_intern(p, (Lattice){ dt.data == TB_FLT_64 ? LATTICE_FLOAT64 : LATTICE_FLOAT32, ._float = { LATTICE_UNKNOWN } });
        }

        case TB_MEMORY: {
            return p->root_mem;
        }

        case TB_CONTROL: return &CTRL_IN_THE_SKY;
        default: return &BOT_IN_THE_SKY;
    }
}

static Lattice* lattice_tuple_from_node(TB_Passes* p, TB_Node* n) {
    assert(n->dt.type == TB_TUPLE);
    // count projs
    int projs = 0;
    FOR_USERS(u, n) {
        if (u->n->type == TB_PROJ) projs++;
    }

    size_t size = sizeof(Lattice) + projs*sizeof(Lattice*);
    Lattice* l = tb_arena_alloc(tmp_arena, size);
    *l = (Lattice){ LATTICE_TUPLE, ._tuple = { projs } };
    FOR_USERS(u, n) {
        if (u->n->type != TB_PROJ) continue;

        int index = TB_NODE_GET_EXTRA_T(u->n, TB_NodeProj)->index;
        l->elems[index] = lattice_from_dt(p, u->n->dt);
    }

    Lattice* k = nl_hashset_put2(&p->type_interner, l, lattice_hash, lattice_cmp);
    if (k) {
        tb_arena_free(tmp_arena, l, size);
        return k;
    } else {
        return l;
    }
}

// known X ^ known X => known X or
// known X ^ unknown => unknown (commutative btw)
#define TRIFECTA_MEET(a, b) ((a).trifecta == (b).trifecta ? (a).trifecta : LATTICE_UNKNOWN)

#define MASK_UPTO(pos) (~UINT64_C(0) >> (64 - pos))
#define BEXTR(src,pos) (((src) >> (pos)) & 1)
uint64_t tb__sxt(uint64_t src, uint64_t src_bits, uint64_t dst_bits) {
    uint64_t sign_bit = BEXTR(src, src_bits-1);
    uint64_t mask = MASK_UPTO(dst_bits) & ~MASK_UPTO(src_bits);

    uint64_t dst = src & ~mask;
    return dst | (sign_bit ? mask : 0);
}

static bool lattice_signed(LatticeInt* l) { return l->min > l->max; }

static LatticeInt lattice_into_unsigned(LatticeInt i) {
    if (i.min > i.max) { SWAP(uint64_t, i.min, i.max); }
    return i;
}

static Lattice* lattice_gimme_int(TB_Passes* p, int64_t min, int64_t max) {
    assert(min <= max);
    return lattice_intern(p, (Lattice){ LATTICE_INT, ._int = { min, max } });
}

static Lattice* lattice_gimme_uint(TB_Passes* p, uint64_t min, uint64_t max) {
    assert(min <= max);
    return lattice_intern(p, (Lattice){ LATTICE_INT, ._int = { min, max } });
}

static Lattice* lattice_new_alias(TB_Passes* p) {
    return lattice_intern(p, (Lattice){ LATTICE_MEM, ._mem = { p->alias_n++ } });
}

static Lattice* lattice_alias(TB_Passes* p, int alias_idx) {
    return lattice_intern(p, (Lattice){ LATTICE_MEM, ._mem = { alias_idx } });
}

static bool l_add_overflow(uint64_t x, uint64_t y, uint64_t mask, uint64_t* out) {
    *out = (x + y) & mask;
    return x && *out < x;
}

static bool l_mul_overflow(uint64_t x, uint64_t y, uint64_t mask, uint64_t* out) {
    *out = (x * y) & mask;
    return x && *out < x;
}

static bool l_sub_overflow(uint64_t x, uint64_t y, uint64_t mask, uint64_t* out) {
    *out = (x - y) & mask;
    return x && *out > x;
}

static bool wrapped_int_lt(int64_t x, int64_t y, int bits) {
    return (int64_t)tb__sxt(x, bits, 64) < (int64_t)tb__sxt(y, bits, 64);
}

static LatticeInt lattice_meet_int(LatticeInt a, LatticeInt b, TB_DataType dt) {
    // [amin, amax] ^ [bmin, bmax] => [min(amin, bmin), max(amax, bmax)]
    int bits = dt.data;
    uint64_t mask = tb__mask(dt.data);

    bool aas = a.min > a.max;
    bool bbs = b.min > b.max;
    if (aas && bbs) {
        if (wrapped_int_lt(b.min, a.min, bits)) a.min = b.min;
        if (wrapped_int_lt(a.max, b.max, bits)) a.max = b.max;
    } else {
        if (!aas && !bbs) {
            a = lattice_into_unsigned(a);
            b = lattice_into_unsigned(b);
        }

        if (b.min < a.min) a.min = b.min;
        if (a.max < b.max) a.max = b.max;
    }

    a.known_zeros &= b.known_zeros;
    a.known_ones &= b.known_ones;
    return a;
}

// generates the greatest lower bound between a and b
static Lattice* lattice_meet(TB_Passes* p, Lattice* a, Lattice* b, TB_DataType dt) {
    // a ^ a = a
    if (a == b) return a;

    // it's commutative, so let's simplify later code this way
    if (a->tag > b->tag) {
        SWAP(Lattice*, a, b);
    }

    switch (a->tag) {
        case LATTICE_BOT: return &BOT_IN_THE_SKY;
        case LATTICE_TOP: return &TOP_IN_THE_SKY;

        case LATTICE_CTRL:
        case LATTICE_XCTRL: {
            // ctrl  ^ ctrl   = ctrl
            // ctrl  ^ xctrl  = bot
            // xctrl ^ xctrl  = xctrl
            return a == b ? a : &BOT_IN_THE_SKY;
        }

        case LATTICE_INT: {
            if (b->tag != LATTICE_INT) {
                return &BOT_IN_THE_SKY;
            }

            LatticeInt i = lattice_meet_int(a->_int, b->_int, dt);
            return lattice_intern(p, (Lattice){ LATTICE_INT, ._int = i });
        }

        case LATTICE_FLOAT32:
        case LATTICE_FLOAT64: {
            if (b->tag != a->tag) {
                return &BOT_IN_THE_SKY;
            }

            LatticeFloat f = { .trifecta = TRIFECTA_MEET(a->_float, b->_float) };
            return lattice_intern(p, (Lattice){ a->tag, ._float = f });
        }

        // all cases that reached down here are bottoms
        case LATTICE_NULL: return &BOT_IN_THE_SKY;

        // ~null ^ sym = ~null
        case LATTICE_XNULL: {
            if (b->tag == LATTICE_PTR) {
                return a;
            } else {
                return &BOT_IN_THE_SKY;
            }
        }

        // symA ^ symB = ~null
        case LATTICE_PTR: {
            if (b->tag == LATTICE_PTR) {
                assert(a->_ptr.sym != b->_ptr.sym);
                return &XNULL_IN_THE_SKY;
            } else {
                return &BOT_IN_THE_SKY;
            }
        }

        default: tb_todo();
    }
}
