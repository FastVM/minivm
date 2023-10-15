// Let's just explain the architecture of the optimizer here.
//
// # Peephole optimizations
//   These are the kind which work locally like 2+2=4 and in TB's design they're
//   performed incrementally which means that certain mutations must go through
//   functions to guarentee they update correctly. Let's go over those:
//
//   set_input(opt, n, in, slot)
//     basically `n->inputs[slot] = in` except it correctly updates the user set
//
// # Implement peepholes
//     TODO
//
#include "../passes.h"
#include <log.h>

thread_local TB_Arena* tmp_arena;

// helps us do some matching later
static TB_Node* unsafe_get_region(TB_Node* n);
static void add_user(TB_Passes* restrict p, TB_Node* n, TB_Node* in, int slot, User* recycled);
static User* remove_user(TB_Passes* restrict p, TB_Node* n, int slot);
static void remove_input(TB_Passes* restrict p, TB_Function* f, TB_Node* n, size_t i);

// transmutations let us generate new nodes from old ones
TB_Node* tb_transmute_to_int(TB_Function* f, TB_Passes* restrict p, TB_DataType dt, int num_words);

static void subsume_node(TB_Passes* restrict p, TB_Function* f, TB_Node* n, TB_Node* new_n);

// *new_node is set to true if we make a new node, it won't set it false for you
static TB_Node* clone_node(TB_Passes* restrict p, TB_Function* f, TB_Node* region, TB_Node* n, bool* new_node);

// node creation helpers
TB_Node* make_dead(TB_Function* f, TB_Passes* restrict p);
TB_Node* make_poison(TB_Function* f, TB_Passes* restrict p, TB_DataType dt);
TB_Node* make_int_node(TB_Function* f, TB_Passes* restrict p, TB_DataType dt, uint64_t x);
TB_Node* make_proj_node(TB_Function* f, TB_Passes* restrict p, TB_DataType dt, TB_Node* src, int i);

static bool remove_pred(TB_Passes* restrict p, TB_Function* f, TB_Node* src, TB_Node* dst);

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
        tb_arena_clear(&p->pinned_thread->tmp_arena);
    } else if (p->pinned_thread != info) {
        tb_panic(
            "TB_Passes are bound to a thread, you can't switch which threads they're run on\n\n"
            "NOTE: if you really need to run across threads you'll need to exit the passes and\n"
            "start anew... though you pay a performance hit everytime you start one"
        );
    }

    tmp_arena = &p->pinned_thread->tmp_arena;
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
    for (User* u = find_users(p, n); u; u = u->next) {
        if (u->slot == slot && is_mem_out_op(u->n)) return u->n;
    }

    return NULL;
}

static TB_Node* single_user(TB_Passes* restrict p, TB_Node* n) {
    User* u = find_users(p, n);
    assert(u && u->next == NULL);
    return u->n;
}

static bool single_use(TB_Passes* restrict p, TB_Node* n) {
    return find_users(p, n)->next == NULL;
}

static bool is_same_align(TB_Node* a, TB_Node* b) {
    TB_NodeMemAccess* aa = TB_NODE_GET_EXTRA(a);
    TB_NodeMemAccess* bb = TB_NODE_GET_EXTRA(b);
    return aa->align == bb->align;
}

