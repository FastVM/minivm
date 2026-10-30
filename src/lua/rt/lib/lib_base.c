
#include "../api/std.h"
#include "../lua.h"
#include "../meta.h"
#include "../prim.h"
#include "../trace.h"
#include "../../../vm/gc.h"
#include "../../../value/error.h"
#include "../../../value/kind.h"
#include "../../../value/obj.h"
#include "../../../value/string.h"
#include "../../../value/table.h"

void mx_std_tostr_into(mx_t *vm, mx_str_t *buf, mx_obj_t value) {
    if (mx_obj_is_int(value)) {
        mx_str_append_int(buf, mx_obj_get_int(value));
    } else if (mx_obj_is_float(value)) {
        mx_str_float(buf, mx_obj_get_float(value));
    } else {
        mx_str_object_tostring(vm, buf, value);
    }
}

bool mx_std_tostring_meta_handler(mx_std_state_t *ls, mx_std_slot_t slot) {
    bool done = false;
    if (mx_std_getmetatable(ls, slot)) {
        mx_std_getmetakey(ls, -1, MX_LUA_META_KEY_TOSTRING);
        if (!mx_std_isnil(ls, -1)) {
            mx_std_pushvalue(ls, slot);
            mx_std_call(ls, 1, 1);
            if (ls->vm->std_error.is_set) {
                mx_std_remove(ls, -2);
            } else if (!mx_std_isstring(ls, -1)) {
                mx_std_error(ls, "'__tostring' must return a string");
            } else {
                mx_std_remove(ls, -2);
            }
            done = true;
        } else {
            mx_std_getmetakey(ls, -2, MX_LUA_META_KEY_NAME);
            if (mx_std_isstring(ls, -1)) {
                mx_std_str_t nm = mx_std_tostring(ls, -1);
                mx_str_t *nbuf = mx_str_new();
                mx_str_append(nbuf, nm.ptr, (uint32_t)nm.len);
                mx_str_format(nbuf, ": %p", mx_obj_data_head(*mx_std_at(ls, slot)));
                mx_std_pop(ls, 3);
                mx_std_pushlstring(ls, nbuf->buf, nbuf->len);
                mx_str_free(nbuf);
                done = true;
            } else {
                mx_std_pop(ls, 3);
            }
        }
    }
    return done;
}

void mx_std_tostring_meta(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_obj_t *at = mx_std_at(ls, slot);
    if (at != NULL && mx_obj_is_string(*at)) {
        mx_std_pushvalue(ls, slot);
    } else if (at != NULL && mx_obj_is_number(*at)) {
        mx_str_t *nbuf = mx_str_new();
        mx_std_tostr_into(ls->vm, nbuf, *at);
        mx_std_pushlstring(ls, nbuf->buf, nbuf->len);
        mx_str_free(nbuf);
    } else if (!mx_std_tostring_meta_handler(ls, slot)) {
        mx_obj_t *at_final = mx_std_at(ls, slot);
        if (at_final == NULL) {
            mx_std_pushnil(ls);
        } else {
            mx_str_t *buf = mx_str_new();
            mx_std_tostr_into(ls->vm, buf, *at_final);
            mx_std_pushlstring(ls, buf->buf, buf->len);
            mx_str_free(buf);
        }
    }
}

int32_t mx_std_base_type(mx_std_state_t *ls) {
    mx_std_checkany(ls, 1);
    mx_lua_type_t type = mx_std_type(ls, 1);
    if (type == MX_LUA_TYPE_NONE) {
        return 0;
    } else {
        mx_std_push(ls, mx_lua_of(ls->vm)->type_names[type]);
        return 1;
    }
}

int32_t mx_std_base_assert(mx_std_state_t *ls) {
    if (mx_std_toboolean(ls, 1)) {
        return mx_std_top(ls);
    } else if (mx_std_isnone(ls, 2)) {
        return mx_std_error(ls, "assertion failed!");
    } else {
        mx_std_pushvalue(ls, 2);
        mx_std_replace(ls, 1);
        mx_std_settop(ls, 1);
        return mx_std_base_error(ls);
    }
}

