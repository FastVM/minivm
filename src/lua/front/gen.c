
#include "parse.h"
#include "../../ir/scratch.h"
#include "../../value/num.h"
#include "../../value/obj.h"

uint32_t mx_gen_name_slot(const mx_parse_names_t *names, const char *name) {
    return (uint32_t)(((uint64_t)(uintptr_t)name * UINT64_C(0x9E3779B97F4A7C15)) >> 32) & (names->cap - 1);
}

const char *mx_gen_name(mx_parse_t *d, const char *name) {
    mx_parse_names_t *names = &d->names;
    if ((names->used + 1) * 2 > names->cap) {
        uint32_t ocap = names->cap;
        const char **ofrom = names->from;
        const char **oto = names->to;
        if (ocap == 0) {
            names->cap = 64;
        } else {
            names->cap = ocap * 2;
        }
        names->from = mx_mem_calloc(sizeof(const char *) * names->cap);
        names->to = mx_mem_calloc(sizeof(const char *) * names->cap);
        for (uint32_t i = 0; i < ocap; i++) {
            if (ofrom[i] != NULL) {
                uint32_t at = mx_gen_name_slot(names, ofrom[i]);
                while (names->from[at] != NULL) {
                    at = (at + 1) & (names->cap - 1);
                }
                names->from[at] = ofrom[i];
                names->to[at] = oto[i];
            }
        }
        mx_mem_free(ofrom);
        mx_mem_free(oto);
    }
    uint32_t at = mx_gen_name_slot(names, name);
    while (names->from[at] != NULL && names->from[at] != name) {
        at = (at + 1) & (names->cap - 1);
    }
    if (names->from[at] == NULL) {
        names->from[at] = name;
        names->to[at] = mx_ir_strdup(d->vm, name);
        d->vm->arenas.ir.names += (uint32_t)strlen(name) + 1;
        names->used += 1;
    }
    return names->to[at];
}

mx_varname_t mx_gen_varname(mx_parse_t *d, const char *name, mx_varhint_t kind) {
    return (mx_varname_t) { .name = mx_gen_name(d, name), .kind = kind };
}

mx_location_range_t mx_gen_range(mx_parse_t *d, uint32_t start, mx_varname_t a, mx_varname_t b) {
    return (mx_location_range_t) {
        .source = d->source,
        .start = start,
        .names = {a, b},
    };
}

uint32_t mx_gen_here(mx_parse_t *d) {
    if (d->lex.last_stop == 0) {
        return 0;
    } else {
        return d->lex.last_stop - 1;
    }
}

mx_ir_arg_t mx_gen_reg_arg(uint32_t reg) {
    return (mx_ir_arg_t) {
        .type = MX_IR_ARG_TYPE_REG,
        .reg = reg,
    };
}

mx_ir_arg_t mx_gen_lit(mx_obj_t value) {
    return (mx_ir_arg_t) {
        .type = MX_IR_ARG_TYPE_LIT,
        .lit = value,
    };
}

mx_ir_arg_t mx_gen_none(void) {
    return (mx_ir_arg_t) {
        .type = MX_IR_ARG_TYPE_NONE,
    };
}

mx_ir_arg_t mx_gen_int(int64_t v) {
    return mx_gen_lit(mx_obj_of_int(v));
}

mx_ir_arg_t *mx_gen_args(mx_parse_t *d, uint32_t n) {
    mx_ir_arg_t *args = mx_scratch_alloc(d->vm, sizeof(mx_ir_arg_t) * (n + 1));
    for (uint32_t i = 0; i <= n; i++) {
        args[i] = mx_gen_none();
    }
    return args;
}

void mx_gen_track(mx_parse_t *d, mx_ir_block_t *block) {
    if (d->blocks.len == d->blocks.cap) {
        d->blocks.cap = d->blocks.cap * 2 + 64;
        d->blocks.ptr = mx_mem_realloc(d->blocks.ptr, sizeof(mx_ir_block_t *) * d->blocks.cap);
    }
    d->blocks.ptr[d->blocks.len] = block;
    d->blocks.len += 1;
    d->vm->arenas.ir.blocks += sizeof(mx_ir_block_t);
}

