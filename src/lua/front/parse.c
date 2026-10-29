
#include "parse.h"
#include "../rt/lua.h"
#include "../rt/prim.h"
#include "../../ir/scratch.h"
#include "../../value/obj.h"

mx_error_t *mx_parse_semerror(mx_parse_t *d, const char *fmt, ...) {
    mx_str_t *msg = mx_str_new();
    va_list ap;
    va_start(ap, fmt);
    mx_str_vformat(msg, fmt, ap);
    va_end(ap);
    return mx_lex_error_finish(&d->lex, msg, mx_gen_here(d));
}

mx_error_t *mx_parse_limit(mx_parse_t *d, uint32_t limit, const char *what) {
    if (d->fn->line == 0) {
        return mx_lex_error_near(&d->lex, "too many %s (limit is %u) in main function", what, limit);
    } else {
        return mx_lex_error_near(&d->lex, "too many %s (limit is %u) in function at line %u", what, limit, d->fn->line);
    }
}

mx_error_t *mx_parse_depth_enter(mx_parse_t *d) {
    d->depth += 1;
    if (d->depth > 1000) {
        return mx_lex_error_near(&d->lex, "chunk has too many syntax levels");
    } else {
        return NULL;
    }
}

void mx_parse_depth_exit(mx_parse_t *d) {
    d->depth -= 1;
}

mx_error_t *mx_parse_check_match(mx_parse_t *d, mx_lex_type_t what, mx_lex_type_t who, uint32_t open_at) {
    if (d->lex.tok.type != what) {
        return mx_lex_error_match(&d->lex, what, who, open_at);
    } else {
        return mx_lex_next(&d->lex);
    }
}

bool mx_parse_block_follow(mx_parse_t *d, bool with_until) {
    switch (d->lex.tok.type) {
        case MX_LEX_TYPE_ELSE:
        case MX_LEX_TYPE_ELSEIF:
        case MX_LEX_TYPE_END:
        case MX_LEX_TYPE_EOF: {
            return true;
        }
        case MX_LEX_TYPE_UNTIL: {
            return with_until;
        }
        default: {
            return false;
        }
    }
}

uint32_t mx_parse_line(mx_parse_t *d, uint32_t at) {
    return mx_source_line(d->source, at);
}

void mx_parse_fn_open(mx_parse_t *d, mx_parse_fn_t *fn, uint32_t line, uint32_t start) {
    *fn = (mx_parse_fn_t) {
        .prev = d->fn,
        .first_var = d->vars.len,
        .first_label = d->labels.len,
        .first_block = d->blocks.len,
        .free_reg = 1,
        .nregs = 1,
        .reg_floor = 1,
        .pack_reg = UINT32_MAX,
        .line = line,
        .last_out = UINT32_MAX,
    };
    d->fn = fn;
    fn->func = mx_ir_func_new(&d->vm->arenas.ir, d->source);
    fn->head = mx_ir_block_new(d->vm, &d->vm->arenas.ir, fn->func);
    mx_gen_track(d, fn->head);
    fn->func->start = start;
    fn->func->head = fn->head;
    fn->head->is_func = true;
    fn->cur = fn->head;
}

void mx_parse_fn_upvals(mx_parse_t *d, mx_parse_fn_t *fn) {
    mx_ir_func_t *func = fn->func;
    func->nupvals = fn->upvals.len;
    if (fn->upvals.len != 0) {
        mx_ir_upval_t *upvals = mx_ir_alloc(d->vm, sizeof(mx_ir_upval_t) * fn->upvals.len);
        mx_kind_t *seen = mx_ir_alloc(d->vm, sizeof(mx_kind_t) * fn->upvals.len);
        for (uint32_t i = 0; i < fn->upvals.len; i++) {
            upvals[i] = fn->upvals.ptr[i];
            upvals[i].name = mx_gen_name(d, fn->upvals.ptr[i].name);
            seen[i] = MX_KIND_NIL;
        }
        func->upvals = upvals;
        func->upval_seen = seen;
    }
}

mx_ir_block_t *mx_parse_fn_close(mx_parse_t *d, uint32_t stop) {
    mx_t *vm = d->vm;
    mx_parse_fn_t *fn = d->fn;
    mx_ir_block_t *head = fn->head;
    mx_ir_func_t *func = fn->func;
    func->nregs = fn->nregs;
    func->nparams = fn->nparams;
    func->is_vararg = fn->is_vararg;
    func->has_cells = fn->has_cells;
    mx_parse_fn_upvals(d, fn);
    if (fn->locals.len != 0) {
        mx_ir_local_t *locals = mx_ir_alloc(vm, sizeof(mx_ir_local_t) * fn->locals.len);
        memcpy(locals, fn->locals.ptr, sizeof(mx_ir_local_t) * fn->locals.len);
        func->locals = (mx_ir_locals_t) { .ptr = locals, .len = fn->locals.len };
    }
    func->stop = stop;
    mx_ir_blocks_seal(vm, &d->blocks, fn->first_block);
    mx_mem_free(fn->upvals.ptr);
    mx_mem_free(fn->upvals.is_const);
    mx_mem_free(fn->locals.ptr);
    d->fn = fn->prev;
    return head;
}

void mx_parse_scope_open(mx_parse_t *d, mx_parse_scope_t *scope, bool is_loop) {
    *scope = (mx_parse_scope_t) {
        .prev = d->fn->scope,
        .nvars = d->vars.len,
        .first_label = d->labels.len,
        .first_goto = d->gotos.len,
        .is_loop = is_loop,
    };
    d->fn->scope = scope;
}

uint32_t mx_parse_reg_level(mx_parse_t *d, uint32_t nvars) {
    for (uint32_t i = nvars; i > d->fn->first_var; i--) {
        if (d->vars.ptr[i - 1].reg != UINT32_MAX) {
            return d->vars.ptr[i - 1].reg + 1;
        }
    }
    return 1;
}

void mx_parse_close_regs(mx_parse_t *d, uint32_t from) {
    mx_ir_arg_t *args = mx_gen_args(d, 1);
    args[0] = mx_gen_int((int64_t)from);
    mx_gen_instr(d, MX_IR_INSTR_OPCODE_CLOSE, mx_gen_none(), args, mx_gen_range(d, mx_gen_here(d), (mx_varname_t) {0}, (mx_varname_t) {0}));
}

void mx_parse_keep(mx_parse_t *d, uint32_t from, uint32_t to) {
    if (from < to) {
        mx_ir_arg_t *args = mx_gen_args(d, 2);
        args[0] = mx_gen_int((int64_t)from);
        args[1] = mx_gen_int((int64_t)to);
        mx_gen_instr(d, MX_IR_INSTR_OPCODE_KEEP, mx_gen_none(), args, mx_gen_range(d, mx_gen_here(d), (mx_varname_t) {0}, (mx_varname_t) {0}));
    }
}

bool mx_parse_has_close(mx_parse_t *d, uint32_t from, uint32_t to) {
    for (uint32_t i = from; i < to; i++) {
        if (d->vars.ptr[i].is_close) {
            return true;
        }
    }
    return false;
}

uint32_t mx_parse_vars_level(mx_parse_t *d, uint32_t from, uint32_t to) {
    for (uint32_t i = from; i < to; i++) {
        if (d->vars.ptr[i].reg != UINT32_MAX) {
            return d->vars.ptr[i].reg;
        }
    }
    return mx_parse_reg_level(d, from);
}

void mx_parse_close_vars(mx_parse_t *d, uint32_t from, uint32_t to, bool has_capture) {
    for (uint32_t i = to; i > from; i--) {
        mx_parse_var_t *var = &d->vars.ptr[i - 1];
        if (var->is_close) {
            mx_ir_arg_t args[1] = {mx_gen_reg_arg(var->reg)};
            mx_gen_call_prim(d, mx_prim_close_call, args, 1, mx_gen_here(d));
        }
    }
    if (has_capture) {
        mx_parse_close_regs(d, mx_parse_vars_level(d, from, to));
    }
}

bool mx_parse_inside_close(mx_parse_t *d) {
    return mx_parse_has_close(d, d->fn->first_var, d->vars.len);
}

void mx_parse_tramp_close(mx_parse_t *d, mx_parse_goto_t *g, uint32_t from, uint32_t to, bool has_capture) {
    if (has_capture || mx_parse_has_close(d, from, to)) {
        mx_ir_block_t *saved = d->fn->cur;
        d->fn->cur = g->tramp;
        mx_parse_close_vars(d, from, to, has_capture);
        g->tramp = mx_gen_cur(d);
        d->fn->cur = saved;
    }
}

mx_parse_label_t *mx_parse_label_find(mx_parse_t *d, const char *name, uint32_t from) {
    for (uint32_t i = from; i < d->labels.len; i++) {
        if (d->labels.ptr[i].name == name) {
            return &d->labels.ptr[i];
        }
    }
    return NULL;
}

void mx_parse_label_push(mx_parse_t *d, mx_parse_label_t label) {
    if (d->labels.len == d->labels.cap) {
        d->labels.cap = d->labels.cap * 2 + 16;
        d->labels.ptr = mx_mem_realloc(d->labels.ptr, sizeof(mx_parse_label_t) * d->labels.cap);
    }
    d->labels.ptr[d->labels.len] = label;
    d->labels.len += 1;
}

mx_ir_block_t *mx_parse_here_block(mx_parse_t *d) {
    mx_ir_block_t *cur = d->fn->cur;
    if (cur == NULL || cur->code_len != 0) {
        cur = mx_gen_block(d);
        mx_gen_enter(d, cur);
    }
    return cur;
}

mx_error_t *mx_parse_gotos_solve(mx_parse_t *d, mx_parse_scope_t *scope) {
    bool has_close = scope->has_capture || mx_parse_has_close(d, scope->nvars, d->vars.len);
    uint32_t i = scope->first_goto;
    while (i < d->gotos.len) {
        mx_parse_goto_t *g = &d->gotos.ptr[i];
        mx_parse_label_t *label = mx_parse_label_find(d, g->name, scope->first_label);
        if (label != NULL) {
            if (g->nvars < label->nvars) {
                const char *var = d->vars.ptr[g->nvars].name;
                if (var == d->gall) {
                    var = "*";
                }
                return mx_parse_semerror(d, "<goto %s> at line %u jumps into the scope of '%s'", g->name, g->line, var);
            }
            if (has_close && label->nvars < g->nvars) {
                mx_parse_tramp_close(d, g, label->nvars, g->nvars, scope->has_capture);
            }
            mx_ir_block_t *saved = d->fn->cur;
            d->fn->cur = g->tramp;
            mx_gen_branch(d, (mx_ir_branch_t) { .op = MX_IR_BRANCH_OPCODE_JUMP, .targets = {label->block, NULL}, .range = g->range });
            d->fn->cur = saved;
            memmove(&d->gotos.ptr[i], &d->gotos.ptr[i + 1], sizeof(mx_parse_goto_t) * (d->gotos.len - i - 1));
            d->gotos.len -= 1;
        } else {
            if (g->nvars > scope->nvars) {
                mx_parse_tramp_close(d, g, scope->nvars, g->nvars, scope->has_capture);
                g->nvars = scope->nvars;
            }
            i += 1;
        }
    }
    return NULL;
}

void mx_parse_vars_pop(mx_parse_t *d, uint32_t to) {
    mx_parse_fn_t *fn = d->fn;
    while (d->vars.len > to) {
        d->vars.len -= 1;
        mx_parse_var_t *var = &d->vars.ptr[d->vars.len];
        if (var->local != UINT32_MAX) {
            fn->locals.ptr[var->local].stop = d->lex.last_stop;
        }
    }
}

mx_error_t *mx_parse_scope_close(mx_parse_t *d) {
    mx_parse_fn_t *fn = d->fn;
    mx_parse_scope_t *scope = fn->scope;
    uint32_t level = mx_parse_vars_level(d, scope->nvars, d->vars.len);
    if (fn->cur != NULL) {
        mx_parse_keep(d, level, mx_gen_nactive_regs(d));
        mx_parse_close_vars(d, scope->nvars, d->vars.len, scope->has_capture && scope->prev != NULL);
    }
    if (scope->is_loop) {
        mx_parse_label_push(
            d,
            (mx_parse_label_t) {
                .name = d->break_name,
                .block = mx_parse_here_block(d),
                .nvars = scope->nvars,
            }
        );
    }
    mx_error_t *error = mx_parse_gotos_solve(d, scope);
    if (error != NULL) {
        return error;
    }
    d->labels.len = scope->first_label;
    mx_parse_vars_pop(d, scope->nvars);
    fn->free_reg = mx_gen_nactive_regs(d);
    fn->scope = scope->prev;
    if (scope->prev == NULL && d->gotos.len > scope->first_goto) {
        mx_parse_goto_t *g = &d->gotos.ptr[scope->first_goto];
        return mx_parse_semerror(d, "no visible label '%s' for <goto> at line %u", g->name, g->line);
    }
    return NULL;
}

mx_error_t *mx_parse_var_push(mx_parse_t *d, mx_parse_var_t var) {
    if (d->vars.len == d->vars.cap) {
        d->vars.cap = d->vars.cap * 2 + 32;
        d->vars.ptr = mx_mem_realloc(d->vars.ptr, sizeof(mx_parse_var_t) * d->vars.cap);
    }
    d->vars.ptr[d->vars.len] = var;
    d->vars.len += 1;
    return NULL;
}

