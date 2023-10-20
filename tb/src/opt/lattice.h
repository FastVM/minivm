#include <hashes.h>

static Lattice* lattice_top(LatticeUniverse* uni, TB_DataType dt);

static uint32_t lattice_hash(void* a) {
    return tb__murmur3_32(a, sizeof(Lattice));
}

static bool lattice_cmp(void* a, void* b) {
    Lattice *aa = a, *bb = b;
    return aa->tag == bb->tag ? memcmp(aa, bb, sizeof(Lattice)) == 0 : false;
}

static bool lattice_is_const_int(Lattice* l) { return l->_int.min == l->_int.max; }

static void lattice_universe_map(LatticeUniverse* uni, TB_Node* n, Lattice* l) {
    // reserve cap, slow path :p
    if (UNLIKELY(n->gvn >= uni->type_cap)) {
        size_t new_cap = tb_next_pow2(n->gvn + 16);
        uni->types = tb_platform_heap_realloc(uni->types, new_cap * sizeof(Lattice*));

        // clear new space
        FOREACH_N(i, uni->type_cap, new_cap) {
            uni->types[i] = NULL;
        }

        uni->type_cap = new_cap;
    }

    uni->types[n->gvn] = l;
}

static Lattice* lattice_universe_get(LatticeUniverse* uni, TB_Node* n) {
    // reserve cap, slow path :p
    if (UNLIKELY(n->gvn >= uni->type_cap)) {
        size_t new_cap = tb_next_pow2(n->gvn + 16);
        uni->types = tb_platform_heap_realloc(uni->types, new_cap * sizeof(Lattice*));

        // clear new space
        FOREACH_N(i, uni->type_cap, new_cap) {
            uni->types[i] = NULL;
        }

        uni->type_cap = new_cap;
    }

    if (uni->types[n->gvn] == NULL) {
        return uni->types[n->gvn] = lattice_top(uni, n->dt);
    } else {
        return uni->types[n->gvn];
    }
}

static Lattice* lattice_intern(LatticeUniverse* uni, Lattice l) {
    Lattice* k = nl_hashset_get2(&uni->pool, &l, lattice_hash, lattice_cmp);
    if (k != NULL) {
        return k;
    }

    // allocate new node
    k = tb_arena_alloc(uni->arena, sizeof(Lattice));
    memcpy(k, &l, sizeof(l));
    nl_hashset_put2(&uni->pool, k, lattice_hash, lattice_cmp);
    return k;
}

static int64_t lattice_int_min(int bits) { return 1ll << (bits - 1); }
static int64_t lattice_int_max(int bits) { return (1ll << (bits - 1)) - 1; }

// constructs a type for a CONTROL node
static Lattice* lattice_ctrl(LatticeUniverse* uni, TB_Node* dom) {
    return lattice_intern(uni, (Lattice){ LATTICE_CONTROL, ._ctrl = { dom } });
}

// maximal subset
static Lattice* lattice_top(LatticeUniverse* uni, TB_DataType dt) {
    switch (dt.type) {
        case TB_INT: {
            assert(dt.data <= 64);
            return lattice_intern(uni, (Lattice){ LATTICE_INT, ._int = { lattice_int_min(dt.data), lattice_int_max(dt.data) } });
        }

        case TB_FLOAT: {
            assert(dt.data == TB_FLT_32 || dt.data == TB_FLT_64);
            return lattice_intern(uni, (Lattice){ dt.data == TB_FLT_64 ? LATTICE_FLOAT64 : LATTICE_FLOAT32, ._float = { LATTICE_UNKNOWN } });
        }

        case TB_PTR: {
            return lattice_intern(uni, (Lattice){ LATTICE_POINTER, ._ptr = { LATTICE_UNKNOWN } });
        }

        default:
        tb_todo();
    }
}

// known X ^ known X => known X or
// known X ^ unknown => unknown (commutative btw)
#define TRIFECTA_MEET(a, b) ((a).trifecta == (b).trifecta ? (a).trifecta : LATTICE_UNKNOWN)

// generates the greatest lower bound between a and b
static Lattice* lattice_meet(LatticeUniverse* uni, Lattice* a, Lattice* b) {
    assert(a->tag == b->tag);
    switch (a->tag) {
        case LATTICE_INT: {
            // [amin, amax] ^ [bmin, bmax] => [min(amin, bmin), max(amax, bmax)]
            LatticeInt aa = a->_int;
            LatticeInt bb = b->_int;

            LatticeInt i = { aa.min, aa.max };
            if (i.min > bb.min) i.min = bb.min;
            if (i.max < bb.max) i.max = bb.max;

            i.known_zeros = aa.known_zeros & bb.known_zeros;
            i.known_ones = aa.known_ones & bb.known_ones;
            return lattice_intern(uni, (Lattice){ LATTICE_INT, ._int = i });
        }

        case LATTICE_FLOAT32:
        case LATTICE_FLOAT64: {
            LatticeFloat f = { .trifecta = TRIFECTA_MEET(a->_float, b->_float) };
            return lattice_intern(uni, (Lattice){ a->tag, ._float = f });
        }

        case LATTICE_POINTER: {
            LatticePointer p = { .trifecta = TRIFECTA_MEET(a->_ptr, b->_ptr) };
            return lattice_intern(uni, (Lattice){ LATTICE_POINTER, ._ptr = p });
        }

        default: tb_todo();
    }
}