int32_t mx_std_base_print(mx_std_state_t *ls) {
    int32_t n = mx_std_top(ls);
    for (int32_t i = 1; i <= n && !ls->vm->std_error.is_set; i++) {
        mx_std_tostring_meta(ls, i);
        mx_std_replace(ls, i);
    }
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    mx_str_t *buf = mx_str_new();
    for (int32_t i = 1; i <= n; i++) {
        if (i > 1) {
            mx_str_append_byte(buf, '\t');
        }
        mx_std_str_t s = mx_std_tostring(ls, i);
        mx_str_append(buf, s.ptr, (uint32_t)s.len);
    }
    mx_str_append_byte(buf, '\n');
    fwrite(buf->buf, 1, buf->len, stdout);
    fflush(stdout);
    mx_str_free(buf);
    return 0;
}

int32_t mx_std_base_tostring(mx_std_state_t *ls) {
    mx_std_checkany(ls, 1);
    mx_std_tostring_meta(ls, 1);
    return 1;
}

const char *mx_std_skip_space(const char *s) {
    while (isspace((uint8_t)*s) != 0) {
        s += 1;
    }
    return s;
}

int32_t mx_std_base_digit(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'z') {
        return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 10;
    } else {
        return MX_STD_MAX_RADIX;
    }
}

int32_t mx_std_base_tonumber(mx_std_state_t *ls) {
    if (mx_std_isnoneornil(ls, 2)) {
        mx_std_checkany(ls, 1);
        if (ls->vm->std_error.is_set) {
            return 0;
        }
        mx_obj_t *at = mx_std_at(ls, 1);
        if (mx_obj_is_number(*at)) {
            mx_std_push(ls, *at);
        } else if (mx_obj_is_string(*at)) {
            mx_str_t *s = mx_obj_get_string(*at);
            mx_std_push(ls, mx_std_str2num(s->buf, s->len).value);
        } else {
            mx_std_pushnil(ls);
        }
        return 1;
    } else {
        int64_t base = mx_std_checkinteger(ls, 2);
        mx_std_checktype(ls, 1, MX_LUA_TYPE_STRING);
        mx_std_str_t sv = mx_std_checkstring(ls, 1);
        const char *str = sv.ptr;
        if (base < MX_STD_MIN_RADIX || base > MX_STD_MAX_RADIX) {
            return mx_std_argerror(ls, 2, "base out of range");
        }
        const char *scan = mx_std_skip_space(str);
        bool neg = false;
        if (*scan == '+' || *scan == '-') {
            neg = *scan == '-';
            scan += 1;
        }
        uint64_t acc = 0;
        bool any = false;
        for (; *scan != '\0' && mx_std_base_digit(*scan) < base; scan++) {
            acc = acc * (uint64_t)base + (uint64_t)mx_std_base_digit(*scan);
            any = true;
        }
        scan = mx_std_skip_space(scan);
        if (!any || scan != str + sv.len) {
            mx_std_pushnil(ls);
        } else if (neg) {
            mx_std_pushinteger(ls, (int64_t)((uint64_t)0 - acc));
        } else {
            mx_std_pushinteger(ls, (int64_t)acc);
        }
        return 1;
    }
}

int32_t mx_std_base_next(mx_std_state_t *ls) {
    mx_std_checktype(ls, 1, MX_LUA_TYPE_TABLE);
    if (mx_std_isnone(ls, 2)) {
        mx_std_pushnil(ls);
    }
    mx_std_settop(ls, 2);
    if (mx_std_next(ls, 1)) {
        return 2;
    } else {
        mx_std_pushnil(ls);
        return 1;
    }
}

int32_t mx_std_base_pairs(mx_std_state_t *ls) {
    mx_std_checkany(ls, 1);
    if (mx_std_getmetatable(ls, 1)) {
        mx_std_getmetakey(ls, -1, MX_LUA_META_KEY_PAIRS);
        mx_obj_t handler = *mx_std_at(ls, -1);
        const mx_resume_t *site = mx_std_site(ls);
        if (ls->can_pend && mx_obj_is_closure(handler) && site != NULL && site->want <= 4) {
            mx_call_pend(ls->vm, MX_RUN_PENDING_KIND_CALL, handler, mx_obj_of_nil(), NULL, 1, mx_std_at(ls, 1));
            return 0;
        } else if (!mx_obj_is_nil(handler)) {
            mx_std_pushvalue(ls, 1);
            mx_std_call(ls, 1, 4);
            return 4;
        } else {
            mx_std_pop(ls, 2);
        }
    }
    mx_std_push(ls, mx_obj_of_ffi(mx_ffi_base_next));
    mx_std_pushvalue(ls, 1);
    mx_std_pushnil(ls);
    mx_std_pushnil(ls);
    return 4;
}