mx_ir_block_t *mx_gen_block(mx_parse_t *d) {
    mx_ir_block_t *block = mx_ir_block_new(d->vm, &d->vm->arenas.ir, d->fn->func);
    mx_gen_track(d, block);
    return block;
}

mx_ir_block_t *mx_gen_cur(mx_parse_t *d) {
    if (d->fn->cur == NULL) {
        d->fn->cur = mx_gen_block(d);
    }
    return d->fn->cur;
}

uint32_t mx_gen_reg(mx_parse_t *d) {
    mx_parse_fn_t *fn = d->fn;
    uint32_t reg = fn->free_reg;
    fn->free_reg += 1;
    if (fn->free_reg > fn->nregs) {
        fn->nregs = fn->free_reg;
    }
    return reg;
}

uint32_t mx_gen_nactive_regs(mx_parse_t *d) {
    mx_parse_fn_t *fn = d->fn;
    uint32_t level = fn->reg_floor;
    for (uint32_t i = d->vars.len; i > fn->first_var; i--) {
        const mx_parse_var_t *var = &d->vars.ptr[i - 1];
        if (var->reg != UINT32_MAX) {
            if (var->reg + 1 > level) {
                level = var->reg + 1;
            }
            return level;
        }
    }
    return level;
}

void mx_gen_free_arg(mx_parse_t *d, mx_ir_arg_t arg) {
    mx_parse_fn_t *fn = d->fn;
    if (arg.type == MX_IR_ARG_TYPE_REG && arg.reg + 1 == fn->free_reg && arg.reg >= mx_gen_nactive_regs(d)) {
        fn->free_reg -= 1;
    }
}

uint32_t mx_gen_instr(mx_parse_t *d, mx_ir_instr_opcode_t op, mx_ir_arg_t out, mx_ir_arg_t *args, mx_location_range_t range) {
    mx_parse_fn_t *fn = d->fn;
    mx_ir_block_t *block = mx_gen_cur(d);
    mx_ir_keep_lits(d->vm, args);
    uint32_t at = mx_ir_block_emit(
        block,
        (mx_ir_instr_t) {
            .op = op,
            .out = out,
            .args = args,
            .range = range,
        }
    );
    fn->last_block = block;
    fn->last_at = at;
    if (out.type == MX_IR_ARG_TYPE_REG) {
        fn->last_out = (uint32_t)out.reg;
    } else {
        fn->last_out = UINT32_MAX;
    }
    return at;
}

void mx_gen_move(mx_parse_t *d, uint32_t reg, mx_ir_arg_t value) {
    if (value.type != MX_IR_ARG_TYPE_REG || value.reg != reg) {
        mx_ir_arg_t *args = mx_gen_args(d, 1);
        args[0] = value;
        mx_gen_instr(d, MX_IR_INSTR_OPCODE_MOVE, mx_gen_reg_arg(reg), args, mx_gen_range(d, mx_gen_here(d), (mx_varname_t) {0}, (mx_varname_t) {0}));
    }
}

void mx_gen_branch(mx_parse_t *d, mx_ir_branch_t branch) {
    mx_ir_block_t *block = mx_gen_cur(d);
    mx_ir_keep_lits(d->vm, branch.args);
    mx_ir_block_set_branch(d->vm, block, branch);
    d->fn->cur = NULL;
}

void mx_gen_jump(mx_parse_t *d, mx_ir_block_t *target) {
    mx_gen_branch(
        d,
        (mx_ir_branch_t) {
            .op = MX_IR_BRANCH_OPCODE_JUMP,
            .targets = {target, NULL},
            .range = mx_gen_range(d, mx_gen_here(d), (mx_varname_t) {0}, (mx_varname_t) {0}),
        }
    );
}

void mx_gen_enter(mx_parse_t *d, mx_ir_block_t *block) {
    if (d->fn->cur != NULL) {
        mx_gen_jump(d, block);
    }
    d->fn->cur = block;
}

