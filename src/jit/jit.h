
#pragma once

#if !defined(MX_HEADER_JIT)
#define MX_HEADER_JIT

#include "x64.h"
#include "../run/hop.h"
#include "../run/run.h"

#if defined(__x86_64__) && defined(__linux__)
#define MX_JIT_SUPPORTED 1
#else
#define MX_JIT_SUPPORTED 0
#endif

#if !defined(MX_JIT_THRESHOLD_DEFAULT)
#define MX_JIT_THRESHOLD_DEFAULT 10
#endif

#define MX_JIT_REGION_BYTES ((size_t)1 << 28)
#define MX_JIT_COLD_OFFSET ((size_t)1 << 27)
#define MX_JIT_MARGIN 65536
#define MX_JIT_HOP_ROOM 8192
#define MX_JIT_NONE 0xff
#define MX_JIT_NGPR 10
#define MX_JIT_NXMM 14
#define MX_JIT_XMM_SCRATCH 15
#define MX_JIT_ENTRY_WORDS 3
#define MX_JIT_STITCH_MAX 32
#define MX_JIT_RECOMPILE_MAX 4
#define MX_JIT_INLINE_DEPTH 3
#define MX_JIT_INLINE_MAX 8
#define MX_JIT_INLINE_BLOCKS 16
#define MX_JIT_MISS_RECOMPILE 256
#define MX_JIT_WANT_MAX 6
#define MX_JIT_EDGE_SIDE ((uint32_t)1 << 30)
#define MX_JIT_KEY_BLOCK (((uintptr_t)1 << 48) - 1)
#define MX_JIT_NHOME (MX_JIT_NGPR + MX_JIT_NXMM)
#define MX_JIT_XMM_ID 16
#define MX_JIT_NPHYS (MX_JIT_XMM_ID + MX_JIT_NXMM)
#define MX_JIT_FRAME_BYTES (8 + (int32_t)(sizeof(mx_obj_t) * MX_RUN_SMALL))

typedef uint8_t mx_jit_slot_t;
enum {
    MX_JIT_SLOT_HOP,
    MX_JIT_SLOT_CMP,
    MX_JIT_SLOT_SET,
    MX_JIT_SLOT_GET_SK,
    MX_JIT_SLOT_GET_IR,
    MX_JIT_SLOT_GET_IK,
    MX_JIT_SLOT_GET_VAR,
    MX_JIT_SLOT_GET_METHOD,
    MX_JIT_SLOT_GET_UPVAL,
    MX_JIT_SLOT_CALL_FAST,
    MX_JIT_SLOT_TABLE_NEW,
    MX_JIT_SLOT_CONCAT,
    MX_JIT_SLOT_CLOSURE,
    MX_JIT_SLOT_CLOSE,
    MX_JIT_SLOT_EQ_STR,
    MX_JIT_SLOT_LINK,
    MX_JIT_SLOT_DELIVER,
    MX_JIT_SLOT_FFI,
    MX_JIT_SLOT_FFI_AFTER,
    MX_JIT_SLOT_RETURN,
    MX_JIT_SLOT_CALL_LUA,
    MX_JIT_SLOT_TAILCALL_LUA,
    MX_JIT_SLOT_META_CMP,
    MX_JIT_SLOT_CELL,
    MX_JIT_SLOT_UNPACK,
    MX_JIT_SLOT_PROBE,
    MX_JIT_SLOT_CHAIN,
    MX_JIT_SLOT_RET_WAYS,
    MX_JIT_SLOT_DELIVER2,
    MX_JIT_SLOT_DELIVER3,
    MX_JIT_SLOT_NEXT,
    MX_JIT_SLOT_LOOKUP,
    MX_JIT_SLOT_LOOKUP_REF,
    MX_JIT_SLOT_T_FFI,
    MX_JIT_SLOT_T_FFI_AFTER,
    MX_JIT_SLOT_T_CALL_LUA,
    MX_JIT_SLOT_T_TAILCALL_LUA,
    MX_JIT_SLOT_T_RETURN,
    MX_JIT_SLOT_T_GET_SK,
    MX_JIT_SLOT_T_GET_IR,
    MX_JIT_SLOT_T_GET_IK,
    MX_JIT_SLOT_T_GET_VAR,
    MX_JIT_SLOT_T_GET_METHOD,
    MX_JIT_SLOT_T_GET_UPVAL,
    MX_JIT_SLOT_COUNT,
};

