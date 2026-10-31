
#include "../api/std.h"
#include "../lua.h"
#include "../meta.h"
#include "../trace.h"
#include "../../../vm/gc.h"
#include "../../../ir/ir.h"
#include "../../../value/error.h"
#include "../../../value/obj.h"
#include "../../../value/table.h"

mx_std_debug_target_t mx_std_debug_target(mx_std_state_t *ls) {
    mx_obj_t *at = mx_std_at(ls, 1);
    if (at != NULL && at->kind == ls->vm->kinds.thread) {
        return (mx_std_debug_target_t) { .th = mx_obj_get_thread(*at), .arg = 1 };
    } else {
        return (mx_std_debug_target_t) { .th = ls->th, .arg = 0 };
    }
}

mx_frame_t mx_std_debug_frame(mx_thread_t *th, int64_t level) {
    if (level < 0 || level > UINT32_MAX) {
        return (mx_frame_t) { .is_valid = false };
    } else {
        return mx_frame_at_level(th, (uint32_t)level);
    }
}

bool mx_std_debug_validopt(const char *what) {
    for (const char *p = what; *p != '\0'; p++) {
        if (strchr("SlnrutfL", *p) == NULL) {
            return false;
        }
    }
    return true;
}

void mx_std_debug_info_source(mx_std_state_t *ls, const mx_ir_block_t *head) {
    const mx_ir_func_t *func = head->func;
    const char *file;
    if (func->source != NULL && func->source->file != NULL) {
        file = func->source->file;
    } else {
        file = "=?";
    }
    mx_std_pushstring(ls, file);
    mx_std_setfield(ls, -2, "source");
    mx_std_pushstring(ls, mx_error_chunkid(file).id);
    mx_std_setfield(ls, -2, "short_src");
    int64_t first;
    int64_t last;
    if (head->is_main) {
        mx_std_pushstring(ls, "main");
        first = 0;
        last = 0;
    } else {
        mx_std_pushstring(ls, "Lua");
        first = mx_source_line(func->source, func->start);
        last = mx_source_line(func->source, func->stop - 1);
    }
    mx_std_setfield(ls, -2, "what");
    mx_std_pushinteger(ls, first);
    mx_std_setfield(ls, -2, "linedefined");
    mx_std_pushinteger(ls, last);
    mx_std_setfield(ls, -2, "lastlinedefined");
}

void mx_std_debug_info_c(mx_std_state_t *ls) {
    mx_std_pushstring(ls, "=[C]");
    mx_std_setfield(ls, -2, "source");
    mx_std_pushstring(ls, "[C]");
    mx_std_setfield(ls, -2, "short_src");
    mx_std_pushstring(ls, "C");
    mx_std_setfield(ls, -2, "what");
    mx_std_pushinteger(ls, -1);
    mx_std_setfield(ls, -2, "linedefined");
    mx_std_pushinteger(ls, -1);
    mx_std_setfield(ls, -2, "lastlinedefined");
}

void mx_std_debug_activelines(mx_std_state_t *ls, mx_ir_block_t *head) {
    mx_ir_blocks_t todo = { .ptr = mx_mem_alloc(sizeof(mx_ir_block_t *) * 16), .len = 1, .cap = 16 };
    todo.ptr[0] = head;
    head->is_marked = true;
    for (uint32_t i = 0; i < todo.len; i++) {
        mx_ir_block_t *block = todo.ptr[i];
        mx_ir_cursor_t cur = { .off = 0 };
        for (mx_ir_step_t step = mx_ir_block_next(block, block->code_len, &cur); step.is_valid; step = mx_ir_block_next(block, block->code_len, &cur)) {
            if (step.instr.op != MX_IR_INSTR_OPCODE_KEEP && step.instr.op != MX_IR_INSTR_OPCODE_VARARGS) {
                mx_std_pushboolean(ls, true);
                mx_std_rawseti(ls, -2, mx_source_line(block->func->source, step.instr.range.start));
            }
        }
        if (block->branch_op != MX_IR_BRANCH_OPCODE_FALL && block->branch_op != MX_IR_BRANCH_OPCODE_JUMP) {
            mx_ir_branch_t br = mx_ir_block_branch(block);
            mx_std_pushboolean(ls, true);
            mx_std_rawseti(ls, -2, mx_source_line(block->func->source, br.range.start));
        }
        for (uint32_t t = 0; t < 2; t++) {
            mx_ir_block_t *target = block->branch_targets[t];
            if (target != NULL && !target->is_marked) {
                target->is_marked = true;
                if (todo.len == todo.cap) {
                    todo.cap *= 2;
                    todo.ptr = mx_mem_realloc(todo.ptr, sizeof(mx_ir_block_t *) * todo.cap);
                }
                todo.ptr[todo.len] = target;
                todo.len += 1;
            }
        }
    }
    for (uint32_t i = 0; i < todo.len; i++) {
        todo.ptr[i]->is_marked = false;
    }
    mx_mem_free(todo.ptr);
    if (!head->is_main) {
        mx_std_pushboolean(ls, true);
        mx_std_rawseti(ls, -2, mx_source_line(head->func->source, head->func->stop - 1));
    }
}