void mx_gen_link(mx_parse_t *d, mx_ir_branch_t branch) {
    mx_ir_block_t *next = mx_gen_block(d);
    branch.targets[0] = next;
    mx_gen_branch(d, branch);
    d->fn->cur = next;
}

uint32_t mx_gen_hole(mx_parse_t *d, mx_ir_block_t *block, uint8_t slot, mx_ir_arg_t value) {
    if (d->holes.len == d->holes.cap) {
        d->holes.cap = d->holes.cap * 2 + 64;
        d->holes.ptr = mx_mem_realloc(d->holes.ptr, sizeof(mx_parse_hole_t) * d->holes.cap);
    }
    uint32_t at = d->holes.len;
    d->holes.ptr[at] = (mx_parse_hole_t) {
        .block = block,
        .value = value,
        .next = MX_PARSE_NO_HOLE,
        .slot = slot,
    };
    d->holes.len += 1;
    return at;
}

uint32_t mx_gen_concat(mx_parse_t *d, uint32_t a, uint32_t b) {
    if (a == MX_PARSE_NO_HOLE) {
        return b;
    } else if (b == MX_PARSE_NO_HOLE) {
        return a;
    } else {
        uint32_t at = a;
        while (d->holes.ptr[at].next != MX_PARSE_NO_HOLE) {
            at = d->holes.ptr[at].next;
        }
        d->holes.ptr[at].next = b;
        return a;
    }
}

void mx_gen_patch(mx_parse_t *d, uint32_t list, mx_ir_block_t *target) {
    for (uint32_t at = list; at != MX_PARSE_NO_HOLE; at = d->holes.ptr[at].next) {
        mx_parse_hole_t *hole = &d->holes.ptr[at];
        hole->block->branch_targets[hole->slot] = target;
    }
}

void mx_gen_patch_here(mx_parse_t *d, uint32_t list) {
    if (list != MX_PARSE_NO_HOLE) {
        mx_ir_block_t *cur = d->fn->cur;
        if (cur == NULL || cur->code_len != 0) {
            cur = mx_gen_block(d);
            mx_gen_enter(d, cur);
        }
        mx_gen_patch(d, list, cur);
    }
}

void mx_gen_strip_values(mx_parse_t *d, uint32_t list) {
    for (uint32_t at = list; at != MX_PARSE_NO_HOLE; at = d->holes.ptr[at].next) {
        d->holes.ptr[at].value = mx_gen_none();
    }
}

uint32_t mx_gen_jump_hole(mx_parse_t *d, mx_ir_arg_t value) {
    mx_ir_block_t *block = mx_gen_cur(d);
    mx_gen_branch(
        d,
        (mx_ir_branch_t) {
            .op = MX_IR_BRANCH_OPCODE_JUMP,
            .range = mx_gen_range(d, mx_gen_here(d), (mx_varname_t) {0}, (mx_varname_t) {0}),
        }
    );
    return mx_gen_hole(d, block, 0, value);
}

void mx_gen_bool_holes(mx_parse_t *d, mx_parse_exp_t *e, mx_ir_arg_t tvalue, mx_ir_arg_t fvalue) {
    mx_ir_block_t *block = mx_gen_cur(d);
    mx_ir_arg_t *args = mx_gen_args(d, 1);
    args[0] = e->arg;
    mx_gen_branch(
        d,
        (mx_ir_branch_t) {
            .op = MX_IR_BRANCH_OPCODE_BOOL,
            .args = args,
            .range = mx_gen_range(d, e->start, e->name, (mx_varname_t) {0}),
        }
    );
    mx_gen_free_arg(d, e->arg);
    e->t = mx_gen_concat(d, e->t, mx_gen_hole(d, block, 0, tvalue));
    e->f = mx_gen_concat(d, e->f, mx_gen_hole(d, block, 1, fvalue));
}

mx_parse_exp_t mx_gen_exp(mx_parse_exp_kind_t kind, uint32_t start) {
    return (mx_parse_exp_t) {
        .kind = kind,
        .t = MX_PARSE_NO_HOLE,
        .f = MX_PARSE_NO_HOLE,
        .start = start,
    };
}

