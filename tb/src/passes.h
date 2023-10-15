#pragma once
#include "tb_internal.h"

#define TB_OPTDEBUG_STATS 0

#define TB_OPTDEBUG_PEEP 0
#define TB_OPTDEBUG_LOOP 0
#define TB_OPTDEBUG_MEM2REG 0
#define TB_OPTDEBUG_CODEGEN 0

#define DO_IF(cond) CONCAT(DO_IF_, cond)
#define DO_IF_0(...)
#define DO_IF_1(...) __VA_ARGS__

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

typedef struct {
    DynArray(TB_Node*) items;

    // uses gvn as key
    size_t visited_cap; // in words
    uint64_t* visited;
} Worklist;

struct TB_Passes {
    TB_Function* f;
    bool scheduled;

    // we use this to verify that we're on the same thread
    // for the entire duration of the TB_Passes.
    TB_ThreadInfo* pinned_thread;

    Worklist worklist;

    // we wanna track locals because it's nice and easy
    DynArray(TB_Node*) locals;

    // this is used to do CSE
    NL_HashSet cse_nodes;

    // debug shit:
    TB_Node* error_n;

    // nice stats
    struct {
        #if TB_OPTDEBUG_STATS
        int initial;
        int cse_hit, cse_miss;
        int peeps, identities, rewrites;
        #endif
    } stats;
};

// it's either START, REGION or control node with CONTROL PROJ predecessor
static bool is_block_begin(TB_Node* n) {
    // regions also have a CONTROL PROJ so we
    // don't need to check them explicitly.
    return n->type == TB_REGION || (n->type == TB_PROJ && n->inputs[0]->type == TB_START);
}

static bool is_block_end(TB_Node* n) {
    return n->type == TB_BRANCH;
}

static bool is_mem_out_op(TB_Node* n) {
    return n->type == TB_END || (n->type >= TB_STORE && n->type <= TB_ATOMIC_CAS) || (n->type == TB_PHI && n->dt.type == TB_MEMORY);
}

// schedule nodes below any of their pinned dependencies
static bool is_pinned(TB_Node* n) {
    return (n->type >= TB_START && n->type <= TB_SAFEPOINT_POLL) || n->type == TB_PROJ || n->type == TB_LOCAL;
}

static bool is_mem_in_op(TB_Node* n) {
    return is_mem_out_op(n) || n->type == TB_SAFEPOINT_POLL || n->type == TB_LOAD;
}

////////////////////////////////
// CFG analysis
////////////////////////////////
static TB_Node* get_block_begin(TB_Node* n) {
    while (!is_block_begin(n)) {
        n = n->inputs[0];
    }
    return n;
}

// shorthand because we use it a lot
static TB_Node* idom(TB_Node* n) {
    if (n->type == TB_PROJ) n = n->inputs[0];

    assert(n->type == TB_START || n->type == TB_REGION);
    return TB_NODE_GET_EXTRA_T(n, TB_NodeRegion)->dom;
}

static int dom_depth(TB_Node* n) {
    if (n == NULL) {
        return 0;
    }

    while (n->type != TB_REGION && n->type != TB_START) {
        n = n->inputs[0];
    }

    return TB_NODE_GET_EXTRA_T(n, TB_NodeRegion)->dom_depth;
}

extern thread_local TB_Arena* tmp_arena;

void verify_tmp_arena(TB_Passes* p);
void set_input(TB_Passes* restrict p, TB_Node* n, TB_Node* in, int slot);

static User* find_users(TB_Passes* restrict p, TB_Node* n) {
    return n->users;
}

// CFG
//   pushes postorder walk into worklist items, also modifies the visited set.
//   some entries will not be START or REGION, instead you'll see
size_t tb_push_postorder(TB_Function* f, Worklist* restrict ws);
//   postorder walk -> dominators
void tb_compute_dominators(TB_Function* f, size_t count, TB_Node** blocks);

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
void sched_walk(TB_Passes* passes, Worklist* ws, DynArray(PhiVal)* phi_vals, TB_Node* bb, TB_Node* n);

static void push_all_nodes(Worklist* restrict ws, TB_Node* n);