uint32_t mx_std_debug_nups(mx_t *vm, mx_obj_t fn) {
    if (mx_obj_is_closure(fn)) {
        return mx_obj_get_closure(fn)->len;
    } else if (fn.kind == vm->kinds.cfn) {
        return ((mx_obj_cfn_t *)mx_obj_data_head(fn))->len;
    } else {
        return 0;
    }
}

int32_t mx_std_debug_getinfo(mx_std_state_t *ls) {
    mx_t *vm = ls->vm;
    mx_std_debug_target_t tg = mx_std_debug_target(ls);
    const char *what = mx_std_optstring(ls, tg.arg + 2, "flnSrtu").ptr;
    if (vm->std_error.is_set) {
        return 0;
    } else if (what[0] == '>') {
        return mx_std_argerror(ls, tg.arg + 2, "invalid option '>'");
    }
    mx_frame_t f;
    mx_obj_t fn;
    if (mx_std_isfunction(ls, tg.arg + 1)) {
        f = (mx_frame_t) { .is_valid = false };
        fn = *mx_std_at(ls, tg.arg + 1);
    } else if (mx_std_isnumber(ls, tg.arg + 1)) {
        f = mx_std_debug_frame(tg.th, mx_std_checkinteger(ls, tg.arg + 1));
        if (vm->std_error.is_set) {
            return 0;
        } else if (!f.is_valid) {
            mx_std_pushnil(ls);
            return 1;
        }
        fn = mx_frame_fn(&f);
    } else {
        return mx_std_argerror(ls, tg.arg + 1, "function or level expected");
    }
    if (!mx_std_debug_validopt(what)) {
        return mx_std_argerror(ls, tg.arg + 2, "invalid option");
    }
    mx_ir_block_t *head;
    if (mx_obj_is_closure(fn)) {
        head = mx_obj_get_closure(fn)->block;
    } else {
        head = NULL;
    }
    mx_std_newtable(ls);
    if (strchr(what, 'S') != NULL && head != NULL) {
        mx_std_debug_info_source(ls, head);
    } else if (strchr(what, 'S') != NULL) {
        mx_std_debug_info_c(ls);
    }
    if (strchr(what, 'l') != NULL) {
        int64_t line;
        if (f.is_valid && f.is_lua) {
            line = mx_frame_line(&f);
        } else {
            line = -1;
        }
        mx_std_pushinteger(ls, line);
        mx_std_setfield(ls, -2, "currentline");
    }
    if (strchr(what, 'u') != NULL) {
        mx_std_pushinteger(ls, mx_std_debug_nups(vm, fn));
        mx_std_setfield(ls, -2, "nups");
        if (head != NULL) {
            mx_std_pushinteger(ls, head->func->nparams);
            mx_std_setfield(ls, -2, "nparams");
            mx_std_pushboolean(ls, head->func->is_vararg);
        } else {
            mx_std_pushinteger(ls, 0);
            mx_std_setfield(ls, -2, "nparams");
            mx_std_pushboolean(ls, true);
        }
        mx_std_setfield(ls, -2, "isvararg");
    }
    if (strchr(what, 'n') != NULL) {
        mx_trace_name_t named;
        if (f.is_valid) {
            named = mx_trace_namewhat(&f);
        } else {
            named = (mx_trace_name_t) { .what = NULL, .name = NULL };
        }
        if (named.what != NULL) {
            mx_std_pushstring(ls, named.name);
            mx_std_setfield(ls, -2, "name");
            mx_std_pushstring(ls, named.what);
        } else {
            mx_std_pushstring(ls, "");
        }
        mx_std_setfield(ls, -2, "namewhat");
    }
    if (strchr(what, 't') != NULL) {
        mx_std_pushboolean(ls, f.is_valid && f.is_lua && mx_link_is_tail(f.th->slots[f.p].bits));
        mx_std_setfield(ls, -2, "istailcall");
        int64_t extra;
        if (f.is_valid && f.is_lua) {
            extra = mx_link_extra(f.th->slots[f.p].bits);
        } else {
            extra = 0;
        }
        mx_std_pushinteger(ls, extra);
        mx_std_setfield(ls, -2, "extraargs");
    }
    if (strchr(what, 'r') != NULL) {
        mx_std_pushinteger(ls, 0);
        mx_std_setfield(ls, -2, "ftransfer");
        mx_std_pushinteger(ls, 0);
        mx_std_setfield(ls, -2, "ntransfer");
    }
    if (strchr(what, 'L') != NULL && head != NULL) {
        mx_std_newtable(ls);
        mx_std_debug_activelines(ls, head);
        mx_std_setfield(ls, -2, "activelines");
    }
    if (strchr(what, 'f') != NULL) {
        mx_std_push(ls, fn);
        mx_std_setfield(ls, -2, "func");
    }
    return 1;
}