static bool is_empty_bb(TB_Passes* restrict p, TB_Node* end) {
    assert(end->type == TB_BRANCH || end->type == TB_UNREACHABLE);
    if (!is_block_begin(end->inputs[0])) {
        return false;
    }

    TB_Node* bb = end->inputs[0];
    for (User* use = find_users(p, bb); use; use = use->next) {
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

// unity build with all the passes
#include "lattice.h"
#include "cfg.h"
#include "cse.h"
#include "dce.h"
#include "fold.h"
#include "mem_opt.h"
#include "loop.h"
#include "branches.h"
#include "print.h"
#include "mem2reg.h"
#include "gcm.h"
#include "libcalls.h"
#include "scheduler.h"

static TB_Node* gvn(TB_Passes* restrict p, TB_Node* n, size_t extra) {
    // try CSE, if we succeed, just delete the node and use the old copy
    TB_Node* k = nl_hashset_put2(&p->cse_nodes, n, cse_hash, cse_compare);
    if (k != NULL) {
        // try free
        tb_arena_free(p->f->arena, n->inputs, sizeof(TB_Node*));
        tb_arena_free(p->f->arena, n, sizeof(TB_Node) + extra);
        return k;
    } else {
        return n;
    }
}

TB_Node* make_poison(TB_Function* f, TB_Passes* restrict p, TB_DataType dt) {
    return gvn(p, tb_alloc_node(f, TB_POISON, dt, 1, 0), 0);
}

TB_Node* make_int_node(TB_Function* f, TB_Passes* restrict p, TB_DataType dt, uint64_t x) {
    TB_Node* n = tb_alloc_node(f, TB_INTEGER_CONST, dt, 1, sizeof(TB_NodeInt));
    TB_NodeInt* i = TB_NODE_GET_EXTRA(n);
    i->value = x;
    return gvn(p, n, sizeof(TB_NodeInt));
}

TB_Node* make_proj_node(TB_Function* f, TB_Passes* restrict p, TB_DataType dt, TB_Node* src, int i) {
    TB_Node* n = tb_alloc_node(f, TB_PROJ, dt, 1, sizeof(TB_NodeProj));
    set_input(p, n, src, 0);
    TB_NODE_SET_EXTRA(n, TB_NodeProj, .index = i);
    return n;
}

static TB_Node* clone_node(TB_Passes* restrict p, TB_Function* f, TB_Node* region, TB_Node* n, bool* new_node) {
    assert(0 && "TODO");
    return NULL;
}

static void remove_input(TB_Passes* restrict p, TB_Function* f, TB_Node* n, size_t i) {
    // remove swap
    n->input_count--;
    if (n->input_count > 0) {
        if (n->input_count != i) {
            set_input(p, n, n->inputs[n->input_count], i);
        }
        set_input(p, n, NULL, n->input_count);
    }
}

// src -//-> dst
static bool remove_pred(TB_Passes* restrict p, TB_Function* f, TB_Node* src, TB_Node* dst) {
    FOREACH_N(i, 0, dst->input_count) {
        if (tb_get_parent_region(dst->inputs[i]) == src) {
            remove_input(p, f, dst, i);

            // update PHIs
            for (User* use = find_users(p, dst); use; use = use->next) {
                if (use->n->type == TB_PHI && use->slot == 0) {
                    remove_input(p, f, use->n, i + 1);
                }
            }

            tb_pass_mark(p, dst);
            tb_pass_mark_users(p, dst);
            return true;
        }
    }

    return false;
}

void tb_pass_kill_node(TB_Passes* restrict p, TB_Node* n) {
    // remove from CSE if we're murdering it
    nl_hashset_remove2(&p->cse_nodes, n, cse_hash, cse_compare);

    if (n->type == TB_LOCAL) {
        // remove from local list
        dyn_array_for(i, p->locals) if (p->locals[i] == n) {
            dyn_array_remove(p->locals, i);
            break;
        }
    }

    FOREACH_N(i, 0, n->input_count) {
        remove_user(p, n, i);
        n->inputs[i] = NULL;
    }

    n->users = NULL;

    n->input_count = 0;
    n->type = TB_NULL;
}

static User* remove_user(TB_Passes* restrict p, TB_Node* n, int slot) {
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

void set_input(TB_Passes* restrict p, TB_Node* n, TB_Node* in, int slot) {
    // recycle the user
    User* old_use = remove_user(p, n, slot);

    n->inputs[slot] = in;
    if (in != NULL) {
        add_user(p, n, in, slot, old_use);
    }
}

// we sometimes get the choice to recycle users because we just deleted something
static void add_user(TB_Passes* restrict p, TB_Node* n, TB_Node* in, int slot, User* recycled) {
    User* use = recycled ? recycled : TB_ARENA_ALLOC(tmp_arena, User);
    use->next = in->users;
    use->n = n;
    use->slot = slot;
    in->users = use;
}

static void tb_pass_mark_users_raw(TB_Passes* restrict p, TB_Node* n) {
    for (User* use = n->users; use; use = use->next) {
        tb_pass_mark(p, use->n);
    }
}

void tb_pass_mark(TB_Passes* opt, TB_Node* n) {
    worklist_push(&opt->worklist, n);
}

void tb_pass_mark_users(TB_Passes* restrict p, TB_Node* n) {
    for (User* use = n->users; use; use = use->next) {
        tb_pass_mark(p, use->n);
        TB_NodeTypeEnum type = use->n->type;

        // tuples changing means their projections did too.
        if (use->n->dt.type == TB_TUPLE || type == TB_PROJ) {
            tb_pass_mark_users(p, use->n);
        }

        // if the store is changed, the users (potential loads) should be notified.
        // (br (cmp ...))
        if (type == TB_CMP_NE || type == TB_CMP_EQ || type == TB_STORE) {
            tb_pass_mark_users_raw(p, use->n);
        }

        if (type == TB_REGION) {
            tb_pass_mark_users_raw(p, use->n);

            TB_NodeRegion* r = TB_NODE_GET_EXTRA(use->n);
            TB_Node* end = r->end;
            if (end->type == TB_BRANCH) {
                tb_pass_mark(p, end);

                // mark direct successors
                TB_NodeBranch* br_info = TB_NODE_GET_EXTRA(end);
                FOREACH_N(i, 0, br_info->succ_count) {
                    tb_pass_mark(p, br_info->succ[i]);
                }
            }
        }
    }
}

static void push_all_bb(Worklist* restrict ws, DynArray(TB_Node*)* stack_ptr, TB_Node* root) {
    if (worklist_test_n_set(ws, root)) {
        return;
    }

    // walk control edges (aka predecessors)
    assert(root->type == TB_START || root->type == TB_REGION);
    TB_NodeRegion* r = TB_NODE_GET_EXTRA(root);
    TB_Node* end = r->end;

    if (end->type == TB_BRANCH) {
        TB_NodeBranch* br = TB_NODE_GET_EXTRA(end);
        FOREACH_REVERSE_N(i, 0, br->succ_count) {
            push_all_bb(ws, stack_ptr, br->succ[i]);
        }
    }

    DynArray(TB_Node*) stack = *stack_ptr;

    // place endpoint, we'll construct the rest from there
    worklist_test_n_set(ws, end);
    dyn_array_put(stack, end);

    while (dyn_array_length(stack)) {
        TB_Node* n = dyn_array_pop(stack);

        // place self first
        dyn_array_put(ws->items, n);

        // push inputs
        FOREACH_N(i, 0, n->input_count) {
            TB_Node* in = n->inputs[i];
            if (in && !worklist_test_n_set(ws, in)) {
                dyn_array_put(stack, in);
            }
        }
    }

    *stack_ptr = stack;
}

static void push_all_nodes(Worklist* restrict ws, TB_Node* root) {
    CUIK_TIMED_BLOCK("push_all_nodes") {
        DynArray(TB_Node*) stack = dyn_array_create(TB_Node*, 1024);
        push_all_bb(ws, &stack, root);
        dyn_array_destroy(stack);
    }
}

static void cool_print_type(TB_Node* n) {
    TB_DataType dt = n->dt;
    if (n->type != TB_START && n->type != TB_REGION && !(n->type == TB_BRANCH && n->input_count == 1)) {
        if (n->type == TB_STORE) {
            dt = n->inputs[3]->dt;
        } else if (n->type == TB_BRANCH) {
            dt = n->inputs[1]->dt;
        } else if (n->type == TB_END) {
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
        printf("%"PRId64, num->value);
    } else if (n->type == TB_SYMBOL) {
        TB_Symbol* sym = TB_NODE_GET_EXTRA_T(n, TB_NodeSymbol)->sym;
        if (sym->name[0]) {
            printf("%s", sym->name);
        } else {
            printf("sym%p", sym);
        }
    } else if (depth >= 1) {
        printf("(v%zu: %s", n->gvn, tb_node_get_name(n));
        cool_print_type(n);
        printf(" ...)");
    } else {
        depth -= (n->type == TB_PROJ);

        printf("(%s", tb_node_get_name(n));
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
static TB_Node* idealize(TB_Passes* restrict p, TB_Function* f, TB_Node* n, TB_PeepholeFlags flags) {
    switch (n->type) {
        case TB_NOT:
        case TB_NEG:
        return ideal_int_unary(p, f, n);

        // integer ops
        case TB_AND:
        case TB_OR:
        case TB_XOR:
        case TB_ADD:
        case TB_SUB:
        case TB_MUL:
        case TB_SHL:
        case TB_SHR:
        case TB_SAR:
        case TB_CMP_EQ:
        case TB_CMP_NE:
        case TB_CMP_SLT:
        case TB_CMP_SLE:
        case TB_CMP_ULT:
        case TB_CMP_ULE:
        return ideal_int_binop(p, f, n);

        // pointer
        case TB_ARRAY_ACCESS:
        return ideal_array_ptr(p, f, n);

        // memory
        case TB_LOAD:
        return (flags & TB_PEEPHOLE_MEMORY) ? ideal_load(p, f, n) : NULL;

        case TB_STORE:
        return (flags & TB_PEEPHOLE_MEMORY) ? ideal_store(p, f, n) : NULL;

        case TB_END:
        return (flags & TB_PEEPHOLE_MEMORY) ? ideal_end(p, f, n) : NULL;

        case TB_MEMCPY:
        return ideal_memcpy(p, f, n);

        // division
        case TB_SDIV:
        case TB_UDIV:
        return ideal_int_div(p, f, n);

        // casting
        case TB_SIGN_EXT:
        case TB_ZERO_EXT:
        return ideal_extension(p, f, n);

        case TB_INT2PTR:
        return ideal_int2ptr(p, f, n);

        // truncate
        case TB_TRUNCATE:
        return ideal_truncate(p, f, n);

        case TB_CALL:
        return ideal_libcall(p, f, n);

        case TB_SELECT:
        return ideal_select(p, f, n);

        // control flow
        case TB_PHI:
        return (flags & TB_PEEPHOLE_PHI) ? ideal_phi(p, f, n) : NULL;

        case TB_REGION:
        return ideal_region(p, f, n);

        case TB_BRANCH:
        return ideal_branch(p, f, n);

        default:
        return NULL;
    }
}

// May return one of the inputs, this is used
static TB_Node* identity(TB_Passes* restrict p, TB_Function* f, TB_Node* n, TB_PeepholeFlags flags) {
    switch (n->type) {
        // integer ops
        case TB_AND:
        case TB_OR:
        case TB_XOR:
        case TB_ADD:
        case TB_SUB:
        case TB_MUL:
        case TB_SHL:
        case TB_SHR:
        case TB_SAR:
        case TB_CMP_EQ:
        case TB_CMP_NE:
        case TB_CMP_SLT:
        case TB_CMP_SLE:
        case TB_CMP_ULT:
        case TB_CMP_ULE:
        return identity_int_binop(p, f, n);

        case TB_SIGN_EXT:
        case TB_ZERO_EXT:
        return identity_extension(p, f, n);

        case TB_MEMBER_ACCESS:
        if (TB_NODE_GET_EXTRA_T(n, TB_NodeMember)->offset == 0) {
            return n->inputs[1];
        }
        return n;

        case TB_LOAD:
        return (flags & TB_PEEPHOLE_MEMORY) ? identity_load(p, f, n) : n;

        // dumb phis
        case TB_PHI: if (flags & TB_PEEPHOLE_PHI) {
            TB_Node* same = NULL;
            FOREACH_N(i, 1, n->input_count) {
                if (n->inputs[i] == n) continue;
                if (same && same != n->inputs[i]) return n;
                same = n->inputs[i];
            }

            assert(same);
            if (same->dt.type == TB_MEMORY) {
                TB_NodeRegion* r = TB_NODE_GET_EXTRA(n->inputs[0]);
                if (r->mem_in == n)  r->mem_in = NULL;
                if (r->mem_out == n) r->mem_out = NULL;
            }

            tb_pass_mark_users(p, n->inputs[0]);
            return same;
        } else {
            return n;
        }

        default:
        return n;
    }
}

static bool is_terminator(TB_Node* n) {
    return n->type == TB_BRANCH || n->type == TB_END || n->type == TB_TRAP || n->type == TB_UNREACHABLE;
}

static TB_Node* unsafe_get_region(TB_Node* n) {
    while (n->type != TB_REGION && n->type != TB_START) {
        n = n->inputs[0];
    }

    return n;
}

static bool peephole(TB_Passes* restrict p, TB_Function* f, TB_Node* n, TB_PeepholeFlags flags) {
    // must've dead sometime between getting scheduled and getting
    // here.
    if (n->type != TB_END && find_users(p, n) == NULL) {
        return false;
    }

    DO_IF(TB_OPTDEBUG_STATS)(p->stats.peeps++);
    DO_IF(TB_OPTDEBUG_PEEP)(printf("peep v%zu? ", n->gvn), print_node_sexpr(n, 0));

    // idealize node (in a loop of course)
    TB_Node* k = idealize(p, f, n, flags);
    DO_IF(TB_OPTDEBUG_PEEP)(int loop_count=0);
    while (k != NULL) {
        DO_IF(TB_OPTDEBUG_STATS)(p->stats.rewrites++);
        DO_IF(TB_OPTDEBUG_PEEP)(printf(" => \x1b[32m"), print_node_sexpr(k, 0), printf("\x1b[0m"));

        // only the n users actually changed
        tb_pass_mark_users(p, n);

        // transfer users from n -> k
        if (n != k) {
            tb_assert(!is_terminator(n), "can't peephole a branch into a new branch");
            subsume_node(p, f, n, k);
            n = k;
        }

        // try again, maybe we get another transformation
        k = idealize(p, f, n, flags);
        DO_IF(TB_OPTDEBUG_PEEP)(if (++loop_count > 10) { log_warn("%p: we looping a lil too much dawg...", n); });
    }

    // convert into matching identity
    k = identity(p, f, n, flags);
    if (n != k) {
        DO_IF(TB_OPTDEBUG_STATS)(p->stats.identities++);
        DO_IF(TB_OPTDEBUG_PEEP)(printf(" => \x1b[33m"), print_node_sexpr(k, 0), printf("\x1b[0m"));

        tb_pass_mark_users(p, n);
        subsume_node(p, f, n, k);
        return k;
    }

    // common subexpression elim
    k = nl_hashset_put2(&p->cse_nodes, n, cse_hash, cse_compare);
    if (k && (k != n)) {
        DO_IF(TB_OPTDEBUG_STATS)(p->stats.cse_hit++);
        DO_IF(TB_OPTDEBUG_PEEP)(printf(" => \x1b[31mCSE\x1b[0m"));

        subsume_node(p, f, n, k);

        // because certain optimizations apply when things are merged
        // we mark ALL users including the ones who didn't get changed.
        tb_pass_mark_users(p, k);
        return k;
    } else {
        DO_IF(TB_OPTDEBUG_STATS)(p->stats.cse_miss++);
    }

    return n;
}

static void subsume_node(TB_Passes* restrict p, TB_Function* f, TB_Node* n, TB_Node* new_n) {
    User* use = find_users(p, n);
    while (use != NULL) {
        tb_assert(use->n->inputs[use->slot] == n, "Mismatch between def-use and use-def data");

        // set_input will delete 'use' so we can't use it afterwards
        TB_Node* use_n = use->n;
        User* next = use->next;

        set_input(p, use->n, new_n, use->slot);
        use = next;
    }

    tb_pass_kill_node(p, n);
}

static void generate_use_lists(TB_Passes* restrict p, TB_Function* f) {
    dyn_array_for(i, p->worklist.items) {
        TB_Node* n = p->worklist.items[i];

        if (n->type == TB_LOCAL) {
            // we don't need to check for duplicates here, the worklist is uniques
            dyn_array_put(p->locals, n);
        }

        FOREACH_N(i, 0, n->input_count) if (n->inputs[i]) {
            add_user(p, n, n->inputs[i], i, NULL);
        }
    }
}

TB_Passes* tb_pass_enter(TB_Function* f, TB_Arena* arena) {
    assert(f->stop_node && "missing return");
    TB_Passes* p = tb_platform_heap_alloc(sizeof(TB_Passes));
    *p = (TB_Passes){ .f = f };

    TB_Arena* old_arena = f->arena;
    f->line_attrib.loc.file = NULL;
    f->arena = arena;

    verify_tmp_arena(p);

    worklist_alloc(&p->worklist, f->node_count);

    // generate early doms
    CUIK_TIMED_BLOCK("doms") {
        size_t block_count = tb_push_postorder(f, &p->worklist);
        tb_compute_dominators(f, block_count, p->worklist.items);
        worklist_clear(&p->worklist);
    }

    // generate work list (put everything)
    CUIK_TIMED_BLOCK("gen worklist") {
        push_all_nodes(&p->worklist, f->start_node);
        DO_IF(TB_OPTDEBUG_STATS)(p->stats.initial = worklist_popcount(&p->worklist));
    }

    DO_IF(TB_OPTDEBUG_PEEP)(log_debug("%s: starting passes with %d nodes", f->super.name, f->node_count));

    // find all outgoing edges
    CUIK_TIMED_BLOCK("gen users") {
        generate_use_lists(p, f);
    }

    return p;
}

void tb_pass_optimize(TB_Passes* p) {
    tb_pass_peephole(p, TB_PEEPHOLE_ALL);
    tb_pass_sroa(p);
    tb_pass_peephole(p, TB_PEEPHOLE_ALL);
    tb_pass_mem2reg(p);
    tb_pass_peephole(p, TB_PEEPHOLE_ALL);
}

void tb_pass_peephole(TB_Passes* p, TB_PeepholeFlags flags) {
    verify_tmp_arena(p);

    if (p->cse_nodes.data == NULL) {
        p->cse_nodes = nl_hashset_alloc(p->f->node_count);
    }

    TB_Function* f = p->f;
    CUIK_TIMED_BLOCK("peephole") {
        TB_Node* n;
        while ((n = worklist_pop(&p->worklist))) {
            if (peephole(p, f, n, flags)) {
                DO_IF(TB_OPTDEBUG_PEEP)(printf("\n"));
            }
        }
    }
}

void tb_pass_exit(TB_Passes* p) {
    TB_Function* f = p->f;

    #if TB_OPTDEBUG_STATS
    push_all_nodes(&p->worklist, f->start_node);
    int final_count = worklist_popcount(&p->worklist);

    double factor = ((double) final_count / (double) p->stats.initial) * 100.0;

    printf("%s: stats:\n", f->super.name);
    printf("  %4d   -> %4d nodes (%.2f%%)\n", p->stats.initial, final_count, factor);
    printf("  %4d CSE hit    %4d CSE miss\n", p->stats.cse_hit, p->stats.cse_miss);
    printf("  %4d peepholes  %4d rewrites    %4d identities\n", p->stats.peeps, p->stats.rewrites, p->stats.identities);
    #endif

    verify_tmp_arena(p);

    worklist_free(&p->worklist);
    nl_hashset_free(p->cse_nodes);
    dyn_array_destroy(p->locals);

    tb_arena_clear(tmp_arena);
}
