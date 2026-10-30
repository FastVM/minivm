
#pragma once

#if !defined(MX_HEADER_LUA_FRONT_PARSE)
#define MX_HEADER_LUA_FRONT_PARSE

#include "lex.h"
#include "../../ir/ir.h"
#include "../../value/error.h"
#include "../../vm/vm.h"

#define MX_PARSE_NO_HOLE UINT32_MAX

typedef uint8_t mx_parse_exp_kind_t;
enum {
    MX_PARSE_EXP_VOID,
    MX_PARSE_EXP_VALUE,
    MX_PARSE_EXP_LOCAL,
    MX_PARSE_EXP_UPVAL,
    MX_PARSE_EXP_INDEX,
    MX_PARSE_EXP_CALL,
    MX_PARSE_EXP_VARARG,
    MX_PARSE_EXP_JUMP,
};

typedef uint8_t mx_parse_found_kind_t;
enum {
    MX_PARSE_FOUND_LOCAL,
    MX_PARSE_FOUND_CONST,
    MX_PARSE_FOUND_UPVAL,
    MX_PARSE_FOUND_GLOBAL,
};

typedef uint8_t mx_parse_prio_t;
enum {
    MX_PARSE_PRIO_NONE,
    MX_PARSE_PRIO_OR,
    MX_PARSE_PRIO_AND,
    MX_PARSE_PRIO_CMP,
    MX_PARSE_PRIO_BOR,
    MX_PARSE_PRIO_BXOR,
    MX_PARSE_PRIO_BAND,
    MX_PARSE_PRIO_SHIFT,
    MX_PARSE_PRIO_CONCAT_RIGHT,
    MX_PARSE_PRIO_CONCAT,
    MX_PARSE_PRIO_ADD,
    MX_PARSE_PRIO_MUL,
    MX_PARSE_PRIO_UNARY,
    MX_PARSE_PRIO_POW_RIGHT,
    MX_PARSE_PRIO_POW,
};

typedef struct mx_parse_t mx_parse_t;
typedef struct mx_parse_fn_t mx_parse_fn_t;
typedef struct mx_parse_scope_t mx_parse_scope_t;
typedef struct mx_parse_var_t mx_parse_var_t;
typedef struct mx_parse_vars_t mx_parse_vars_t;
typedef struct mx_parse_hole_t mx_parse_hole_t;
typedef struct mx_parse_holes_t mx_parse_holes_t;
typedef struct mx_parse_goto_t mx_parse_goto_t;
typedef struct mx_parse_gotos_t mx_parse_gotos_t;
typedef struct mx_parse_label_t mx_parse_label_t;
typedef struct mx_parse_labels_t mx_parse_labels_t;
typedef struct mx_parse_upvals_t mx_parse_upvals_t;
typedef struct mx_parse_locals_t mx_parse_locals_t;
typedef struct mx_parse_names_t mx_parse_names_t;
typedef struct mx_parse_binds_t mx_parse_binds_t;
typedef struct mx_parse_call_t mx_parse_call_t;
typedef struct mx_parse_exp_t mx_parse_exp_t;
typedef struct mx_parse_found_t mx_parse_found_t;
typedef struct mx_parse_prios_t mx_parse_prios_t;
typedef struct mx_parse_compiled_t mx_parse_compiled_t;

struct mx_parse_var_t {
    const char *name;
    mx_obj_t value;
    uint32_t reg;
    uint32_t local;
    uint32_t shadow;
    bool is_const: 1;
    bool is_close: 1;
    bool is_global: 1;
    bool is_ctc: 1;
};

struct mx_parse_vars_t {
    mx_parse_var_t *ptr;
    uint32_t len;
    uint32_t cap;
};

struct mx_parse_hole_t {
    mx_ir_block_t *block;
    mx_ir_arg_t value;
    uint32_t next;
    uint8_t slot;
};

struct mx_parse_holes_t {
    mx_parse_hole_t *ptr;
    uint32_t len;
    uint32_t cap;
};

struct mx_parse_goto_t {
    const char *name;
    mx_ir_block_t *tramp;
    mx_location_range_t range;
    uint32_t line;
    uint32_t nvars;
};

struct mx_parse_gotos_t {
    mx_parse_goto_t *ptr;
    uint32_t len;
    uint32_t cap;
};

struct mx_parse_label_t {
    const char *name;
    mx_ir_block_t *block;
    uint32_t line;
    uint32_t nvars;
};

struct mx_parse_labels_t {
    mx_parse_label_t *ptr;
    uint32_t len;
    uint32_t cap;
};