int32_t mx_std_base_inext(mx_std_state_t *ls) {
    mx_lua_type_t t = mx_std_type(ls, 1);
    if (t != MX_LUA_TYPE_TABLE && t != MX_LUA_TYPE_STRING && !mx_std_getmetatable(ls, 1)) {
        return mx_std_typeerror(ls, 1, "table");
    } else if (t != MX_LUA_TYPE_TABLE && t != MX_LUA_TYPE_STRING) {
        mx_std_pop(ls, 1);
    }
    int64_t i = mx_std_checkinteger(ls, 2) + 1;
    mx_std_geti(ls, 1, i);
    if (mx_std_isnil(ls, -1)) {
        return 0;
    } else {
        mx_std_pushinteger(ls, i);
        mx_std_insert(ls, -2);
        return 2;
    }
}

int32_t mx_std_base_ipairs(mx_std_state_t *ls) {
    mx_std_checkany(ls, 1);
    mx_std_push(ls, mx_obj_of_ffi(mx_ffi_base_inext));
    mx_std_pushvalue(ls, 1);
    mx_std_pushinteger(ls, 0);
    return 3;
}

int32_t mx_std_base_select(mx_std_state_t *ls) {
    int32_t n = mx_std_top(ls);
    mx_obj_t *first = mx_std_at(ls, 1);
    if (first != NULL && mx_obj_is_string(*first)) {
        const char *s = mx_obj_get_string(*first)->buf;
        if (s[0] == '#') {
            mx_std_pushinteger(ls, n - 1);
            return 1;
        }
    }
    int64_t idx = mx_std_checkinteger(ls, 1);
    if (idx < 0) {
        idx = n + idx;
    } else if (idx > n) {
        idx = n;
    }
    if (idx < 1) {
        return mx_std_argerror(ls, 1, "index out of range");
    }
    return n - (int32_t)idx;
}

int32_t mx_std_base_rawequal(mx_std_state_t *ls) {
    mx_std_checkany(ls, 1);
    mx_std_checkany(ls, 2);
    mx_std_pushboolean(ls, mx_std_rawequal(ls, 1, 2));
    return 1;
}

int32_t mx_std_base_rawlen(mx_std_state_t *ls) {
    mx_lua_type_t t = mx_std_type(ls, 1);
    if (t != MX_LUA_TYPE_TABLE && t != MX_LUA_TYPE_STRING) {
        return mx_std_typeerror(ls, 1, "table or string");
    } else {
        mx_std_pushinteger(ls, (int64_t)mx_std_rawlen(ls, 1));
        return 1;
    }
}

int32_t mx_std_base_rawget(mx_std_state_t *ls) {
    mx_std_checktype(ls, 1, MX_LUA_TYPE_TABLE);
    mx_std_checkany(ls, 2);
    mx_std_settop(ls, 2);
    mx_std_rawget(ls, 1);
    return 1;
}

int32_t mx_std_base_rawset(mx_std_state_t *ls) {
    mx_std_checktype(ls, 1, MX_LUA_TYPE_TABLE);
    mx_std_checkany(ls, 2);
    mx_std_checkany(ls, 3);
    mx_obj_t *key = mx_std_at(ls, 2);
    if (mx_std_isnoneornil(ls, 2)) {
        return mx_std_error_noloc(ls, "table index is nil");
    } else if (key != NULL && mx_obj_is_float(*key) && mx_obj_get_float(*key) != mx_obj_get_float(*key)) {
        return mx_std_error_noloc(ls, "table index is NaN");
    } else {
        mx_std_settop(ls, 3);
        mx_std_rawset(ls, 1);
        mx_std_pushvalue(ls, 1);
        return 1;
    }
}

int32_t mx_std_base_setmetatable(mx_std_state_t *ls) {
    mx_std_checktype(ls, 1, MX_LUA_TYPE_TABLE);
    if (!mx_std_isnoneornil(ls, 2) && !mx_std_istable(ls, 2)) {
        return mx_std_typeerror(ls, 2, "nil or table");
    } else if (mx_std_getmetatable(ls, 1)) {
        mx_std_getmetakey(ls, -1, MX_LUA_META_KEY_METATABLE);
        if (!mx_std_isnil(ls, -1)) {
            return mx_std_error(ls, "cannot change a protected metatable");
        } else {
            mx_std_pop(ls, 2);
        }
    }
    mx_std_settop(ls, 2);
    mx_std_setmetatable(ls, 1);
    mx_std_pushvalue(ls, 1);
    return 1;
}