typedef uint8_t mx_jit_form_t;
enum {
    MX_JIT_FORM_RR,
    MX_JIT_FORM_RI,
    MX_JIT_FORM_IR,
};

typedef uint8_t mx_jit_plan_t;
enum {
    MX_JIT_PLAN_NONE,
    MX_JIT_PLAN_TAKEN,
    MX_JIT_PLAN_OTHER,
};

typedef struct mx_jit_cache_t mx_jit_cache_t;
typedef struct mx_jit_cc_t mx_jit_cc_t;
typedef struct mx_jit_opnd_t mx_jit_opnd_t;
typedef struct mx_jit_home_t mx_jit_home_t;
typedef struct mx_jit_move_t mx_jit_move_t;
typedef struct mx_jit_inline_t mx_jit_inline_t;
typedef struct mx_jit_way_t mx_jit_way_t;
typedef struct mx_jit_miss_t mx_jit_miss_t;

struct mx_jit_way_t {
    uintptr_t *code;
    mx_kind_t kind;
};

struct mx_jit_miss_t {
    uint8_t *start;
    uint8_t *found;
};

struct mx_jit_cache_t {
    uint64_t bits;
    uint32_t reg;
    uint32_t age;
    bool is_used: 1;
    bool is_dirty: 1;
    bool is_pinned: 1;
    bool is_const: 1;
};

struct mx_jit_home_t {
    uint64_t bits;
    uint32_t reg;
    uint8_t phys;
    bool is_dirty: 1;
    bool is_const: 1;
};

struct mx_jit_move_t {
    uint8_t src;
    uint8_t dst;
};

struct mx_jit_inline_t {
    const mx_resume_t *rec;
    mx_version_t *rb;
    uintptr_t *code;
    uint8_t *tail;
    uint8_t *link_tail;
    uint32_t k;
    uint32_t nregs;
    uint32_t frame;
    uint32_t chain;
};

struct mx_jit_label_t {
    uintptr_t *code;
    uint8_t *at;
    uint32_t chain;
    uint32_t nhome;
    mx_jit_home_t homes[MX_JIT_NHOME];
};

struct mx_jit_pend_t {
    uintptr_t *code;
    uintptr_t *from;
    uint8_t *site;
    mx_version_t *rb;
    uint8_t *tail;
    uint8_t *link_tail;
    uint32_t off;
    uint32_t nregs;
    uint32_t frame;
    uint32_t chain;
    uint32_t ninline;
    uint32_t nhome;
    uint32_t val_out;
    mx_kind_t val_kind;
    bool has_val: 1;
    mx_jit_inline_t stack[MX_JIT_INLINE_DEPTH];
    mx_jit_home_t homes[MX_JIT_NHOME];
};

struct mx_jit_cc_t {
    mx_t *vm;
    mx_version_t *rb;
    uintptr_t *code;
    uintptr_t *first;
    uintptr_t *ip;
    uint8_t *entry;
    uint8_t *tail;
    uint8_t *link_tail;
    uint8_t *hop_exit;
    uint8_t *slow_join;
    uint8_t *first_tail;
    uint8_t *head_label;
    mx_jit_label_t *labels;
    mx_jit_pend_t *pend;
    mx_x64_t hot;
    mx_x64_t cold;
    uint32_t nregs;
    uint32_t frame;
    uint32_t age;
    uint32_t nhead;
    uint32_t nsnap;
    uint32_t nlabels;
    uint32_t npend;
    uint32_t pend_at;
    uint32_t off;
    uint32_t chain;
    uint32_t next_chain;
    uint32_t ninline;
    uint32_t inlined;
    mx_jit_cache_t g[16];
    mx_jit_cache_t x[MX_JIT_NXMM];
    mx_jit_home_t head[MX_JIT_NHOME];
    mx_jit_home_t snap[MX_JIT_NHOME];
    mx_jit_inline_t stack[MX_JIT_INLINE_DEPTH];
};