mx_parse_exp_t mx_gen_exp_arg(mx_ir_arg_t arg, uint32_t start) {
    mx_parse_exp_t e = mx_gen_exp(MX_PARSE_EXP_VALUE, start);
    e.arg = arg;
    return e;
}

bool mx_gen_has_jumps(const mx_parse_exp_t *e) {
    return e->t != MX_PARSE_NO_HOLE || e->f != MX_PARSE_NO_HOLE;
}

bool mx_gen_is_multi(const mx_parse_exp_t *e) {
    return e->kind == MX_PARSE_EXP_CALL || e->kind == MX_PARSE_EXP_VARARG;
}

bool mx_gen_is_lit(const mx_parse_exp_t *e) {
    return e->kind == MX_PARSE_EXP_VALUE && e->arg.type == MX_IR_ARG_TYPE_LIT && !mx_gen_has_jumps(e);
}

bool mx_gen_is_numeral(const mx_parse_exp_t *e) {
    return mx_gen_is_lit(e) && mx_obj_is_number(e->arg.lit);
}

mx_ir_arg_t mx_gen_table_get(mx_parse_t *d, mx_ir_arg_t table, mx_ir_arg_t key, mx_location_range_t range) {
    mx_gen_free_arg(d, key);
    mx_gen_free_arg(d, table);
    uint32_t reg = mx_gen_reg(d);
    mx_gen_table_get_into(d, table, key, range, reg);
    return mx_gen_reg_arg(reg);
}

void mx_gen_table_get_into(mx_parse_t *d, mx_ir_arg_t table, mx_ir_arg_t key, mx_location_range_t range, uint32_t reg) {
    mx_ir_arg_t *args = mx_gen_args(d, 2);
    args[0] = table;
    args[1] = key;
    mx_gen_link(
        d,
        (mx_ir_branch_t) {
            .op = MX_IR_BRANCH_OPCODE_TABLE_GET,
            .out = mx_gen_reg_arg(reg),
            .args = args,
            .range = range,
        }
    );
}

void mx_gen_call(mx_parse_t *d, mx_parse_exp_t *e, uint32_t want) {
    mx_parse_call_t *call = e->call;
    bool is_freeing = true;
    while (is_freeing) {
        uint32_t before = d->fn->free_reg;
        for (uint32_t i = call->nargs + 1; i > 0; i--) {
            mx_gen_free_arg(d, call->args[i - 1]);
        }
        is_freeing = d->fn->free_reg != before;
    }
    mx_ir_arg_t out;
    if (want == 0 || want == MX_IR_WANT_MULTI) {
        out = mx_gen_none();
    } else {
        out = mx_gen_reg_arg(d->fn->free_reg);
        for (uint32_t i = 0; i < want; i++) {
            mx_gen_reg(d);
        }
    }
    mx_gen_link(
        d,
        (mx_ir_branch_t) {
            .op = MX_IR_BRANCH_OPCODE_CALL,
            .out = out,
            .args = call->args,
            .want = want,
            .range = call->range,
        }
    );
    if (want == MX_IR_WANT_MULTI) {
        e->kind = MX_PARSE_EXP_VALUE;
        e->arg = (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_SPREAD };
    } else if (want == 0) {
        e->kind = MX_PARSE_EXP_VOID;
    } else {
        e->kind = MX_PARSE_EXP_VALUE;
        e->arg = out;
    }
    e->name = (mx_varname_t) {0};
    e->call = NULL;
}