mx_error_t *mx_parse_var_new(mx_parse_t *d, const char *name, uint32_t reg, bool is_const, bool is_close, uint32_t start) {
    mx_parse_fn_t *fn = d->fn;
    uint32_t count = 0;
    for (uint32_t i = fn->first_var; i < d->vars.len; i++) {
        if (!d->vars.ptr[i].is_global && d->vars.ptr[i].name != d->pack_name) {
            count += 1;
        }
    }
    if (count >= 200) {
        return mx_parse_limit(d, 200, "local variables");
    }
    if (fn->locals.len == fn->locals.cap) {
        fn->locals.cap = fn->locals.cap * 2 + 8;
        fn->locals.ptr = mx_mem_realloc(fn->locals.ptr, sizeof(mx_ir_local_t) * fn->locals.cap);
    }
    uint32_t local = fn->locals.len;
    fn->locals.ptr[local] = (mx_ir_local_t) {
        .name = mx_gen_name(d, name),
        .reg = reg,
        .start = start,
        .stop = UINT32_MAX,
    };
    fn->locals.len += 1;
    return mx_parse_var_push(
        d,
        (mx_parse_var_t) {
            .name = name,
            .reg = reg,
            .local = local,
            .is_const = is_const,
            .is_close = is_close,
        }
    );
}

mx_error_t *mx_parse_var_const(mx_parse_t *d, const char *name, mx_obj_t value) {
    return mx_parse_var_push(
        d,
        (mx_parse_var_t) {
            .name = name,
            .reg = UINT32_MAX,
            .local = UINT32_MAX,
            .value = value,
            .is_const = true,
            .is_ctc = true,
        }
    );
}

mx_error_t *mx_parse_var_global(mx_parse_t *d, const char *name, bool is_const) {
    return mx_parse_var_push(
        d,
        (mx_parse_var_t) {
            .name = name,
            .reg = UINT32_MAX,
            .local = UINT32_MAX,
            .is_const = is_const,
            .is_global = true,
        }
    );
}

void mx_parse_mark_capture(mx_parse_fn_t *fn, uint32_t index) {
    mx_parse_scope_t *scope = fn->scope;
    while (scope->nvars > index) {
        scope = scope->prev;
    }
    scope->has_capture = true;
    fn->has_cells = true;
}

mx_error_t *mx_parse_upval_new(mx_parse_t *d, mx_parse_fn_t *fn, const char *name, uint32_t index, bool is_local, bool is_const, uint32_t *out) {
    if (fn->upvals.len >= 255) {
        mx_parse_fn_t *saved = d->fn;
        d->fn = fn;
        mx_error_t *error = mx_parse_limit(d, 255, "upvalues");
        d->fn = saved;
        return error;
    }
    if (fn->upvals.len == fn->upvals.cap) {
        fn->upvals.cap = fn->upvals.cap * 2 + 4;
        fn->upvals.ptr = mx_mem_realloc(fn->upvals.ptr, sizeof(mx_ir_upval_t) * fn->upvals.cap);
        fn->upvals.is_const = mx_mem_realloc(fn->upvals.is_const, sizeof(bool) * fn->upvals.cap);
    }
    *out = fn->upvals.len;
    fn->upvals.ptr[fn->upvals.len] = (mx_ir_upval_t) {
        .name = name,
        .index = index,
        .is_local = is_local,
    };
    fn->upvals.is_const[fn->upvals.len] = is_const;
    fn->upvals.len += 1;
    return NULL;
}

mx_error_t *mx_parse_resolve(mx_parse_t *d, mx_parse_fn_t *fn, const char *name, bool is_base, int64_t *coll, mx_parse_found_t *out) {
    for (uint32_t i = d->vars.len; i > fn->first_var; i--) {
        mx_parse_var_t *var = &d->vars.ptr[i - 1];
        if (var->is_global) {
            if (var->name == d->gall) {
                if (*coll < 0) {
                    *coll = (int64_t)(i - 1);
                }
            } else if (var->name == name) {
                *out = (mx_parse_found_t) { .kind = MX_PARSE_FOUND_GLOBAL, .index = i - 1, .is_const = var->is_const };
                return NULL;
            } else if (*coll == -1) {
                *coll = -2;
            }
        } else if (var->name == name) {
            if (var->is_ctc) {
                *out = (mx_parse_found_t) { .kind = MX_PARSE_FOUND_CONST, .index = i - 1, .is_const = true };
            } else {
                if (!is_base) {
                    mx_parse_mark_capture(fn, i - 1);
                }
                *out = (mx_parse_found_t) { .kind = MX_PARSE_FOUND_LOCAL, .index = i - 1, .is_const = var->is_const };
            }
            return NULL;
        }
    }
    for (uint32_t i = 0; i < fn->upvals.len; i++) {
        if (fn->upvals.ptr[i].name == name) {
            *out = (mx_parse_found_t) { .kind = MX_PARSE_FOUND_UPVAL, .index = i, .is_const = fn->upvals.is_const[i] };
            return NULL;
        }
    }
    if (fn->prev == NULL) {
        *out = (mx_parse_found_t) { .kind = MX_PARSE_FOUND_GLOBAL, .index = UINT32_MAX };
        return NULL;
    }
    mx_parse_found_t up;
    mx_error_t *error = mx_parse_resolve(d, fn->prev, name, false, coll, &up);
    if (error != NULL) {
        return error;
    } else if (up.kind == MX_PARSE_FOUND_LOCAL) {
        uint32_t index;
        error = mx_parse_upval_new(d, fn, name, d->vars.ptr[up.index].reg, true, up.is_const, &index);
        *out = (mx_parse_found_t) { .kind = MX_PARSE_FOUND_UPVAL, .index = index, .is_const = up.is_const };
        return error;
    } else if (up.kind == MX_PARSE_FOUND_UPVAL) {
        uint32_t index;
        error = mx_parse_upval_new(d, fn, name, up.index, false, up.is_const, &index);
        *out = (mx_parse_found_t) { .kind = MX_PARSE_FOUND_UPVAL, .index = index, .is_const = up.is_const };
        return error;
    } else {
        *out = up;
        return NULL;
    }
}

mx_error_t *mx_parse_var_exp(mx_parse_t *d, const char *name, uint32_t start, mx_parse_found_t found, mx_parse_exp_t *e) {
    switch (found.kind) {
        case MX_PARSE_FOUND_LOCAL: {
            *e = mx_gen_exp(MX_PARSE_EXP_LOCAL, start);
            e->arg = mx_gen_reg_arg(d->vars.ptr[found.index].reg);
            e->name = mx_gen_varname(d, name, MX_VARHINT_LOCAL);
            break;
        }
        case MX_PARSE_FOUND_CONST: {
            *e =mx_gen_exp_arg(mx_gen_lit(d->vars.ptr[found.index].value), start);
            e->name = mx_gen_varname(d, name, MX_VARHINT_NONE);
            e->is_var = true;
            break;
        }
        default: {
            *e = mx_gen_exp(MX_PARSE_EXP_UPVAL, start);
            e->index = found.index;
            e->name = mx_gen_varname(d, name, MX_VARHINT_UPVALUE);
            break;
        }
    }
    e->is_const = found.is_const;
    return NULL;
}

mx_error_t *mx_parse_env_exp(mx_parse_t *d, const char *name, uint32_t start, mx_parse_exp_t *e) {
    int64_t coll = -1;
    mx_parse_found_t found;
    mx_error_t *error = mx_parse_resolve(d, d->fn, d->env_name, true, &coll, &found);
    if (error != NULL) {
        return error;
    } else if (found.kind == MX_PARSE_FOUND_GLOBAL) {
        return mx_parse_semerror(d, "_ENV is global when accessing variable '%s'", name);
    }
    mx_parse_exp_t env;
    mx_parse_var_exp(d, d->env_name, start, found, &env);
    mx_gen_to_arg(d, &env);
    *e = mx_gen_exp(MX_PARSE_EXP_INDEX, start);
    e->arg = env.arg;
    e->key = mx_gen_lit(mx_obj_intern(d->vm, name));
    e->base = env.name;
    e->name = mx_gen_varname(d, name, MX_VARHINT_GLOBAL);
    return NULL;
}

mx_error_t *mx_parse_name_exp(mx_parse_t *d, const char *name, uint32_t start, mx_parse_exp_t *e) {
    int64_t coll = -1;
    mx_parse_found_t found;
    mx_error_t *error = mx_parse_resolve(d, d->fn, name, true, &coll, &found);
    if (error != NULL) {
        return error;
    } else if (found.kind != MX_PARSE_FOUND_GLOBAL) {
        return mx_parse_var_exp(d, name, start, found, e);
    } else if (found.index == UINT32_MAX && coll == -2) {
        return mx_parse_semerror(d, "variable '%s' not declared", name);
    }
    bool is_const;
    if (found.index != UINT32_MAX) {
        is_const = found.is_const;
    } else {
        is_const = coll >= 0 && d->vars.ptr[coll].is_const;
    }
    error = mx_parse_env_exp(d, name, start, e);
    e->is_const = is_const;
    return error;
}

mx_error_t *mx_parse_check_readonly(mx_parse_t *d, const mx_parse_exp_t *e) {
    if (e->is_const) {
        return mx_parse_semerror(d, "attempt to assign to const variable '%s'", e->name.name != NULL ? e->name.name : "?");
    } else {
        return NULL;
    }
}

bool mx_parse_is_var(const mx_parse_exp_t *e) {
    return e->kind == MX_PARSE_EXP_LOCAL || e->kind == MX_PARSE_EXP_UPVAL || e->kind == MX_PARSE_EXP_INDEX || e->is_var;
}

mx_varname_t mx_parse_operand_name(mx_parse_t *d, const mx_parse_exp_t *e) {
    if (e->name.kind != MX_VARHINT_NONE) {
        return e->name;
    } else if (e->kind == MX_PARSE_EXP_VALUE && e->arg.type == MX_IR_ARG_TYPE_LIT && mx_obj_is_string(e->arg.lit)) {
        return (mx_varname_t) {
            .name = mx_gen_name(d, mx_scratch_intern(d->vm, mx_obj_get_string(e->arg.lit)->buf)),
            .kind = MX_VARHINT_CONSTANT,
        };
    } else {
        return (mx_varname_t) {0};
    }
}

mx_error_t *mx_parse_funcargs(mx_parse_t *d, mx_parse_exp_t *e, mx_ir_arg_t self, uint32_t call_at) {
    mx_varname_t callee = e->name;
    mx_gen_to_arg(d, e);
    mx_ir_arg_t fn_arg = e->arg;
    mx_ir_arg_t fixed[256];
    uint32_t nfixed = 0;
    if (self.type != MX_IR_ARG_TYPE_NONE) {
        fixed[nfixed] = self;
        nfixed += 1;
    }
    mx_parse_exp_t last = mx_gen_exp(MX_PARSE_EXP_VOID, d->lex.tok.start);
    mx_error_t *error = NULL;
    switch (d->lex.tok.type) {
        case MX_LEX_TYPE_LPAREN: {
            uint32_t open_at = d->lex.tok.start;
            error = mx_lex_next(&d->lex);
            if (error == NULL && d->lex.tok.type != MX_LEX_TYPE_RPAREN) {
                error = mx_parse_expr(d, &last);
                while (error == NULL && d->lex.tok.type == MX_LEX_TYPE_COMMA) {
                    error = mx_lex_next(&d->lex);
                    if (error == NULL) {
                        if (nfixed >= 250) {
                            return mx_lex_error_near(&d->lex, "function or expression needs too many registers");
                        }
                        mx_gen_to_arg(d, &last);
                        fixed[nfixed] = last.arg;
                        nfixed += 1;
                        error = mx_parse_expr(d, &last);
                    }
                }
            }
            if (error == NULL) {
                error = mx_parse_check_match(d, MX_LEX_TYPE_RPAREN, MX_LEX_TYPE_LPAREN, open_at);
            }
            break;
        }
        case MX_LEX_TYPE_LBRACE: {
            error = mx_parse_constructor(d, &last);
            break;
        }
        case MX_LEX_TYPE_STRING: {
            last = mx_gen_exp_arg(mx_gen_lit(mx_lex_tok_value(&d->lex.tok)), d->lex.tok.start);
            error = mx_lex_next(&d->lex);
            break;
        }
        default: {
            error = mx_lex_error_near(&d->lex, "function arguments expected");
            break;
        }
    }
    if (error != NULL) {
        return error;
    }
    if (mx_gen_is_multi(&last)) {
        mx_gen_to_spread(d, &last);
    } else if (last.kind != MX_PARSE_EXP_VOID) {
        mx_gen_to_arg(d, &last);
    }
    uint32_t nargs = nfixed;
    if (last.kind != MX_PARSE_EXP_VOID) {
        nargs += 1;
    }
    mx_parse_call_t *call = mx_scratch_alloc(d->vm, sizeof(mx_parse_call_t));
    *call = (mx_parse_call_t) {
        .args = mx_gen_args(d, nargs + 1),
        .range = mx_gen_range(d, call_at, callee, (mx_varname_t) {0}),
        .nargs = nargs,
    };
    call->args[0] = fn_arg;
    for (uint32_t i = 0; i < nfixed; i++) {
        call->args[i + 1] = fixed[i];
    }
    if (last.kind != MX_PARSE_EXP_VOID) {
        call->args[nargs] = last.arg;
    }
    *e = mx_gen_exp(MX_PARSE_EXP_CALL, call_at);
    e->call = call;
    return NULL;
}

