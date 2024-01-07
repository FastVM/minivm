// Let's just explain the architecture of the optimizer here.
//
// # Peephole optimizations
//   These are the kind which work locally like 2+2=4 and in TB's design they're
//   performed incrementally which means that certain mutations must go through
//   functions to guarentee they update correctly. Let's go over those:
//
//   set_input(f, n, in, slot)
//     basically `n->inputs[slot] = in` except it correctly updates the user set
//
// # How to implement peepholes
//     TODO
//
#include "passes.h"
#include <log.h>

thread_local TB_Arena* tmp_arena;

// helps us do some matching later
static User* remove_user(TB_Node* n, int slot);
static void remove_input(TB_Function* f, TB_Node* n, size_t i);

static void subsume_node(TB_Function* f, TB_Node* n, TB_Node* new_n);
static void subsume_node2(TB_Function* f, TB_Node* n, TB_Node* new_n);

// node creation helpers
TB_Node* make_poison(TB_Function* f, TB_DataType dt);
TB_Node* dead_node(TB_Function* f, TB_Passes* restrict p);
TB_Node* make_int_node(TB_Function* f, TB_Passes* restrict p, TB_DataType dt, uint64_t x);
TB_Node* make_proj_node(TB_Function* f, TB_DataType dt, TB_Node* src, int i);

static size_t tb_pass_update_cfg(TB_Passes* p, Worklist* ws, bool preserve);

////////////////////////////////
// Worklist
////////////////////////////////
void worklist_alloc(Worklist* restrict ws, size_t initial_cap) {
    ws->visited_cap = (initial_cap + 63) / 64;
    ws->visited = tb_platform_heap_alloc(ws->visited_cap * sizeof(uint64_t));
    ws->items = dyn_array_create(uint64_t, ws->visited_cap * 64);
    FOREACH_N(i, 0, ws->visited_cap) {
        ws->visited[i] = 0;
    }
}

void worklist_free(Worklist* restrict ws) {
    tb_platform_heap_free(ws->visited);
    dyn_array_destroy(ws->items);
}

void worklist_clear_visited(Worklist* restrict ws) {
    CUIK_TIMED_BLOCK("clear visited") {
        memset(ws->visited, 0, ws->visited_cap * sizeof(uint64_t));
    }
}

void worklist_clear(Worklist* restrict ws) {
    CUIK_TIMED_BLOCK("clear worklist") {
        memset(ws->visited, 0, ws->visited_cap * sizeof(uint64_t));
        dyn_array_clear(ws->items);
    }
}

void worklist_remove(Worklist* restrict ws, TB_Node* n) {
    uint64_t gvn_word = n->gvn / 64; // which word this ID is at
    if (gvn_word >= ws->visited_cap) return;

    uint64_t gvn_mask = 1ull << (n->gvn % 64);
    ws->visited[gvn_word] &= ~gvn_mask;
}

// checks if node is visited but doesn't push item
bool worklist_test(Worklist* restrict ws, TB_Node* n) {
    uint64_t gvn_word = n->gvn / 64; // which word this ID is at
    if (gvn_word >= ws->visited_cap) return false;

    uint64_t gvn_mask = 1ull << (n->gvn % 64);
    return ws->visited[gvn_word] & gvn_mask;
}

bool worklist_test_n_set(Worklist* restrict ws, TB_Node* n) {
    uint64_t gvn_word = n->gvn / 64; // which word this ID is at

    // resize?
    if (gvn_word >= ws->visited_cap) {
        size_t new_cap = gvn_word + 16;
        ws->visited = tb_platform_heap_realloc(ws->visited, new_cap * sizeof(uint64_t));

        // clear new space
        FOREACH_N(i, ws->visited_cap, new_cap) {
            ws->visited[i] = 0;
        }

        ws->visited_cap = new_cap;
    }

    uint64_t gvn_mask = 1ull << (n->gvn % 64);
    if (ws->visited[gvn_word] & gvn_mask) {
        return true;
    } else {
        ws->visited[gvn_word] |= gvn_mask;
        return false;
    }
}

void worklist_push(Worklist* restrict ws, TB_Node* restrict n) {
    if (!worklist_test_n_set(ws, n)) {
        dyn_array_put(ws->items, n);
    }
}

TB_Node* worklist_pop(Worklist* ws) {
    if (dyn_array_length(ws->items)) {
        TB_Node* n = dyn_array_pop(ws->items);
        uint64_t gvn_word = n->gvn / 64;
        uint64_t gvn_mask = 1ull << (n->gvn % 64);

        ws->visited[gvn_word] &= ~gvn_mask;
        return n;
    } else {
        return NULL;
    }
}

int worklist_popcount(Worklist* ws) {
    int sum = 0;
    for (size_t i = 0; i < ws->visited_cap; i++) {
        sum += tb_popcount64(ws->visited[i]);
    }
    return sum;
}

void verify_tmp_arena(TB_Passes* p) {
    // once passes are run on a thread, they're pinned to it.
    TB_Module* m = p->f->super.module;
    TB_ThreadInfo* info = tb_thread_info(m);

    if (p->pinned_thread == NULL) {
        p->pinned_thread = info;
        tb_arena_clear(p->pinned_thread->tmp_arena);
    } else if (p->pinned_thread != info) {
        tb_panic(
            "TB_Passes are bound to a thread, you can't switch which threads they're run on\n\n"
            "NOTE: if you really need to run across threads you'll need to exit the passes and\n"
            "start anew... though you pay a performance hit everytime you start one"
        );
    }

    tmp_arena = p->pinned_thread->tmp_arena;
}

static int bits_in_data_type(int pointer_size, TB_DataType dt) {
    switch (dt.type) {
        case TB_INT: return dt.data;
        case TB_PTR: return pointer_size;
        case TB_FLOAT:
        if (dt.data == TB_FLT_32) return 32;
        if (dt.data == TB_FLT_64) return 64;
        return 0;
        default: return 0;
    }
}