void mx_gen_discharge(mx_parse_t *d, mx_parse_exp_t *e) {
    switch (e->kind) {
        case MX_PARSE_EXP_LOCAL: {
            e->kind = MX_PARSE_EXP_VALUE;
            break;
        }
        case MX_PARSE_EXP_UPVAL: {
            uint32_t reg = mx_gen_reg(d);
            mx_ir_arg_t *args = mx_gen_args(d, 1);
            args[0] = mx_gen_int((int64_t)e->index);
            mx_gen_link(
                d,
                (mx_ir_branch_t) {
                    .op = MX_IR_BRANCH_OPCODE_UPVAL_GET,
                    .out = mx_gen_reg_arg(reg),
                    .args = args,
                    .range = mx_gen_range(d, e->start, (mx_varname_t) {0}, (mx_varname_t) {0}),
                }
            );
            e->kind = MX_PARSE_EXP_VALUE;
            e->arg = mx_gen_reg_arg(reg);
            break;
        }
        case MX_PARSE_EXP_INDEX: {
            e->arg = mx_gen_table_get(d, e->arg, e->key, mx_gen_range(d, e->start, e->base, (mx_varname_t) {0}));
            e->kind = MX_PARSE_EXP_VALUE;
            break;
        }
        case MX_PARSE_EXP_CALL: {
            mx_gen_call(d, e, 1);
            break;
        }
        case MX_PARSE_EXP_VARARG: {
            e->arg = mx_gen_table_get(d, mx_gen_reg_arg(d->fn->pack_reg), mx_gen_int(1), mx_gen_range(d, e->start, (mx_varname_t) {0}, (mx_varname_t) {0}));
            e->kind = MX_PARSE_EXP_VALUE;
            break;
        }
        default: {
            break;
        }
    }
    e->is_var = false;
}

void mx_gen_materialize(mx_parse_t *d, mx_parse_exp_t *e, uint32_t reg) {
    mx_ir_block_t *join = mx_gen_block(d);
    if (d->fn->cur != NULL) {
        mx_gen_jump(d, join);
    }
    mx_ir_block_t *bools[2] = {NULL, NULL};
    uint32_t lists[2] = {e->t, e->f};
    for (uint32_t side = 0; side < 2; side++) {
        uint32_t at = lists[side];
        while (at != MX_PARSE_NO_HOLE) {
            mx_parse_hole_t hole = d->holes.ptr[at];
            if (hole.value.type != MX_IR_ARG_TYPE_NONE) {
                mx_ir_block_t *b = mx_gen_block(d);
                d->fn->cur = b;
                mx_gen_move(d, reg, hole.value);
                mx_gen_jump(d, join);
                hole.block->branch_targets[hole.slot] = b;
            } else {
                if (bools[side] == NULL) {
                    bools[side] = mx_gen_block(d);
                    d->fn->cur = bools[side];
                    mx_gen_move(d, reg, mx_gen_lit(mx_obj_of_boolean(side == 0)));
                    mx_gen_jump(d, join);
                }
                hole.block->branch_targets[hole.slot] = bools[side];
            }
            at = hole.next;
        }
    }
    d->fn->cur = join;
    e->t = MX_PARSE_NO_HOLE;
    e->f = MX_PARSE_NO_HOLE;
}

void mx_gen_to_reg(mx_parse_t *d, mx_parse_exp_t *e, uint32_t reg) {
    mx_gen_discharge(d, e);
    mx_parse_fn_t *fn = d->fn;
    bool is_last = e->kind == MX_PARSE_EXP_VALUE
        && e->arg.type == MX_IR_ARG_TYPE_REG
        && fn->cur != NULL
        && fn->last_block == fn->cur
        && fn->last_out == e->arg.reg
        && e->arg.reg >= mx_gen_nactive_regs(d);
    if (is_last) {
        mx_ir_block_retarget(fn->cur, fn->last_at, reg);
        fn->last_out = reg;
    } else if (e->kind == MX_PARSE_EXP_VALUE) {
        mx_gen_move(d, reg, e->arg);
    }
    if (mx_gen_has_jumps(e) || e->kind == MX_PARSE_EXP_JUMP) {
        mx_gen_materialize(d, e, reg);
    }
    e->kind = MX_PARSE_EXP_VALUE;
    e->arg = mx_gen_reg_arg(reg);
}

void mx_gen_to_next(mx_parse_t *d, mx_parse_exp_t *e) {
    mx_gen_discharge(d, e);
    if (e->kind == MX_PARSE_EXP_VALUE) {
        mx_gen_free_arg(d, e->arg);
    }
    mx_gen_to_reg(d, e, mx_gen_reg(d));
}