mx_error_t *mx_parse_primary(mx_parse_t *d, mx_parse_exp_t *e) {
    uint32_t start = d->lex.tok.start;
    switch (d->lex.tok.type) {
        case MX_LEX_TYPE_LPAREN: {
            mx_error_t *error = mx_lex_next(&d->lex);
            if (error == NULL) {
                error = mx_parse_expr(d, e);
            }
            if (error == NULL) {
                error = mx_parse_check_match(d, MX_LEX_TYPE_RPAREN, MX_LEX_TYPE_LPAREN, start);
            }
            if (error == NULL) {
                mx_gen_discharge(d, e);
            }
            return error;
        }
        case MX_LEX_TYPE_NAME: {
            const char *name = d->lex.tok.name;
            mx_error_t *error = mx_lex_next(&d->lex);
            if (error == NULL) {
                error = mx_parse_name_exp(d, name, start, e);
            }
            return error;
        }
        default: {
            return mx_lex_error_near(&d->lex, "unexpected symbol");
        }
    }
}

mx_error_t *mx_parse_field_name(mx_parse_t *d, const char **name) {
    mx_lex_name_t got = mx_lex_name(&d->lex);
    *name = got.name;
    return got.error;
}

mx_error_t *mx_parse_index_key(mx_parse_t *d, mx_parse_exp_t *key) {
    mx_error_t *error = mx_lex_next(&d->lex);
    if (error == NULL) {
        error = mx_parse_expr(d, key);
    }
    if (error == NULL) {
        mx_gen_to_arg(d, key);
        error = mx_lex_expect(&d->lex, MX_LEX_TYPE_RBRACKET);
    }
    return error;
}

void mx_parse_indexed(mx_parse_t *d, mx_parse_exp_t *e, mx_ir_arg_t key, mx_varname_t name, uint32_t at) {
    mx_varname_t base = mx_parse_operand_name(d, e);
    mx_gen_to_arg(d, e);
    mx_ir_arg_t table = e->arg;
    *e = mx_gen_exp(MX_PARSE_EXP_INDEX, at);
    e->arg = table;
    e->key = key;
    e->base = base;
    e->name = name;
}

mx_error_t *mx_parse_suffixed(mx_parse_t *d, mx_parse_exp_t *e) {
    mx_error_t *error = mx_parse_primary(d, e);
    while (error == NULL) {
        switch (d->lex.tok.type) {
            case MX_LEX_TYPE_DOT: {
                const char *field;
                error = mx_lex_next(&d->lex);
                if (error == NULL) {
                    uint32_t at = d->lex.tok.start;
                    error = mx_parse_field_name(d, &field);
                    if (error == NULL) {
                        mx_parse_indexed(d, e, mx_gen_lit(mx_obj_intern(d->vm, field)), mx_gen_varname(d, field, MX_VARHINT_FIELD), at);
                    }
                }
                break;
            }
            case MX_LEX_TYPE_LBRACKET: {
                mx_gen_to_arg(d, e);
                mx_parse_exp_t key = mx_gen_exp(MX_PARSE_EXP_VOID, d->lex.tok.start);
                error = mx_parse_index_key(d, &key);
                if (error == NULL) {
                    mx_varname_t name = {0};
                    if (key.arg.type == MX_IR_ARG_TYPE_LIT && mx_obj_is_string(key.arg.lit)) {
                        name = mx_parse_operand_name(d, &key);
                        name.kind = MX_VARHINT_FIELD;
                    }
                    mx_parse_indexed(d, e, key.arg, name, mx_gen_here(d));
                }
                break;
            }
            case MX_LEX_TYPE_COLON: {
                const char *method;
                error = mx_lex_next(&d->lex);
                uint32_t at = d->lex.tok.start;
                if (error == NULL) {
                    error = mx_parse_field_name(d, &method);
                }
                if (error == NULL) {
                    mx_varname_t base = mx_parse_operand_name(d, e);
                    mx_gen_to_any(d, e);
                    mx_ir_arg_t self = e->arg;
                    mx_ir_arg_t func = mx_gen_reg_arg(mx_gen_reg(d));
                    mx_gen_table_get_into(d, self, mx_gen_lit(mx_obj_intern(d->vm, method)), mx_gen_range(d, at, base, (mx_varname_t) {0}), (uint32_t)func.reg);
                    *e = mx_gen_exp_arg(func, at);
                    e->name = mx_gen_varname(d, method, MX_VARHINT_METHOD);
                    error = mx_parse_funcargs(d, e, self, d->lex.tok.start);
                }
                break;
            }
            case MX_LEX_TYPE_LPAREN:
            case MX_LEX_TYPE_STRING:
            case MX_LEX_TYPE_LBRACE: {
                error = mx_parse_funcargs(d, e, mx_gen_none(), d->lex.tok.start);
                break;
            }
            default: {
                return NULL;
            }
        }
    }
    return error;
}

mx_error_t *mx_parse_simple(mx_parse_t *d, mx_parse_exp_t *e) {
    uint32_t start = d->lex.tok.start;
    switch (d->lex.tok.type) {
        case MX_LEX_TYPE_NUMBER:
        case MX_LEX_TYPE_STRING: {
            *e = mx_gen_exp_arg(mx_gen_lit(mx_lex_tok_value(&d->lex.tok)), start);
            return mx_lex_next(&d->lex);
        }
        case MX_LEX_TYPE_NIL: {
            *e = mx_gen_exp_arg(mx_gen_lit(mx_obj_of_nil()), start);
            return mx_lex_next(&d->lex);
        }
        case MX_LEX_TYPE_TRUE: {
            *e = mx_gen_exp_arg(mx_gen_lit(mx_obj_of_boolean(true)), start);
            return mx_lex_next(&d->lex);
        }
        case MX_LEX_TYPE_FALSE: {
            *e = mx_gen_exp_arg(mx_gen_lit(mx_obj_of_boolean(false)), start);
            return mx_lex_next(&d->lex);
        }
        case MX_LEX_TYPE_DOTS: {
            if (!d->fn->is_vararg) {
                return mx_lex_error_near(&d->lex, "cannot use '...' outside a vararg function");
            }
            *e = mx_gen_exp(MX_PARSE_EXP_VARARG, start);
            return mx_lex_next(&d->lex);
        }
        case MX_LEX_TYPE_LBRACE: {
            return mx_parse_constructor(d, e);
        }
        case MX_LEX_TYPE_FUNCTION: {
            mx_error_t *error = mx_lex_next(&d->lex);
            if (error == NULL) {
                error = mx_parse_body(d, e, false, mx_parse_line(d, start), start);
            }
            return error;
        }
        case MX_LEX_TYPE_IF: {
            if (!mx_lua_of(d->vm)->is_ext) {
                return mx_parse_suffixed(d, e);
            }
            uint32_t reg = mx_gen_reg(d);
            mx_error_t *error = mx_parse_value_if(d, e, reg);
            *e = mx_gen_exp_arg(mx_gen_reg_arg(reg), start);
            return error;
        }
        case MX_LEX_TYPE_DO: {
            if (!mx_lua_of(d->vm)->is_ext) {
                return mx_parse_suffixed(d, e);
            }
            uint32_t reg = mx_gen_reg(d);
            mx_error_t *error = mx_parse_value_do(d, reg);
            *e = mx_gen_exp_arg(mx_gen_reg_arg(reg), start);
            return error;
        }
        default: {
            return mx_parse_suffixed(d, e);
        }
    }
}

mx_parse_prios_t mx_parse_binop_prio(mx_lex_type_t type) {
    switch (type) {
        case MX_LEX_TYPE_OR: {
            return (mx_parse_prios_t) {MX_PARSE_PRIO_OR, MX_PARSE_PRIO_OR};
        }
        case MX_LEX_TYPE_AND: {
            return (mx_parse_prios_t) {MX_PARSE_PRIO_AND, MX_PARSE_PRIO_AND};
        }
        case MX_LEX_TYPE_LT:
        case MX_LEX_TYPE_GT:
        case MX_LEX_TYPE_LE:
        case MX_LEX_TYPE_GE:
        case MX_LEX_TYPE_NE:
        case MX_LEX_TYPE_EQ2: {
            return (mx_parse_prios_t) {MX_PARSE_PRIO_CMP, MX_PARSE_PRIO_CMP};
        }
        case MX_LEX_TYPE_PIPE: {
            return (mx_parse_prios_t) {MX_PARSE_PRIO_BOR, MX_PARSE_PRIO_BOR};
        }
        case MX_LEX_TYPE_TILDE: {
            return (mx_parse_prios_t) {MX_PARSE_PRIO_BXOR, MX_PARSE_PRIO_BXOR};
        }
        case MX_LEX_TYPE_AMP: {
            return (mx_parse_prios_t) {MX_PARSE_PRIO_BAND, MX_PARSE_PRIO_BAND};
        }
        case MX_LEX_TYPE_SHL:
        case MX_LEX_TYPE_SHR: {
            return (mx_parse_prios_t) {MX_PARSE_PRIO_SHIFT, MX_PARSE_PRIO_SHIFT};
        }
        case MX_LEX_TYPE_CONCAT: {
            return (mx_parse_prios_t) {MX_PARSE_PRIO_CONCAT, MX_PARSE_PRIO_CONCAT_RIGHT};
        }
        case MX_LEX_TYPE_PLUS:
        case MX_LEX_TYPE_MINUS: {
            return (mx_parse_prios_t) {MX_PARSE_PRIO_ADD, MX_PARSE_PRIO_ADD};
        }
        case MX_LEX_TYPE_STAR:
        case MX_LEX_TYPE_SLASH:
        case MX_LEX_TYPE_SLASH2:
        case MX_LEX_TYPE_PERCENT: {
            return (mx_parse_prios_t) {MX_PARSE_PRIO_MUL, MX_PARSE_PRIO_MUL};
        }
        case MX_LEX_TYPE_CARET: {
            return (mx_parse_prios_t) {MX_PARSE_PRIO_POW, MX_PARSE_PRIO_POW_RIGHT};
        }
        default: {
            return (mx_parse_prios_t) {MX_PARSE_PRIO_NONE, MX_PARSE_PRIO_NONE};
        }
    }
}

mx_ir_instr_opcode_t mx_parse_arith_op(mx_lex_type_t type) {
    switch (type) {
        case MX_LEX_TYPE_PLUS: {
            return MX_IR_INSTR_OPCODE_ADD;
        }
        case MX_LEX_TYPE_MINUS: {
            return MX_IR_INSTR_OPCODE_SUB;
        }
        case MX_LEX_TYPE_STAR: {
            return MX_IR_INSTR_OPCODE_MUL;
        }
        case MX_LEX_TYPE_SLASH: {
            return MX_IR_INSTR_OPCODE_DIV;
        }
        case MX_LEX_TYPE_SLASH2: {
            return MX_IR_INSTR_OPCODE_IDIV;
        }
        case MX_LEX_TYPE_PERCENT: {
            return MX_IR_INSTR_OPCODE_MOD;
        }
        case MX_LEX_TYPE_CARET: {
            return MX_IR_INSTR_OPCODE_POW;
        }
        case MX_LEX_TYPE_AMP: {
            return MX_IR_INSTR_OPCODE_BAND;
        }
        case MX_LEX_TYPE_PIPE: {
            return MX_IR_INSTR_OPCODE_BOR;
        }
        case MX_LEX_TYPE_TILDE: {
            return MX_IR_INSTR_OPCODE_BXOR;
        }
        case MX_LEX_TYPE_SHL: {
            return MX_IR_INSTR_OPCODE_SHL;
        }
        case MX_LEX_TYPE_SHR: {
            return MX_IR_INSTR_OPCODE_SHR;
        }
        default: {
            return MX_IR_INSTR_OPCODE_CONCAT;
        }
    }
}

bool mx_parse_is_cmp(mx_lex_type_t op) {
    return op == MX_LEX_TYPE_LT || op == MX_LEX_TYPE_GT || op == MX_LEX_TYPE_LE || op == MX_LEX_TYPE_GE
        || op == MX_LEX_TYPE_EQ2 || op == MX_LEX_TYPE_NE;
}

void mx_parse_infix(mx_parse_t *d, mx_lex_type_t op, mx_parse_exp_t *e) {
    if (op == MX_LEX_TYPE_AND) {
        mx_gen_go_if_true(d, e);
    } else if (op == MX_LEX_TYPE_OR) {
        mx_gen_go_if_false(d, e);
    } else if (!mx_gen_is_numeral(e)) {
        mx_gen_to_arg(d, e);
    }
}