static char* lil_name(TB_Function* f, const char* fmt, ...) {
    char* buf = TB_ARENA_ALLOC(tmp_arena, 30);

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, 30, fmt, ap);
    va_end(ap);
    return buf;
}

static TB_Node* mem_user(TB_Passes* restrict p, TB_Node* n, int slot) {
    FOR_USERS(u, n) {
        if ((u->n->type == TB_PROJ && u->n->dt.type == TB_MEMORY) ||
            (u->slot == slot && is_mem_out_op(u->n))) {
            return u->n;
        }
    }

    return NULL;
}

static bool single_use(TB_Node* n) {
    return n->users->next == NULL;
}

static bool is_same_align(TB_Node* a, TB_Node* b) {
    TB_NodeMemAccess* aa = TB_NODE_GET_EXTRA(a);
    TB_NodeMemAccess* bb = TB_NODE_GET_EXTRA(b);
    return aa->align == bb->align;
}

static bool is_empty_bb(TB_Passes* restrict p, TB_Node* end) {
    assert(end->type == TB_BRANCH || end->type == TB_UNREACHABLE);
    if (!cfg_is_bb_entry(end->inputs[0])) {
        return false;
    }

    TB_Node* bb = end->inputs[0];
    FOR_USERS(use, bb) {
        TB_Node* n = use->n;
        if (use->n != end) return false;
    }

    return true;
}

static bool is_if_branch(TB_Node* n, uint64_t* falsey) {
    if (n->type == TB_BRANCH && n->input_count == 2 && TB_NODE_GET_EXTRA_T(n, TB_NodeBranch)->succ_count == 2) {
        *falsey = TB_NODE_GET_EXTRA_T(n, TB_NodeBranch)->keys[0];
        return true;
    }

    return false;
}

// incremental dominators, plays nice with peepholes and has
// a limited walk of 20 steps.
static TB_Node* fast_idom(TB_Node* bb) {
    int steps = 0;
    while (steps < FAST_IDOM_LIMIT && bb->type != TB_REGION && bb->type != TB_ROOT) {
        bb = bb->inputs[0];
        steps++;
    }

    return bb;
}

static bool fast_dommy(TB_Node* expected_dom, TB_Node* bb) {
    int steps = 0;
    while (steps < FAST_IDOM_LIMIT && bb != expected_dom && bb->type != TB_REGION && bb->type != TB_ROOT) {
        bb = bb->inputs[0];
        steps++;
    }

    return bb == expected_dom;
}

static bool slow_dommy(TB_CFG* cfg, TB_Node* expected_dom, TB_Node* bb) {
    while (bb != NULL && expected_dom != bb) {
        TB_Node* new_bb = idom(cfg, bb);
        if (new_bb == NULL || new_bb == bb) {
            return false;
        }
        bb = new_bb;
    }

    return true;
}

// unity build with all the passes
#include "lattice.h"
#include "cfg.h"
#include "gvn.h"
#include "fold.h"
#include "mem_opt.h"
#include "sroa.h"
#include "loop.h"
#include "branches.h"
#include "print.h"
#include "mem2reg.h"
#include "gcm.h"
#include "libcalls.h"
#include "scheduler.h"

static Lattice* sccp_int(TB_Passes* restrict p, TB_Node* n) {
    assert(n->type == TB_INTEGER_CONST);
    TB_NodeInt* num = TB_NODE_GET_EXTRA(n);
    if (n->dt.type == TB_PTR) {
        return num->value ? &XNULL_IN_THE_SKY : &NULL_IN_THE_SKY;
    } else {
        return lattice_intern(p, (Lattice){ LATTICE_INT, ._int = { num->value, num->value, ~num->value, num->value } });
    }
}

static Lattice* sccp_proj(TB_Passes* restrict p, TB_Node* n) {
    assert(n->type == TB_PROJ);
    Lattice* l = lattice_universe_get(p, n->inputs[0]);
    if (l == &TOP_IN_THE_SKY) {
        return &TOP_IN_THE_SKY;
    } else if (l == &BOT_IN_THE_SKY) {
        return &BOT_IN_THE_SKY;
    } else {
        assert(l->tag == LATTICE_TUPLE);
        int index = TB_NODE_GET_EXTRA_T(n, TB_NodeProj)->index;
        return l->elems[index];
    }
}

static Lattice* sccp_ctrl(TB_Passes* restrict p, TB_Node* n) {
    return lattice_universe_get(p, n->inputs[0]);
}

static Lattice* sccp_ptr_vals(TB_Passes* restrict p, TB_Node* n) {
    if (n->type == TB_LOCAL) {
        return &XNULL_IN_THE_SKY;
    } else {
        assert(n->type == TB_SYMBOL);
        return lattice_intern(p, (Lattice){ LATTICE_PTR, ._ptr = { TB_NODE_GET_EXTRA_T(n, TB_NodeSymbol)->sym } });
    }
}

static Lattice* sccp_lookup(TB_Passes* restrict p, TB_Node* n) {
    TB_NodeLookup* l = TB_NODE_GET_EXTRA(n);
    TB_DataType dt = n->dt;
    assert(dt.type == TB_INT);

    LatticeInt a = { l->entries[0].val, l->entries[0].val, l->entries[0].val, ~l->entries[0].val };
    FOREACH_N(i, 1, n->input_count) {
        LatticeInt b = { l->entries[i].val, l->entries[i].val, l->entries[i].val, ~l->entries[i].val };
        a = lattice_meet_int(a, b, dt);
    }

    return lattice_intern(p, (Lattice){ LATTICE_INT, ._int = a });
}

static Lattice* sccp_meetchads(TB_Passes* restrict p, TB_Node* n) {
    assert(n->type == TB_REGION || n->type == TB_SELECT || n->type == TB_PHI);
    int start = n->type == TB_REGION ? 0 : 1;
    if (n->type == TB_SELECT) start = 2 ;

    Lattice* l = lattice_universe_get(p, n->inputs[start]);
    FOREACH_N(i, start+1, n->input_count) {
        l = lattice_meet(p, l, lattice_universe_get(p, n->inputs[i]), n->dt);
    }
    return l;
}

