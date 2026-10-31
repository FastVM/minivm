
#include "trace.h"
#include "lua.h"
#include "prim.h"
#include "../../value/kind.h"
#include "../../value/obj.h"
#include "../../value/table.h"

bool mx_trace_is_close_call(const mx_resume_t *site) {
    if (site->site_at < site->site->code_len) {
        return false;
    }
    mx_ir_branch_t br = mx_ir_block_branch(site->site);
    if (br.op != MX_IR_BRANCH_OPCODE_CALL) {
        return false;
    }
    mx_ir_arg_t args[MX_IR_BRANCH_MAX_ARGS + 1];
    mx_ir_branch_args(&br, args);
    return args[0].type == MX_IR_ARG_TYPE_LIT && args[0].lit.kind == MX_KIND_FFI && mx_obj_get_ffi(args[0].lit) == mx_prim_close_call;
}

mx_trace_name_t mx_trace_namewhat(const mx_frame_t *f) {
    if ((f->tag != MX_LINK_LUA && f->tag != MX_LINK_C) || mx_link_is_tail(f->th->slots[f->p].bits)) {
        return (mx_trace_name_t) { .what = NULL, .name = NULL };
    }
    const mx_resume_t *site = (const mx_resume_t *)f->th->slots[f->p + 1].bits;
    if (site == NULL) {
        return (mx_trace_name_t) { .what = NULL, .name = NULL };
    } else if (site->is_meta) {
        return (mx_trace_name_t) { .what = "metamethod", .name = mx_op_names[site->meta] };
    } else if (mx_trace_is_close_call(site)) {
        return (mx_trace_name_t) { .what = "metamethod", .name = "close" };
    }
    mx_varname_t name = mx_resume_range(site).names[0];
    if (name.kind != MX_VARHINT_NONE && name.name != NULL) {
        return (mx_trace_name_t) { .what = mx_varhint_names[name.kind], .name = name.name };
    } else {
        return (mx_trace_name_t) { .what = NULL, .name = NULL };
    }
}

bool mx_trace_global_name(mx_t *vm, mx_obj_t fn, mx_str_t *out) {
    mx_lua_t *lua = mx_lua_of(vm);
    mx_obj_t loaded = mx_table_get(mx_obj_get_table(lua->registry), mx_obj_intern(vm, "_LOADED"));
    if (!mx_obj_is_table(loaded)) {
        return false;
    }
    mx_obj_table_t *mods = mx_obj_get_table(loaded);
    mx_obj_t pair[2];
    mx_obj_t mkey = mx_obj_of_nil();
    while (mx_table_next_into(vm, mods, mkey, pair) == MX_TABLE_STEP_PAIR) {
        mkey = pair[0];
        mx_obj_t mod = pair[1];
        if (mx_obj_is_string(mkey) && mx_obj_is_table(mod)) {
            mx_obj_table_t *t = mx_obj_get_table(mod);
            mx_obj_t entry[2];
            mx_obj_t key = mx_obj_of_nil();
            while (mx_table_next_into(vm, t, key, entry) == MX_TABLE_STEP_PAIR) {
                key = entry[0];
                if (mx_obj_is_string(key) && entry[1].kind == fn.kind && entry[1].val.bits == fn.val.bits) {
                    mx_str_t *mname = mx_obj_get_string(mkey);
                    if (strcmp(mname->buf, "_G") != 0) {
                        mx_str_format(out, "%s.", mname->buf);
                    }
                    mx_str_format(out, "%s", mx_obj_get_string(key)->buf);
                    return true;
                }
            }
        }
    }
    return false;
}

void mx_trace_short_src(const mx_frame_t *f, mx_str_t *out) {
    if (f->is_lua) {
        mx_error_chunkid_t id = mx_error_chunkid(mx_frame_func(f)->func->source->file);
        mx_str_format(out, "%s", id.id);
    } else {
        mx_str_format(out, "[C]");
    }
}

void mx_trace_funcname(mx_t *vm, mx_str_t *out, const mx_frame_t *f) {
    mx_str_t *global = mx_str_new();
    mx_trace_name_t named = mx_trace_namewhat(f);
    if (named.what != NULL) {
        mx_str_format(out, "%s '%s'", named.what, named.name);
    } else if (f->is_lua && mx_frame_func(f)->is_main) {
        mx_str_format(out, "main chunk");
    } else if (f->is_lua) {
        mx_ir_block_t *head = mx_frame_func(f);
        mx_str_format(out, "function <");
        mx_trace_short_src(f, out);
        mx_str_format(out, ":%u>", mx_source_line(head->func->source, head->func->start));
    } else if (mx_trace_global_name(vm, mx_frame_fn(f), global)) {
        mx_str_format(out, "function '%s'", global->buf);
    } else {
        mx_str_format(out, "?");
    }
    mx_str_free(global);
}

void mx_trace_line(mx_t *vm, mx_str_t *out, const mx_frame_t *f) {
    mx_str_format(out, "\n\t");
    mx_trace_short_src(f, out);
    int64_t line = mx_frame_line(f);
    if (line > 0) {
        mx_str_format(out, ":%d", (int32_t)line);
    }
    mx_str_format(out, ": in ");
    mx_trace_funcname(vm, out, f);
}

void mx_trace_append(mx_t *vm, mx_str_t *out, mx_thread_t *th, uint32_t level) {
    uint32_t total = mx_frame_count(th);
    mx_frame_t last = mx_frame_at_level(th, total - 1);
    bool has_host = total > 0 && last.tag == MX_LINK_ENTRY && mx_link_caller(th->slots[last.p].bits) == MX_THREAD_NONE;
    uint32_t count;
    if (has_host) {
        count = total + 1;
    } else {
        count = total;
    }
    mx_str_format(out, "stack traceback:");
    if (level >= count) {
        return;
    }
    uint32_t n = count - level;
    mx_frame_iter_t it = mx_frame_iter(th);
    mx_frame_t f = mx_frame_next(&it);
    for (uint32_t i = 0; i < level && f.is_valid; i++) {
        f = mx_frame_next(&it);
    }
    for (uint32_t i = 0; i < n; i++) {
        bool is_skipped = n > MX_TRACE_LEVELS_HEAD + MX_TRACE_LEVELS_TAIL && i >= MX_TRACE_LEVELS_HEAD && i < n - MX_TRACE_LEVELS_TAIL;
        if (is_skipped && i == MX_TRACE_LEVELS_HEAD) {
            mx_str_format(out, "\n\t...\t(skipping %u levels)", n - MX_TRACE_LEVELS_HEAD - MX_TRACE_LEVELS_TAIL);
        } else if (!is_skipped && f.is_valid && mx_link_is_tail(th->slots[f.p].bits)) {
            mx_trace_line(vm, out, &f);
            mx_str_format(out, "\n\t(...tail calls...)");
        } else if (!is_skipped && f.is_valid) {
            mx_trace_line(vm, out, &f);
        } else if (!is_skipped) {
            mx_str_format(out, "\n\t[C]: in ?");
        }
        if (f.is_valid) {
            f = mx_frame_next(&it);
        }
    }
}