void mx_parse_compare(mx_parse_t *d, mx_lex_type_t op, mx_parse_exp_t *e1, mx_parse_exp_t *e2, uint32_t at) {
    mx_gen_to_arg(d, e1);
    mx_gen_to_arg(d, e2);
    mx_gen_free_arg(d, e2->arg);
    mx_gen_free_arg(d, e1->arg);
    mx_ir_branch_opcode_t bop;
    mx_ir_arg_t a;
    mx_ir_arg_t b;
    uint8_t tslot = 0;
    switch (op) {
        case MX_LEX_TYPE_LT: {
            bop = MX_IR_BRANCH_OPCODE_IF_LT;
            a = e1->arg;
            b = e2->arg;
            break;
        }
        case MX_LEX_TYPE_GT: {
            bop = MX_IR_BRANCH_OPCODE_IF_LT;
            a = e2->arg;
            b = e1->arg;
            break;
        }
        case MX_LEX_TYPE_LE: {
            bop = MX_IR_BRANCH_OPCODE_IF_LE;
            a = e1->arg;
            b = e2->arg;
            break;
        }
        case MX_LEX_TYPE_GE: {
            bop = MX_IR_BRANCH_OPCODE_IF_LE;
            a = e2->arg;
            b = e1->arg;
            break;
        }
        case MX_LEX_TYPE_EQ2: {
            bop = MX_IR_BRANCH_OPCODE_IF_EQ;
            a = e1->arg;
            b = e2->arg;
            break;
        }
        default: {
            bop = MX_IR_BRANCH_OPCODE_IF_EQ;
            a = e1->arg;
            b = e2->arg;
            tslot = 1;
            break;
        }
    }
    mx_ir_block_t *block = mx_gen_cur(d);
    mx_ir_arg_t *args = mx_gen_args(d, 2);
    args[0] = a;
    args[1] = b;
    mx_gen_branch(
        d,
        (mx_ir_branch_t) {
            .op = bop,
            .args = args,
            .range = mx_gen_range(d, at, (mx_varname_t) {0}, (mx_varname_t) {0}),
        }
    );
    *e1 = mx_gen_exp(MX_PARSE_EXP_JUMP, at);
    e1->t = mx_gen_hole(d, block, tslot, mx_gen_none());
    e1->f = mx_gen_hole(d, block, (uint8_t)(1 - tslot), mx_gen_none());
}

void mx_parse_posfix(mx_parse_t *d, mx_lex_type_t op, mx_parse_exp_t *e1, mx_parse_exp_t *e2, uint32_t at) {
    if (op == MX_LEX_TYPE_AND) {
        mx_gen_discharge(d, e2);
        e2->f = mx_gen_concat(d, e2->f, e1->f);
        *e1 = *e2;
        e1->name = (mx_varname_t) {0};
    } else if (op == MX_LEX_TYPE_OR) {
        mx_gen_discharge(d, e2);
        e2->t = mx_gen_concat(d, e2->t, e1->t);
        *e1 = *e2;
        e1->name = (mx_varname_t) {0};
    } else if (mx_parse_is_cmp(op)) {
        mx_parse_compare(d, op, e1, e2, at);
    } else {
        mx_ir_instr_opcode_t iop = mx_parse_arith_op(op);
        mx_obj_t folded;
        if (iop != MX_IR_INSTR_OPCODE_CONCAT && mx_gen_is_numeral(e1) && mx_gen_is_numeral(e2) && mx_gen_fold_arith(iop, e1->arg.lit, e2->arg.lit, &folded)) {
            *e1 = mx_gen_exp_arg(mx_gen_lit(folded), e1->start);
        } else {
            mx_gen_to_arg(d, e1);
            mx_gen_to_arg(d, e2);
            mx_varname_t lhs = mx_parse_operand_name(d, e1);
            mx_varname_t rhs = mx_parse_operand_name(d, e2);
            mx_gen_free_arg(d, e2->arg);
            mx_gen_free_arg(d, e1->arg);
            uint32_t reg = mx_gen_reg(d);
            mx_ir_arg_t *args = mx_gen_args(d, 2);
            args[0] = e1->arg;
            args[1] = e2->arg;
            mx_gen_instr(d, iop, mx_gen_reg_arg(reg), args, mx_gen_range(d, at, lhs, rhs));
            *e1 = mx_gen_exp_arg(mx_gen_reg_arg(reg), at);
        }
    }
}

mx_error_t *mx_parse_prefix(mx_parse_t *d, mx_lex_type_t op, mx_parse_exp_t *e, uint32_t at) {
    if (op == MX_LEX_TYPE_NOT) {
        mx_gen_not(d, e);
        return NULL;
    }
    mx_gen_discharge(d, e);
    mx_ir_instr_opcode_t iop;
    if (op == MX_LEX_TYPE_MINUS) {
        iop = MX_IR_INSTR_OPCODE_NEG;
    } else if (op == MX_LEX_TYPE_TILDE) {
        iop = MX_IR_INSTR_OPCODE_BNOT;
    } else {
        iop = MX_IR_INSTR_OPCODE_TABLE_LEN;
    }
    mx_obj_t folded;
    bool is_folded;
    if (op == MX_LEX_TYPE_MINUS && mx_gen_is_numeral(e)) {
        is_folded = mx_gen_fold_arith(MX_IR_INSTR_OPCODE_NEG, e->arg.lit, mx_obj_of_int(0), &folded);
    } else if (op == MX_LEX_TYPE_TILDE && mx_gen_is_numeral(e)) {
        is_folded = mx_gen_fold_arith(MX_IR_INSTR_OPCODE_BNOT, e->arg.lit, mx_obj_of_int(0), &folded);
    } else {
        is_folded = false;
    }
    if (is_folded) {
        *e = mx_gen_exp_arg(mx_gen_lit(folded), at);
    } else {
        mx_gen_to_arg(d, e);
        mx_varname_t name = mx_parse_operand_name(d, e);
        mx_gen_free_arg(d, e->arg);
        uint32_t reg = mx_gen_reg(d);
        mx_ir_arg_t *args = mx_gen_args(d, 1);
        args[0] = e->arg;
        mx_gen_instr(d, iop, mx_gen_reg_arg(reg), args, mx_gen_range(d, at, name, (mx_varname_t) {0}));
        *e = mx_gen_exp_arg(mx_gen_reg_arg(reg), at);
    }
    return NULL;
}

mx_error_t *mx_parse_binops(mx_parse_t *d, mx_parse_exp_t *e, mx_parse_prio_t limit) {
    mx_parse_prios_t prio = mx_parse_binop_prio(d->lex.tok.type);
    while (prio.left > limit) {
        mx_lex_type_t op = d->lex.tok.type;
        uint32_t at = d->lex.tok.start;
        mx_error_t *error = mx_lex_next(&d->lex);
        if (error != NULL) {
            return error;
        }
        mx_parse_infix(d, op, e);
        mx_parse_exp_t e2;
        error = mx_parse_subexpr(d, &e2, prio.right);
        if (error != NULL) {
            return error;
        }
        mx_parse_posfix(d, op, e, &e2, at);
        prio = mx_parse_binop_prio(d->lex.tok.type);
    }
    return NULL;
}

mx_error_t *mx_parse_subexpr(mx_parse_t *d, mx_parse_exp_t *e, mx_parse_prio_t limit) {
    mx_error_t *error = mx_parse_depth_enter(d);
    if (error != NULL) {
        return error;
    }
    mx_lex_type_t op = d->lex.tok.type;
    if (op == MX_LEX_TYPE_NOT || op == MX_LEX_TYPE_MINUS || op == MX_LEX_TYPE_HASH || op == MX_LEX_TYPE_TILDE) {
        uint32_t at = d->lex.tok.start;
        error = mx_lex_next(&d->lex);
        if (error == NULL) {
            error = mx_parse_subexpr(d, e, MX_PARSE_PRIO_UNARY);
        }
        if (error == NULL) {
            error = mx_parse_prefix(d, op, e, at);
        }
    } else {
        error = mx_parse_simple(d, e);
    }
    if (error == NULL) {
        error = mx_parse_binops(d, e, limit);
    }
    mx_parse_depth_exit(d);
    return error;
}

mx_error_t *mx_parse_expr(mx_parse_t *d, mx_parse_exp_t *e) {
    return mx_parse_subexpr(d, e, MX_PARSE_PRIO_NONE);
}

mx_error_t *mx_parse_constructor(mx_parse_t *d, mx_parse_exp_t *e) {
    uint32_t open_at = d->lex.tok.start;
    uint32_t treg = mx_gen_reg(d);
    uint32_t floor = d->fn->reg_floor;
    d->fn->reg_floor = treg + 1;
    mx_ir_arg_t *args = mx_gen_args(d, 1);
    args[0] = mx_gen_int(0);
    mx_ir_block_t *new_block = mx_gen_cur(d);
    uint32_t new_at = mx_gen_instr(d, MX_IR_INSTR_OPCODE_TABLE_NEW, mx_gen_reg_arg(treg), args, mx_gen_range(d, open_at, (mx_varname_t) {0}, (mx_varname_t) {0}));
    mx_ir_arg_t table = mx_gen_reg_arg(treg);
    mx_error_t *error = mx_lex_next(&d->lex);
    uint32_t npos = 0;
    uint32_t nhash = 0;
    mx_parse_exp_t pending = mx_gen_exp(MX_PARSE_EXP_VOID, open_at);
    mx_parse_exp_t target = mx_gen_exp(MX_PARSE_EXP_INDEX, open_at);
    target.arg = table;
    while (error == NULL && d->lex.tok.type != MX_LEX_TYPE_RBRACE) {
        if (pending.kind != MX_PARSE_EXP_VOID) {
            target.key = mx_gen_int((int64_t)npos);
            mx_gen_store(d, &target, &pending);
            pending.kind = MX_PARSE_EXP_VOID;
            d->fn->free_reg = treg + 1;
        }
        if (d->lex.tok.type == MX_LEX_TYPE_NAME && mx_lex_peek(&d->lex) == MX_LEX_TYPE_ASSIGN) {
            target.key = mx_gen_lit(mx_obj_intern(d->vm, d->lex.tok.name));
            error = mx_lex_next(&d->lex);
            if (error == NULL) {
                error = mx_lex_next(&d->lex);
            }
            mx_parse_exp_t value;
            if (error == NULL) {
                error = mx_parse_expr(d, &value);
            }
            if (error == NULL) {
                mx_gen_store(d, &target, &value);
                nhash += 1;
            }
        } else if (d->lex.tok.type == MX_LEX_TYPE_LBRACKET) {
            mx_parse_exp_t key = mx_gen_exp(MX_PARSE_EXP_VOID, d->lex.tok.start);
            error = mx_parse_index_key(d, &key);
            if (error == NULL) {
                error = mx_lex_expect(&d->lex, MX_LEX_TYPE_ASSIGN);
            }
            mx_parse_exp_t value;
            if (error == NULL) {
                error = mx_parse_expr(d, &value);
            }
            if (error == NULL) {
                target.key = key.arg;
                mx_gen_store(d, &target, &value);
                nhash += 1;
            }
        } else {
            error = mx_parse_expr(d, &pending);
            npos += 1;
        }
        d->fn->free_reg = treg + 1;
        if (error == NULL) {
            mx_lex_check_t sep = mx_lex_check(&d->lex, MX_LEX_TYPE_COMMA);
            error = sep.error;
            if (error == NULL && !sep.is_match) {
                sep = mx_lex_check(&d->lex, MX_LEX_TYPE_SEMI);
                error = sep.error;
                if (error == NULL && !sep.is_match) {
                    break;
                }
            }
        }
    }
    if (error == NULL) {
        error = mx_parse_check_match(d, MX_LEX_TYPE_RBRACE, MX_LEX_TYPE_LBRACE, open_at);
    }
    if (error != NULL) {
        d->fn->reg_floor = floor;
        return error;
    }
    if (mx_gen_is_multi(&pending)) {
        mx_gen_to_spread(d, &pending);
        mx_ir_arg_t pargs[3] = {table, mx_gen_int((int64_t)npos), pending.arg};
        mx_gen_call_prim(d, mx_prim_table_append, pargs, 3, open_at);
        npos -= 1;
    } else if (pending.kind != MX_PARSE_EXP_VOID) {
        target.key = mx_gen_int((int64_t)npos);
        mx_gen_store(d, &target, &pending);
    }
    d->fn->reg_floor = floor;
    d->fn->free_reg = treg + 1;
    if (npos > 0xffff) {
        npos = 0xffff;
    }
    if (nhash > 0xffff) {
        nhash = 0xffff;
    }
    mx_ir_block_patch_lit(new_block, new_at, mx_obj_of_int((int64_t)(((uint64_t)nhash << MX_IR_TABLE_HINT_BITS) | npos)));
    *e = mx_gen_exp_arg(table, open_at);
    return NULL;
}

void mx_parse_fn_abandon(mx_parse_t *d) {
    mx_parse_fn_t *fn = d->fn;
    mx_mem_free(fn->upvals.ptr);
    mx_mem_free(fn->upvals.is_const);
    mx_mem_free(fn->locals.ptr);
    d->vars.len = fn->first_var;
    d->fn = fn->prev;
}

void mx_parse_return(mx_parse_t *d, mx_ir_arg_t *args) {
    mx_gen_branch(
        d,
        (mx_ir_branch_t) {
            .op = MX_IR_BRANCH_OPCODE_RETURN,
            .args = args,
            .range = mx_gen_range(d, mx_gen_here(d), (mx_varname_t) {0}, (mx_varname_t) {0}),
        }
    );
}

