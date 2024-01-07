// This is the general codegen framework, it combines together the various components in
// the target dependent part, if you see any of these TODOs and wanna contribute, those
// are good places to go.
//
// This can be broken down into a few steps:
//
//   GCM => Init => Per BB: { Local sched, Tiling } => Dataflow => Regalloc => Emit
//
// Here's where you can find more info on these:
//
//   * GCM: handled by the tb_pass_schedule (TODO ways to skew global scheduling).
//
//   * Init: This is where you can process ABI details and fill in certain important details
//     for later like ctx.sched which is the scheduler, and ctx.regalloc which is the register
//     allocator.
//
//   * Local sched: handled by ctx.sched, it's just a function you can replace with your
//     own based on whatever you wanna achieve, for now i've only got the topo sort greedy
//     sched (TODO implement a latency-based list scheduler)
//
//   * Tiling: handled by isel_node, by default we walk the BB backwards building one tile
//     per node, we can call fold_node to say we expended a use of a value, this way if we
//     we've folded all uses of a value we don't materialize it alone. isel_node has to fill
//     in the register constraints, it'll return the output mask and construct input masks.
//
//   * Regalloc: handled by ctx.regalloc (TODO implement more register allocators)
//
//   * Emit: handled by emit_tile, writes out the bytes now that we've completed regalloc.
//
#pragma once

#include "opt/passes.h"
#include "emitter.h"

enum {
    // all we can fit into 3bits, but also... 8 classes is a lot.
    //
    // * x86 has 3 currently: GPR, Vector, and Stack.
    MAX_REG_CLASSES = 8,

    // every platform has a stack regclass, the mask is actually an offset
    // on the stack (useful for parameter passing).
    REG_CLASS_STK = 0,
};

// represents a set of registers, usually for register constraints.
// we can also say that a value might fit into the stack with may_spill.
typedef struct {
    uint64_t class     : 3;
    uint64_t may_spill : 1;
    uint64_t mask      : 60;
} RegMask;

#define REGMASK(c, m)  (RegMask){ REG_CLASS_ ## c, 0, m }
#define REGMASK2(c, m) (RegMask){ REG_CLASS_ ## c, 1, m }
#define REGEMPTY (RegMask){ 0 }

typedef struct {
    int start, end;
} LiveRange;

typedef struct {
    int may_spill : 1;
    int pos       : 31;
} UsePos;

typedef enum {
    // single node tile
    TILE_NORMAL,
    // SoN doesn't have a jump op, this serves that purpose
    TILE_GOTO,
    // performs a move operation between two live intervals
    TILE_SPILL_MOVE,
    // debug line
    TILE_LOCATION,
} TileTag;

typedef struct Tile Tile;
typedef struct LiveInterval LiveInterval;

typedef struct {
    LiveInterval* src;
    RegMask mask;
} TileInput;

struct LiveInterval {
    int id; // used by live sets
    int active_range;

    RegMask mask;
    TB_DataType dt;

    Tile* tile;

    int8_t class;
    int8_t reg;
    int8_t assigned;

    LiveInterval* hint;
    LiveInterval* split_kid;

    int use_cap, use_count;
    UsePos* uses;

    int range_cap, range_count;
    LiveRange* ranges;
};

// represents a pattern of instructions
struct Tile {
    struct Tile* prev;
    struct Tile* next;

    TileTag tag : 8;
    uint32_t flags : 24;

    int time;

    TB_Node* n;
    LiveInterval* interval;

    union {
        // tag = TILE_GOTO, this is the successor
        TB_Node* succ;

        // tag = TILE_SPILL_MOVE
        TB_DataType spill_dt;

        // tag = TILE_NORMAL
        void* aux;

        // tag = TILE_LOCATION
        TB_NodeLocation* loc;
    };

    int in_count;
    TileInput* ins;
};

typedef struct {
    int id;

    TB_Node* n;
    TB_Node* end_n;

    Tile* start;
    Tile* end;

    // dataflow
    Set gen, kill;
    Set live_in, live_out;
} MachineBB;