const mx_ir_local_t *mx_std_debug_param(const mx_ir_func_t *func, int64_t n) {
    int64_t seen = 0;
    for (uint32_t i = 0; i < func->locals.len; i++) {
        if (func->locals.ptr[i].is_param) {
            seen += 1;
            if (seen == n) {
                return &func->locals.ptr[i];
            }
        }
    }
    return NULL;
}

mx_obj_table_t *mx_std_debug_varargs(mx_t *vm, const mx_frame_t *f, int64_t i) {
    const mx_ir_func_t *func = mx_frame_func(f)->func;
    for (uint32_t l = 0; l < func->locals.len; l++) {
        if (func->locals.ptr[l].is_pack) {
            mx_obj_t pack = mx_frame_reg(f, func->locals.ptr[l].reg);
            if (!mx_obj_is_table(pack)) {
                return NULL;
            }
            mx_obj_t count;
            if (mx_ir_local_is_hidden_pack(&func->locals.ptr[l])) {
                count = mx_obj_of_int((int64_t)mx_obj_get_table(pack)->array_len);
            } else {
                count = mx_table_get(mx_obj_get_table(pack), mx_std_char_str(vm, 'n'));
            }
            if (mx_obj_is_int(count) && i >= 1 && i <= mx_obj_get_int(count)) {
                return mx_obj_get_table(pack);
            } else {
                return NULL;
            }
        }
    }
    return NULL;
}

bool mx_std_debug_is_self(mx_std_state_t *ls, const mx_frame_t *f) {
    return f->th == ls->th && f->p + 4 == ls->base;
}