mx_error_t *mx_parse_body(mx_parse_t *d, mx_parse_exp_t *e, bool is_method, uint32_t line, uint32_t start) {
    mx_parse_fn_t fn;
    mx_parse_fn_open(d, &fn, line, start);
    uint32_t stop;
    mx_error_t *error = mx_parse_func_body(d, &fn, is_method, start, &stop);
    if (error != NULL) {
        mx_parse_fn_abandon(d);
        return error;
    }
    mx_ir_block_t *head = mx_parse_fn_close(d, stop);
    uint32_t reg = mx_gen_reg(d);
    mx_ir_arg_t *args = mx_gen_args(d, 1);
    args[0] = mx_gen_lit(mx_obj_of_block(head));
    mx_gen_instr(d, MX_IR_INSTR_OPCODE_CLOSURE, mx_gen_reg_arg(reg), args, mx_gen_range(d, start, (mx_varname_t) {0}, (mx_varname_t) {0}));
    *e = mx_gen_exp_arg(mx_gen_reg_arg(reg), start);
    return NULL;
}

mx_error_t *mx_parse_func_body(mx_parse_t *d, mx_parse_fn_t *fn, bool is_method, uint32_t start, uint32_t *stop) {
    mx_parse_scope_t scope;
    mx_parse_scope_open(d, &scope, false);
    mx_error_t *error = mx_lex_expect(&d->lex, MX_LEX_TYPE_LPAREN);
    if (error == NULL && is_method) {
        error = mx_parse_var_new(d, d->self_name, mx_gen_reg(d), false, false, start);
        fn->nparams += 1;
    }
    const char *pack_name = NULL;
    bool is_more = error == NULL && d->lex.tok.type != MX_LEX_TYPE_RPAREN;
    while (is_more) {
        if (d->lex.tok.type == MX_LEX_TYPE_NAME) {
            const char *name = d->lex.tok.name;
            error = mx_lex_next(&d->lex);
            if (error == NULL) {
                error = mx_parse_var_new(d, name, mx_gen_reg(d), false, false, start);
                fn->nparams += 1;
            }
            if (error == NULL) {
                mx_lex_check_t comma = mx_lex_check(&d->lex, MX_LEX_TYPE_COMMA);
                error = comma.error;
                is_more = comma.is_match;
            }
        } else if (d->lex.tok.type == MX_LEX_TYPE_DOTS) {
            fn->is_vararg = true;
            error = mx_lex_next(&d->lex);
            if (error == NULL && d->lex.tok.type == MX_LEX_TYPE_NAME) {
                pack_name = d->lex.tok.name;
                error = mx_lex_next(&d->lex);
            }
            is_more = false;
        } else {
            error = mx_lex_error_near(&d->lex, "<name> or '...' expected");
        }
        is_more = is_more && error == NULL;
    }
    if (error == NULL) {
        error = mx_lex_expect(&d->lex, MX_LEX_TYPE_RPAREN);
    }
    if (error == NULL && fn->is_vararg) {
        fn->pack_reg = mx_gen_reg(d);
        mx_gen_instr(d, MX_IR_INSTR_OPCODE_VARARGS, mx_gen_reg_arg(fn->pack_reg), NULL, mx_gen_range(d, start, (mx_varname_t) {0}, (mx_varname_t) {0}));
        if (pack_name == NULL) {
            pack_name = d->pack_name;
        }
        error = mx_parse_var_new(d, pack_name, fn->pack_reg, true, false, start);
        if (error == NULL) {
            fn->locals.ptr[fn->locals.len - 1].is_pack = true;
        }
    }
    for (uint32_t i = 0; i < fn->locals.len && i < fn->nparams; i++) {
        fn->locals.ptr[i].is_param = true;
    }
    if (error == NULL) {
        error = mx_parse_statlist(d, UINT32_MAX);
    }
    *stop = d->lex.tok.stop;
    if (error == NULL) {
        error = mx_parse_check_match(d, MX_LEX_TYPE_END, MX_LEX_TYPE_FUNCTION, start);
    }
    if (error == NULL) {
        error = mx_parse_scope_close(d);
    }
    if (error == NULL && fn->cur != NULL) {
        mx_parse_return(d, NULL);
    }
    return error;
}

mx_error_t *mx_parse_block(mx_parse_t *d) {
    mx_parse_scope_t scope;
    mx_parse_scope_open(d, &scope, false);
    mx_error_t *error = mx_parse_statlist(d, UINT32_MAX);
    if (error == NULL) {
        error = mx_parse_scope_close(d);
    }
    return error;
}

mx_error_t *mx_parse_explist(mx_parse_t *d, mx_parse_exp_t *e, uint32_t *count) {
    uint32_t n = 1;
    mx_error_t *error = mx_parse_expr(d, e);
    while (error == NULL && d->lex.tok.type == MX_LEX_TYPE_COMMA) {
        error = mx_lex_next(&d->lex);
        if (error == NULL) {
            mx_gen_to_next(d, e);
            error = mx_parse_expr(d, e);
            n += 1;
        }
    }
    *count = n;
    return error;
}

void mx_parse_adjust(mx_parse_t *d, uint32_t base, uint32_t nvars, uint32_t nexps, mx_parse_exp_t *e) {
    if (mx_gen_is_multi(e)) {
        uint32_t extra;
        if (nvars + 1 > nexps) {
            extra = nvars + 1 - nexps;
        } else {
            extra = 0;
        }
        mx_gen_multi(d, e, extra);
    } else {
        if (e->kind != MX_PARSE_EXP_VOID) {
            mx_gen_to_next(d, e);
        }
        for (uint32_t i = nexps; i < nvars; i++) {
            mx_gen_move(d, mx_gen_reg(d), mx_gen_lit(mx_obj_of_nil()));
        }
    }
    d->fn->free_reg = base + nvars;
    if (d->fn->free_reg > d->fn->nregs) {
        d->fn->nregs = d->fn->free_reg;
    }
}

mx_error_t *mx_parse_retstat(mx_parse_t *d) {
    mx_ir_arg_t vals[256];
    uint32_t nvals = 0;
    bool is_close = mx_parse_inside_close(d);
    mx_error_t *error = NULL;
    if (!mx_parse_block_follow(d, true) && d->lex.tok.type != MX_LEX_TYPE_SEMI) {
        mx_parse_exp_t e;
        error = mx_parse_expr(d, &e);
        while (error == NULL && d->lex.tok.type == MX_LEX_TYPE_COMMA) {
            if (nvals >= 250) {
                return mx_parse_limit(d, 250, "returns");
            }
            error = mx_lex_next(&d->lex);
            if (error == NULL) {
                if (is_close) {
                    mx_gen_to_next(d, &e);
                } else {
                    mx_gen_to_arg(d, &e);
                }
                vals[nvals] = e.arg;
                nvals += 1;
                error = mx_parse_expr(d, &e);
            }
        }
        if (error != NULL) {
            return error;
        }
        if (e.kind == MX_PARSE_EXP_CALL && nvals == 0 && !is_close) {
            mx_gen_branch(
                d,
                (mx_ir_branch_t) {
                    .op = MX_IR_BRANCH_OPCODE_CALL,
                    .args = e.call->args,
                    .want = MX_IR_WANT_MULTI,
                    .range = e.call->range,
                }
            );
            return mx_lex_check(&d->lex, MX_LEX_TYPE_SEMI).error;
        } else if (mx_gen_is_multi(&e)) {
            mx_gen_to_spread(d, &e);
            if (is_close) {
                mx_ir_arg_t pargs[1] = {e.arg};
                mx_ir_arg_t pack = mx_gen_call_prim(d, mx_prim_pack, pargs, 1, mx_gen_here(d));
                mx_parse_close_vars(d, d->fn->first_var, d->vars.len, false);
                mx_ir_arg_t *uargs = mx_gen_args(d, 1);
                uargs[0] = pack;
                mx_gen_instr(d, MX_IR_INSTR_OPCODE_UNPACK, mx_gen_none(), uargs, mx_gen_range(d, mx_gen_here(d), (mx_varname_t) {0}, (mx_varname_t) {0}));
                is_close = false;
            }
        } else if (is_close) {
            mx_gen_to_next(d, &e);
        } else {
            mx_gen_to_arg(d, &e);
        }
        vals[nvals] = e.arg;
        nvals += 1;
    }
    if (is_close) {
        mx_parse_close_vars(d, d->fn->first_var, d->vars.len, false);
    }
    mx_parse_keep(d, 1, mx_gen_nactive_regs(d));
    mx_ir_arg_t *args = mx_gen_args(d, nvals);
    for (uint32_t i = 0; i < nvals; i++) {
        args[i] = vals[i];
    }
    mx_parse_return(d, args);
    return mx_lex_check(&d->lex, MX_LEX_TYPE_SEMI).error;
}

void mx_parse_check_conflict(mx_parse_t *d, mx_parse_exp_t *targets, uint32_t ntargets, const mx_parse_exp_t *v) {
    bool is_conflict = false;
    uint32_t extra = d->fn->free_reg;
    for (uint32_t i = 0; i < ntargets; i++) {
        mx_parse_exp_t *t = &targets[i];
        if (t->kind == MX_PARSE_EXP_INDEX) {
            if (t->arg.type == MX_IR_ARG_TYPE_REG && t->arg.reg == v->arg.reg) {
                is_conflict = true;
                t->arg = mx_gen_reg_arg(extra);
            }
            if (t->key.type == MX_IR_ARG_TYPE_REG && t->key.reg == v->arg.reg) {
                is_conflict = true;
                t->key = mx_gen_reg_arg(extra);
            }
        }
    }
    if (is_conflict) {
        mx_gen_move(d, mx_gen_reg(d), v->arg);
    }
}

mx_error_t *mx_parse_restassign(mx_parse_t *d, mx_parse_exp_t *first) {
    mx_parse_exp_t targets[200];
    uint32_t ntargets = 1;
    targets[0] = *first;
    if (!mx_parse_is_var(first)) {
        return mx_lex_error_near(&d->lex, "syntax error");
    }
    mx_error_t *error = mx_parse_check_readonly(d, first);
    while (error == NULL && d->lex.tok.type == MX_LEX_TYPE_COMMA) {
        error = mx_lex_next(&d->lex);
        mx_parse_exp_t v;
        if (error == NULL) {
            error = mx_parse_suffixed(d, &v);
        }
        if (error == NULL && !mx_parse_is_var(&v)) {
            error = mx_lex_error_near(&d->lex, "syntax error");
        }
        if (error == NULL) {
            error = mx_parse_check_readonly(d, &v);
        }
        if (error == NULL && ntargets >= 200) {
            error = mx_parse_limit(d, 200, "variables in assignment");
        }
        if (error == NULL) {
            if (v.kind == MX_PARSE_EXP_LOCAL) {
                mx_parse_check_conflict(d, targets, ntargets, &v);
            }
            targets[ntargets] = v;
            ntargets += 1;
        }
    }
    if (error == NULL) {
        error = mx_lex_expect(&d->lex, MX_LEX_TYPE_ASSIGN);
    }
    if (error != NULL) {
        return error;
    }
    uint32_t base = d->fn->free_reg;
    mx_parse_exp_t e;
    uint32_t nexps;
    error = mx_parse_explist(d, &e, &nexps);
    if (error != NULL) {
        return error;
    } else if (nexps == 1 && ntargets == 1) {
        mx_gen_discharge(d, &e);
        mx_gen_store(d, &targets[0], &e);
        return NULL;
    }
    mx_parse_adjust(d, base, ntargets, nexps, &e);
    for (uint32_t i = ntargets; i > 0; i--) {
        mx_parse_exp_t value = mx_gen_exp_arg(mx_gen_reg_arg(base + i - 1), e.start);
        mx_gen_store(d, &targets[i - 1], &value);
    }
    return NULL;
}

mx_error_t *mx_parse_exprstat(mx_parse_t *d, uint32_t value_reg, bool *produced) {
    mx_lex_type_t first = d->lex.tok.type;
    mx_parse_exp_t e;
    mx_error_t *error;
    if (value_reg != UINT32_MAX && first != MX_LEX_TYPE_NAME && first != MX_LEX_TYPE_LPAREN) {
        error = mx_parse_expr(d, &e);
    } else {
        error = mx_parse_suffixed(d, &e);
        if (error != NULL) {
            return error;
        } else if (d->lex.tok.type == MX_LEX_TYPE_ASSIGN || d->lex.tok.type == MX_LEX_TYPE_COMMA) {
            return mx_parse_restassign(d, &e);
        }
        bool is_call = e.kind == MX_PARSE_EXP_CALL;
        bool is_last = mx_parse_block_follow(d, true);
        if (is_call && (value_reg == UINT32_MAX || (!is_last && mx_parse_binop_prio(d->lex.tok.type).left == MX_PARSE_PRIO_NONE))) {
            mx_gen_call(d, &e, 0);
            return NULL;
        } else if (value_reg == UINT32_MAX) {
            return mx_lex_error_near(&d->lex, "syntax error");
        }
        error = mx_parse_binops(d, &e, MX_PARSE_PRIO_NONE);
    }
    if (error == NULL) {
        mx_gen_to_reg(d, &e, value_reg);
        *produced = true;
        if (!mx_parse_block_follow(d, true)) {
            error = mx_lex_error_near(&d->lex, "syntax error");
        }
    }
    return error;
}