void mx_gen_to_any(mx_parse_t *d, mx_parse_exp_t *e) {
    mx_gen_discharge(d, e);
    if (e->kind == MX_PARSE_EXP_VALUE && e->arg.type == MX_IR_ARG_TYPE_REG) {
        if (!mx_gen_has_jumps(e)) {
            return;
        } else if (e->arg.reg >= mx_gen_nactive_regs(d)) {
            mx_gen_to_reg(d, e, (uint32_t)e->arg.reg);
            return;
        }
    }
    mx_gen_to_next(d, e);
}

void mx_gen_to_arg(mx_parse_t *d, mx_parse_exp_t *e) {
    mx_gen_discharge(d, e);
    if (e->kind != MX_PARSE_EXP_VALUE || mx_gen_has_jumps(e)) {
        mx_gen_to_any(d, e);
    }
}

void mx_gen_to_spread(mx_parse_t *d, mx_parse_exp_t *e) {
    if (e->kind == MX_PARSE_EXP_CALL) {
        mx_gen_call(d, e, MX_IR_WANT_MULTI);
    } else {
        mx_ir_arg_t *args = mx_gen_args(d, 1);
        args[0] = mx_gen_reg_arg(d->fn->pack_reg);
        mx_gen_instr(d, MX_IR_INSTR_OPCODE_UNPACK, mx_gen_none(), args, mx_gen_range(d, e->start, (mx_varname_t) {0}, (mx_varname_t) {0}));
        e->kind = MX_PARSE_EXP_VALUE;
        e->arg = (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_SPREAD };
    }
}

void mx_gen_multi(mx_parse_t *d, mx_parse_exp_t *e, uint32_t want) {
    if (e->kind == MX_PARSE_EXP_CALL) {
        mx_gen_call(d, e, want);
    } else {
        mx_location_range_t range = mx_gen_range(d, e->start, (mx_varname_t) {0}, (mx_varname_t) {0});
        for (uint32_t i = 0; i < want; i++) {
            mx_gen_table_get(d, mx_gen_reg_arg(d->fn->pack_reg), mx_gen_int((int64_t)i + 1), range);
        }
        e->kind = MX_PARSE_EXP_VOID;
    }
}

bool mx_gen_lit_truthy(mx_obj_t v) {
    return !mx_obj_is_nil(v) && !(mx_obj_is_boolean(v) && !mx_obj_get_boolean(v));
}

void mx_gen_go_if_true(mx_parse_t *d, mx_parse_exp_t *e) {
    mx_gen_discharge(d, e);
    if (e->kind == MX_PARSE_EXP_VALUE) {
        if (e->arg.type == MX_IR_ARG_TYPE_LIT && mx_gen_lit_truthy(e->arg.lit)) {
            e->kind = MX_PARSE_EXP_JUMP;
        } else if (e->arg.type == MX_IR_ARG_TYPE_LIT) {
            e->f = mx_gen_concat(d, e->f, mx_gen_jump_hole(d, e->arg));
            e->kind = MX_PARSE_EXP_JUMP;
        } else {
            mx_gen_bool_holes(d, e, mx_gen_none(), e->arg);
            e->kind = MX_PARSE_EXP_JUMP;
        }
    }
    mx_gen_patch_here(d, e->t);
    e->t = MX_PARSE_NO_HOLE;
}

void mx_gen_go_if_false(mx_parse_t *d, mx_parse_exp_t *e) {
    mx_gen_discharge(d, e);
    if (e->kind == MX_PARSE_EXP_VALUE) {
        if (e->arg.type == MX_IR_ARG_TYPE_LIT && !mx_gen_lit_truthy(e->arg.lit)) {
            e->kind = MX_PARSE_EXP_JUMP;
        } else if (e->arg.type == MX_IR_ARG_TYPE_LIT) {
            e->t = mx_gen_concat(d, e->t, mx_gen_jump_hole(d, e->arg));
            e->kind = MX_PARSE_EXP_JUMP;
        } else {
            mx_gen_bool_holes(d, e, e->arg, mx_gen_none());
            e->kind = MX_PARSE_EXP_JUMP;
        }
    }
    mx_gen_patch_here(d, e->f);
    e->f = MX_PARSE_NO_HOLE;
}