int32_t mx_std_base_getmetatable(mx_std_state_t *ls) {
    mx_std_checkany(ls, 1);
    mx_obj_t *at = mx_std_at(ls, 1);
    if (at != NULL && mx_obj_is_string(*at)) {
        mx_std_push(ls, mx_lua_of(ls->vm)->string_meta);
        return 1;
    } else if (!mx_std_getmetatable(ls, 1)) {
        mx_std_pushnil(ls);
        return 1;
    } else {
        mx_std_getmetakey(ls, -1, MX_LUA_META_KEY_METATABLE);
        if (!mx_std_isnil(ls, -1)) {
            mx_std_remove(ls, -2);
        } else {
            mx_std_pop(ls, 1);
        }
        return 1;
    }
}

int32_t mx_std_base_error(mx_std_state_t *ls) {
    mx_obj_t val = mx_obj_of_nil();
    if (!mx_std_isnone(ls, 1)) {
        val = *mx_std_at(ls, 1);
    }
    int64_t level = mx_std_optinteger(ls, 2, 1);
    if (mx_obj_is_nil(val)) {
        val = mx_obj_new_string(ls->vm, "<no error object>");
    } else if (mx_obj_is_string(val) && level > 0) {
        mx_str_t *msg = mx_str_new_gc(ls->vm);
        if (level <= UINT32_MAX) {
            mx_std_where(ls, (uint32_t)level, msg);
        }
        mx_str_t *src = mx_obj_get_string(val);
        mx_str_append(msg, src->buf, src->len);
        val = mx_obj_of_string(msg);
    }
    ls->error = mx_error_value(ls->vm, val);
    mx_std_raise(ls);
    return 0;
}

int32_t mx_std_base_collectgarbage(mx_std_state_t *ls) {
    const char *const actions[] = {"collect", "stop", "restart", "count", "step", "isrunning", "incremental", "generational", "param", NULL};
    const char *const params[] = {
        [MX_GC_PARAM_PAUSE] = "pause",
        [MX_GC_PARAM_STEPMUL] = "stepmul",
        [MX_GC_PARAM_STEPSIZE] = "stepsize",
        [MX_GC_PARAM_MINORMUL] = "minormul",
        [MX_GC_PARAM_MAJORMINOR] = "majorminor",
        [MX_GC_PARAM_MINORMAJOR] = "minormajor",
        [MX_GC_PARAM_COUNT] = NULL,
    };
    mx_t *vm = ls->vm;
    mx_gc_t *gc = vm->gc;
    int32_t action = mx_std_checkoption(ls, 1, "collect", actions);
    if (vm->std_error.is_set) {
        return 0;
    } else if (gc->is_finalizing) {
        mx_std_pushboolean(ls, false);
        return 1;
    }
    switch (action) {
        case 0: {
            gc->is_major_forced = true;
            mx_std_sync(ls);
            mx_gc_collect(vm);
            mx_std_pushinteger(ls, 0);
            break;
        }
        case 1:
        case 2: {
            vm->is_gc_stopped = action == 1;
            mx_std_pushinteger(ls, 0);
            break;
        }
        case 3: {
            mx_std_pushfloat(ls, (double)mx_gc_bytes(vm) / 1024.0);
            break;
        }
        case 4: {
            int64_t kilobytes = mx_std_optinteger(ls, 2, 0);
            if (kilobytes < 0) {
                kilobytes = 0;
            }
            mx_std_pushboolean(ls, mx_gc_step(vm, (size_t)kilobytes));
            break;
        }
        case 5: {
            mx_std_pushboolean(ls, !vm->is_gc_stopped);
            break;
        }
        case 6:
        case 7: {
            if (gc->is_generational) {
                mx_std_pushstring(ls, "generational");
            } else {
                mx_std_pushstring(ls, "incremental");
            }
            gc->is_generational = action == 7;
            gc->is_major_mode = false;
            break;
        }
        default: {
            int32_t which = mx_std_checkoption(ls, 2, NULL, params);
            if (vm->std_error.is_set) {
                return 0;
            }
            int64_t prev = gc->params[which];
            if (!mx_std_isnoneornil(ls, 3)) {
                int64_t v = mx_std_checkinteger(ls, 3);
                if (v < 0) {
                    v = 0;
                }
                if (v > UINT32_MAX) {
                    v = UINT32_MAX;
                }
                gc->params[which] = (uint32_t)v;
            }
            mx_std_pushinteger(ls, prev);
            break;
        }
    }
    return 1;
}