typedef struct {
    TB_SymbolPatch* patch;
    TB_Location* loc;
    TB_Location* end;
    Comment* comment;
} Disasm;

// Static-sized hash map
typedef struct {
    TB_Node* k;
    MachineBB* v;
} NodeToBB;

typedef struct {
    // number of times we asked for it
    // to materialize, only certain nodes
    // can be rematerialized several times
    // like constants.
    int mat_count;
    int use_count;
    Tile* tile;
} ValueDesc;

typedef struct Ctx Ctx;
typedef void (*TB_RegAlloc)(Ctx* restrict ctx, TB_Arena* arena);
typedef bool (*TB_2Addr)(TB_Node* n);

typedef struct {
    uint32_t* pos;
    uint32_t target;
} JumpTablePatch;

typedef struct {
    int class, reg;
    int stk;
} CalleeSpill;

struct Ctx {
    TB_Passes* p;
    TB_CGEmitter emit;

    TB_Module* module;
    TB_Function* f;
    TB_FeatureSet features;

    // user-provided details
    TB_Scheduler sched;
    TB_RegAlloc regalloc;
    TB_2Addr _2addr;

    // target-dependent index
    int abi_index;
    int fallthrough;

    uint8_t prologue_length;
    uint8_t epilogue_length;
    uint8_t nop_pads;

    // TB_Node* -> MachineBB*
    struct {
        size_t exp;
        NodeToBB* entries;
    } node_to_bb;

    // Basic blocks
    int bb_count;
    MachineBB* machine_bbs;

    // Values
    ValueDesc* values; // [n.gvn]
    LiveInterval** id2interval; // [tile.id]
    LiveInterval* fixed[MAX_REG_CLASSES];

    // Regalloc
    int interval_count;
    int stack_header;
    int stack_usage;
    int num_fixed;
    int num_classes;
    int num_regs[MAX_REG_CLASSES];
    uint64_t callee_saved[MAX_REG_CLASSES];

    // where scratch registers can go, a mask is used to avoid
    // allocating special regiters.
    RegMask normie_mask[MAX_REG_CLASSES];

    DynArray(CalleeSpill) callee_spills;
    NL_Map(TB_Node*, int) stack_slots;
    DynArray(TB_StackSlot) debug_stack_slots;
    DynArray(JumpTablePatch) jump_table_patches;

    // Line info
    MachineBB* current_emit_bb;
    int current_emit_bb_pos;

    DynArray(TB_Location) locations;
};

void tb__lsra(Ctx* restrict ctx, TB_Arena* arena);
void tb__chaitin(Ctx* restrict ctx, TB_Arena* arena);

void tb__print_regmask(RegMask mask);

static int fixed_reg_mask(RegMask mask) {
    if (mask.class == REG_CLASS_STK) {
        return mask.mask;
    } else {
        return tb_popcount64(mask.mask) == 1 ? 63 - tb_clz64(mask.mask) : -1;
    }
}

static uint32_t node_to_bb_hash(void* ptr) { return (((uintptr_t) ptr) * 11400714819323198485ull) >> 32ull; }
static MachineBB* node_to_bb(Ctx* restrict ctx, TB_Node* n) {
    uint32_t h = node_to_bb_hash(n);

    size_t mask = (1 << ctx->node_to_bb.exp) - 1;
    size_t first = h & mask, i = first;
    do {
        if (ctx->node_to_bb.entries[i].k == n) {
            return ctx->node_to_bb.entries[i].v;
        }

        i = (i + 1) & mask;
    } while (i != first);

    abort();
}

static void node_to_bb_put(Ctx* restrict ctx, TB_Node* n, MachineBB* bb) {
    uint32_t h = node_to_bb_hash(n);

    size_t mask = (1 << ctx->node_to_bb.exp) - 1;
    size_t first = h & mask, i = first;
    do {
        if (ctx->node_to_bb.entries[i].k == NULL) {
            ctx->node_to_bb.entries[i].k = n;
            ctx->node_to_bb.entries[i].v = bb;
            return;
        }

        i = (i + 1) & mask;
    } while (i != first);

    abort();
}