mx_error_t *mx_parse_attrib(mx_parse_t *d, bool *is_const, bool *is_close) {
    mx_lex_check_t lt = mx_lex_check(&d->lex, MX_LEX_TYPE_LT);
    if (lt.error != NULL || !lt.is_match) {
        return lt.error;
    }
    const char *name;
    mx_error_t *error = mx_parse_field_name(d, &name);
    if (error == NULL) {
        error = mx_lex_expect(&d->lex, MX_LEX_TYPE_GT);
    }
    if (error != NULL) {
        return error;
    } else if (strcmp(name, "const") == 0) {
        *is_const = true;
        *is_close = false;
        return NULL;
    } else if (strcmp(name, "close") == 0) {
        *is_const = true;
        *is_close = true;
        return NULL;
    } else {
        return mx_parse_semerror(d, "unknown attribute '%s'", name);
    }
}

mx_error_t *mx_parse_localfunc(mx_parse_t *d, uint32_t start) {
    const char *name;
    mx_error_t *error = mx_parse_field_name(d, &name);
    if (error != NULL) {
        return error;
    }
    uint32_t reg = mx_gen_reg(d);
    error = mx_parse_var_new(d, name, reg, false, false, start);
    mx_parse_exp_t b;
    if (error == NULL) {
        error = mx_parse_body(d, &b, false, mx_parse_line(d, start), start);
    }
    if (error == NULL) {
        mx_gen_to_reg(d, &b, reg);
        d->fn->locals.ptr[d->vars.ptr[d->vars.len - 1].local].start = d->lex.last_stop;
    }
    return error;
}

mx_error_t *mx_parse_localstat(mx_parse_t *d) {
    const char *names[200];
    bool consts[200];
    bool closes[200];
    uint32_t nvars = 0;
    bool def_const = false;
    bool def_close = false;
    uint32_t nclose = 0;
    mx_error_t *error = mx_parse_attrib(d, &def_const, &def_close);
    bool is_more = error == NULL;
    while (is_more) {
        if (nvars >= 200) {
            return mx_parse_limit(d, 200, "local variables");
        }
        error = mx_parse_field_name(d, &names[nvars]);
        consts[nvars] = def_const;
        closes[nvars] = def_close;
        if (error == NULL) {
            error = mx_parse_attrib(d, &consts[nvars], &closes[nvars]);
        }
        if (error == NULL && closes[nvars]) {
            nclose += 1;
            if (nclose > 1) {
                error = mx_parse_semerror(d, "multiple to-be-closed variables in local list");
            }
        }
        nvars += 1;
        if (error == NULL) {
            mx_lex_check_t comma = mx_lex_check(&d->lex, MX_LEX_TYPE_COMMA);
            error = comma.error;
            is_more = comma.is_match;
        }
        is_more = is_more && error == NULL;
    }
    if (error != NULL) {
        return error;
    }
    uint32_t base = d->fn->free_reg;
    mx_parse_exp_t e = mx_gen_exp(MX_PARSE_EXP_VOID, d->lex.tok.start);
    uint32_t nexps = 0;
    mx_lex_check_t assign = mx_lex_check(&d->lex, MX_LEX_TYPE_ASSIGN);
    error = assign.error;
    if (error == NULL && assign.is_match) {
        error = mx_parse_explist(d, &e, &nexps);
    }
    if (error != NULL) {
        return error;
    }
    uint32_t nregs = nvars;
    bool is_ctc = nvars == nexps && consts[nvars - 1] && !closes[nvars - 1] && mx_gen_is_lit(&e);
    if (is_ctc) {
        nregs -= 1;
        d->fn->free_reg = base + nregs;
    } else {
        mx_parse_adjust(d, base, nvars, nexps, &e);
    }
    uint32_t at = d->lex.last_stop;
    for (uint32_t i = 0; i < nregs && error == NULL; i++) {
        error = mx_parse_var_new(d, names[i], base + i, consts[i], closes[i], at);
    }
    if (error == NULL && is_ctc) {
        error = mx_parse_var_const(d, names[nvars - 1], e.arg.lit);
    }
    for (uint32_t i = 0; i < nregs && error == NULL; i++) {
        if (closes[i]) {
            mx_ir_arg_t args[2] = {mx_gen_reg_arg(base + i), mx_gen_lit(mx_obj_intern(d->vm, names[i]))};
            mx_gen_call_prim(d, mx_prim_close_check, args, 2, at);
        }
    }
    return error;
}

mx_error_t *mx_parse_test_then(mx_parse_t *d, uint32_t value_reg, uint32_t *escape) {
    mx_error_t *error = mx_lex_next(&d->lex);
    mx_parse_exp_t cond;
    if (error == NULL) {
        error = mx_parse_expr(d, &cond);
    }
    if (error == NULL) {
        error = mx_lex_expect(&d->lex, MX_LEX_TYPE_THEN);
    }
    if (error != NULL) {
        return error;
    }
    uint32_t jf = mx_gen_cond(d, &cond);
    if (value_reg == UINT32_MAX) {
        error = mx_parse_block(d);
    } else {
        mx_parse_scope_t scope;
        mx_parse_scope_open(d, &scope, false);
        error = mx_parse_statlist(d, value_reg);
        if (error == NULL) {
            error = mx_parse_scope_close(d);
        }
    }
    if (error == NULL) {
        bool is_else = d->lex.tok.type == MX_LEX_TYPE_ELSE || d->lex.tok.type == MX_LEX_TYPE_ELSEIF;
        if ((is_else || value_reg != UINT32_MAX) && d->fn->cur != NULL) {
            *escape = mx_gen_concat(d, *escape, mx_gen_jump_hole(d, mx_gen_none()));
        }
        mx_gen_patch_here(d, jf);
    }
    return error;
}

mx_error_t *mx_parse_ifstat(mx_parse_t *d, uint32_t value_reg, uint32_t start) {
    uint32_t escape = MX_PARSE_NO_HOLE;
    mx_error_t *error = mx_parse_test_then(d, value_reg, &escape);
    while (error == NULL && d->lex.tok.type == MX_LEX_TYPE_ELSEIF) {
        error = mx_parse_test_then(d, value_reg, &escape);
    }
    if (error == NULL && d->lex.tok.type == MX_LEX_TYPE_ELSE) {
        error = mx_lex_next(&d->lex);
        if (error == NULL && value_reg == UINT32_MAX) {
            error = mx_parse_block(d);
        } else if (error == NULL) {
            mx_parse_scope_t scope;
            mx_parse_scope_open(d, &scope, false);
            error = mx_parse_statlist(d, value_reg);
            if (error == NULL) {
                error = mx_parse_scope_close(d);
            }
        }
    } else if (error == NULL && value_reg != UINT32_MAX) {
        mx_gen_move(d, value_reg, mx_gen_lit(mx_obj_of_nil()));
    }
    if (error == NULL) {
        error = mx_parse_check_match(d, MX_LEX_TYPE_END, MX_LEX_TYPE_IF, start);
    }
    if (error == NULL) {
        mx_gen_patch_here(d, escape);
    }
    return error;
}

mx_error_t *mx_parse_value_if(mx_parse_t *d, mx_parse_exp_t *e, uint32_t reg) {
    uint32_t floor = d->fn->reg_floor;
    d->fn->reg_floor = reg + 1;
    mx_error_t *error = mx_parse_ifstat(d, reg, d->lex.tok.start);
    d->fn->reg_floor = floor;
    return error;
}

mx_error_t *mx_parse_value_do(mx_parse_t *d, uint32_t reg) {
    uint32_t start = d->lex.tok.start;
    uint32_t floor = d->fn->reg_floor;
    d->fn->reg_floor = reg + 1;
    mx_error_t *error = mx_lex_next(&d->lex);
    mx_parse_scope_t scope;
    if (error == NULL) {
        mx_parse_scope_open(d, &scope, false);
        error = mx_parse_statlist(d, reg);
    }
    if (error == NULL) {
        error = mx_parse_scope_close(d);
    }
    if (error == NULL) {
        error = mx_parse_check_match(d, MX_LEX_TYPE_END, MX_LEX_TYPE_DO, start);
    }
    d->fn->reg_floor = floor;
    return error;
}

mx_error_t *mx_parse_whilestat(mx_parse_t *d, uint32_t start) {
    mx_error_t *error = mx_lex_next(&d->lex);
    if (error != NULL) {
        return error;
    }
    mx_ir_block_t *top = mx_gen_block(d);
    mx_gen_enter(d, top);
    mx_parse_exp_t cond;
    error = mx_parse_expr(d, &cond);
    if (error != NULL) {
        return error;
    }
    uint32_t exit = mx_gen_cond(d, &cond);
    mx_parse_scope_t scope;
    mx_parse_scope_open(d, &scope, true);
    error = mx_lex_expect(&d->lex, MX_LEX_TYPE_DO);
    if (error == NULL) {
        error = mx_parse_block(d);
    }
    if (error == NULL) {
        if (d->fn->cur != NULL) {
            mx_gen_jump(d, top);
        }
        error = mx_parse_check_match(d, MX_LEX_TYPE_END, MX_LEX_TYPE_WHILE, start);
    }
    if (error == NULL) {
        error = mx_parse_scope_close(d);
    }
    if (error == NULL) {
        mx_gen_patch_here(d, exit);
    }
    return error;
}

mx_error_t *mx_parse_repeatstat(mx_parse_t *d, uint32_t start) {
    mx_error_t *error = mx_lex_next(&d->lex);
    if (error != NULL) {
        return error;
    }
    mx_ir_block_t *top = mx_gen_block(d);
    mx_gen_enter(d, top);
    mx_parse_scope_t loop;
    mx_parse_scope_t body;
    mx_parse_scope_open(d, &loop, true);
    mx_parse_scope_open(d, &body, false);
    error = mx_parse_statlist(d, UINT32_MAX);
    if (error == NULL) {
        error = mx_parse_check_match(d, MX_LEX_TYPE_UNTIL, MX_LEX_TYPE_REPEAT, start);
    }
    mx_parse_exp_t cond;
    if (error == NULL) {
        error = mx_parse_expr(d, &cond);
    }
    if (error != NULL) {
        return error;
    }
    uint32_t again = mx_gen_cond(d, &cond);
    if (body.has_capture || mx_parse_has_close(d, body.nvars, d->vars.len)) {
        mx_ir_block_t *saved = d->fn->cur;
        mx_ir_block_t *closing = mx_gen_block(d);
        d->fn->cur = closing;
        mx_parse_close_vars(d, body.nvars, d->vars.len, body.has_capture);
        mx_gen_jump(d, top);
        d->fn->cur = saved;
        mx_gen_patch(d, again, closing);
    } else {
        mx_gen_patch(d, again, top);
    }
    error = mx_parse_scope_close(d);
    if (error == NULL) {
        error = mx_parse_scope_close(d);
    }
    return error;
}

void mx_parse_for_test(mx_parse_t *d, mx_ir_arg_t iter, mx_ir_arg_t stop, mx_ir_arg_t step, mx_ir_block_t *body, mx_ir_block_t *exit, uint32_t at) {
    mx_location_range_t range = mx_gen_range(d, at, (mx_varname_t) {0}, (mx_varname_t) {0});
    if (step.type == MX_IR_ARG_TYPE_LIT) {
        mx_ir_arg_t *args = mx_gen_args(d, 2);
        if (mx_obj_get_number(step.lit) < 0) {
            args[0] = stop;
            args[1] = iter;
        } else {
            args[0] = iter;
            args[1] = stop;
        }
        mx_gen_branch(d, (mx_ir_branch_t) { .op = MX_IR_BRANCH_OPCODE_IF_LE, .args = args, .targets = {body, exit}, .range = range });
    } else {
        mx_ir_block_t *rising = mx_gen_block(d);
        mx_ir_block_t *falling = mx_gen_block(d);
        mx_ir_arg_t *sargs = mx_gen_args(d, 2);
        sargs[0] = mx_gen_int(0);
        sargs[1] = step;
        mx_gen_branch(d, (mx_ir_branch_t) { .op = MX_IR_BRANCH_OPCODE_IF_LT, .args = sargs, .targets = {rising, falling}, .range = range });
        d->fn->cur = rising;
        mx_ir_arg_t *rargs = mx_gen_args(d, 2);
        rargs[0] = iter;
        rargs[1] = stop;
        mx_gen_branch(d, (mx_ir_branch_t) { .op = MX_IR_BRANCH_OPCODE_IF_LE, .args = rargs, .targets = {body, exit}, .range = range });
        d->fn->cur = falling;
        mx_ir_arg_t *fargs = mx_gen_args(d, 2);
        fargs[0] = stop;
        fargs[1] = iter;
        mx_gen_branch(d, (mx_ir_branch_t) { .op = MX_IR_BRANCH_OPCODE_IF_LE, .args = fargs, .targets = {body, exit}, .range = range });
    }
}

bool mx_parse_is_nonzero_lit(const mx_parse_exp_t *e) {
    return mx_gen_is_numeral(e) && mx_obj_get_number(e->arg.lit) != 0;
}