// this is where the vtable goes for all peepholes
#include "peeps.h"

TB_Node* tb_pass_gvn_node(TB_Function* f, TB_Node* n) {
    size_t extra = extra_bytes(n);
    return tb__gvn(f, n, extra);
}

TB_Node* tb__gvn(TB_Function* f, TB_Node* n, size_t extra) {
    // try GVN, if we succeed, just delete the node and use the old copy
    TB_Node* k = nl_hashset_put2(&f->gvn_nodes, n, gvn_hash, gvn_compare);
    if (k != NULL) {
        // remove users
        FOREACH_REVERSE_N(i, 0, n->input_count) {
            User* u = remove_user(n, i);
            if (u) { tb_arena_free(f->arena, u, sizeof(User)); }

            n->inputs[i] = NULL;
        }

        // try free
        tb_arena_free(f->arena, n->inputs, n->input_cap * sizeof(TB_Node*));
        tb_arena_free(f->arena, n, sizeof(TB_Node) + extra);
        return k;
    } else {
        return n;
    }
}

TB_Node* make_poison(TB_Function* f, TB_DataType dt) {
    TB_Node* n = tb_alloc_node(f, TB_POISON, dt, 1, 0);
    set_input(f, n, f->root_node, 0);
    return tb__gvn(f, n, 0);
}

TB_Node* make_int_node(TB_Function* f, TB_Passes* restrict p, TB_DataType dt, uint64_t x) {
    uint64_t mask = tb__mask(dt.data);
    x &= mask;

    TB_Node* n = tb_alloc_node(f, TB_INTEGER_CONST, dt, 1, sizeof(TB_NodeInt));
    TB_NodeInt* i = TB_NODE_GET_EXTRA(n);
    i->value = x;

    set_input(f, n, f->root_node, 0);

    Lattice* l;
    if (dt.type == TB_INT) {
        l = lattice_intern(p, (Lattice){ LATTICE_INT, ._int = { x, x, ~x & mask, x } });
    } else {
        l = x ? &XNULL_IN_THE_SKY : &NULL_IN_THE_SKY;
    }
    lattice_universe_map(p, n, l);
    return tb__gvn(f, n, sizeof(TB_NodeInt));
}

TB_Node* dead_node(TB_Function* f, TB_Passes* p) {
    TB_Node* n = tb_alloc_node(f, TB_DEAD, TB_TYPE_CONTROL, 1, 0);
    set_input(f, n, f->root_node, 0);
    lattice_universe_map(p, n, &XCTRL_IN_THE_SKY);
    return tb__gvn(f, n, 0);
}

TB_Node* make_proj_node(TB_Function* f, TB_DataType dt, TB_Node* src, int i) {
    TB_Node* n = tb_alloc_node(f, TB_PROJ, dt, 1, sizeof(TB_NodeProj));
    set_input(f, n, src, 0);
    TB_NODE_SET_EXTRA(n, TB_NodeProj, .index = i);
    return n;
}

static void remove_input(TB_Function* f, TB_Node* n, size_t i) {
    // remove swap
    n->input_count--;
    if (n->input_count > 0) {
        if (n->input_count != i) {
            set_input(f, n, n->inputs[n->input_count], i);
        }
        set_input(f, n, NULL, n->input_count);
    }
}

void tb_pass_kill_node(TB_Function* f, TB_Node* n) {
    // remove from CSE if we're murdering it
    nl_hashset_remove2(&f->gvn_nodes, n, gvn_hash, gvn_compare);

    FOREACH_N(i, 0, n->input_count) {
        remove_user(n, i);
        n->inputs[i] = NULL;
    }

    // assert(n->users == NULL && "we can't kill nodes with users, that's fucking rude");
    n->input_count = 0;
    n->type = TB_NULL;
}

static User* remove_user(TB_Node* n, int slot) {
    // early out: there was no previous input
    if (n->inputs[slot] == NULL) return NULL;

    TB_Node* old = n->inputs[slot];
    User* old_use = old->users;
    if (old_use == NULL) return NULL;

    // remove old user (this must succeed unless our users go desync'd)
    for (User* prev = NULL; old_use; prev = old_use, old_use = old_use->next) {
        if (old_use->slot == slot && old_use->n == n) {
            // remove
            if (prev != NULL) {
                prev->next = old_use->next;
            } else {
                old->users = old_use->next;
            }

            return old_use;
        }
    }

    tb_panic("Failed to remove non-existent user %p from %p (slot %d)", old, n, slot);
}

void set_input(TB_Function* f, TB_Node* n, TB_Node* in, int slot) {
    // recycle the user
    User* old_use = remove_user(n, slot);

    n->inputs[slot] = in;
    if (in != NULL) {
        add_user(f, n, in, slot, old_use);
    }
}

// we sometimes get the choice to recycle users because we just deleted something
void add_user(TB_Function* f, TB_Node* n, TB_Node* in, int slot, User* recycled) {
    User* use = recycled ? recycled : TB_ARENA_ALLOC(f->arena, User);
    use->next = in->users;
    use->n = n;
    use->slot = slot;
    in->users = use;
}

static void tb_pass_mark_users_raw(TB_Passes* restrict p, TB_Node* n) {
    FOR_USERS(use, n) {
        tb_pass_mark(p, use->n);
    }
}

void tb_pass_mark(TB_Passes* opt, TB_Node* n) {
    worklist_push(&opt->worklist, n);
}

void tb_pass_mark_users(TB_Passes* restrict p, TB_Node* n) {
    FOR_USERS(use, n) {
        tb_pass_mark(p, use->n);
        TB_NodeTypeEnum type = use->n->type;

        // tuples changing means their projections did too.
        if (type == TB_PROJ) {
            tb_pass_mark_users(p, use->n);
        }

        // (br (cmp a b)) => ...
        // (or (shl a 24) (shr a 40)) => ...
        // (trunc (mul a b)) => ...
        if ((type >= TB_CMP_EQ && type <= TB_CMP_FLE) || type == TB_SHL || type == TB_SHR || type == TB_MUL) {
            tb_pass_mark_users_raw(p, use->n);
        }
    }
}