int32_t mx_std_debug_getlocal(mx_std_state_t *ls) {
    mx_t *vm = ls->vm;
    mx_std_debug_target_t tg = mx_std_debug_target(ls);
    int64_t n = mx_std_checkinteger(ls, tg.arg + 2);
    if (vm->std_error.is_set) {
        return 0;
    } else if (mx_std_isfunction(ls, tg.arg + 1)) {
        mx_obj_t fn = *mx_std_at(ls, tg.arg + 1);
        const mx_ir_local_t *param;
        if (mx_obj_is_closure(fn)) {
            param = mx_std_debug_param(mx_obj_get_closure(fn)->block->func, n);
        } else {
            param = NULL;
        }
        if (param == NULL) {
            mx_std_pushnil(ls);
        } else {
            mx_std_pushstring(ls, param->name);
        }
        return 1;
    }
    mx_frame_t f = mx_std_debug_frame(tg.th, mx_std_checkinteger(ls, tg.arg + 1));
    if (vm->std_error.is_set) {
        return 0;
    } else if (!f.is_valid) {
        return mx_std_argerror(ls, tg.arg + 1, "level out of range");
    } else if (!f.is_lua && mx_std_debug_is_self(ls, &f) && n >= 1 && n <= ls->top) {
        mx_std_pushstring(ls, "(C temporary)");
        mx_std_pushvalue(ls, (mx_std_slot_t)n);
        return 2;
    } else if (!f.is_lua) {
        mx_std_pushnil(ls);
        return 1;
    } else if (n < 0) {
        mx_obj_table_t *pack = mx_std_debug_varargs(vm, &f, -n);
        if (pack == NULL) {
            mx_std_pushnil(ls);
            return 1;
        }
        mx_std_pushstring(ls, "(vararg)");
        mx_std_push(ls, mx_table_get(pack, mx_obj_of_int(-n)));
        return 2;
    }
    const mx_ir_local_t *local = mx_frame_local(&f, n);
    if (local == NULL) {
        mx_std_pushnil(ls);
        return 1;
    }
    mx_std_pushstring(ls, local->name);
    if (mx_ir_local_is_hidden_pack(local)) {
        mx_std_pushnil(ls);
    } else {
        mx_std_push(ls, mx_frame_reg(&f, local->reg));
    }
    return 2;
}

int32_t mx_std_debug_setlocal(mx_std_state_t *ls) {
    mx_t *vm = ls->vm;
    mx_std_debug_target_t tg = mx_std_debug_target(ls);
    int64_t level = mx_std_checkinteger(ls, tg.arg + 1);
    int64_t n = mx_std_checkinteger(ls, tg.arg + 2);
    if (vm->std_error.is_set) {
        return 0;
    }
    mx_frame_t f = mx_std_debug_frame(tg.th, level);
    if (!f.is_valid) {
        return mx_std_argerror(ls, tg.arg + 1, "level out of range");
    }
    mx_std_checkany(ls, tg.arg + 3);
    if (vm->std_error.is_set) {
        return 0;
    }
    mx_obj_t v = *mx_std_at(ls, tg.arg + 3);
    if (!f.is_lua && mx_std_debug_is_self(ls, &f) && n >= 1 && n <= ls->top) {
        mx_std_copy(ls, tg.arg + 3, (mx_std_slot_t)n);
        mx_std_pushstring(ls, "(C temporary)");
    } else if (!f.is_lua) {
        mx_std_pushnil(ls);
    } else if (n < 0) {
        mx_obj_table_t *pack = mx_std_debug_varargs(vm, &f, -n);
        if (pack == NULL) {
            mx_std_pushnil(ls);
        } else {
            mx_table_set(vm, pack, mx_obj_of_int(-n), v);
            mx_std_pushstring(ls, "(vararg)");
        }
    } else {
        const mx_ir_local_t *local = mx_frame_local(&f, n);
        if (local != NULL && local->is_pack) {
            mx_std_pushstring(ls, local->name);
        } else if (local == NULL || !mx_frame_set_reg(vm, &f, local->reg, v)) {
            mx_std_pushnil(ls);
        } else {
            mx_std_pushstring(ls, local->name);
        }
    }
    return 1;
}

const char *mx_std_debug_upname(mx_t *vm, mx_obj_t fn, int64_t n) {
    if (n < 1 || n > mx_std_debug_nups(vm, fn)) {
        return NULL;
    } else if (mx_obj_is_closure(fn)) {
        const char *name = mx_obj_get_closure(fn)->block->func->upvals[n - 1].name;
        if (name == NULL) {
            return "(no name)";
        } else {
            return name;
        }
    } else {
        return "";
    }
}

int32_t mx_std_debug_getupvalue(mx_std_state_t *ls) {
    int64_t n = mx_std_checkinteger(ls, 2);
    mx_std_checktype(ls, 1, MX_LUA_TYPE_FUNCTION);
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    mx_obj_t fn = *mx_std_at(ls, 1);
    const char *name = mx_std_debug_upname(ls->vm, fn, n);
    if (name == NULL) {
        return 0;
    }
    mx_std_pushstring(ls, name);
    if (mx_obj_is_closure(fn)) {
        mx_std_push(ls, mx_upval_get(mx_obj_get_closure(fn)->upvals[n - 1]));
    } else {
        mx_std_push(ls, ((mx_obj_cfn_t *)mx_obj_data_head(fn))->values[n - 1]);
    }
    return 2;
}

