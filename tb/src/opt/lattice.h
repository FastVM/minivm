#include <hashes.h>

// TODO(NeGate): implement dual? from there i can do join with
//
// dual(dual(x) ^ dual(y)) = join(x, y)
typedef struct {
    uint64_t bot, top;

    // for known bit analysis
    uint64_t known_zeros;
    uint64_t known_ones;
} LatticeInt;

// a simplification of the set of all pointers (or floats)
typedef enum {
    LATTICE_UNKNOWN,        // top aka {nan, non-nan} or for pointers {null, non-null}

    LATTICE_KNOWN_NAN = 1,  // {nan}
    LATTICE_KNOWN_NOT_NAN,  // {non-nan}

    LATTICE_KNOWN_NULL = 1, // {null}
    LATTICE_KNOWN_NOT_NULL  // {non-null}
} LatticeTrifecta;

typedef struct {
    LatticeTrifecta trifecta;
} LatticeFloat;

// TODO(NeGate): we might wanna store more info like aliasing, ownership and alignment.
typedef struct {
    LatticeTrifecta trifecta;
} LatticePointer;

// Represents the fancier type system within the optimizer, it's
// all backed by my shitty understanding of lattice theory
typedef struct {
    enum {
        LATTICE_INT,
        LATTICE_FLOAT32,
        LATTICE_FLOAT64,
        LATTICE_POINTER,
    } tag;
    uint32_t pad;
    union {
        LatticeInt _int;
        LatticeFloat _float;
        LatticePointer _ptr;
    };
} Lattice;

// hash-consing because there's a lot of
// redundant types we might construct.
typedef struct {
    NL_HashSet pool;
} LatticeUniverse;

static uint32_t lattice_hash(void* a) {
    return tb__murmur3_32(a, sizeof(Lattice));
}

static bool lattice_cmp(void* a, void* b) {
    Lattice *aa = a, *bb = b;
    return aa->tag == bb->tag ? memcmp(aa, bb, sizeof(Lattice)) == 0 : false;
}

// maximal subset
static Lattice lattice_top(TB_DataType dt) {
    switch (dt.type) {
        case TB_INT: {
            assert(dt.data <= 64);
            uint64_t max_bits = UINT64_MAX >> dt.data;
            tb_todo();

            return (Lattice){ LATTICE_INT, ._int = { 0, max_bits } };
        }

        case TB_FLOAT: {
            assert(dt.data == TB_FLT_32 || dt.data == TB_FLT_64);
            return (Lattice){ dt.data == TB_FLT_64 ? LATTICE_FLOAT64 : LATTICE_FLOAT32, ._float = { LATTICE_UNKNOWN } };
        }

        case TB_PTR: {
            return (Lattice){ LATTICE_POINTER, ._ptr = { LATTICE_UNKNOWN } };
        }

        default:
        tb_todo();
    }
}

// known X ^ known X => known X or
// known X ^ unknown => unknown (commutative btw)
#define TRIFECTA_MEET(a, b) ((a).trifecta == (b).trifecta ? (a).trifecta : LATTICE_UNKNOWN)

// generates the greatest lower bound between a and b
static Lattice lattice_meet(const Lattice* a, const Lattice* b) {
    assert(a->tag == b->tag);
    switch (a->tag) {
        case LATTICE_INT: {
            // [amin, amax] ^ [bmin, bmax] => [min(amin, bmin), max(amax, bmax)]
            LatticeInt aa = a->_int;
            LatticeInt bb = b->_int;

            LatticeInt i = { aa.bot, aa.top };
            if (i.bot > bb.bot) i.bot = bb.bot;
            if (i.top < bb.top) i.top = bb.top;

            i.known_zeros = aa.known_zeros & bb.known_zeros;
            i.known_ones = aa.known_ones & bb.known_ones;
            return (Lattice){ LATTICE_INT, ._int = i };
        }

        case LATTICE_FLOAT32:
        case LATTICE_FLOAT64: {
            LatticeFloat f = { .trifecta = TRIFECTA_MEET(a->_float, b->_float) };
            return (Lattice){ a->tag, ._float = f };
        }

        case LATTICE_POINTER: {
            LatticePointer p = { .trifecta = TRIFECTA_MEET(a->_ptr, b->_ptr) };
            return (Lattice){ LATTICE_POINTER, ._ptr = p };
        }

        default: tb_todo();
    }
}