void mx_gen_not(mx_parse_t *d, mx_parse_exp_t *e) {
    mx_gen_discharge(d, e);
    if (e->kind == MX_PARSE_EXP_VALUE && e->arg.type == MX_IR_ARG_TYPE_LIT) {
        e->arg = mx_gen_lit(mx_obj_of_boolean(!mx_gen_lit_truthy(e->arg.lit)));
    } else if (e->kind == MX_PARSE_EXP_VALUE) {
        mx_gen_bool_holes(d, e, mx_gen_none(), mx_gen_none());
        e->kind = MX_PARSE_EXP_JUMP;
    }
    uint32_t t = e->t;
    e->t = e->f;
    e->f = t;
    mx_gen_strip_values(d, e->t);
    mx_gen_strip_values(d, e->f);
    e->name = (mx_varname_t) {0};
}

uint32_t mx_gen_cond(mx_parse_t *d, mx_parse_exp_t *e) {
    if (mx_gen_is_lit(e) && mx_obj_is_nil(e->arg.lit)) {
        e->arg = mx_gen_lit(mx_obj_of_boolean(false));
    }
    mx_gen_go_if_true(d, e);
    return e->f;
}

mx_ir_arg_t mx_gen_call_prim(mx_parse_t *d, mx_ffi_t *fn, mx_ir_arg_t *args, uint32_t nargs, uint32_t start) {
    mx_parse_call_t *call = mx_scratch_alloc(d->vm, sizeof(mx_parse_call_t));
    *call = (mx_parse_call_t) {
        .args = mx_gen_args(d, nargs + 1),
        .range = mx_gen_range(d, start, (mx_varname_t) {0}, (mx_varname_t) {0}),
        .nargs = nargs,
    };
    call->args[0] = mx_gen_lit(mx_obj_of_ffi(fn));
    for (uint32_t i = 0; i < nargs; i++) {
        call->args[i + 1] = args[i];
    }
    mx_parse_exp_t e = mx_gen_exp(MX_PARSE_EXP_CALL, start);
    e.call = call;
    mx_gen_call(d, &e, 1);
    return e.arg;
}

void mx_gen_store(mx_parse_t *d, const mx_parse_exp_t *target, mx_parse_exp_t *value) {
    switch (target->kind) {
        case MX_PARSE_EXP_LOCAL: {
            mx_gen_discharge(d, value);
            if (value->kind == MX_PARSE_EXP_VALUE) {
                mx_gen_free_arg(d, value->arg);
            }
            mx_gen_to_reg(d, value, (uint32_t)target->arg.reg);
            break;
        }
        case MX_PARSE_EXP_UPVAL: {
            mx_gen_to_arg(d, value);
            mx_ir_arg_t *args = mx_gen_args(d, 2);
            args[0] = mx_gen_int((int64_t)target->index);
            args[1] = value->arg;
            mx_gen_instr(d, MX_IR_INSTR_OPCODE_UPVAL_SET, mx_gen_none(), args, mx_gen_range(d, mx_gen_here(d), (mx_varname_t) {0}, (mx_varname_t) {0}));
            mx_gen_free_arg(d, value->arg);
            break;
        }
        default: {
            mx_gen_to_arg(d, value);
            mx_ir_arg_t *args = mx_gen_args(d, 3);
            args[0] = target->arg;
            args[1] = target->key;
            args[2] = value->arg;
            mx_gen_instr(d, MX_IR_INSTR_OPCODE_TABLE_SET, mx_gen_none(), args, mx_gen_range(d, mx_gen_here(d), target->base, (mx_varname_t) {0}));
            mx_gen_free_arg(d, value->arg);
            break;
        }
    }
}

bool mx_gen_fold_int(mx_obj_t v, int64_t *out) {
    mx_obj_int_t got = mx_obj_to_int(v);
    *out = got.value;
    return got.is_int;
}