int32_t mx_std_base_warn(mx_std_state_t *ls) {
    int32_t n = mx_std_top(ls);
    mx_std_checkstring(ls, 1);
    for (int32_t i = 2; i <= n; i++) {
        mx_std_checkstring(ls, i);
    }
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    for (int32_t i = 1; i <= n; i++) {
        mx_lua_warning(ls->vm, mx_std_tostring(ls, i).ptr, i < n);
    }
    return 0;
}

void mx_ffi_base_assert(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_base_assert, "assert", args, (int32_t)nargs);
}

void mx_ffi_base_error(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_base_error, "error", args, (int32_t)nargs);
}

void mx_ffi_base_getmetatable(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_ffi_base_getmetatable_fast(vm, nargs, args)) {
        mx_std_dispatch(vm, mx_std_base_getmetatable, "getmetatable", args, (int32_t)nargs);
    }
}

bool mx_ffi_base_getmetatable_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs == 0) {
        return false;
    } else if (mx_obj_is_table(args[0])) {
        mx_obj_table_t *meta = mx_obj_get_table(args[0])->meta;
        mx_obj_t out = mx_obj_of_nil();
        if (meta != NULL) {
            out = mx_table_get(meta, mx_lua_of(vm)->meta_keys[MX_LUA_META_KEY_METATABLE]);
            if (mx_obj_is_nil(out)) {
                out = mx_obj_of_table(meta);
            }
        }
        args[0] = out;
        vm->rets.len = 1;
    } else {
        mx_obj_table_t *meta = mx_meta_table_of(vm, args[0]);
        mx_obj_t out = mx_obj_of_nil();
        if (meta != NULL) {
            out = mx_table_get(meta, mx_lua_of(vm)->meta_keys[MX_LUA_META_KEY_METATABLE]);
            if (mx_obj_is_nil(out)) {
                out = mx_obj_of_table(meta);
            }
        } else if (mx_obj_is_string(args[0])) {
            out = mx_lua_of(vm)->string_meta;
        }
        args[0] = out;
        vm->rets.len = 1;
    }
    return true;
}

void mx_ffi_base_ipairs(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_ffi_base_ipairs_fast(vm, nargs, args)) {
        mx_std_dispatch(vm, mx_std_base_ipairs, "ipairs", args, (int32_t)nargs);
    }
}

bool mx_ffi_base_ipairs_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs == 0) {
        return false;
    }
    args[1] = args[0];
    args[0] = mx_obj_of_ffi(mx_ffi_base_inext);
    args[2] = mx_obj_of_int(0);
    vm->rets.len = 3;
    return true;
}

void mx_ffi_base_pairs(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_ffi_base_pairs_fast(vm, nargs, args)) {
        mx_std_dispatch(vm, mx_std_base_pairs, "pairs", args, (int32_t)nargs);
    }
}

bool mx_ffi_base_pairs_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs == 0 || args[0].kind != vm->kinds.table) {
        return false;
    }
    mx_obj_table_t *meta = mx_obj_get_table(args[0])->meta;
    if (meta != NULL && !mx_obj_is_nil(mx_table_get(meta, mx_lua_of(vm)->meta_keys[MX_LUA_META_KEY_PAIRS]))) {
        return false;
    }
    args[1] = args[0];
    args[0] = mx_obj_of_ffi(mx_ffi_base_next);
    args[2] = mx_obj_of_nil();
    args[3] = mx_obj_of_nil();
    vm->rets.len = 4;
    return true;
}

void mx_ffi_base_print(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_base_print, "print", args, (int32_t)nargs);
}

void mx_ffi_base_rawequal(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_base_rawequal, "rawequal", args, (int32_t)nargs);
}

void mx_ffi_base_rawget(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_ffi_base_rawget_fast(vm, nargs, args)) {
        mx_std_dispatch(vm, mx_std_base_rawget, "rawget", args, (int32_t)nargs);
    }
}

bool mx_ffi_base_rawget_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs < 2 || args[0].kind != vm->kinds.table) {
        return false;
    }
    args[0] = mx_table_get(mx_obj_get_table(args[0]), args[1]);
    vm->rets.len = 1;
    return true;
}

