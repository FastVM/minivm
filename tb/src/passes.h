#pragma once
#include "tb_internal.h"

#define TB_OPTDEBUG_STATS   0
#define TB_OPTDEBUG_PEEP    0
#define TB_OPTDEBUG_LOOP    0
#define TB_OPTDEBUG_SROA    0
#define TB_OPTDEBUG_GCM     0
#define TB_OPTDEBUG_MEM2REG 0
#define TB_OPTDEBUG_CODEGEN 0

#define TB_OPTDEBUG(cond) CONCAT(DO_IF_, CONCAT(TB_OPTDEBUG_, cond))

#define DO_IF(cond) CONCAT(DO_IF_, cond)
#define DO_IF_0(...)
#define DO_IF_1(...) __VA_ARGS__

////////////////////////////////
// SCCP
////////////////////////////////
// TODO(NeGate): implement dual? from there i can do join with
// dual(dual(x) ^ dual(y)) = join(x, y)
typedef struct {
    int64_t min, max;

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

typedef struct {
    TB_Node* idom;
} LatticeControl;

// Represents the fancier type system within the optimizer, it's
// all backed by my shitty understanding of lattice theory
typedef struct {
    enum {
        LATTICE_INT,
        LATTICE_FLOAT32,
        LATTICE_FLOAT64,
        LATTICE_POINTER,
        LATTICE_CONTROL,
    } tag;
    uint32_t pad;
    union {
        LatticeInt _int;
        LatticeFloat _float;
        LatticePointer _ptr;
        LatticeControl _ctrl;
    };
} Lattice;

// hash-consing because there's a lot of
// redundant types we might construct.
typedef struct {
    TB_Arena* arena;
    NL_HashSet pool;

    // track a lattice per node (basically all get one
    // so a non-sparse array works)
    size_t type_cap;
    Lattice** types;
} LatticeUniverse;

////////////////////////////////
// CFG
////////////////////////////////
typedef struct {
    size_t stride;
    uint64_t arr[];
} TB_DominanceFrontiers;

static void tb_dommy_fronts_put(TB_DominanceFrontiers* df, size_t i, size_t j) {
    size_t word_i = j / 64;
    df->arr[i * df->stride + word_i] |= 1ull << (j % 64);
}

static bool tb_dommy_fronts_get(TB_DominanceFrontiers* df, size_t i, size_t j) {
    size_t word_i = j / 64;
    return df->arr[i * df->stride + word_i] & (1ull << (j % 64));
}

typedef struct {
    TB_Node *phi, *n;
    int dst, src;
} PhiVal;

typedef struct TB_BasicBlock {
    TB_Node* dom;
    TB_Node* end;
    int id, dom_depth;

    TB_Node* mem_in;
    NL_HashSet items;
} TB_BasicBlock;

typedef struct TB_CFG {
    size_t block_count;
    NL_Map(TB_Node*, TB_BasicBlock) node_to_block;
} TB_CFG;

typedef NL_Map(TB_Node*, TB_BasicBlock*) TB_Scheduled;

////////////////////////////////
// Core optimizer
////////////////////////////////
typedef struct {
    DynArray(TB_Node*) items;

    // uses gvn as key
    size_t visited_cap; // in words
    uint64_t* visited;
} Worklist;

struct TB_Passes {
    TB_Function* f;
    TB_Scheduled scheduled;

    // we use this to verify that we're on the same thread
    // for the entire duration of the TB_Passes.
    TB_ThreadInfo* pinned_thread;

    Worklist worklist;

    // sometimes we be using arrays of nodes, let's just keep one around for a bit
    DynArray(TB_Node*) stack;

    // we wanna track locals because it's nice and easy
    DynArray(TB_Node*) locals;

    // tracks the fancier type system
    LatticeUniverse universe;

    // this is used to do GVN
    NL_HashSet gvn_nodes;

    // debug shit:
    TB_Node* error_n;

    // nice stats
    struct {
        #if TB_OPTDEBUG_PEEP
        int time;
        #endif

        #if TB_OPTDEBUG_STATS
        int initial;
        int gvn_hit, gvn_miss;
        int peeps, identities, rewrites;
        #endif
    } stats;
};

static bool cfg_is_terminator(TB_Node* n) {
    return n->type == TB_BRANCH || n->type == TB_UNREACHABLE || n->type == TB_TRAP || n->type == TB_END;
}

static bool ctrl_out_as_cproj_but_not_branch(TB_Node* n) {
    return n->type == TB_CALL || n->type == TB_SYSCALL || n->type == TB_READ || n->type == TB_WRITE;
}

// includes tuples which have control flow
static bool cfg_is_control(TB_Node* n) {
    // easy case
    if (n->dt.type == TB_CONTROL) return true;
    if (n->dt.type != TB_TUPLE) return false;

    // harder case is figuring out which tuples have control outputs (without manually
    // checking which is annoying and slow)
    //
    //     branch, debugbreak, trap, unreachable, dead  OR  call, syscall, safepoint
    return (n->type >= TB_BRANCH && n->type <= TB_DEAD) || (n->type >= TB_CALL && n->type <= TB_SAFEPOINT_POLL);
}

static bool cfg_is_bb_entry(TB_Node* n) {
    if (n->type == TB_REGION) {
        return true;
    } else if (n->type == TB_PROJ && (n->inputs[0]->type == TB_START || n->inputs[0]->type == TB_BRANCH)) {
        // Start's control proj or a branch target
        return true;
    } else {
        return false;
    }
}

static TB_Node* cfg_get_fallthru(TB_Node* n) {
    if (n->type == TB_PROJ && n->dt.type == TB_CONTROL && n->inputs[0]->type != TB_START) {
        // if it's single user and that user is the terminator we can skip it in the fallthrough logic
        return n->users->next == NULL && n->users->n->type == TB_REGION ? n->users->n : n;
    } else {
        return n;
    }
}

static bool is_mem_out_op(TB_Node* n) {
    return n->dt.type == TB_MEMORY || (n->type >= TB_STORE && n->type <= TB_ATOMIC_CAS) || (n->type >= TB_CALL && n->type <= TB_SAFEPOINT_POLL);
}

static bool is_pinned(TB_Node* n) {
    return (n->type >= TB_START && n->type <= TB_SAFEPOINT_POLL) || n->type == TB_PROJ;
}

static bool is_mem_in_op(TB_Node* n) {
    return is_mem_out_op(n) || n->type == TB_SAFEPOINT_POLL || n->type == TB_LOAD;
}

////////////////////////////////
// CFG analysis
////////////////////////////////
// if we see a branch projection, it may either be a BB itself
// or if it enters a REGION directly, then that region is the BB.
static TB_Node* cfg_next_bb_after_cproj(TB_Node* n) {
    assert(n->type == TB_PROJ);
    return n->users->n->type == TB_REGION ? n->users->n : n;
}

static TB_Node* cfg_next_region_control(TB_Node* n) {
    if (n->type != TB_REGION) {
        for (User* u = n->users; u; u = u->next) {
            if (u->n->type == TB_REGION && u->n->input_count == 1) {
                return u->n;
            }
        }
    }

    return n;
}

static User* cfg_next_user(TB_Node* n) {
    for (User* u = n->users; u; u = u->next) {
        if (cfg_is_control(u->n)) {
            return u;
        }
    }

    return NULL;
}

static bool cfg_is_unreachable(TB_Node* n) {
    for (User* u = n->users; u; u = u->next) {
        if (u->n->type == TB_UNREACHABLE) {
            return true;
        }
    }

    return false;
}

static TB_Node* cfg_next_control(TB_Node* n) {
    for (User* u = n->users; u; u = u->next) {
        if (cfg_is_control(u->n)) {
            return u->n;
        }
    }

    return NULL;
}

static TB_Node* get_pred(TB_Node* n, int i) {
    TB_Node* base = n;
    n = n->inputs[i];

    if (base->type == TB_REGION && n->type == TB_PROJ) {
        TB_Node* parent = n->inputs[0];

        // start or cprojs with multiple users (it's a BB) will just exit
        if (parent->type == TB_START || (!ctrl_out_as_cproj_but_not_branch(parent) && n->users->next != NULL)) {
            return n;
        }
        n = parent;
    }

    while (!cfg_is_bb_entry(n)) {
        n = n->inputs[0];
    }

    return n;
}

static TB_Node* next_control(TB_Node* n) {
    // unless it's a branch (aka a terminator), it'll have one successor
    TB_Node* next = NULL;
    for (User* u = n->users; u; u = u->next) {
        TB_Node* succ = u->n;

        // we can't treat regions in the chain
        if (succ->type == TB_REGION) break;

        // we've found the next step in control flow
        if (cfg_is_control(succ)) {
            return succ;
        }
    }

    return NULL;
}

static TB_Node* get_block_begin(TB_Node* n) {
    while (!cfg_is_bb_entry(n)) {
        n = n->inputs[0];
    }
    return n;
}

static TB_BasicBlock* idom_bb(TB_Passes* p, TB_BasicBlock* bb) {
    ptrdiff_t search = nl_map_get(p->scheduled, bb->dom);
    return search >= 0 ? p->scheduled[search].v : NULL;
}

// shorthand because we use it a lot
static TB_Node* idom(TB_CFG* cfg, TB_Node* n) {
    if (cfg->node_to_block == NULL) return NULL;
    ptrdiff_t search = nl_map_get(cfg->node_to_block, n);
    return search >= 0 ? cfg->node_to_block[search].v.dom : NULL;
}

static int dom_depth(TB_CFG* cfg, TB_Node* n) {
    return nl_map_get_checked(cfg->node_to_block, n).dom_depth;
}

extern thread_local TB_Arena* tmp_arena;

void verify_tmp_arena(TB_Passes* p);
void set_input(TB_Passes* restrict p, TB_Node* n, TB_Node* in, int slot);

static User* find_users(TB_Passes* restrict p, TB_Node* n) {
    return n->users;
}

// CFG
//   pushes postorder walk into worklist items, also modifies the visited set.
TB_CFG tb_compute_rpo(TB_Function* f, TB_Passes* restrict p);
TB_CFG tb_compute_rpo2(TB_Function* f, Worklist* ws, DynArray(TB_Node*)* tmp_stack);
void tb_free_cfg(TB_CFG* cfg);
//   postorder walk -> dominators
void tb_compute_dominators(TB_Function* f, TB_Passes* restrict p, TB_CFG cfg);
void tb_compute_dominators2(TB_Function* f, Worklist* ws, TB_CFG cfg);

// Worklist API
void worklist_alloc(Worklist* restrict ws, size_t initial_cap);
void worklist_free(Worklist* restrict ws);
void worklist_clear(Worklist* restrict ws);
void worklist_clear_visited(Worklist* restrict ws);
bool worklist_test(Worklist* restrict ws, TB_Node* n);
bool worklist_test_n_set(Worklist* restrict ws, TB_Node* n);
void worklist_push(Worklist* restrict ws, TB_Node* restrict n);
int worklist_popcount(Worklist* ws);
TB_Node* worklist_pop(Worklist* ws);

// Local scheduler
void sched_walk(TB_Passes* passes, Worklist* ws, DynArray(PhiVal)* phi_vals, TB_BasicBlock* bb, TB_Node* n, bool is_end);

static void push_all_nodes(TB_Passes* restrict passes, Worklist* restrict ws, TB_Function* f);

void tb_pass_schedule(TB_Passes* opt, TB_CFG cfg);