struct mx_parse_upvals_t {
    mx_ir_upval_t *ptr;
    bool *is_const;
    uint32_t len;
    uint32_t cap;
};

struct mx_parse_locals_t {
    mx_ir_local_t *ptr;
    uint32_t len;
    uint32_t cap;
};

struct mx_parse_binds_t {
    const char **keys;
    uint32_t *vals;
    uint32_t used;
    uint32_t cap;
};

struct mx_parse_names_t {
    const char **from;
    const char **to;
    uint32_t used;
    uint32_t cap;
};

struct mx_parse_scope_t {
    mx_parse_scope_t *prev;
    uint32_t nvars;
    uint32_t first_label;
    uint32_t first_goto;
    bool is_loop: 1;
    bool has_capture: 1;
};

struct mx_parse_fn_t {
    mx_parse_fn_t *prev;
    mx_ir_func_t *func;
    mx_ir_block_t *head;
    mx_ir_block_t *cur;
    mx_ir_block_t *last_block;
    mx_parse_scope_t *scope;
    mx_parse_upvals_t upvals;
    mx_parse_locals_t locals;
    uint32_t first_var;
    uint32_t first_label;
    uint32_t first_block;
    uint32_t free_reg;
    uint32_t nregs;
    uint32_t reg_floor;
    uint32_t nparams;
    uint32_t pack_reg;
    uint32_t line;
    uint32_t last_at;
    uint32_t last_out;
    bool is_vararg: 1;
    bool has_cells: 1;
};

struct mx_parse_call_t {
    mx_ir_arg_t *args;
    mx_location_range_t range;
    uint32_t nargs;
};

struct mx_parse_exp_t {
    mx_ir_arg_t arg;
    mx_ir_arg_t key;
    mx_parse_call_t *call;
    mx_varname_t name;
    mx_varname_t base;
    uint32_t index;
    uint32_t t;
    uint32_t f;
    uint32_t start;
    mx_parse_exp_kind_t kind;
    bool is_var: 1;
    bool is_const: 1;
};

struct mx_parse_found_t {
    uint32_t index;
    mx_parse_found_kind_t kind;
    bool is_const: 1;
};

struct mx_parse_prios_t {
    mx_parse_prio_t left;
    mx_parse_prio_t right;
};

struct mx_parse_t {
    mx_t *vm;
    mx_lex_t lex;
    const mx_source_t *source;
    mx_parse_fn_t *fn;
    mx_parse_vars_t vars;
    mx_parse_holes_t holes;
    mx_parse_gotos_t gotos;
    mx_parse_labels_t labels;
    mx_parse_names_t names;
    mx_parse_binds_t binds;
    uint32_t nglobals;
    mx_ir_blocks_t blocks;
    const char *env_name;
    const char *break_name;
    const char *self_name;
    const char *pack_name;
    const char *gall;
    uint32_t depth;
};

struct mx_parse_compiled_t {
    mx_ir_block_t *entry;
    mx_error_t *error;
};

