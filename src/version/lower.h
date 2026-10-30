
#pragma once

#if !defined(MX_HEADER_VERSION_LOWER)
#define MX_HEADER_VERSION_LOWER

#include "version.h"
#include "../run/hop.h"
#include "../value/kind.h"

#define MX_LOWER_CHAIN_MAX 16

typedef struct mx_lower_t mx_lower_t;
typedef struct mx_lower_snap_t mx_lower_snap_t;

struct mx_lower_snap_t {
    const mx_kind_t *kinds;
    uint32_t nkinds;
};

struct mx_lower_t {
    mx_t *vm;
    mx_version_t *rb;
    mx_kind_t *types;
    bool *fresh;
    mx_ir_block_t *block;
    mx_ir_block_t *visited[MX_LOWER_CHAIN_MAX];
    uintptr_t *start;
    uintptr_t *w;
    uintptr_t *end;
    uintptr_t *last_at;
    const mx_kind_t *snap;
    mx_arena_mark_t mark;
    mx_lower_recs_t recs;
    mx_lower_fixups_t refs;
    uint32_t nvisited;
    uint32_t nsnap;
    uint32_t nregs;
    uint32_t kslot;
    uint32_t frame;
    uint32_t at;
    uint32_t next_at;
    uint32_t site;
    bool is_hooked: 1;
    bool needs_call_hook: 1;
};

void mx_lower_reserve(mx_lower_t *l, size_t n);
void mx_lower_word(mx_lower_t *l, uintptr_t v);
void mx_lower_hop(mx_lower_t *l, mx_hop_t hop);
mx_kind_t mx_lower_kind(mx_lower_t *l, mx_ir_arg_t arg);
uintptr_t mx_lower_tag(mx_lower_t *l, mx_ir_arg_t arg);
uintptr_t mx_lower_value(mx_ir_arg_t arg);
void mx_lower_arg(mx_lower_t *l, mx_ir_arg_t arg);
void mx_lower_hot(mx_lower_t *l);
void mx_lower_move_i(mx_lower_t *l, uint32_t out, mx_obj_t v);
void mx_lower_write(mx_lower_t *l, uint32_t out, mx_kind_t kind);
mx_lower_snap_t mx_lower_snapshot(mx_lower_t *l);
mx_resume_t *mx_lower_rec(mx_lower_t *l, mx_resume_t init);
bool mx_lower_has_cell(mx_lower_t *l, uint32_t out, uint32_t count);
uint32_t mx_lower_rec_index(const mx_lower_t *l, const mx_resume_t *rec);
void mx_lower_fixup_push(mx_lower_fixups_t *fixups, uint32_t rec, size_t at);
void mx_lower_rec_word(mx_lower_t *l, mx_resume_t *rec);
void mx_lower_void(mx_lower_t *l);
uint32_t mx_lower_unknown_reg(mx_lower_t *l, mx_ir_arg_t *args, uint32_t n);
void mx_lower_cell(mx_lower_t *l, uint32_t reg);
void mx_lower_hook(mx_lower_t *l, uintptr_t event);
mx_op_t mx_lower_op_of(mx_ir_instr_opcode_t op);
void mx_lower_meta_op(mx_lower_t *l, mx_op_t op, mx_ir_instr_t *in, uint32_t nargs);
mx_kind_t mx_lower_arith_kind(mx_ir_instr_opcode_t op, mx_kind_t a, mx_kind_t b);
mx_hop_t mx_lower_binop_hop(mx_ir_instr_opcode_t op, mx_kind_t want);
mx_ir_arg_t mx_lower_as_float(mx_lower_t *l, mx_ir_arg_t arg, uint32_t scratch);
void mx_lower_arith(mx_lower_t *l, mx_ir_instr_t *in, mx_kind_t want);
mx_obj_t mx_lower_lit_string(mx_lower_t *l, mx_obj_t v);
mx_ir_arg_t mx_lower_to_string(mx_lower_t *l, mx_ir_arg_t arg, uint32_t scratch);
bool mx_lower_is_int_reg(mx_lower_t *l, mx_ir_arg_t arg);
void mx_lower_concat(mx_lower_t *l, mx_ir_instr_t *in);
bool mx_lower_needs_meta(mx_lower_t *l, mx_ir_instr_t *in, mx_kind_t a, mx_kind_t b);
void mx_lower_closure(mx_lower_t *l, mx_ir_instr_t *in);
void mx_lower_instr(mx_lower_t *l, mx_ir_instr_t *in);
void mx_lower_goto(mx_lower_t *l, mx_ir_block_t *target);
void mx_lower_cmp_hop(mx_lower_t *l, mx_hop_t hop, mx_ir_branch_t *br, uintptr_t a, uintptr_t b);
mx_ir_arg_t mx_lower_lit_reg(mx_lower_t *l, mx_ir_arg_t arg, uint32_t scratch);
void mx_lower_cmp_form(mx_lower_t *l, mx_hop_t rr, mx_ir_branch_t *br, mx_ir_arg_t a, mx_ir_arg_t b);
void mx_lower_meta_cmp(mx_lower_t *l, mx_ir_branch_t *br, mx_op_t op, mx_ir_arg_t a, mx_ir_arg_t b);
mx_ir_arg_t mx_lower_exact_float(mx_ir_arg_t arg);
void mx_lower_compare(mx_lower_t *l, mx_ir_branch_t *br, mx_ir_arg_t a, mx_ir_arg_t b);
void mx_lower_args(mx_lower_t *l, mx_ir_arg_t *args, uint32_t from, uint32_t nargs);
void mx_lower_close_all(mx_lower_t *l);
void mx_lower_call(mx_lower_t *l, mx_ir_branch_t *br, mx_ir_arg_t *args, uint32_t nargs);
void mx_lower_return(mx_lower_t *l, mx_ir_arg_t *args, uint32_t nargs);
void mx_lower_table_get(mx_lower_t *l, mx_ir_branch_t *br, mx_ir_arg_t *args);
void mx_lower_branch(mx_lower_t *l, mx_ir_branch_t *br);
bool mx_lower_chain(mx_lower_t *l, mx_ir_block_t *target);
bool mx_lower_guard(mx_lower_t *l, mx_ir_branch_t *br);
void mx_lower_try_fuse(mx_lower_t *l, uintptr_t *br_at);
void mx_lower_finish(mx_lower_t *l);
void mx_lower_version(mx_t *vm, mx_version_t *rb);

#endif