struct mx_jit_opnd_t {
    uint64_t value;
    bool is_imm: 1;
};

extern const mx_x64_reg_t mx_jit_gprs[MX_JIT_NGPR];

static inline uint32_t mx_jit_key_len(uintptr_t key) {
    return (uint32_t)(key >> 48) & 0x3fff;
}

void mx_jit_init(mx_t *vm);
void mx_jit_free(mx_t *vm);
bool mx_jit_compile(mx_t *vm, mx_version_t *rb);

void mx_jit_rt_hop(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_hop_t hop);
bool mx_jit_rt_cmp(mx_t *vm, mx_val_t *base, const uintptr_t *ip, mx_hop_t hop);
bool mx_jit_rt_set(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_hop_t hop);
uintptr_t *mx_jit_rt_found(mx_t *vm, mx_val_t *base, const mx_resume_t *rec, mx_run_found_t got, mx_version_t *first);
uintptr_t *mx_jit_rt_get_sk(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_version_t *first);
uintptr_t *mx_jit_rt_get_ir(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_version_t *first);
uintptr_t *mx_jit_rt_get_ik(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_version_t *first);
uintptr_t *mx_jit_rt_get_var(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_version_t *first);
uintptr_t *mx_jit_rt_get_method(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_version_t *first);
uintptr_t *mx_jit_rt_get_upval(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_version_t *first);
void mx_jit_rt_table_new(mx_t *vm, mx_val_t *base, uintptr_t *ip);
void mx_jit_rt_concat(mx_t *vm, mx_val_t *base, uintptr_t *ip);
void mx_jit_rt_closure(mx_t *vm, mx_val_t *base, uintptr_t *ip);
void mx_jit_rt_close(mx_t *vm, mx_val_t *base, uintptr_t *ip);
bool mx_jit_rt_eq_str(mx_t *vm, mx_val_t *base, const uintptr_t *ip);
uintptr_t *mx_jit_rt_call_fast(mx_t *vm, mx_val_t *base, uintptr_t *ip);
uintptr_t *mx_jit_rt_multi(mx_t *vm, const mx_resume_t *rec, const mx_obj_t *vals, uint32_t n);
uintptr_t *mx_jit_rt_deliver(mx_t *vm, mx_val_t *base, const mx_resume_t *rec, const mx_obj_t *vals);
uintptr_t *mx_jit_rt_deliver_n(mx_t *vm, mx_val_t *base, const mx_resume_t *rec, const mx_obj_t *vals, uint32_t got, bool is_resumed);
mx_jit_exit_t mx_jit_rt_ffi(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_version_t *rb);
mx_jit_exit_t mx_jit_rt_ffi_after(mx_t *vm, uint32_t boff, uintptr_t *ip, mx_version_t *rb);
mx_jit_exit_t mx_jit_rt_return(mx_t *vm, mx_val_t *base, const uintptr_t *ip, mx_hop_t hop);
mx_jit_exit_t mx_jit_rt_call_lua(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_version_t *rb);
mx_jit_exit_t mx_jit_rt_tailcall_lua(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_version_t *rb);
uintptr_t *mx_jit_rt_meta_cmp(mx_t *vm, mx_val_t *base, uintptr_t *ip);
uintptr_t *mx_jit_rt_cell(mx_t *vm, mx_val_t *base, uintptr_t *ip);
bool mx_jit_rt_unpack(mx_t *vm, mx_val_t *base, uintptr_t *ip);
uintptr_t *mx_jit_rt_link(mx_t *vm, uintptr_t *word, uint32_t site, uint32_t info, mx_version_t *rb, mx_version_t *first);