uint32_t mx_gen_name_slot(const mx_parse_names_t *names, const char *name);
const char *mx_gen_name(mx_parse_t *d, const char *name);
mx_varname_t mx_gen_varname(mx_parse_t *d, const char *name, mx_varhint_t kind);
mx_location_range_t mx_gen_range(mx_parse_t *d, uint32_t start, mx_varname_t a, mx_varname_t b);
uint32_t mx_gen_here(mx_parse_t *d);
mx_ir_arg_t mx_gen_reg_arg(uint32_t reg);
mx_ir_arg_t mx_gen_lit(mx_obj_t value);
mx_ir_arg_t mx_gen_none(void);
mx_ir_arg_t mx_gen_int(int64_t v);
mx_ir_arg_t *mx_gen_args(mx_parse_t *d, uint32_t n);
void mx_gen_track(mx_parse_t *d, mx_ir_block_t *block);
mx_ir_block_t *mx_gen_block(mx_parse_t *d);
mx_ir_block_t *mx_gen_cur(mx_parse_t *d);
uint32_t mx_gen_reg(mx_parse_t *d);
uint32_t mx_gen_nactive_regs(mx_parse_t *d);
void mx_gen_free_arg(mx_parse_t *d, mx_ir_arg_t arg);
uint32_t mx_gen_instr(mx_parse_t *d, mx_ir_instr_opcode_t op, mx_ir_arg_t out, mx_ir_arg_t *args, mx_location_range_t range);
void mx_gen_move(mx_parse_t *d, uint32_t reg, mx_ir_arg_t value);
void mx_gen_branch(mx_parse_t *d, mx_ir_branch_t branch);
void mx_gen_jump(mx_parse_t *d, mx_ir_block_t *target);
void mx_gen_enter(mx_parse_t *d, mx_ir_block_t *block);
void mx_gen_link(mx_parse_t *d, mx_ir_branch_t branch);
uint32_t mx_gen_hole(mx_parse_t *d, mx_ir_block_t *block, uint8_t slot, mx_ir_arg_t value);
uint32_t mx_gen_concat(mx_parse_t *d, uint32_t a, uint32_t b);
void mx_gen_patch(mx_parse_t *d, uint32_t list, mx_ir_block_t *target);
void mx_gen_patch_here(mx_parse_t *d, uint32_t list);
void mx_gen_strip_values(mx_parse_t *d, uint32_t list);
uint32_t mx_gen_jump_hole(mx_parse_t *d, mx_ir_arg_t value);
void mx_gen_bool_holes(mx_parse_t *d, mx_parse_exp_t *e, mx_ir_arg_t tvalue, mx_ir_arg_t fvalue);
mx_parse_exp_t mx_gen_exp(mx_parse_exp_kind_t kind, uint32_t start);
mx_parse_exp_t mx_gen_exp_arg(mx_ir_arg_t arg, uint32_t start);
bool mx_gen_has_jumps(const mx_parse_exp_t *e);
bool mx_gen_is_multi(const mx_parse_exp_t *e);
bool mx_gen_is_lit(const mx_parse_exp_t *e);
bool mx_gen_is_numeral(const mx_parse_exp_t *e);
mx_ir_arg_t mx_gen_table_get(mx_parse_t *d, mx_ir_arg_t table, mx_ir_arg_t key, mx_location_range_t range);
void mx_gen_table_get_into(mx_parse_t *d, mx_ir_arg_t table, mx_ir_arg_t key, mx_location_range_t range, uint32_t reg);
void mx_gen_call(mx_parse_t *d, mx_parse_exp_t *e, uint32_t want);
void mx_gen_discharge(mx_parse_t *d, mx_parse_exp_t *e);
void mx_gen_materialize(mx_parse_t *d, mx_parse_exp_t *e, uint32_t reg);
void mx_gen_to_reg(mx_parse_t *d, mx_parse_exp_t *e, uint32_t reg);
void mx_gen_to_next(mx_parse_t *d, mx_parse_exp_t *e);
void mx_gen_to_any(mx_parse_t *d, mx_parse_exp_t *e);
void mx_gen_to_arg(mx_parse_t *d, mx_parse_exp_t *e);
void mx_gen_to_spread(mx_parse_t *d, mx_parse_exp_t *e);
void mx_gen_multi(mx_parse_t *d, mx_parse_exp_t *e, uint32_t want);
bool mx_gen_lit_truthy(mx_obj_t v);
void mx_gen_go_if_true(mx_parse_t *d, mx_parse_exp_t *e);
void mx_gen_go_if_false(mx_parse_t *d, mx_parse_exp_t *e);
void mx_gen_not(mx_parse_t *d, mx_parse_exp_t *e);
uint32_t mx_gen_cond(mx_parse_t *d, mx_parse_exp_t *e);
mx_ir_arg_t mx_gen_call_prim(mx_parse_t *d, mx_ffi_t *fn, mx_ir_arg_t *args, uint32_t nargs, uint32_t start);
void mx_gen_store(mx_parse_t *d, const mx_parse_exp_t *target, mx_parse_exp_t *value);
bool mx_gen_fold_int(mx_obj_t v, int64_t *out);
bool mx_gen_fold_arith(mx_ir_instr_opcode_t op, mx_obj_t a, mx_obj_t b, mx_obj_t *out);