int32_t mx_std_debug_setupvalue(mx_std_state_t *ls) {
    mx_t *vm = ls->vm;
    mx_std_checkany(ls, 3);
    int64_t n = mx_std_checkinteger(ls, 2);
    mx_std_checktype(ls, 1, MX_LUA_TYPE_FUNCTION);
    if (vm->std_error.is_set) {
        return 0;
    }
    mx_obj_t fn = *mx_std_at(ls, 1);
    const char *name = mx_std_debug_upname(vm, fn, n);
    if (name == NULL) {
        return 0;
    }
    mx_obj_t v = *mx_std_at(ls, 3);
    if (mx_obj_is_closure(fn)) {
        mx_upval_set(vm, mx_obj_get_closure(fn)->upvals[n - 1], v);
    } else {
        mx_obj_cfn_t *cfn = (mx_obj_cfn_t *)mx_obj_data_head(fn);
        mx_gc_write_barrier(vm, cfn, v);
        cfn->values[n - 1] = v;
    }
    mx_std_pushstring(ls, name);
    return 1;
}

void *mx_std_debug_upid(mx_std_state_t *ls, mx_std_slot_t fslot, mx_std_slot_t nslot) {
    int64_t n = mx_std_checkinteger(ls, nslot);
    mx_std_checktype(ls, fslot, MX_LUA_TYPE_FUNCTION);
    if (ls->vm->std_error.is_set) {
        return NULL;
    }
    mx_obj_t fn = *mx_std_at(ls, fslot);
    if (n < 1 || n > mx_std_debug_nups(ls->vm, fn)) {
        return NULL;
    } else if (mx_obj_is_closure(fn)) {
        mx_obj_upval_t *uv = mx_obj_get_closure(fn)->upvals[n - 1];
        mx_gc_note_ident_key(ls->vm, uv);
        return uv;
    } else {
        mx_obj_cfn_t *cfn = (mx_obj_cfn_t *)mx_obj_data_head(fn);
        mx_gc_note_ident_key(ls->vm, cfn);
        return &cfn->values[n - 1];
    }
}

int32_t mx_std_debug_upvalueid(mx_std_state_t *ls) {
    void *id = mx_std_debug_upid(ls, 1, 2);
    if (id == NULL) {
        mx_std_pushnil(ls);
    } else {
        mx_std_push(ls, mx_obj_of_lightuserdata(id));
    }
    return 1;
}

int32_t mx_std_debug_upvaluejoin(mx_std_state_t *ls) {
    mx_t *vm = ls->vm;
    if (mx_std_debug_upid(ls, 1, 2) == NULL) {
        return mx_std_argerror(ls, 2, "invalid upvalue index");
    } else if (mx_std_debug_upid(ls, 3, 4) == NULL) {
        return mx_std_argerror(ls, 4, "invalid upvalue index");
    }
    mx_obj_t f1 = *mx_std_at(ls, 1);
    mx_obj_t f2 = *mx_std_at(ls, 3);
    if (!mx_obj_is_closure(f1)) {
        return mx_std_argerror(ls, 1, "Lua function expected");
    } else if (!mx_obj_is_closure(f2)) {
        return mx_std_argerror(ls, 3, "Lua function expected");
    }
    mx_obj_closure_t *cl = mx_obj_get_closure(f1);
    mx_obj_upval_t *uv = mx_obj_get_closure(f2)->upvals[mx_std_checkinteger(ls, 4) - 1];
    mx_gc_write_barrier(vm, cl, mx_obj_of_data(uv));
    cl->upvals[mx_std_checkinteger(ls, 2) - 1] = uv;
    return 0;
}