mx_jit_bit_t mx_jit_probe(const void *zeroed, size_t size);
mx_jit_layout_t mx_jit_layout(void);
void mx_jit_epilogue(mx_x64_t *a);
void mx_jit_stubs(mx_t *vm);
void mx_jit_hop_map(mx_t *vm);
mx_hop_t mx_jit_hop_of(const mx_t *vm, uintptr_t word);

int32_t mx_jit_disp(uint32_t reg);
int32_t mx_jit_home_disp(const mx_jit_cc_t *cc, uint32_t abs);
bool mx_jit_is_caller_saved(mx_x64_reg_t r);
uint8_t mx_jit_find(const mx_jit_cache_t *bank, uint32_t n, uint32_t reg);
uint8_t mx_jit_find_g(const mx_jit_cc_t *cc, uint32_t reg);
uint8_t mx_jit_find_x(const mx_jit_cc_t *cc, uint32_t reg);
bool mx_jit_is_cached(const mx_jit_cc_t *cc, uint32_t reg);
void mx_jit_set(mx_jit_cc_t *cc, mx_jit_cache_t *c, uint32_t reg, bool is_dirty);
mx_x64_reg_t mx_jit_alloc_g(mx_jit_cc_t *cc);
uint32_t mx_jit_alloc_x(mx_jit_cc_t *cc);
void mx_jit_drop(mx_jit_cc_t *cc, uint32_t reg);
void mx_jit_drop_all(mx_jit_cc_t *cc);
void mx_jit_drop_caller_saved(mx_jit_cc_t *cc);
void mx_jit_unpin(mx_jit_cc_t *cc);
mx_x64_reg_t mx_jit_use_g(mx_jit_cc_t *cc, uint32_t reg);
uint32_t mx_jit_use_x(mx_jit_cc_t *cc, uint32_t reg);
mx_x64_reg_t mx_jit_def_g(mx_jit_cc_t *cc, uint32_t reg);
uint32_t mx_jit_def_x(mx_jit_cc_t *cc, uint32_t reg);
void mx_jit_spill(const mx_jit_cc_t *cc, mx_x64_t *a);
void mx_jit_flush(mx_jit_cc_t *cc);
bool mx_jit_keep(const mx_jit_cc_t *cc, uint32_t reg, const uint64_t *live0, const uint64_t *live1);
void mx_jit_flush_live(mx_jit_cc_t *cc, mx_x64_t *a, uint32_t skip, const uint64_t *live0, const uint64_t *live1);
bool mx_jit_can_stitch(const mx_jit_cc_t *cc, const uintptr_t *code);
void mx_jit_tails(mx_jit_cc_t *cc);
uint32_t mx_jit_stitch(mx_jit_cc_t *cc, uintptr_t *code);
uint32_t mx_jit_continue(mx_jit_cc_t *cc, uintptr_t *code);
const mx_jit_label_t *mx_jit_label_of(const mx_jit_cc_t *cc, const uintptr_t *code);
uint32_t mx_jit_capture(const mx_jit_cc_t *cc, mx_jit_home_t *homes);
void mx_jit_restore(mx_jit_cc_t *cc, const mx_jit_home_t *homes, uint32_t n);
void mx_jit_snapshot(mx_jit_cc_t *cc);
void mx_jit_move_phys(mx_jit_cc_t *cc, uint8_t dst, uint8_t src);
void mx_jit_parallel_moves(mx_jit_cc_t *cc, mx_jit_move_t *moves, uint32_t n);
void mx_jit_merge(mx_jit_cc_t *cc, const mx_jit_home_t *homes, uint32_t n, const uint64_t *live);
void mx_jit_back_edge(mx_jit_cc_t *cc);
void mx_jit_push_pend(mx_jit_cc_t *cc, uintptr_t *code, uint8_t *site);
void mx_jit_kind_ways(mx_jit_cc_t *cc, const mx_resume_t *rec, uint32_t out, mx_kind_t hot, mx_x64_reg_t src, int32_t kind_at, int32_t val_at, uint8_t *stub, uintptr_t flags);
uint32_t mx_jit_resume(mx_jit_cc_t *cc);
void mx_jit_unit(mx_jit_cc_t *cc);
void mx_jit_recompile(mx_t *vm, mx_version_t *rb);
const uint64_t *mx_jit_live(mx_jit_cc_t *cc, mx_ir_block_t *block);