mx_error_t *mx_parse_fornum(mx_parse_t *d, const char *name, uint32_t start) {
    mx_parse_exp_t first;
    mx_parse_exp_t limit;
    mx_parse_exp_t step = mx_gen_exp_arg(mx_gen_int(1), start);
    mx_error_t *error = mx_lex_next(&d->lex);
    if (error == NULL) {
        error = mx_parse_expr(d, &first);
    }
    if (error == NULL) {
        mx_gen_to_arg(d, &first);
        error = mx_lex_expect(&d->lex, MX_LEX_TYPE_COMMA);
    }
    if (error == NULL) {
        error = mx_parse_expr(d, &limit);
    }
    if (error == NULL) {
        mx_gen_to_arg(d, &limit);
        mx_lex_check_t comma = mx_lex_check(&d->lex, MX_LEX_TYPE_COMMA);
        error = comma.error;
        if (error == NULL && comma.is_match) {
            error = mx_parse_expr(d, &step);
            if (error == NULL) {
                mx_gen_to_arg(d, &step);
            }
        }
    }
    if (error == NULL) {
        error = mx_lex_expect(&d->lex, MX_LEX_TYPE_DO);
    }
    if (error != NULL) {
        return error;
    }
    uint32_t base = d->fn->free_reg;
    mx_ir_arg_t stop_reg = mx_gen_reg_arg(base);
    mx_ir_arg_t step_reg = mx_gen_reg_arg(base + 1);
    mx_ir_arg_t iter = mx_gen_reg_arg(base + 2);
    d->fn->free_reg = base + 3;
    if (d->fn->nregs < base + 3) {
        d->fn->nregs = base + 3;
    }
    const char *state = mx_scratch_intern(d->vm, "(for state)");
    error = mx_parse_var_new(d, state, base, false, false, d->lex.last_stop);
    if (error == NULL) {
        error = mx_parse_var_new(d, state, base + 1, false, false, d->lex.last_stop);
    }
    if (error == NULL) {
        error = mx_parse_var_push(d, (mx_parse_var_t) { .name = state, .reg = base + 2, .local = UINT32_MAX });
    }
    if (error != NULL) {
        return error;
    }
    bool is_step_lit = mx_parse_is_nonzero_lit(&step);
    mx_ir_arg_t step_arg;
    if (is_step_lit) {
        step_arg = step.arg;
    } else {
        mx_ir_arg_t args[1] = {step.arg};
        step_arg = mx_gen_call_prim(d, mx_prim_forprep_step, args, 1, start);
    }
    mx_gen_move(d, (uint32_t)step_reg.reg, step_arg);
    if (!is_step_lit) {
        step_arg = step_reg;
    }
    if (is_step_lit && mx_parse_is_nonzero_lit(&first)) {
        mx_obj_t v = first.arg.lit;
        if (!mx_obj_is_int(v) || !mx_obj_is_int(step.arg.lit)) {
            v = mx_obj_of_number(mx_obj_get_number(v));
        }
        mx_gen_move(d, (uint32_t)iter.reg, mx_gen_lit(v));
    } else {
        mx_ir_arg_t args[2] = {first.arg, step_arg};
        mx_gen_move(d, (uint32_t)iter.reg, mx_gen_call_prim(d, mx_prim_forprep_initial, args, 2, start));
    }
    bool is_unit = is_step_lit && mx_obj_is_int(step.arg.lit) && (mx_obj_get_int(step.arg.lit) == 1 || mx_obj_get_int(step.arg.lit) == -1);
    mx_ir_arg_t stop_arg;
    if (is_unit && mx_gen_is_numeral(&limit) && mx_obj_is_int(limit.arg.lit)) {
        stop_arg = limit.arg;
        mx_gen_move(d, (uint32_t)stop_reg.reg, stop_arg);
    } else {
        mx_ir_arg_t args[3] = {limit.arg, iter, step_arg};
        mx_gen_move(d, (uint32_t)stop_reg.reg, mx_gen_call_prim(d, mx_prim_forprep_limit, args, 3, start));
        stop_arg = stop_reg;
    }
    d->fn->free_reg = base + 3;
    mx_ir_block_t *body = mx_gen_block(d);
    mx_ir_block_t *exit = mx_gen_block(d);
    mx_parse_for_test(d, iter, stop_arg, step_arg, body, exit, start);
    d->fn->cur = body;
    mx_parse_scope_t scope;
    mx_parse_scope_open(d, &scope, false);
    error = mx_parse_var_new(d, name, base + 2, true, false, d->lex.last_stop);
    if (error == NULL) {
        error = mx_parse_block(d);
    }
    if (error == NULL) {
        error = mx_parse_scope_close(d);
    }
    if (error != NULL) {
        return error;
    }
    if (d->fn->cur != NULL) {
        mx_ir_block_t *advance = mx_gen_block(d);
        mx_ir_arg_t *eargs = mx_gen_args(d, 2);
        eargs[0] = iter;
        eargs[1] = stop_arg;
        mx_gen_branch(d, (mx_ir_branch_t) { .op = MX_IR_BRANCH_OPCODE_IF_EQ, .args = eargs, .targets = {exit, advance}, .range = mx_gen_range(d, start, (mx_varname_t) {0}, (mx_varname_t) {0}) });
        d->fn->cur = advance;
        mx_ir_arg_t *aargs = mx_gen_args(d, 2);
        aargs[0] = iter;
        aargs[1] = step_arg;
        mx_gen_instr(d, MX_IR_INSTR_OPCODE_ADD, iter, aargs, mx_gen_range(d, start, (mx_varname_t) {0}, (mx_varname_t) {0}));
        mx_parse_for_test(d, iter, stop_arg, step_arg, body, exit, start);
    }
    d->fn->cur = exit;
    return NULL;
}

mx_error_t *mx_parse_forlist(mx_parse_t *d, const char *first, uint32_t start) {
    const char *names[200];
    uint32_t nnames = 1;
    names[0] = first;
    mx_error_t *error = NULL;
    while (error == NULL && d->lex.tok.type == MX_LEX_TYPE_COMMA) {
        error = mx_lex_next(&d->lex);
        if (error == NULL && nnames >= 200) {
            error = mx_parse_limit(d, 200, "local variables");
        }
        if (error == NULL) {
            error = mx_parse_field_name(d, &names[nnames]);
            nnames += 1;
        }
    }
    if (error == NULL) {
        error = mx_lex_expect(&d->lex, MX_LEX_TYPE_IN);
    }
    uint32_t base = d->fn->free_reg;
    mx_parse_exp_t e;
    uint32_t nexps;
    if (error == NULL) {
        error = mx_parse_explist(d, &e, &nexps);
    }
    if (error == NULL) {
        error = mx_lex_expect(&d->lex, MX_LEX_TYPE_DO);
    }
    if (error != NULL) {
        return error;
    }
    mx_parse_adjust(d, base, 4, nexps, &e);
    const char *state = mx_scratch_intern(d->vm, "(for state)");
    uint32_t at = d->lex.last_stop;
    error = mx_parse_var_new(d, state, base, false, false, at);
    if (error == NULL) {
        error = mx_parse_var_new(d, state, base + 1, false, false, at);
    }
    if (error == NULL) {
        error = mx_parse_var_new(d, state, base + 3, false, true, at);
    }
    if (error == NULL) {
        error = mx_parse_var_push(d, (mx_parse_var_t) { .name = state, .reg = base + 2, .local = UINT32_MAX });
    }
    if (error != NULL) {
        return error;
    }
    mx_ir_arg_t cargs[2] = {mx_gen_reg_arg(base + 3), mx_gen_lit(mx_obj_intern(d->vm, "(for state)"))};
    mx_gen_call_prim(d, mx_prim_close_check, cargs, 2, start);
    d->fn->free_reg = base + 4;
    mx_ir_block_t *top = mx_gen_block(d);
    mx_ir_block_t *exit = mx_gen_block(d);
    mx_gen_enter(d, top);
    mx_parse_call_t *call = mx_scratch_alloc(d->vm, sizeof(mx_parse_call_t));
    *call = (mx_parse_call_t) {
        .args = mx_gen_args(d, 3),
        .range = mx_gen_range(d, start, mx_gen_varname(d, mx_scratch_intern(d->vm, "for iterator"), MX_VARHINT_FOR_ITERATOR), (mx_varname_t) {0}),
        .nargs = 2,
    };
    call->args[0] = mx_gen_reg_arg(base);
    call->args[1] = mx_gen_reg_arg(base + 1);
    call->args[2] = mx_gen_reg_arg(base + 2);
    mx_parse_exp_t ce = mx_gen_exp(MX_PARSE_EXP_CALL, start);
    ce.call = call;
    mx_gen_call(d, &ce, nnames);
    uint32_t first_reg = base + 4;
    mx_parse_scope_t scope;
    mx_parse_scope_open(d, &scope, false);
    at = d->lex.last_stop;
    for (uint32_t i = 0; i < nnames && error == NULL; i++) {
        error = mx_parse_var_new(d, names[i], first_reg + i, i == 0, false, at);
    }
    if (error != NULL) {
        return error;
    }
    mx_ir_block_t *cont = mx_gen_block(d);
    mx_ir_arg_t *eargs = mx_gen_args(d, 2);
    eargs[0] = mx_gen_reg_arg(first_reg);
    eargs[1] = mx_gen_lit(mx_obj_of_nil());
    mx_gen_branch(d, (mx_ir_branch_t) { .op = MX_IR_BRANCH_OPCODE_IF_EQ, .args = eargs, .targets = {exit, cont}, .range = mx_gen_range(d, start, (mx_varname_t) {0}, (mx_varname_t) {0}) });
    d->fn->cur = cont;
    mx_gen_move(d, base + 2, mx_gen_reg_arg(first_reg));
    error = mx_parse_block(d);
    if (error == NULL) {
        error = mx_parse_scope_close(d);
    }
    if (error != NULL) {
        return error;
    }
    if (d->fn->cur != NULL) {
        mx_gen_jump(d, top);
    }
    d->fn->cur = exit;
    return NULL;
}

mx_error_t *mx_parse_forstat(mx_parse_t *d, uint32_t start) {
    mx_parse_scope_t scope;
    mx_parse_scope_open(d, &scope, true);
    mx_error_t *error = mx_lex_next(&d->lex);
    const char *name = NULL;
    if (error == NULL) {
        error = mx_parse_field_name(d, &name);
    }
    if (error == NULL) {
        if (d->lex.tok.type == MX_LEX_TYPE_ASSIGN) {
            error = mx_parse_fornum(d, name, start);
        } else if (d->lex.tok.type == MX_LEX_TYPE_COMMA || d->lex.tok.type == MX_LEX_TYPE_IN) {
            error = mx_parse_forlist(d, name, start);
        } else {
            error = mx_lex_error_near(&d->lex, "'=' or 'in' expected");
        }
    }
    if (error == NULL) {
        error = mx_parse_check_match(d, MX_LEX_TYPE_END, MX_LEX_TYPE_FOR, start);
    }
    if (error == NULL) {
        error = mx_parse_scope_close(d);
    }
    return error;
}

mx_error_t *mx_parse_funcstat(mx_parse_t *d, uint32_t start) {
    mx_error_t *error = mx_lex_next(&d->lex);
    uint32_t at = d->lex.tok.start;
    const char *name = NULL;
    mx_parse_exp_t v;
    if (error == NULL) {
        error = mx_parse_field_name(d, &name);
    }
    if (error == NULL) {
        error = mx_parse_name_exp(d, name, at, &v);
    }
    bool is_method = false;
    while (error == NULL && (d->lex.tok.type == MX_LEX_TYPE_DOT || d->lex.tok.type == MX_LEX_TYPE_COLON) && !is_method) {
        is_method = d->lex.tok.type == MX_LEX_TYPE_COLON;
        error = mx_lex_next(&d->lex);
        at = d->lex.tok.start;
        const char *field;
        if (error == NULL) {
            error = mx_parse_field_name(d, &field);
        }
        if (error == NULL) {
            mx_varhint_t kind;
            if (is_method) {
                kind = MX_VARHINT_METHOD;
            } else {
                kind = MX_VARHINT_FIELD;
            }
            mx_parse_indexed(d, &v, mx_gen_lit(mx_obj_intern(d->vm, field)), mx_gen_varname(d, field, kind), at);
        }
    }
    if (error == NULL) {
        error = mx_parse_check_readonly(d, &v);
    }
    mx_parse_exp_t b;
    if (error == NULL) {
        error = mx_parse_body(d, &b, is_method, mx_parse_line(d, start), start);
    }
    if (error == NULL) {
        mx_gen_store(d, &v, &b);
    }
    return error;
}

mx_error_t *mx_parse_labelstat(mx_parse_t *d, uint32_t start) {
    const char *name;
    mx_error_t *error = mx_parse_field_name(d, &name);
    if (error == NULL) {
        error = mx_lex_expect(&d->lex, MX_LEX_TYPE_LABEL);
    }
    while (error == NULL && (d->lex.tok.type == MX_LEX_TYPE_SEMI || d->lex.tok.type == MX_LEX_TYPE_LABEL)) {
        bool produced = false;
        error = mx_parse_statement(d, UINT32_MAX, &produced);
    }
    if (error != NULL) {
        return error;
    }
    mx_parse_label_t *old = mx_parse_label_find(d, name, d->fn->first_label);
    if (old != NULL) {
        return mx_parse_semerror(d, "label '%s' already defined on line %u", name, old->line);
    }
    uint32_t nvars;
    if (mx_parse_block_follow(d, false)) {
        nvars = d->fn->scope->nvars;
    } else {
        nvars = d->vars.len;
    }
    mx_parse_label_push(
        d,
        (mx_parse_label_t) {
            .name = name,
            .block = mx_parse_here_block(d),
            .line = mx_parse_line(d, start),
            .nvars = nvars,
        }
    );
    return NULL;
}