static void push_all_nodes(TB_Passes* restrict p, Worklist* restrict ws, TB_Function* f) {
    CUIK_TIMED_BLOCK("push_all_nodes") {
        worklist_test_n_set(ws, f->root_node);
        dyn_array_put(ws->items, f->root_node);

        for (size_t i = 0; i < dyn_array_length(ws->items); i++) {
            TB_Node* n = ws->items[i];

            FOR_USERS(use, n) {
                TB_Node* out = use->n;
                if (!worklist_test_n_set(ws, out)) {
                    dyn_array_put(ws->items, out);
                }
            }
        }

        CUIK_TIMED_BLOCK("reversing") {
            size_t last = dyn_array_length(ws->items) - 1;
            FOREACH_N(i, 0, dyn_array_length(ws->items) / 2) {
                SWAP(TB_Node*, ws->items[i], ws->items[last - i]);
            }
        }
    }
}

static void cool_print_type(TB_Node* n) {
    TB_DataType dt = n->dt;
    if (n->type != TB_ROOT && n->type != TB_REGION && !(n->type == TB_BRANCH && n->input_count == 1)) {
        if (n->type == TB_STORE) {
            dt = n->inputs[3]->dt;
        } else if (n->type == TB_BRANCH) {
            dt = n->inputs[1]->dt;
        } else if (n->type == TB_ROOT) {
            dt = n->input_count > 1 ? n->inputs[1]->dt : TB_TYPE_VOID;
        } else if (n->type >= TB_CMP_EQ && n->type <= TB_CMP_FLE) {
            dt = TB_NODE_GET_EXTRA_T(n, TB_NodeCompare)->cmp_dt;
        }
        printf(".");
        print_type(dt);
    }
}

void print_node_sexpr(TB_Node* n, int depth) {
    if (n->type == TB_INTEGER_CONST) {
        TB_NodeInt* num = TB_NODE_GET_EXTRA(n);
        if (n->dt.type == TB_PTR) {
            printf("%#"PRIx64, num->value);
        } else {
            printf("%"PRId64, tb__sxt(num->value, n->dt.data, 64));
        }
    } else if (n->type == TB_SYMBOL) {
        TB_Symbol* sym = TB_NODE_GET_EXTRA_T(n, TB_NodeSymbol)->sym;
        if (sym->name[0]) {
            printf("%s", sym->name);
        } else {
            printf("sym%p", sym);
        }
    } else if (depth >= 1) {
        printf("(v%u: %s", n->gvn, tb_node_get_name(n));
        cool_print_type(n);
        printf(" ...)");
    } else {
        depth -= (n->type == TB_PROJ);

        printf("(v%u: %s", n->gvn, tb_node_get_name(n));
        cool_print_type(n);
        FOREACH_N(i, 0, n->input_count) if (n->inputs[i]) {
            if (i == 0) printf(" @");
            else printf(" ");

            print_node_sexpr(n->inputs[i], depth + 1);
        }

        switch (n->type) {
            case TB_ARRAY_ACCESS:
            printf(" %"PRId64, TB_NODE_GET_EXTRA_T(n, TB_NodeArray)->stride);
            break;

            case TB_MEMBER_ACCESS:
            printf(" %"PRId64, TB_NODE_GET_EXTRA_T(n, TB_NodeMember)->offset);
            break;

            case TB_PROJ:
            printf(" %d", TB_NODE_GET_EXTRA_T(n, TB_NodeProj)->index);
            break;
        }
        printf(")");
    }
}

// Returns NULL or a modified node (could be the same node, we can stitch it back into place)
static TB_Node* idealize(TB_Passes* restrict p, TB_Function* f, TB_Node* n) {
    NodeIdealize ideal = vtables[n->type].idealize;
    return ideal ? ideal(p, f, n) : NULL;
}

static TB_Node* identity(TB_Passes* restrict p, TB_Function* f, TB_Node* n) {
    NodeIdentity identity = vtables[n->type].identity;
    return identity ? identity(p, f, n) : n;
}

static Lattice* sccp(TB_Passes* restrict p, TB_Node* n) {
    NodeConstprop constprop = vtables[n->type].constprop;
    return constprop ? constprop(p, n) : NULL;
}

// converts constant Lattice into constant node
static TB_Node* try_as_const(TB_Passes* restrict p, TB_Node* n, Lattice* l) {
    // already a constant?
    if (n->type == TB_SYMBOL || n->type == TB_INTEGER_CONST || n->type == TB_FLOAT32_CONST || n->type == TB_FLOAT64_CONST) {
        return NULL;
    }

    switch (l->tag) {
        case LATTICE_INT: {
            // degenerate range
            if (l->_int.min == l->_int.max) {
                return make_int_node(p->f, p, n->dt, l->_int.max);
            }

            // all bits are known
            uint64_t mask = tb__mask(n->dt.data);
            if ((l->_int.known_zeros | l->_int.known_ones) == mask) {
                return make_int_node(p->f, p, n->dt, l->_int.known_ones);
            }

            return NULL;
        }

        case LATTICE_NULL:
        return make_int_node(p->f, p, n->dt, 0);

        case LATTICE_PTR: {
            return make_int_node(p->f, p, n->dt, 0);
        }

        case LATTICE_TUPLE: {
            if (n->type != TB_BRANCH) return NULL;

            // check if tuple is constant path
            int trues = 0;
            FOREACH_N(i, 0, l->_tuple.count) {
                if (l->elems[i] == &CTRL_IN_THE_SKY) {
                    trues++;
                } else if (l->elems[i] != &XCTRL_IN_THE_SKY) {
                    return NULL;
                }
            }

            if (trues == 1) {
                TB_Node* dead = dead_node(p->f, p);
                TB_Node* ctrl = n->inputs[0];

                FOR_USERS(u, n) {
                    if (u->n->type == TB_PROJ) {
                        int index = TB_NODE_GET_EXTRA_T(n, TB_NodeProj)->index;
                        TB_Node* in = l->elems[index] == &CTRL_IN_THE_SKY ? ctrl : dead;
                        subsume_node(p->f, u->n, ctrl);
                    }
                }

                // no more projections, kill the branch
                tb_pass_kill_node(p->f, n);
                tb_pass_mark_users(p, dead);
                return ctrl;
            } else {
                return NULL;
            }
        }

        default: return NULL;
    }
}