int32_t mx_std_debug_traceback(mx_std_state_t *ls) {
    mx_std_debug_target_t tg = mx_std_debug_target(ls);
    mx_std_str_t msg = mx_std_tostring(ls, tg.arg + 1);
    if (msg.ptr == NULL && !mx_std_isnoneornil(ls, tg.arg + 1)) {
        mx_std_pushvalue(ls, tg.arg + 1);
        return 1;
    }
    int64_t def;
    if (tg.th == ls->th) {
        def = 1;
    } else {
        def = 0;
    }
    int64_t level = mx_std_optinteger(ls, tg.arg + 2, def);
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    mx_str_t *out = mx_str_new();
    if (msg.ptr != NULL) {
        mx_str_append(out, msg.ptr, (uint32_t)msg.len);
        mx_str_format(out, "\n");
    }
    uint32_t from;
    if (level >= 0 && level < UINT32_MAX) {
        from = (uint32_t)level;
    } else {
        from = UINT32_MAX;
    }
    mx_trace_append(ls->vm, out, tg.th, from);
    mx_std_pushlstring(ls, out->buf, out->len);
    mx_str_free(out);
    return 1;
}

int32_t mx_std_debug_debug(mx_std_state_t *ls) {
    char line[4096];
    while (true) {
        fputs("lua_debug> ", stderr);
        fflush(stderr);
        if (fgets(line, sizeof(line), stdin) == NULL || strcmp(line, "cont\n") == 0) {
            return 0;
        }
        int32_t top = ls->top;
        if (mx_std_load_chunk(ls, line, strlen(line), "=(debug command)", "bt", mx_lua_of(ls->vm)->std) != 1 || mx_std_pcall(ls, 0, 0, 0) != MX_STD_OK) {
            mx_std_tostring_meta(ls, -1);
            fprintf(stderr, "%s\n", mx_std_tostring(ls, -1).ptr);
        }
        ls->top = top;
    }
}

mx_obj_t mx_std_debug_hook_fire(mx_t *vm, const char *event, int64_t line) {
    mx_lua_hook_t *hook = &mx_lua_of(vm)->hook;
    mx_obj_t at;
    if (line >= 0) {
        at = mx_obj_of_int(line);
    } else {
        at = mx_obj_of_nil();
    }
    mx_obj_t args[2] = { mx_obj_intern(vm, event), at };
    hook->is_in_hook = true;
    mx_obj_t got = mx_call(vm, hook->fn, 2, args);
    hook->is_in_hook = false;
    if (mx_obj_is_error(got)) {
        return got;
    } else {
        return mx_obj_of_nil();
    }
}

void mx_std_debug_hook_returned(mx_lua_hook_t *hook, mx_thread_t *th) {
    mx_frame_iter_t it = mx_frame_iter(th);
    mx_frame_t f = mx_frame_next(&it);
    hook->thread = th;
    hook->last_frame = MX_THREAD_NONE;
    if (f.tag == MX_LINK_ENTRY || f.tag == MX_LINK_BOTTOM) {
        return;
    }
    mx_frame_t up = mx_frame_next(&it);
    while (up.is_valid && !up.is_lua) {
        up = mx_frame_next(&it);
    }
    if (up.is_valid) {
        hook->last_frame = up.p;
        hook->last_line = mx_frame_line(&up);
        hook->is_resumed = true;
    }
}

mx_obj_t mx_std_debug_hook_step(mx_t *vm, mx_thread_t *th, const mx_resume_t *at) {
    mx_lua_hook_t *hook = &mx_lua_of(vm)->hook;
    mx_frame_t f = mx_frame_at_level(th, 0);
    int64_t line = mx_frame_line(&f);
    uint64_t pos = ((uint64_t)at->target->id << 32) | at->resume_at;
    bool is_new = th != hook->thread || f.p != hook->last_frame;
    bool is_back = !hook->is_resumed && pos <= hook->last_at;
    bool is_line = (hook->mask & MX_LUA_HOOK_LINE) != 0 && line > 0 && (is_new || is_back || line != hook->last_line);
    hook->thread = th;
    hook->last_frame = f.p;
    hook->last_line = line;
    hook->last_at = pos;
    hook->is_resumed = false;
    if ((hook->mask & MX_LUA_HOOK_COUNT) != 0) {
        hook->counter -= 1;
        if (hook->counter <= 0) {
            hook->counter = hook->count;
            mx_obj_t err = mx_std_debug_hook_fire(vm, "count", -1);
            if (mx_obj_is_error(err)) {
                return err;
            }
        }
    }
    if (is_line) {
        return mx_std_debug_hook_fire(vm, "line", line);
    } else {
        return mx_obj_of_nil();
    }
}