bool mx_gen_fold_arith(mx_ir_instr_opcode_t op, mx_obj_t a, mx_obj_t b, mx_obj_t *out) {
    if (!mx_obj_is_number(a) || !mx_obj_is_number(b)) {
        return false;
    }
    switch (op) {
        case MX_IR_INSTR_OPCODE_BAND:
        case MX_IR_INSTR_OPCODE_BOR:
        case MX_IR_INSTR_OPCODE_BXOR:
        case MX_IR_INSTR_OPCODE_SHL:
        case MX_IR_INSTR_OPCODE_SHR:
        case MX_IR_INSTR_OPCODE_BNOT: {
            int64_t x;
            int64_t y;
            if (!mx_gen_fold_int(a, &x) || !mx_gen_fold_int(b, &y)) {
                return false;
            } else if (op == MX_IR_INSTR_OPCODE_BAND) {
                *out = mx_obj_of_int(x & y);
            } else if (op == MX_IR_INSTR_OPCODE_BOR) {
                *out = mx_obj_of_int(x | y);
            } else if (op == MX_IR_INSTR_OPCODE_BXOR) {
                *out = mx_obj_of_int(x ^ y);
            } else if (op == MX_IR_INSTR_OPCODE_SHL) {
                *out = mx_obj_of_int(mx_num_shl(x, y));
            } else if (op == MX_IR_INSTR_OPCODE_SHR) {
                *out = mx_obj_of_int(mx_num_shr(x, y));
            } else {
                *out = mx_obj_of_int(~x);
            }
            return true;
        }
        default: {
            break;
        }
    }
    bool is_div = op == MX_IR_INSTR_OPCODE_DIV || op == MX_IR_INSTR_OPCODE_IDIV || op == MX_IR_INSTR_OPCODE_MOD;
    if (is_div && mx_obj_get_number(b) == 0) {
        return false;
    }
    bool ints = mx_obj_is_int(a) && mx_obj_is_int(b) && op != MX_IR_INSTR_OPCODE_DIV && op != MX_IR_INSTR_OPCODE_POW;
    if (ints) {
        int64_t x = mx_obj_get_int(a);
        int64_t y = mx_obj_get_int(b);
        switch (op) {
            case MX_IR_INSTR_OPCODE_ADD: {
                *out = mx_obj_of_int((int64_t)((uint64_t)x + (uint64_t)y));
                break;
            }
            case MX_IR_INSTR_OPCODE_SUB: {
                *out = mx_obj_of_int((int64_t)((uint64_t)x - (uint64_t)y));
                break;
            }
            case MX_IR_INSTR_OPCODE_MUL: {
                *out = mx_obj_of_int((int64_t)((uint64_t)x * (uint64_t)y));
                break;
            }
            case MX_IR_INSTR_OPCODE_IDIV: {
                *out = mx_obj_of_int(mx_num_idiv(x, y));
                break;
            }
            case MX_IR_INSTR_OPCODE_MOD: {
                *out = mx_obj_of_int(mx_num_imod(x, y));
                break;
            }
            default: {
                *out = mx_obj_of_int((int64_t)(0 - (uint64_t)x));
                break;
            }
        }
        return true;
    }
    double x = mx_obj_get_number(a);
    double y = mx_obj_get_number(b);
    double r;
    switch (op) {
        case MX_IR_INSTR_OPCODE_ADD: {
            r = x + y;
            break;
        }
        case MX_IR_INSTR_OPCODE_SUB: {
            r = x - y;
            break;
        }
        case MX_IR_INSTR_OPCODE_MUL: {
            r = x * y;
            break;
        }
        case MX_IR_INSTR_OPCODE_DIV: {
            r = x / y;
            break;
        }
        case MX_IR_INSTR_OPCODE_IDIV: {
            r = floor(x / y);
            break;
        }
        case MX_IR_INSTR_OPCODE_MOD: {
            r = mx_num_fmod(x, y);
            break;
        }
        case MX_IR_INSTR_OPCODE_POW: {
            r = pow(x, y);
            break;
        }
        default: {
            r = -x;
            break;
        }
    }
    if (r != r || r == 0) {
        return false;
    } else {
        *out = mx_obj_of_number(r);
        return true;
    }
}