static void validate_node_users(TB_Node* n) {
    if (n != NULL) {
        FOR_USERS(use, n) {
            tb_assert(use->n->inputs[use->slot] == n, "Mismatch between def-use and use-def data");
        }
    }
}

static void print_lattice(Lattice* l, TB_DataType dt) {
    switch (l->tag) {
        case LATTICE_BOT: printf("[bot]"); break;
        case LATTICE_TOP: printf("[top]"); break;

        case LATTICE_CTRL:  printf("[ctrl]"); break;
        case LATTICE_XCTRL: printf("[~ctrl]"); break;

        case LATTICE_FLOAT32: printf("[f32]"); break;
        case LATTICE_FLOAT64: printf("[f64]"); break;

        case LATTICE_NULL:  printf("[null]"); break;
        case LATTICE_XNULL: printf("[~null]"); break;
        case LATTICE_PTR:   printf("[%s]", l->_ptr.sym->name); break;

        case LATTICE_MEM:   printf("mem[%d]", l->_mem.alias_idx); break;

        case LATTICE_TUPLE: {
            printf("[");
            FOREACH_N(i, 0, l->_tuple.count) {
                if (i) printf(", ");
                print_lattice(l->elems[i], TB_TYPE_I64);
            }
            printf("]");
            break;
        }
        case LATTICE_INT: {
            assert(dt.type == TB_INT);
            if (l->_int.min == l->_int.max) {
                printf("[%"PRId64, tb__sxt(l->_int.min, dt.data, 64));
            } else if (l->_int.min > l->_int.max) {
                printf("[%"PRId64",%"PRId64, tb__sxt(l->_int.min, dt.data, 64), tb__sxt(l->_int.max, dt.data, 64));
            } else {
                printf("[%"PRIu64",%"PRIu64, l->_int.min, l->_int.max);
            }

            uint64_t known = l->_int.known_zeros | l->_int.known_ones;
            if (known && known != UINT64_MAX) {
                printf("; zeros=%#"PRIx64", ones=%#"PRIx64, l->_int.known_zeros, l->_int.known_ones);
            }
            printf("]");
            break;
        }

        default:
        break;
    }
}

// because certain optimizations apply when things are the same
// we mark ALL users including the ones who didn't get changed
// when subsuming.
TB_Node* tb_pass_peephole_node(TB_Passes* p, TB_Node* n) {
    TB_Function* f = p->f;

    // idealize can modify the node, make sure it's not in the GVN pool at the time
    nl_hashset_remove2(&f->gvn_nodes, n, gvn_hash, gvn_compare);

    // idealize node (this can technically run an arbitrary number of times
    // but in practice we should only hit a node like once or twice)
    TB_Node* k = idealize(p, f, n);
    DO_IF(TB_OPTDEBUG_PEEP)(int loop_count=0);
    while (k != NULL) {
        DO_IF(TB_OPTDEBUG_STATS)(p->stats.rewrites++);
        DO_IF(TB_OPTDEBUG_PEEP)(printf(" => \x1b[32m"), print_node_sexpr(k, 0), printf("\x1b[0m"));

        // transfer users from n -> k
        if (n != k) {
            subsume_node(f, n, k);
            n = k;
        }

        // mark post subsume since previous users of n might have
        // name equality based opts.
        tb_pass_mark_users(p, n);

        // try again, maybe we get another transformation
        k = idealize(p, f, n);
        DO_IF(TB_OPTDEBUG_PEEP)(if (++loop_count > 5) { log_warn("%p: we looping a lil too much dawg...", n); });
    }

    // generate fancier type (SCCP)
    if (n->dt.type != TB_MEMORY) {
        Lattice* new_type = sccp(p, n);

        // no type provided? just make a not-so-form fitting bottom type
        if (new_type == NULL) {
            Lattice* old_type = lattice_universe_get(p, n);
            new_type = old_type != &TOP_IN_THE_SKY ? old_type : lattice_from_dt(p, n->dt);
        }

        // print fancy type
        DO_IF(TB_OPTDEBUG_PEEP)(printf(" => \x1b[93m"), print_lattice(new_type, n->dt), printf("\x1b[0m"));

        // types that consist of one possible value are made into value constants.
        k = try_as_const(p, n, new_type);
        if (k != NULL) {
            DO_IF(TB_OPTDEBUG_PEEP)(printf(" => \x1b[96m"), print_node_sexpr(k, 0), printf("\x1b[0m"));

            subsume_node(f, n, k);
            tb_pass_mark_users(p, k);
            return k;
        } else if (lattice_universe_map_progress(p, n, new_type)) {
            tb_pass_mark_users(p, n);
        }
    }

    // convert into matching identity
    k = identity(p, f, n);
    if (n != k) {
        DO_IF(TB_OPTDEBUG_STATS)(p->stats.identities++);
        DO_IF(TB_OPTDEBUG_PEEP)(printf(" => \x1b[33m"), print_node_sexpr(k, 0), printf("\x1b[0m"));

        subsume_node(f, n, k);
        tb_pass_mark_users(p, k);
        return k;
    }

    // global value numbering
    k = nl_hashset_put2(&f->gvn_nodes, n, gvn_hash, gvn_compare);
    if (k && (k != n)) {
        DO_IF(TB_OPTDEBUG_STATS)(p->stats.gvn_hit++);
        DO_IF(TB_OPTDEBUG_PEEP)(printf(" => \x1b[31mGVN\x1b[0m"));

        subsume_node(f, n, k);
        tb_pass_mark_users(p, k);
        return k;
    } else {
        DO_IF(TB_OPTDEBUG_STATS)(p->stats.gvn_miss++);
    }

    return n;
}