void mx_parse_goto_push(mx_parse_t *d, const char *name, uint32_t start) {
    mx_ir_block_t *tramp = mx_gen_block(d);
    if (d->fn->cur != NULL) {
        mx_parse_keep(d, 1, mx_gen_nactive_regs(d));
        mx_gen_jump(d, tramp);
    }
    if (d->gotos.len == d->gotos.cap) {
        d->gotos.cap = d->gotos.cap * 2 + 16;
        d->gotos.ptr = mx_mem_realloc(d->gotos.ptr, sizeof(mx_parse_goto_t) * d->gotos.cap);
    }
    d->gotos.ptr[d->gotos.len] = (mx_parse_goto_t) {
        .name = name,
        .tramp = tramp,
        .range = mx_gen_range(d, start, (mx_varname_t) {0}, (mx_varname_t) {0}),
        .line = mx_parse_line(d, start),
        .nvars = d->vars.len,
    };
    d->gotos.len += 1;
    d->fn->cur = NULL;
}

mx_error_t *mx_parse_breakstat(mx_parse_t *d, uint32_t start) {
    mx_parse_scope_t *scope = d->fn->scope;
    while (scope != NULL && !scope->is_loop) {
        scope = scope->prev;
    }
    if (scope == NULL) {
        return mx_lex_error_near(&d->lex, "break outside loop");
    }
    mx_error_t *error = mx_lex_next(&d->lex);
    if (error == NULL) {
        mx_parse_goto_push(d, d->break_name, start);
    }
    return error;
}

mx_error_t *mx_parse_gotostat(mx_parse_t *d, uint32_t start) {
    const char *name;
    mx_error_t *error = mx_parse_field_name(d, &name);
    if (error == NULL) {
        mx_parse_goto_push(d, name, start);
    }
    return error;
}

mx_error_t *mx_parse_checkglobal(mx_parse_t *d, mx_parse_exp_t *target, const char *name, uint32_t at) {
    mx_ir_arg_t value = mx_gen_table_get(d, target->arg, target->key, mx_gen_range(d, at, target->base, (mx_varname_t) {0}));
    mx_ir_arg_t args[2] = {value, mx_gen_lit(mx_obj_intern(d->vm, name))};
    mx_gen_call_prim(d, mx_prim_global_fresh, args, 2, at);
    return NULL;
}

mx_error_t *mx_parse_global_attrib(mx_parse_t *d, bool *is_const) {
    bool is_close = false;
    mx_error_t *error = mx_parse_attrib(d, is_const, &is_close);
    if (error == NULL && is_close) {
        error = mx_parse_semerror(d, "global variables cannot be to-be-closed");
    }
    return error;
}

mx_error_t *mx_parse_globalstat(mx_parse_t *d, uint32_t start) {
    mx_error_t *error = mx_lex_next(&d->lex);
    if (error != NULL) {
        return error;
    }
    if (d->lex.tok.type == MX_LEX_TYPE_FUNCTION) {
        const char *name;
        error = mx_lex_next(&d->lex);
        uint32_t at = d->lex.tok.start;
        if (error == NULL) {
            error = mx_parse_field_name(d, &name);
        }
        if (error == NULL) {
            error = mx_parse_var_global(d, name, false);
        }
        mx_parse_exp_t target;
        if (error == NULL) {
            error = mx_parse_env_exp(d, name, at, &target);
        }
        mx_parse_exp_t b;
        if (error == NULL) {
            error = mx_parse_body(d, &b, false, mx_parse_line(d, start), start);
        }
        if (error == NULL) {
            error = mx_parse_checkglobal(d, &target, name, start);
        }
        if (error == NULL) {
            mx_gen_store(d, &target, &b);
        }
        return error;
    }
    bool def_const = false;
    error = mx_parse_global_attrib(d, &def_const);
    if (error != NULL) {
        return error;
    }
    mx_lex_check_t star = mx_lex_check(&d->lex, MX_LEX_TYPE_STAR);
    if (star.error != NULL) {
        return star.error;
    } else if (star.is_match) {
        return mx_parse_var_global(d, d->gall, def_const);
    }
    const char *names[200];
    bool consts[200];
    uint32_t nnames = 0;
    bool is_more = true;
    while (is_more) {
        if (nnames >= 200) {
            return mx_parse_limit(d, 200, "global variables");
        }
        consts[nnames] = def_const;
        error = mx_parse_field_name(d, &names[nnames]);
        if (error == NULL) {
            error = mx_parse_global_attrib(d, &consts[nnames]);
        }
        nnames += 1;
        if (error == NULL) {
            mx_lex_check_t comma = mx_lex_check(&d->lex, MX_LEX_TYPE_COMMA);
            error = comma.error;
            is_more = comma.is_match;
        }
        is_more = is_more && error == NULL;
    }
    if (error != NULL) {
        return error;
    }
    mx_lex_check_t assign = mx_lex_check(&d->lex, MX_LEX_TYPE_ASSIGN);
    if (assign.error != NULL) {
        return assign.error;
    } else if (assign.is_match) {
        mx_parse_exp_t targets[200];
        for (uint32_t i = 0; i < nnames && error == NULL; i++) {
            error = mx_parse_env_exp(d, names[i], start, &targets[i]);
        }
        uint32_t base = d->fn->free_reg;
        mx_parse_exp_t e;
        uint32_t nexps;
        if (error == NULL) {
            error = mx_parse_explist(d, &e, &nexps);
        }
        if (error != NULL) {
            return error;
        }
        mx_parse_adjust(d, base, nnames, nexps, &e);
        for (uint32_t i = nnames; i > 0 && error == NULL; i--) {
            error = mx_parse_checkglobal(d, &targets[i - 1], names[i - 1], start);
            mx_parse_exp_t value = mx_gen_exp_arg(mx_gen_reg_arg(base + i - 1), start);
            mx_gen_store(d, &targets[i - 1], &value);
        }
    }
    for (uint32_t i = 0; i < nnames && error == NULL; i++) {
        error = mx_parse_var_global(d, names[i], consts[i]);
    }
    return error;
}

mx_error_t *mx_parse_statement(mx_parse_t *d, uint32_t value_reg, bool *produced) {
    mx_error_t *error = mx_parse_depth_enter(d);
    if (error != NULL) {
        return error;
    }
    mx_arena_mark_t mark = mx_scratch_mark(d->vm);
    uint32_t start = d->lex.tok.start;
    switch (d->lex.tok.type) {
        case MX_LEX_TYPE_SEMI: {
            error = mx_lex_next(&d->lex);
            break;
        }
        case MX_LEX_TYPE_IF: {
            if (value_reg != UINT32_MAX) {
                *produced = true;
            }
            error = mx_parse_ifstat(d, value_reg, start);
            break;
        }
        case MX_LEX_TYPE_WHILE: {
            error = mx_parse_whilestat(d, start);
            break;
        }
        case MX_LEX_TYPE_DO: {
            if (value_reg != UINT32_MAX) {
                *produced = true;
                error = mx_parse_value_do(d, value_reg);
            } else {
                error = mx_lex_next(&d->lex);
                if (error == NULL) {
                    error = mx_parse_block(d);
                }
                if (error == NULL) {
                    error = mx_parse_check_match(d, MX_LEX_TYPE_END, MX_LEX_TYPE_DO, start);
                }
            }
            break;
        }
        case MX_LEX_TYPE_FOR: {
            error = mx_parse_forstat(d, start);
            break;
        }
        case MX_LEX_TYPE_REPEAT: {
            error = mx_parse_repeatstat(d, start);
            break;
        }
        case MX_LEX_TYPE_FUNCTION: {
            error = mx_parse_funcstat(d, start);
            break;
        }
        case MX_LEX_TYPE_LOCAL: {
            error = mx_lex_next(&d->lex);
            if (error == NULL && d->lex.tok.type == MX_LEX_TYPE_FUNCTION) {
                error = mx_lex_next(&d->lex);
                if (error == NULL) {
                    error = mx_parse_localfunc(d, start);
                }
            } else if (error == NULL) {
                error = mx_parse_localstat(d);
            }
            break;
        }
        case MX_LEX_TYPE_LABEL: {
            error = mx_lex_next(&d->lex);
            if (error == NULL) {
                error = mx_parse_labelstat(d, start);
            }
            break;
        }
        case MX_LEX_TYPE_RETURN: {
            error = mx_lex_next(&d->lex);
            if (error == NULL) {
                error = mx_parse_retstat(d);
            }
            break;
        }
        case MX_LEX_TYPE_BREAK: {
            error = mx_parse_breakstat(d, start);
            break;
        }
        case MX_LEX_TYPE_GOTO: {
            error = mx_lex_next(&d->lex);
            if (error == NULL) {
                error = mx_parse_gotostat(d, start);
            }
            break;
        }
        default: {
            if (mx_lex_is_global(&d->lex)) {
                error = mx_parse_globalstat(d, start);
            } else {
                error = mx_parse_exprstat(d, value_reg, produced);
            }
            break;
        }
    }
    d->fn->free_reg = mx_gen_nactive_regs(d);
    mx_scratch_release(d->vm, mark);
    mx_parse_depth_exit(d);
    return error;
}

mx_error_t *mx_parse_statlist(mx_parse_t *d, uint32_t value_reg) {
    if (value_reg != UINT32_MAX) {
        mx_gen_move(d, value_reg, mx_gen_lit(mx_obj_of_nil()));
    }
    while (!mx_parse_block_follow(d, true)) {
        bool is_return = d->lex.tok.type == MX_LEX_TYPE_RETURN;
        bool produced = false;
        mx_error_t *error = mx_parse_statement(d, value_reg, &produced);
        if (error != NULL) {
            return error;
        } else if (is_return) {
            return NULL;
        } else if (value_reg != UINT32_MAX && !produced && mx_parse_block_follow(d, true) && d->fn->cur != NULL) {
            mx_gen_move(d, value_reg, mx_gen_lit(mx_obj_of_nil()));
        }
    }
    return NULL;
}

void mx_parse_init(mx_parse_t *d, mx_t *vm, const mx_source_t *source) {
    *d = (mx_parse_t) {
        .vm = vm,
        .source = source,
    };
    mx_lex_init(&d->lex, vm, source);
    d->env_name = mx_scratch_intern(vm, "_ENV");
    d->break_name = mx_scratch_intern(vm, "break");
    d->self_name = mx_scratch_intern(vm, "self");
    d->pack_name = mx_scratch_intern(vm, "(vararg table)");
    d->gall = mx_scratch_intern(vm, "*");
}

void mx_parse_free(mx_parse_t *d) {
    for (uint32_t i = 0; i < d->blocks.len; i++) {
        mx_mem_free(d->blocks.ptr[i]->code);
        d->blocks.ptr[i]->code = NULL;
        d->blocks.ptr[i]->code_len = 0;
    }
    mx_mem_free(d->blocks.ptr);
    mx_mem_free(d->vars.ptr);
    mx_mem_free(d->holes.ptr);
    mx_mem_free(d->gotos.ptr);
    mx_mem_free(d->labels.ptr);
    mx_mem_free(d->names.from);
    mx_mem_free(d->names.to);
    mx_lex_free(&d->lex);
}

mx_parse_compiled_t mx_parse_chunk(mx_t *vm, const char *src, uint32_t len, const char *file) {
    mx_parse_t d;
    mx_parse_init(&d, vm, mx_source_new(vm, file, src, len));
    mx_parse_fn_t fn;
    mx_parse_scope_t scope;
    mx_parse_fn_open(&d, &fn, 0, 0);
    uint32_t env_index;
    mx_parse_upval_new(&d, &fn, d.env_name, 0, false, false, &env_index);
    mx_parse_scope_open(&d, &scope, false);
    fn.is_vararg = true;
    fn.pack_reg = mx_gen_reg(&d);
    mx_gen_instr(&d, MX_IR_INSTR_OPCODE_VARARGS, mx_gen_reg_arg(fn.pack_reg), NULL, mx_gen_range(&d, 0, (mx_varname_t) {0}, (mx_varname_t) {0}));
    mx_error_t *error = mx_parse_var_new(&d, d.pack_name, fn.pack_reg, false, false, 0);
    if (error == NULL) {
        fn.locals.ptr[0].is_pack = true;
        error = mx_lex_next(&d.lex);
    }
    if (error == NULL) {
        error = mx_parse_statlist(&d, UINT32_MAX);
    }
    if (error == NULL && d.lex.tok.type != MX_LEX_TYPE_EOF) {
        error = mx_lex_error_expected(&d.lex, MX_LEX_TYPE_EOF);
    }
    if (error == NULL) {
        error = mx_parse_scope_close(&d);
    }
    mx_ir_block_t *entry = NULL;
    if (error == NULL) {
        if (fn.cur != NULL) {
            mx_parse_return(&d, NULL);
        }
        entry = mx_parse_fn_close(&d, len);
        entry->is_main = true;
    } else {
        while (d.fn != NULL) {
            mx_parse_fn_abandon(&d);
        }
    }
    mx_parse_free(&d);
    return (mx_parse_compiled_t) { .entry = entry, .error = error };
}