void mx_ffi_base_rawlen(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_base_rawlen, "rawlen", args, (int32_t)nargs);
}

void mx_ffi_base_rawset(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_base_rawset, "rawset", args, (int32_t)nargs);
}

void mx_ffi_base_select(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_ffi_base_select_fast(vm, nargs, args)) {
        mx_std_dispatch(vm, mx_std_base_select, "select", args, (int32_t)nargs);
    }
}

bool mx_ffi_base_select_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs == 0) {
        return false;
    }
    int64_t n = (int64_t)nargs;
    if (mx_obj_is_string(args[0]) && mx_obj_get_string(args[0])->buf[0] == '#') {
        args[0] = mx_obj_of_int(n - 1);
        vm->rets.len = 1;
        return true;
    } else if (!mx_obj_is_int(args[0])) {
        return false;
    }
    int64_t idx = mx_obj_get_int(args[0]);
    if (idx < 0) {
        idx = n + idx;
    } else if (idx > n) {
        idx = n;
    }
    if (idx < 1) {
        return false;
    }
    uint32_t count = (uint32_t)(n - idx);
    memmove(args, args + idx, sizeof(mx_obj_t) * count);
    vm->rets.len = count;
    return true;
}

void mx_ffi_base_setmetatable(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_ffi_base_setmetatable_fast(vm, nargs, args)) {
        mx_std_dispatch(vm, mx_std_base_setmetatable, "setmetatable", args, (int32_t)nargs);
    }
}

// Anything that would raise an error is left to mx_std_base_setmetatable.
bool mx_ffi_base_setmetatable_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs < 2 || args[0].kind != vm->kinds.table || (args[1].kind != vm->kinds.table && !mx_obj_is_nil(args[1]))) {
        return false;
    }
    mx_lua_t *lua = mx_lua_of(vm);
    mx_obj_table_t *t = mx_obj_get_table(args[0]);
    if (t->meta != NULL && !mx_obj_is_nil(mx_table_get(t->meta, lua->meta_keys[MX_LUA_META_KEY_METATABLE]))) {
        return false;
    }
    mx_obj_table_t *meta = NULL;
    if (!mx_obj_is_nil(args[1])) {
        meta = mx_obj_get_table(args[1]);
        mx_gc_write_barrier(vm, t, args[1]);
    }
    t->meta = meta;
    if (meta != NULL && !mx_obj_is_nil(mx_table_get(meta, lua->meta_keys[MX_LUA_META_KEY_GC]))) {
        mx_gc_note_finalizer(vm, t);
    }
    vm->rets.len = 1;
    return true;
}

void mx_ffi_base_tonumber(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_ffi_base_tonumber_fast(vm, nargs, args)) {
        mx_std_dispatch(vm, mx_std_base_tonumber, "tonumber", args, (int32_t)nargs);
    }
}

bool mx_ffi_base_tonumber_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs != 1) {
        return false;
    } else if (mx_obj_is_string(args[0])) {
        mx_str_t *s = mx_obj_get_string(args[0]);
        args[0] = mx_std_str2num(s->buf, s->len).value;
    } else if (!mx_obj_is_number(args[0])) {
        return false;
    }
    vm->rets.len = 1;
    return true;
}

void mx_ffi_base_type(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_ffi_base_type_fast(vm, nargs, args)) {
        mx_std_dispatch(vm, mx_std_base_type, "type", args, (int32_t)nargs);
    }
}

bool mx_ffi_base_type_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs == 0) {
        return false;
    }
    args[0] = mx_lua_of(vm)->type_names[mx_meta_lua_type(vm, args[0])];
    vm->rets.len = 1;
    return true;
}

void mx_ffi_base_tostring(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs >= 1 && mx_obj_is_string(args[0])) {
        vm->rets.len = 1;
    } else if (nargs >= 1 && mx_obj_is_number(args[0])) {
        mx_str_t *buf = mx_str_new_gc(vm);
        if (mx_obj_is_int(args[0])) {
            mx_str_append_int(buf, mx_obj_get_int(args[0]));
        } else {
            mx_str_float(buf, mx_obj_get_float(args[0]));
        }
        args[0] = mx_obj_of_string(buf);
        vm->rets.len = 1;
    } else {
        mx_std_dispatch(vm, mx_std_base_tostring, "tostring", args, (int32_t)nargs);
    }
}