static void subsume_node2(TB_Function* f, TB_Node* n, TB_Node* new_n) {
    CUIK_TIMED_BLOCK("subsume") {
        User* use = n->users;
        while (use != NULL) {
            tb_assert(use->n->inputs[use->slot] == n, "Mismatch between def-use and use-def data");

            // set_input will delete 'use' so we can't use it afterwards
            TB_Node* use_n = use->n;
            User* next = use->next;

            set_input(f, use->n, new_n, use->slot);
            use = next;
        }
    }
}

static void subsume_node(TB_Function* f, TB_Node* n, TB_Node* new_n) {
    subsume_node2(f, n, new_n);
    tb_pass_kill_node(f, n);
}

TB_Passes* tb_pass_enter(TB_Function* f, TB_Arena* arena) {
    assert(f->root_node && "missing root node");

    TB_Passes* p = tb_platform_heap_alloc(sizeof(TB_Passes));
    *p = (TB_Passes){ .f = f };

    f->arena = arena;

    verify_tmp_arena(p);
    worklist_alloc(&p->worklist, f->node_count);

    // generate work list (put everything)
    CUIK_TIMED_BLOCK("gen worklist") {
        push_all_nodes(p, &p->worklist, f);

        DO_IF(TB_OPTDEBUG_STATS)(p->stats.initial = worklist_popcount(&p->worklist));
    }

    DO_IF(TB_OPTDEBUG_PEEP)(log_debug("%s: starting passes with %d nodes", f->super.name, f->node_count));

    return p;
}

void tb_pass_sroa(TB_Passes* p) {
    CUIK_TIMED_BLOCK("sroa") {
        verify_tmp_arena(p);

        TB_Function* f = p->f;
        Worklist* ws = &p->worklist;

        int pointer_size = f->super.module->codegen->pointer_size;
        TB_Node* root = f->root_node;

        // write initial locals
        FOR_USERS(u, root) {
            if (u->n->type == TB_LOCAL) {
                worklist_push(&p->worklist, u->n);
            }
        }

        // i think the SROA'd pieces can't themselves split more? that should something we check
        size_t local_count = dyn_array_length(ws->items);
        for (size_t i = 0; i < local_count; i++) {
            assert(ws->items[i]->type == TB_LOCAL);
            sroa_rewrite(p, pointer_size, root, ws->items[i]);
        }
    }
}

typedef union {
    uint64_t i;
    User* ctrl;
} Value;

typedef struct {
    Worklist* ws;
    Value* vals;
    bool* ready;
    int phi_i;
} Interp;

static Value* in_val(Interp* vm, TB_Node* n, int i) { return &vm->vals[n->inputs[i]->gvn]; }
static Value eval(Interp* vm, TB_Node* n) {
    printf("  EVAL v%u\n", n->gvn);
    switch (n->type) {
        case TB_INTEGER_CONST: return (Value){ .i = TB_NODE_GET_EXTRA_T(n, TB_NodeInt)->value };

        case TB_ADD: {
            uint64_t a = in_val(vm, n, 1)->i;
            uint64_t b = in_val(vm, n, 2)->i;
            return (Value){ .i = a + b };
        }

        case TB_CMP_SLT: {
            uint64_t a = in_val(vm, n, 1)->i;
            uint64_t b = in_val(vm, n, 2)->i;
            return (Value){ .i = a < b };
        }

        case TB_BRANCH: {
            TB_NodeBranch* br = TB_NODE_GET_EXTRA(n);
            uint64_t key = in_val(vm, n, 1)->i;
            int index = 0;

            FOREACH_N(i, 0, br->succ_count - 1) {
                if (key == br->keys[i]) {
                    index = i + 1;
                    break;
                }
            }

            User* ctrl = proj_with_index(n, index);
            return (Value){ .ctrl = ctrl };
        }

        case TB_REGION:
        return (Value){ .ctrl = cfg_next_user(n) };

        case TB_PROJ:
        if (n->dt.type == TB_MEMORY) {
            return (Value){ .i = 0 };
        } else if (n->dt.type == TB_CONTROL) {
            return (Value){ .ctrl = cfg_next_user(n) };
        } else {
            tb_todo();
        }

        // control nodes
        case TB_ROOT: {
            uint64_t v = in_val(vm, n, 3)->i;

            printf("END %"PRIu64"\n", v);
            return (Value){ .ctrl = NULL };
        }

        default: tb_todo();
    }
}

static bool is_ready(Interp* vm, TB_Node* n) {
    FOREACH_N(i, 1, n->input_count) {
        if (!vm->ready[n->inputs[i]->gvn]) {
            return false;
        }
    }

    return true;
}

static void dirty_deps(Interp* vm, TB_Node* n) {
    printf("    DIRTY v%u\n", n->gvn);
    vm->ready[n->gvn] = false;

    FOR_USERS(u, n) {
        if (u->n->type != TB_PHI && vm->ready[u->n->gvn]) {
            dirty_deps(vm, u->n);
        }
    }
}