mx_obj_t mx_std_debug_hook(mx_t *vm, mx_thread_t *th, const mx_resume_t *at, uint32_t event) {
    mx_lua_hook_t *hook = &mx_lua_of(vm)->hook;
    if (hook->is_in_hook || mx_obj_is_nil(hook->fn) || !th->has_hook) {
        return mx_obj_of_nil();
    }
    switch (event) {
        case 0: {
            return mx_std_debug_hook_step(vm, th, at);
        }
        case 1: {
            const char *name;
            if (hook->is_tail) {
                name = "tail call";
            } else {
                name = "call";
            }
            hook->is_tail = false;
            hook->thread = th;
            hook->last_frame = MX_THREAD_NONE;
            if ((hook->mask & MX_LUA_HOOK_CALL) != 0) {
                return mx_std_debug_hook_fire(vm, name, -1);
            } else {
                return mx_obj_of_nil();
            }
        }
        case 2: {
            mx_obj_t err;
            if ((hook->mask & MX_LUA_HOOK_RETURN) != 0) {
                err = mx_std_debug_hook_fire(vm, "return", -1);
            } else {
                err = mx_obj_of_nil();
            }
            mx_std_debug_hook_returned(hook, th);
            return err;
        }
        default: {
            hook->is_tail = true;
            return mx_obj_of_nil();
        }
    }
}

uint32_t mx_std_debug_flag(char c) {
    switch (c) {
        case 'c': {
            return MX_LUA_HOOK_CALL;
        }
        case 'r': {
            return MX_LUA_HOOK_RETURN;
        }
        case 'l': {
            return MX_LUA_HOOK_LINE;
        }
        default: {
            return 0;
        }
    }
}

int32_t mx_std_debug_sethook(mx_std_state_t *ls) {
    mx_t *vm = ls->vm;
    mx_std_debug_target_t tg = mx_std_debug_target(ls);
    mx_lua_hook_t *hook = &mx_lua_of(vm)->hook;
    mx_obj_t fn;
    uint32_t mask;
    int64_t count;
    if (mx_std_isnoneornil(ls, tg.arg + 1)) {
        fn = mx_obj_of_nil();
        mask = 0;
        count = 0;
    } else {
        const char *smask = mx_std_checkstring(ls, tg.arg + 2).ptr;
        mx_std_checktype(ls, tg.arg + 1, MX_LUA_TYPE_FUNCTION);
        count = mx_std_optinteger(ls, tg.arg + 3, 0);
        if (vm->std_error.is_set) {
            return 0;
        }
        if (count > 0) {
            mask = MX_LUA_HOOK_COUNT;
        } else {
            mask = 0;
        }
        for (const char *p = smask; *p != '\0'; p++) {
            mask |= mx_std_debug_flag(*p);
        }
        if (mask == 0) {
            fn = mx_obj_of_nil();
        } else {
            fn = *mx_std_at(ls, tg.arg + 1);
        }
    }
    hook->fn = fn;
    tg.th->has_hook = mask != 0;
    hook->mask = mask;
    hook->count = count;
    hook->counter = count;
    hook->is_tail = false;
    hook->thread = NULL;
    hook->last_frame = MX_THREAD_NONE;
    vm->hooks.debug = mx_std_debug_hook;
    vm->is_hooked = mask != 0;
    vm->interrupt = 1;
    return 0;
}

int32_t mx_std_debug_gethook(mx_std_state_t *ls) {
    mx_lua_hook_t *hook = &mx_lua_of(ls->vm)->hook;
    if (mx_obj_is_nil(hook->fn) || !mx_std_debug_target(ls).th->has_hook) {
        mx_std_pushnil(ls);
        return 1;
    }
    char mask[3];
    uint32_t n = 0;
    for (const char *p = "crl"; *p != '\0'; p++) {
        if ((hook->mask & mx_std_debug_flag(*p)) != 0) {
            mask[n] = *p;
            n += 1;
        }
    }
    mx_std_push(ls, hook->fn);
    mx_std_pushlstring(ls, mask, n);
    mx_std_pushinteger(ls, hook->count);
    return 3;
}