mx_error_t *mx_parse_semerror(mx_parse_t *d, const char *fmt, ...);
mx_error_t *mx_parse_limit(mx_parse_t *d, uint32_t limit, const char *what);
mx_error_t *mx_parse_depth_enter(mx_parse_t *d);
void mx_parse_depth_exit(mx_parse_t *d);
mx_error_t *mx_parse_check_match(mx_parse_t *d, mx_lex_type_t what, mx_lex_type_t who, uint32_t open_at);
bool mx_parse_block_follow(mx_parse_t *d, bool with_until);
uint32_t mx_parse_line(mx_parse_t *d, uint32_t at);
void mx_parse_fn_open(mx_parse_t *d, mx_parse_fn_t *fn, uint32_t line, uint32_t start);
void mx_parse_fn_upvals(mx_parse_t *d, mx_parse_fn_t *fn);
mx_ir_block_t *mx_parse_fn_close(mx_parse_t *d, uint32_t stop);
void mx_parse_fn_abandon(mx_parse_t *d);
void mx_parse_scope_open(mx_parse_t *d, mx_parse_scope_t *scope, bool is_loop);
mx_error_t *mx_parse_scope_close(mx_parse_t *d);
uint32_t mx_parse_reg_level(mx_parse_t *d, uint32_t nvars);
uint32_t mx_parse_vars_level(mx_parse_t *d, uint32_t from, uint32_t to);
void mx_parse_close_regs(mx_parse_t *d, uint32_t from);
void mx_parse_keep(mx_parse_t *d, uint32_t from, uint32_t to);
bool mx_parse_has_close(mx_parse_t *d, uint32_t from, uint32_t to);
void mx_parse_close_vars(mx_parse_t *d, uint32_t from, uint32_t to, bool has_capture);
bool mx_parse_inside_close(mx_parse_t *d);
void mx_parse_tramp_close(mx_parse_t *d, mx_parse_goto_t *g, uint32_t from, uint32_t to, bool has_capture);
mx_parse_label_t *mx_parse_label_find(mx_parse_t *d, const char *name, uint32_t from);
void mx_parse_label_push(mx_parse_t *d, mx_parse_label_t label);
mx_ir_block_t *mx_parse_here_block(mx_parse_t *d);
mx_error_t *mx_parse_gotos_solve(mx_parse_t *d, mx_parse_scope_t *scope);
uint32_t mx_parse_bind_home(const mx_parse_binds_t *binds, const char *name);
uint32_t mx_parse_bind_slot(mx_parse_t *d, const char *name);
void mx_parse_vars_pop(mx_parse_t *d, uint32_t to);
mx_error_t *mx_parse_var_push(mx_parse_t *d, mx_parse_var_t var);
mx_error_t *mx_parse_var_new(mx_parse_t *d, const char *name, uint32_t reg, bool is_const, bool is_close, uint32_t start);
mx_error_t *mx_parse_var_const(mx_parse_t *d, const char *name, mx_obj_t value);
mx_error_t *mx_parse_var_global(mx_parse_t *d, const char *name, bool is_const);
void mx_parse_mark_capture(mx_parse_fn_t *fn, uint32_t index);
mx_error_t *mx_parse_upval_new(mx_parse_t *d, mx_parse_fn_t *fn, const char *name, uint32_t index, bool is_local, bool is_const, uint32_t *out);
mx_error_t *mx_parse_resolve(mx_parse_t *d, mx_parse_fn_t *fn, const char *name, bool is_base, int64_t *coll, mx_parse_found_t *out);
mx_error_t *mx_parse_var_exp(mx_parse_t *d, const char *name, uint32_t start, mx_parse_found_t found, mx_parse_exp_t *e);
mx_error_t *mx_parse_env_exp(mx_parse_t *d, const char *name, uint32_t start, mx_parse_exp_t *e);
mx_error_t *mx_parse_name_exp(mx_parse_t *d, const char *name, uint32_t start, mx_parse_exp_t *e);
mx_error_t *mx_parse_check_readonly(mx_parse_t *d, const mx_parse_exp_t *e);
bool mx_parse_is_var(const mx_parse_exp_t *e);
mx_varname_t mx_parse_operand_name(mx_parse_t *d, const mx_parse_exp_t *e);
mx_error_t *mx_parse_funcargs(mx_parse_t *d, mx_parse_exp_t *e, mx_ir_arg_t self, uint32_t call_at);
mx_error_t *mx_parse_primary(mx_parse_t *d, mx_parse_exp_t *e);
mx_error_t *mx_parse_field_name(mx_parse_t *d, const char **name);
mx_error_t *mx_parse_index_key(mx_parse_t *d, mx_parse_exp_t *key);
void mx_parse_indexed(mx_parse_t *d, mx_parse_exp_t *e, mx_ir_arg_t key, mx_varname_t name, uint32_t at);
mx_error_t *mx_parse_suffixed(mx_parse_t *d, mx_parse_exp_t *e);
mx_error_t *mx_parse_simple(mx_parse_t *d, mx_parse_exp_t *e);
mx_parse_prios_t mx_parse_binop_prio(mx_lex_type_t type);
mx_ir_instr_opcode_t mx_parse_arith_op(mx_lex_type_t type);
bool mx_parse_is_cmp(mx_lex_type_t op);
void mx_parse_infix(mx_parse_t *d, mx_lex_type_t op, mx_parse_exp_t *e);
void mx_parse_compare(mx_parse_t *d, mx_lex_type_t op, mx_parse_exp_t *e1, mx_parse_exp_t *e2, uint32_t at);
void mx_parse_posfix(mx_parse_t *d, mx_lex_type_t op, mx_parse_exp_t *e1, mx_parse_exp_t *e2, uint32_t at);
mx_error_t *mx_parse_prefix(mx_parse_t *d, mx_lex_type_t op, mx_parse_exp_t *e, uint32_t at);
mx_error_t *mx_parse_binops(mx_parse_t *d, mx_parse_exp_t *e, mx_parse_prio_t limit);
mx_error_t *mx_parse_subexpr(mx_parse_t *d, mx_parse_exp_t *e, mx_parse_prio_t limit);
mx_error_t *mx_parse_expr(mx_parse_t *d, mx_parse_exp_t *e);
mx_error_t *mx_parse_constructor(mx_parse_t *d, mx_parse_exp_t *e);
void mx_parse_return(mx_parse_t *d, mx_ir_arg_t *args);
mx_error_t *mx_parse_func_body(mx_parse_t *d, mx_parse_fn_t *fn, bool is_method, uint32_t start, uint32_t *stop);
mx_error_t *mx_parse_body(mx_parse_t *d, mx_parse_exp_t *e, bool is_method, uint32_t line, uint32_t start);
mx_error_t *mx_parse_block(mx_parse_t *d);
mx_error_t *mx_parse_explist(mx_parse_t *d, mx_parse_exp_t *e, uint32_t *count);
void mx_parse_adjust(mx_parse_t *d, uint32_t base, uint32_t nvars, uint32_t nexps, mx_parse_exp_t *e);
mx_error_t *mx_parse_retstat(mx_parse_t *d);
void mx_parse_check_conflict(mx_parse_t *d, mx_parse_exp_t *targets, uint32_t ntargets, const mx_parse_exp_t *v);
mx_error_t *mx_parse_restassign(mx_parse_t *d, mx_parse_exp_t *first);
mx_error_t *mx_parse_exprstat(mx_parse_t *d, uint32_t value_reg, bool *produced);
mx_error_t *mx_parse_attrib(mx_parse_t *d, bool *is_const, bool *is_close);
mx_error_t *mx_parse_localfunc(mx_parse_t *d, uint32_t start);
mx_error_t *mx_parse_localstat(mx_parse_t *d);
mx_error_t *mx_parse_test_then(mx_parse_t *d, uint32_t value_reg, uint32_t *escape);
mx_error_t *mx_parse_ifstat(mx_parse_t *d, uint32_t value_reg, uint32_t start);
mx_error_t *mx_parse_value_if(mx_parse_t *d, mx_parse_exp_t *e, uint32_t reg);
mx_error_t *mx_parse_value_do(mx_parse_t *d, uint32_t reg);
mx_error_t *mx_parse_whilestat(mx_parse_t *d, uint32_t start);
mx_error_t *mx_parse_repeatstat(mx_parse_t *d, uint32_t start);
void mx_parse_for_test(mx_parse_t *d, mx_ir_arg_t iter, mx_ir_arg_t stop, mx_ir_arg_t step, mx_ir_block_t *body, mx_ir_block_t *exit, uint32_t at);
bool mx_parse_is_nonzero_lit(const mx_parse_exp_t *e);
mx_error_t *mx_parse_fornum(mx_parse_t *d, const char *name, uint32_t start);
mx_error_t *mx_parse_forlist(mx_parse_t *d, const char *first, uint32_t start);
mx_error_t *mx_parse_forstat(mx_parse_t *d, uint32_t start);
mx_error_t *mx_parse_funcstat(mx_parse_t *d, uint32_t start);
mx_error_t *mx_parse_labelstat(mx_parse_t *d, uint32_t start);
void mx_parse_goto_push(mx_parse_t *d, const char *name, uint32_t start);
mx_error_t *mx_parse_breakstat(mx_parse_t *d, uint32_t start);
mx_error_t *mx_parse_gotostat(mx_parse_t *d, uint32_t start);
mx_error_t *mx_parse_checkglobal(mx_parse_t *d, mx_parse_exp_t *target, const char *name, uint32_t at);
mx_error_t *mx_parse_global_attrib(mx_parse_t *d, bool *is_const);
mx_error_t *mx_parse_globalstat(mx_parse_t *d, uint32_t start);
mx_error_t *mx_parse_statement(mx_parse_t *d, uint32_t value_reg, bool *produced);
mx_error_t *mx_parse_statlist(mx_parse_t *d, uint32_t value_reg);
void mx_parse_init(mx_parse_t *d, mx_t *vm, const mx_source_t *source, const char *src);
void mx_parse_free(mx_parse_t *d);
mx_parse_compiled_t mx_parse_chunk(mx_t *vm, const char *src, uint32_t len, const char *file);

#endif