void dummy_interp(TB_Passes* p) {
    TB_Function* f = p->f;
    TB_Arena* arena = get_temporary_arena(f->super.module);

    TB_Node* ip = cfg_next_control(f->root_node);

    // We need to generate a CFG
    TB_CFG cfg = tb_compute_rpo(f, p);
    // And perform global scheduling
    tb_pass_schedule(p, cfg, false);

    Interp vm = {
        .ws = &p->worklist,
        .vals = tb_arena_alloc(arena, f->node_count * sizeof(Value)),
        .ready = tb_arena_alloc(arena, f->node_count * sizeof(bool))
    };

    int last_edge = 0;
    while (ip) {
        printf("IP = v%u\n", ip->gvn);

        worklist_clear(&p->worklist);

        // push all direct users of the parent's users (our antideps)
        FOR_USERS(u, ip->inputs[last_edge]) {
            if (is_ready(&vm, u->n)) {
                worklist_push(&p->worklist, u->n);
            }
        }

        if (ip->type != TB_REGION) {
            FOREACH_N(i, 1, ip->input_count) {
                worklist_push(&p->worklist, ip->inputs[i]);
            }
        }

        if (is_ready(&vm, ip)) {
            worklist_push(&p->worklist, ip);
        }

        size_t i = 0;
        for (; i < dyn_array_length(p->worklist.items); i++) {
            TB_Node* n = p->worklist.items[i];
            if (n->type == TB_PHI) continue;

            vm.vals[n->gvn] = eval(&vm, n);
            vm.ready[n->gvn] = true;

            FOR_USERS(u, n) {
                if (is_ready(&vm, u->n)) {
                    worklist_push(&p->worklist, u->n);
                }
            }

            // advance now
            if (n == ip) {
                dyn_array_set_length(p->worklist.items, i + 1);
                break;
            }
        }

        if (ip->type == TB_REGION) {
            vm.vals[ip->gvn] = eval(&vm, ip);
            vm.ready[ip->gvn] = true;
        } else {
            assert(is_ready(&vm, ip));
        }

        User* succ = vm.vals[ip->gvn].ctrl;
        if (succ == NULL) {
            break;
        }

        last_edge = succ->slot;
        ip = succ->n;

        if (ip->type == TB_REGION) {
            FOR_USERS(u, ip) {
                TB_Node* phi = u->n;
                if (phi->type == TB_PHI) {
                    TB_Node* in = phi->inputs[1 + last_edge];
                    if (is_ready(&vm, in)) {
                        worklist_push(&p->worklist, in);
                    }
                }
            }

            for (; i < dyn_array_length(p->worklist.items); i++) {
                TB_Node* n = p->worklist.items[i];
                if (n->type == TB_PHI) continue;

                vm.vals[n->gvn] = eval(&vm, n);
                vm.ready[n->gvn] = true;
            }

            FOR_USERS(u, ip) {
                TB_Node* phi = u->n;
                if (phi->type == TB_PHI) {
                    printf("  PHI = v%u (v%u)\n", phi->gvn, phi->inputs[1 + last_edge]->gvn);

                    Value* v = &vm.vals[phi->inputs[1 + last_edge]->gvn];
                    vm.vals[phi->gvn] = *v;

                    dirty_deps(&vm, phi);
                    vm.ready[phi->gvn] = true;
                }
            }
        }
    }
}

void tb_pass_optimize(TB_Passes* p) {
    tb_pass_peephole(p);
    tb_pass_split_locals(p);
    tb_pass_peephole(p);
    tb_pass_loop(p);
    tb_pass_peephole(p);

    // tb_pass_print(p);
    // dummy_interp(p);
}

static size_t tb_pass_update_cfg(TB_Passes* p, Worklist* ws, bool preserve) {
    TB_Function* f = p->f;

    p->cfg = tb_compute_rpo2(f, ws);
    tb_compute_dominators2(f, ws, p->cfg);

    if (!preserve) {
        tb_free_cfg(&p->cfg);
    }

    return p->cfg.block_count;
}

void tb_pass_prep(TB_Passes* p) {
    TB_Function* f = p->f;

    if (p->types == NULL) {
        TB_ThreadInfo* info = tb_thread_info(f->super.module);

        CUIK_TIMED_BLOCK("allocate type array") {
            size_t count = (f->node_count + 63ull) & ~63ull;
            p->type_interner = nl_hashset_alloc(64);
            p->type_cap = count;
            p->types = tb_platform_heap_alloc(count * sizeof(Lattice*));
            FOREACH_N(i, 0, count) {
                p->types[i] = &TOP_IN_THE_SKY;
            }

            nl_hashset_put2(&p->type_interner, &BOT_IN_THE_SKY,   lattice_hash, lattice_cmp);
            nl_hashset_put2(&p->type_interner, &TOP_IN_THE_SKY,   lattice_hash, lattice_cmp);
            nl_hashset_put2(&p->type_interner, &CTRL_IN_THE_SKY,  lattice_hash, lattice_cmp);
            nl_hashset_put2(&p->type_interner, &XCTRL_IN_THE_SKY, lattice_hash, lattice_cmp);
            nl_hashset_put2(&p->type_interner, &NULL_IN_THE_SKY,  lattice_hash, lattice_cmp);
            nl_hashset_put2(&p->type_interner, &XNULL_IN_THE_SKY, lattice_hash, lattice_cmp);
            nl_hashset_put2(&p->type_interner, &FALSE_IN_THE_SKY, lattice_hash, lattice_cmp);
            nl_hashset_put2(&p->type_interner, &TRUE_IN_THE_SKY,  lattice_hash, lattice_cmp);

            // place ROOT type
            p->root_mem = lattice_new_alias(p);
            p->types[f->root_node->gvn] = lattice_tuple_from_node(p, f->root_node);
        }
    }
}

void tb_pass_peephole(TB_Passes* p) {
    verify_tmp_arena(p);
    TB_Function* f = p->f;

    // allocates shit, imporant shit
    tb_pass_prep(p);

    CUIK_TIMED_BLOCK("peephole") {
        TB_Node* n;
        while ((n = worklist_pop(&p->worklist))) {
            DO_IF(TB_OPTDEBUG_STATS)(p->stats.peeps++);
            DO_IF(TB_OPTDEBUG_PEEP)(printf("peep t=%d? ", ++p->stats.time), print_node_sexpr(n, 0));

            // must've dead sometime between getting scheduled and getting here.
            if (n->type != TB_PROJ && n->users == NULL) {
                DO_IF(TB_OPTDEBUG_PEEP)(printf(" => \x1b[196mKILL\x1b[0m\n"));
                tb_pass_kill_node(f, n);
            } else {
                tb_pass_peephole_node(p, n);
                DO_IF(TB_OPTDEBUG_PEEP)(printf("\n"));
            }
        }
    }
}