int32_t mx_std_debug_getuservalue(mx_std_state_t *ls) {
    int64_t n = mx_std_optinteger(ls, 2, 1);
    mx_obj_t *at = mx_std_at(ls, 1);
    if (at == NULL || !mx_std_udata_is(ls->vm, *at) || n < 1 || (uint64_t)n > mx_std_udata_nuv(*at)) {
        mx_std_pushnil(ls);
        return 1;
    }
    mx_std_push(ls, mx_table_get(mx_obj_get_table(*at), mx_obj_of_int(n)));
    mx_std_pushboolean(ls, true);
    return 2;
}

int32_t mx_std_debug_setuservalue(mx_std_state_t *ls) {
    mx_obj_t *at = mx_std_at(ls, 1);
    if (at != NULL && mx_obj_is_lightuserdata(*at)) {
        return mx_std_typeerror(ls, 1, "userdata");
    }
    mx_std_checktype(ls, 1, MX_LUA_TYPE_USERDATA);
    mx_std_checkany(ls, 2);
    int64_t n = mx_std_optinteger(ls, 3, 1);
    if (ls->vm->std_error.is_set) {
        return 0;
    } else if (!mx_std_udata_is(ls->vm, *at) || n < 1 || (uint64_t)n > mx_std_udata_nuv(*at)) {
        mx_std_pushnil(ls);
    } else {
        mx_table_set(ls->vm, mx_obj_get_table(*at), mx_obj_of_int(n), *mx_std_at(ls, 2));
        mx_std_pushvalue(ls, 1);
    }
    return 1;
}

int32_t mx_std_debug_getmetatable(mx_std_state_t *ls) {
    mx_std_checkany(ls, 1);
    if (!ls->vm->std_error.is_set && !mx_std_getmetatable(ls, 1)) {
        mx_std_pushnil(ls);
    }
    return 1;
}

int32_t mx_std_debug_setmetatable(mx_std_state_t *ls) {
    mx_lua_type_t t = mx_std_type(ls, 2);
    if (t != MX_LUA_TYPE_NIL && t != MX_LUA_TYPE_TABLE) {
        return mx_std_typeerror(ls, 2, "nil or table");
    }
    mx_std_settop(ls, 2);
    mx_std_setmetatable(ls, 1);
    return 1;
}

int32_t mx_std_debug_getregistry(mx_std_state_t *ls) {
    mx_std_push(ls, mx_lua_of(ls->vm)->registry);
    return 1;
}

const mx_std_reg_t mx_std_debug_regs[] = {
    {"traceback", mx_std_debug_traceback, NULL},
    {"getinfo", mx_std_debug_getinfo, NULL},
    {"debug", mx_std_debug_debug, NULL},
    {"sethook", mx_std_debug_sethook, NULL},
    {"gethook", mx_std_debug_gethook, NULL},
    {"getlocal", mx_std_debug_getlocal, NULL},
    {"setlocal", mx_std_debug_setlocal, NULL},
    {"getupvalue", mx_std_debug_getupvalue, NULL},
    {"setupvalue", mx_std_debug_setupvalue, NULL},
    {"upvalueid", mx_std_debug_upvalueid, NULL},
    {"upvaluejoin", mx_std_debug_upvaluejoin, NULL},
    {"getuservalue", mx_std_debug_getuservalue, NULL},
    {"setuservalue", mx_std_debug_setuservalue, NULL},
    {"getmetatable", mx_std_debug_getmetatable, NULL},
    {"setmetatable", mx_std_debug_setmetatable, NULL},
    {"getregistry", mx_std_debug_getregistry, NULL},
    {NULL, NULL, NULL},
};

void mx_std_open_debug(mx_std_state_t *ls) {
    mx_std_newlib(ls, "debug", mx_std_debug_regs);
    mx_std_pop(ls, 1);
}