void mx_jit_exit_to(mx_jit_cc_t *cc, mx_x64_t *a);
uint8_t *mx_jit_exit_stub(mx_jit_cc_t *cc);
void mx_jit_stop(mx_jit_cc_t *cc);
void mx_jit_call_rt(mx_jit_cc_t *cc, mx_x64_t *a, mx_jit_slot_t slot, mx_hop_t hop);
uint8_t *mx_jit_slow_stub(mx_jit_cc_t *cc, mx_hop_t hop);
void mx_jit_slow_join(mx_jit_cc_t *cc);
uint8_t *mx_jit_get_stub(mx_jit_cc_t *cc, mx_hop_t hop);
mx_jit_slot_t mx_jit_get_slot(mx_hop_t hop);
mx_jit_slot_t mx_jit_hop_slot(mx_hop_t hop);
void mx_jit_hop_call(mx_jit_cc_t *cc, mx_hop_t hop);
uint8_t *mx_jit_entry_of(const mx_jit_cc_t *cc, const uintptr_t *code, uint32_t info);
void mx_jit_edge_stub(mx_jit_cc_t *cc, uint8_t *site, uintptr_t *word, uint32_t info);
void mx_jit_edge(mx_jit_cc_t *cc, mx_x64_cc_t cond, uintptr_t *word, uint32_t info);
void mx_jit_edge_jmp(mx_jit_cc_t *cc, uintptr_t *word, uint32_t info);