void tb_pass_exit(TB_Passes* p) {
    CUIK_TIMED_BLOCK("exit") {
        verify_tmp_arena(p);

        TB_Function* f = p->f;

        #if TB_OPTDEBUG_STATS
        push_all_nodes(p, &p->worklist, f);
        int final_count = worklist_popcount(&p->worklist);
        double factor = ((double) final_count / (double) p->stats.initial) * 100.0;

        printf("%s: stats:\n", f->super.name);
        printf("  %4d   -> %4d nodes (%.2f%%)\n", p->stats.initial, final_count, factor);
        printf("  %4d GVN hit    %4d GVN miss\n", p->stats.gvn_hit, p->stats.gvn_miss);
        printf("  %4d peepholes  %4d rewrites    %4d identities\n", p->stats.peeps, p->stats.rewrites, p->stats.identities);
        #endif

        worklist_free(&p->worklist);

        if (p->types != NULL) {
            nl_hashset_free(p->type_interner);
            tb_platform_heap_free(p->types);
        }

        tb_arena_clear(tmp_arena);
        tb_platform_heap_free(p);
    }
}

typedef struct {
    bool on_stack;
    int index, low_link;
} SCCNode;

typedef struct {
    TB_Arena* arena;
    size_t fn_count;
    NL_Table nodes;

    size_t stk_cnt;
    TB_Function** stk;

    size_t ws_cnt;
    TB_Function** ws;

    int index;
} SCC;

static TB_Function* static_call_site(TB_Node* n) {
    // is this call site a static function call
    assert(n->type == TB_CALL || n->type == TB_TAILCALL);
    if (n->inputs[2]->type != TB_SYMBOL) return NULL;

    TB_Symbol* target = TB_NODE_GET_EXTRA_T(n->inputs[2], TB_NodeSymbol)->sym;
    if (atomic_load_explicit(&target->tag, memory_order_relaxed) != TB_SYMBOL_FUNCTION) return NULL;

    return (TB_Function*) target;
}

static SCCNode* scc_walk(SCC* restrict scc, TB_Function* f) {
    SCCNode* n = tb_arena_alloc(scc->arena, sizeof(SCCNode));
    n->index = scc->index;
    n->low_link = scc->index;
    n->on_stack = true;
    scc->index += 1;
    nl_table_put(&scc->nodes, f, n);

    scc->stk[scc->stk_cnt++] = f;

    // consider the successors
    TB_Node* callgraph = f->root_node->inputs[3];
    FOREACH_N(i, 1, callgraph->input_count) {
        TB_Node* call = callgraph->inputs[i];
        TB_Function* target = static_call_site(call);
        if (target != NULL) {
            SCCNode* succ = nl_table_get(&scc->nodes, target);
            if (succ == NULL) {
                succ = scc_walk(scc, target);
                if (n->low_link > succ->low_link) { n->low_link = succ->low_link; }
            } else if (succ->on_stack) {
                if (n->low_link > succ->index) { n->low_link = succ->index; }
            }
        }
    }

    // we're the root, construct an SCC
    if (n->low_link == n->index) {
        TB_Function* kid_f;
        do {
            assert(scc->stk_cnt > 0);
            kid_f = scc->stk[--scc->stk_cnt];

            SCCNode* kid_n = nl_table_get(&scc->nodes, kid_f);
            kid_n->on_stack = false;
            scc->ws[scc->ws_cnt++] = kid_f;
        } while (kid_f != f);
    }

    return n;
}

void tb_module_prepare_ipo(TB_Module* m) {
    SCC scc = { 0 };
    scc.arena    = get_temporary_arena(m);
    scc.fn_count = m->symbol_count[TB_SYMBOL_FUNCTION];
    scc.ws       = tb_arena_alloc(scc.arena, scc.fn_count * sizeof(TB_Function*));

    CUIK_TIMED_BLOCK("build SCC") {
        TB_ArenaSavepoint sp = tb_arena_save(scc.arena);
        scc.stk      = tb_arena_alloc(scc.arena, scc.fn_count * sizeof(TB_Function*));
        scc.nodes    = nl_table_arena_alloc(scc.arena, scc.fn_count);

        // build strongly connected components
        TB_ThreadInfo* info = atomic_load_explicit(&m->first_info_in_module, memory_order_relaxed);
        while (info != NULL) {
            TB_Symbol** syms = (TB_Symbol**) info->symbols.data;
            if (syms == NULL) continue;

            FOREACH_N(i, 0, 1ull << info->symbols.exp) {
                TB_Symbol* s = syms[i];
                if (s == NULL || s == NL_HASHSET_TOMB) continue;
                if (atomic_load_explicit(&s->tag, memory_order_relaxed) != TB_SYMBOL_FUNCTION) continue;

                if (nl_table_get(&scc.nodes, s) == NULL) {
                    scc_walk(&scc, (TB_Function*) s);
                }
            }

            info = info->next_in_module;
        }
        tb_arena_restore(scc.arena, sp);
    }

    // we've got our bottom up ordering on the worklist... start trying to inline callsites
    TB_OPTDEBUG(INLINE)(printf("BOTTOM-UP ORDER:\n"));
    FOREACH_N(i, 0, scc.ws_cnt) {
        TB_Function* f = scc.ws[i];

        TB_OPTDEBUG(INLINE)(printf("* FUNCTION: %s\n", f->super.name));

        TB_Node* callgraph = f->root_node->inputs[3];
        FOREACH_N(i, 1, callgraph->input_count) {
            TB_Node* call = callgraph->inputs[i];
            TB_Function* target = static_call_site(call);
            if (target != NULL) {
                TB_OPTDEBUG(INLINE)(printf("  -> %s (from v%u)\n", target->super.name, call->gvn));
            }
        }
    }

}

bool tb_module_ipo(TB_Module* m) {
    return false;
}