void mx_ffi_base_next(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_ffi_base_next_fast(vm, nargs, args)) {
        mx_std_dispatch(vm, mx_std_base_next, "next", args, (int32_t)nargs);
    }
}

bool mx_ffi_base_next_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs == 0 || args[0].kind != vm->kinds.table) {
        return false;
    }
    mx_obj_t key = mx_obj_of_nil();
    if (nargs >= 2) {
        key = mx_obj_of_raw(args[1].kind, args[1].val);
    }
    mx_table_step_t step = mx_table_next_into(vm, mx_obj_get_table(args[0]), key, args);
    if (step == MX_TABLE_STEP_PAIR) {
        vm->rets.len = 2;
        return true;
    } else if (step == MX_TABLE_STEP_END) {
        args[0] = mx_obj_of_nil();
        vm->rets.len = 1;
        return true;
    } else {
        return false;
    }
}

void mx_ffi_base_inext(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_ffi_base_inext_fast(vm, nargs, args)) {
        mx_std_dispatch(vm, mx_std_base_inext, "for iterator", args, (int32_t)nargs);
    }
}

bool mx_ffi_base_inext_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs < 2 || !mx_obj_is_table(args[0]) || !mx_obj_is_int(args[1])) {
        return false;
    }
    mx_obj_table_t *t = mx_obj_get_table(args[0]);
    if (!t->is_plain || t->meta != NULL) {
        return false;
    }
    int64_t i = mx_obj_get_int(args[1]) + 1;
    mx_obj_t v;
    if (i >= 1 && (uint64_t)i <= (uint64_t)t->array_len) {
        v = t->array[i - 1];
    } else {
        v = mx_table_get(t, mx_obj_of_int(i));
    }
    if (mx_obj_is_nil(v)) {
        args[0] = mx_obj_of_nil();
        vm->rets.len = 0;
    } else {
        args[0] = mx_obj_of_int(i);
        args[1] = v;
        vm->rets.len = 2;
    }
    return true;
}

const mx_std_reg_t mx_std_base_regs[] = {
    {"assert", mx_std_base_assert, mx_ffi_base_assert},
    {"error", mx_std_base_error, mx_ffi_base_error},
    {"getmetatable", mx_std_base_getmetatable, mx_ffi_base_getmetatable},
    {"ipairs", mx_std_base_ipairs, mx_ffi_base_ipairs},
    {"next", mx_std_base_next, mx_ffi_base_next},
    {"pairs", mx_std_base_pairs, mx_ffi_base_pairs},
    {"print", mx_std_base_print, mx_ffi_base_print},
    {"rawequal", mx_std_base_rawequal, mx_ffi_base_rawequal},
    {"rawget", mx_std_base_rawget, mx_ffi_base_rawget},
    {"rawlen", mx_std_base_rawlen, mx_ffi_base_rawlen},
    {"rawset", mx_std_base_rawset, mx_ffi_base_rawset},
    {"select", mx_std_base_select, mx_ffi_base_select},
    {"setmetatable", mx_std_base_setmetatable, mx_ffi_base_setmetatable},
    {"tonumber", mx_std_base_tonumber, mx_ffi_base_tonumber},
    {"tostring", mx_std_base_tostring, mx_ffi_base_tostring},
    {"type", mx_std_base_type, mx_ffi_base_type},
    {NULL, NULL, NULL},
};

void mx_std_open_base(mx_std_state_t *ls) {
    mx_std_push(ls, mx_lua_of(ls->vm)->std);
    mx_std_register(ls, mx_std_base_regs);
    mx_std_pop(ls, 1);
    mx_std_push(ls, mx_obj_of_ffi(mx_prim_pcall));
    mx_std_setglobal(ls, "pcall");
    mx_std_push(ls, mx_obj_of_ffi(mx_prim_xpcall));
    mx_std_setglobal(ls, "xpcall");
    mx_std_pushcfn(ls, mx_std_base_warn, "warn");
    mx_std_setglobal(ls, "warn");
    mx_std_pushcfn(ls, mx_std_base_collectgarbage, "collectgarbage");
    mx_std_setglobal(ls, "collectgarbage");
    mx_std_pushstring(ls, "Lua 5.5");
    mx_std_setglobal(ls, "_VERSION");
    mx_std_push(ls, mx_lua_of(ls->vm)->std);
    mx_std_setglobal(ls, "_G");
}