uint8_t *mx_jit_label(mx_x64_t *a, mx_x64_cc_t cond);
uint8_t *mx_jit_label_jmp(mx_x64_t *a);
bool mx_jit_fits32(uint64_t imm);
uintptr_t *mx_jit_cell(mx_jit_cc_t *cc, const uintptr_t *code);
const uint8_t *mx_jit_pool(mx_jit_cc_t *cc, uint64_t bits);
void mx_jit_load_lit_x(mx_jit_cc_t *cc, uint32_t x, uint64_t bits);
void mx_jit_load_reg(mx_jit_cc_t *cc, mx_x64_reg_t d, uint32_t reg);
void mx_jit_load_arg(mx_jit_cc_t *cc, mx_x64_reg_t d, const uintptr_t *arg);
void mx_jit_store_arg(mx_jit_cc_t *cc, mx_x64_reg_t base, int32_t disp, const uintptr_t *arg, mx_x64_reg_t scratch);
void mx_jit_move_r(mx_jit_cc_t *cc, uint32_t out, uint32_t a);
void mx_jit_move_i(mx_jit_cc_t *cc, uint32_t out, uint64_t bits);
mx_x64_reg_t mx_jit_target_g(mx_jit_cc_t *cc, uint32_t out, uint32_t a, mx_x64_reg_t ra);
uint32_t mx_jit_target_x(mx_jit_cc_t *cc, uint32_t out, uint32_t a, uint32_t ra);
void mx_jit_alu(mx_jit_cc_t *cc, mx_x64_alu_t op, uint32_t out, uint32_t a, uint32_t b);
void mx_jit_alu_i(mx_jit_cc_t *cc, mx_x64_alu_t op, uint32_t out, uint32_t a, uint64_t imm);
void mx_jit_sub_ir(mx_jit_cc_t *cc, uint32_t out, uint32_t a, uint64_t imm);
void mx_jit_mul(mx_jit_cc_t *cc, uint32_t out, uint32_t a, uint32_t b);
void mx_jit_mul_i(mx_jit_cc_t *cc, uint32_t out, uint32_t a, uint64_t imm);
void mx_jit_load_opnd(mx_jit_cc_t *cc, mx_x64_reg_t d, mx_jit_opnd_t o);
void mx_jit_divmod(mx_jit_cc_t *cc, bool is_mod, uint32_t out, mx_jit_opnd_t a, mx_jit_opnd_t d);
bool mx_jit_divmod_pow2(mx_jit_cc_t *cc, bool is_mod, uint32_t out, uint32_t a, int64_t d);
void mx_jit_shift_i(mx_jit_cc_t *cc, bool is_left, uint32_t out, uint32_t a, int64_t count);
void mx_jit_sse(mx_jit_cc_t *cc, mx_x64_sse_t op, uint32_t out, uint32_t a, uint32_t b);
void mx_jit_sse_ri(mx_jit_cc_t *cc, mx_x64_sse_t op, uint32_t out, uint32_t a, uint64_t imm);
void mx_jit_sse_ir(mx_jit_cc_t *cc, mx_x64_sse_t op, uint32_t out, uint32_t a, uint64_t imm);
void mx_jit_to_f64(mx_jit_cc_t *cc, uint32_t x, uint32_t reg);
void mx_jit_neg_f64(mx_jit_cc_t *cc, uint32_t out, uint32_t a);
void mx_jit_unary_i(mx_jit_cc_t *cc, bool is_neg, uint32_t out, uint32_t a);
mx_jit_plan_t mx_jit_plan(const mx_jit_cc_t *cc);
void mx_jit_flush_plan(mx_jit_cc_t *cc, mx_jit_plan_t plan);
uint8_t *mx_jit_side(mx_jit_cc_t *cc, uintptr_t *word, mx_ir_block_t *block);
uint32_t mx_jit_edges(mx_jit_cc_t *cc, mx_jit_plan_t plan, mx_x64_cc_t taken);
uint32_t mx_jit_br_i(mx_jit_cc_t *cc, mx_x64_cc_t rel, mx_x64_cc_t swapped, mx_jit_form_t form);
uint32_t mx_jit_br_f(mx_jit_cc_t *cc, uint32_t rel, mx_jit_form_t form);
uint32_t mx_jit_br_bool(mx_jit_cc_t *cc);
uint32_t mx_jit_br_rt(mx_jit_cc_t *cc, mx_hop_t hop);
void mx_jit_len_table(mx_jit_cc_t *cc);
bool mx_jit_kind_is_ref(mx_kind_t kind);
void mx_jit_barrier(mx_jit_cc_t *cc, mx_x64_reg_t t, mx_kind_t kind, uint8_t *stub);
uint32_t mx_jit_size_for(uint64_t hint);
void mx_jit_check_slot(mx_jit_cc_t *cc, mx_x64_reg_t t, uint64_t hint, uint64_t key, uint8_t *miss);
uint32_t mx_jit_size_max(uint64_t hint, uint32_t hash);
void mx_jit_insert_key(mx_jit_cc_t *cc, mx_x64_reg_t t, uint64_t hint, uint64_t key, mx_kind_t kind, const uintptr_t *arg, uint8_t *stub);
void mx_jit_check_entries(mx_jit_cc_t *cc, mx_x64_reg_t t, uint64_t hint, uint8_t *miss);
void mx_jit_init_sk(mx_jit_cc_t *cc);
void mx_jit_init_ik(mx_jit_cc_t *cc);
void mx_jit_set_sk(mx_jit_cc_t *cc);
void mx_jit_set_int(mx_jit_cc_t *cc, mx_hop_t hop);
uint8_t *mx_jit_append(mx_jit_cc_t *cc, mx_x64_reg_t t, mx_kind_t kind, const uintptr_t *arg, uint8_t *stub);
bool mx_jit_has_hot(const uintptr_t *hot);
uint32_t mx_jit_finish_get(mx_jit_cc_t *cc, const mx_resume_t *rec, uint32_t out, uintptr_t *word, mx_kind_t kind, mx_x64_reg_t src, int32_t disp);
uint32_t mx_jit_finish_rax(mx_jit_cc_t *cc, const mx_resume_t *rec, uint32_t out, uintptr_t *word, mx_kind_t kind);
mx_jit_way_t mx_jit_value_way(const mx_resume_t *rec);
mx_jit_way_t mx_jit_any_way(const mx_resume_t *rec);
void mx_jit_upval_fetch(mx_jit_cc_t *cc, mx_x64_reg_t cl, uint32_t index);
void mx_jit_reg_ways(mx_jit_cc_t *cc, const mx_resume_t *rec, uint32_t out, mx_kind_t hot, uint8_t *stub);
void mx_jit_lua_frame(mx_jit_cc_t *cc, mx_x64_reg_t fn, uintptr_t key, uintptr_t *code, uint8_t *fail);
void mx_jit_tail_frame(mx_jit_cc_t *cc, mx_x64_reg_t fn, uintptr_t key, uintptr_t *code, uint8_t *fail);
uint32_t mx_jit_call_ways(const mx_resume_cache_t *cache, uintptr_t *keys, uintptr_t **codes, bool is_spread);
void mx_jit_set_var(mx_jit_cc_t *cc);
void mx_jit_cold_nil(mx_jit_cc_t *cc, const mx_resume_t *rec, uint32_t out, uintptr_t *code);
mx_jit_miss_t mx_jit_miss_seq(mx_jit_cc_t *cc, const mx_resume_t *rec, uint32_t out, mx_x64_reg_t t, mx_x64_reg_t k, mx_kind_t key_kind, uint64_t lit, int32_t at, uint8_t *stub);
void mx_jit_stub_lookup_ref(mx_jit_t *jit, mx_x64_t *a);
void mx_jit_stub_probe(mx_jit_t *jit, mx_x64_t *a);
void mx_jit_stub_chain(mx_jit_t *jit, mx_x64_t *a);
void mx_jit_stub_leave(mx_x64_t *a);
void mx_jit_stub_frame(mx_jit_t *jit, mx_x64_t *a, mx_jit_slot_t tramp, mx_jit_slot_t call, mx_x64_reg_t second);
void mx_jit_stub_code(mx_jit_t *jit, mx_x64_t *a, mx_jit_slot_t tramp, mx_jit_slot_t call, bool has_base);
void mx_jit_stub_lookup(mx_t *vm, mx_x64_t *a);
void mx_jit_tail_code(mx_jit_cc_t *cc, mx_x64_t *a);
void mx_jit_stub_ret_ways(mx_jit_t *jit, mx_x64_t *a);
void mx_jit_stub_deliver(mx_jit_t *jit, mx_x64_t *a, uint32_t want);
void mx_jit_stub_next(mx_jit_t *jit, mx_x64_t *a);
uint8_t *mx_jit_ways_stub(mx_jit_cc_t *cc, const mx_resume_t *rec, mx_x64_reg_t src, int32_t at, uint8_t *fallback);
void mx_jit_return_store(mx_jit_cc_t *cc, uint32_t n);
uint32_t mx_jit_get_sk(mx_jit_cc_t *cc);
uint32_t mx_jit_get_int(mx_jit_cc_t *cc, mx_hop_t hop);
void mx_jit_upval_load(mx_jit_cc_t *cc, mx_x64_reg_t cl, uint32_t index, mx_kind_t kind, uint8_t *miss);
uint32_t mx_jit_upval_get(mx_jit_cc_t *cc);
void mx_jit_upval_guard(mx_jit_cc_t *cc);
const mx_jit_cache_t *mx_jit_const_of(const mx_jit_cc_t *cc, uint32_t reg);
uint32_t mx_jit_goto_side(mx_jit_cc_t *cc, uintptr_t *word, mx_ir_block_t *block);
uint32_t mx_jit_goto_code(mx_jit_cc_t *cc, const mx_resume_t *rec, uintptr_t *code);
uint32_t mx_jit_intrinsic_done(mx_jit_cc_t *cc, const mx_resume_t *rec, mx_kind_t kind);
void mx_jit_move_arg(mx_jit_cc_t *cc, uint32_t out, const uintptr_t *arg);
mx_jit_way_t mx_jit_inext_way(const mx_resume_t *rec);
uint32_t mx_jit_inext(mx_jit_cc_t *cc, mx_jit_way_t way);
uint8_t *mx_jit_intrinsic_stub(mx_jit_cc_t *cc);
uint32_t mx_jit_intrinsic(mx_jit_cc_t *cc);
bool mx_jit_can_inline(const mx_jit_cc_t *cc, const mx_resume_t *rec, const uintptr_t *code);
uint32_t mx_jit_call_inline(mx_jit_cc_t *cc, mx_x64_reg_t fn, uintptr_t key, uintptr_t *code, uint8_t *stub);
uint32_t mx_jit_return_inline(mx_jit_cc_t *cc, const uintptr_t *arg);
void mx_jit_spread_guard(mx_jit_cc_t *cc, mx_kind_t kind, uint8_t *stub);
uint32_t mx_jit_call_lua(mx_jit_cc_t *cc);
bool mx_jit_tail_overlaps(const uintptr_t *args, uint32_t n);
void mx_jit_tailcall_lua(mx_jit_cc_t *cc);
void mx_jit_call_fast_rt(mx_jit_cc_t *cc);
uint32_t mx_jit_call_fast(mx_jit_cc_t *cc);
uint32_t mx_jit_method_get(mx_jit_cc_t *cc);
void mx_jit_return_n(mx_jit_cc_t *cc);
void mx_jit_frame_seq(mx_jit_cc_t *cc, mx_x64_t *a, mx_jit_slot_t slot);
uint8_t *mx_jit_frame_stub(mx_jit_cc_t *cc, mx_jit_slot_t slot);
void mx_jit_code_call(mx_jit_cc_t *cc, mx_jit_slot_t slot, mx_hop_t hop);
void mx_jit_call_frame(mx_jit_cc_t *cc, mx_jit_slot_t slot);
uint32_t mx_jit_call_ffi(mx_jit_cc_t *cc);
uint32_t mx_jit_call_ffi_result(mx_jit_cc_t *cc, const mx_resume_t *rec, int32_t at, uint8_t *after);
uint32_t mx_jit_many(mx_jit_cc_t *cc, const mx_resume_t *rec, uintptr_t key, uintptr_t *code, mx_x64_reg_t src, int32_t at, uint8_t *stub);
uint32_t mx_jit_get_var(mx_jit_cc_t *cc);
uint32_t mx_jit_br_eq_str(mx_jit_cc_t *cc);
uint32_t mx_jit_br_eq_f(mx_jit_cc_t *cc, uint32_t left, uint32_t right);
void mx_jit_upval_set(mx_jit_cc_t *cc);
uint8_t *mx_jit_return_stub(mx_jit_cc_t *cc, mx_hop_t hop);
void mx_jit_return_tail(mx_jit_cc_t *cc);
void mx_jit_return_head(mx_jit_cc_t *cc, uint8_t *exit);
void mx_jit_return_enter(mx_jit_cc_t *cc);
void mx_jit_return1(mx_jit_cc_t *cc);
void mx_jit_return0(mx_jit_cc_t *cc);
uint32_t mx_jit_hop(mx_jit_cc_t *cc, mx_hop_t hop);

#endif
