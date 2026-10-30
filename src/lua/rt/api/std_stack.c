
#include "std.h"
#include "../lua.h"
#include "../meta.h"
#include "../../../vm/gc.h"
#include "../../../value/error.h"
#include "../../../value/kind.h"
#include "../../../value/obj.h"
#include "../../../value/string.h"
#include "../../../value/table.h"

void mx_std_state_enter(mx_std_state_t *ls, mx_t *vm, mx_obj_t *args, int32_t nargs) {
    mx_thread_t *th = vm->thread;
    *ls = (mx_std_state_t) {
        .vm = vm,
        .th = th,
        .base = (uint32_t)((mx_val_t *)args - th->slots),
        .top = nargs,
        .error = mx_obj_of_nil(),
    };
    if (th->frame != MX_THREAD_NONE && th->frame + 4 == ls->base) {
        mx_obj_t fn = *mx_thread_objs(th, th->frame + 2);
        if (fn.kind == vm->kinds.cfn) {
            ls->upvals = (mx_obj_cfn_t *)mx_ref_obj(fn.val.ref);
        }
    }
}

void mx_std_dispatch(mx_t *vm, mx_std_fn_t *fn, const char *fnname, mx_obj_t *args, int32_t nargs) {
    mx_std_state_t ls;
    mx_std_state_enter(&ls, vm, args, nargs);
    ls.fnname = fnname;
    ls.fn = fn;
    ls.can_pend = vm->pending.is_ok;
    vm->pending.is_ok = false;
    mx_std_dispatch_finish(&ls, fn(&ls));
}

void mx_std_dispatch_finish(mx_std_state_t *ls, int32_t nret) {
    mx_t *vm = ls->vm;
    vm->pending.is_ok = ls->can_pend;
    if (vm->pending.is_set) {
        return;
    }
    mx_obj_t *res = mx_std_slots(ls);
    if (vm->std_error.is_set) {
        vm->std_error.is_set = false;
        res[0] = vm->std_error.value;
        vm->std_error.value = mx_obj_of_nil();
        vm->rets.len = 1;
        return;
    }
    if (nret < 0) {
        nret = 0;
    }
    int32_t first = ls->top - nret;
    if (first != 0 && nret == 1) {
        res[0] = res[first];
    } else if (first != 0) {
        memmove(res, res + first, sizeof(mx_obj_t) * (size_t)nret);
    }
    if (nret == 0) {
        res[0] = mx_obj_of_nil();
    }
    vm->rets.len = (uint32_t)nret;
}

mx_obj_t *mx_std_slots(mx_std_state_t *ls) {
    return mx_thread_objs(ls->th, ls->base);
}

void mx_std_sync(mx_std_state_t *ls) {
    ls->th->top = ls->base + 2 * (uint32_t)ls->top;
}

bool mx_std_grow(mx_std_state_t *ls, int32_t extra) {
    uint64_t need = (uint64_t)ls->base + 2 * ((uint64_t)ls->top + (uint64_t)extra) + MX_RUN_SLACK;
    if (need > ls->th->limit || extra >= 1000000) {
        return false;
    } else {
        mx_thread_reserve(ls->vm, ls->th, (uint32_t)need);
        return true;
    }
}

const mx_resume_t *mx_std_site(mx_std_state_t *ls) {
    mx_thread_t *th = ls->th;
    if (th->frame != MX_THREAD_NONE && th->frame + 4 == ls->base) {
        return (const mx_resume_t *)th->slots[th->frame + 1].bits;
    } else {
        return NULL;
    }
}

mx_varname_t mx_std_site_name(mx_std_state_t *ls) {
    const mx_resume_t *site = mx_std_site(ls);
    if (site == NULL) {
        return (mx_varname_t) { .name = NULL, .kind = MX_VARHINT_NONE };
    } else {
        return mx_resume_range(site).names[0];
    }
}

void mx_std_where(mx_std_state_t *ls, uint32_t level, mx_str_t *out) {
    const mx_resume_t *site = NULL;
    if (level == 1) {
        site = mx_std_site(ls);
    } else if (level > 1) {
        mx_frame_t f = mx_frame_at_level(ls->th, level);
        if (f.is_valid && f.is_lua) {
            site = f.at;
        }
    }
    if (site != NULL) {
        mx_location_range_t range = mx_resume_range(site);
        mx_error_where(out, &range);
    }
}

mx_std_slot_t mx_std_absindex(mx_std_state_t *ls, mx_std_slot_t slot) {
    if (slot > 0 || slot <= MX_STD_REGISTRYINDEX) {
        return slot;
    } else {
        return ls->top + slot + 1;
    }
}

mx_obj_t *mx_std_pseudo_at(mx_std_state_t *ls, mx_std_slot_t slot) {
    if (slot == MX_STD_REGISTRYINDEX) {
        return &mx_lua_of(ls->vm)->registry;
    } else {
        uint32_t up = (uint32_t)(MX_STD_REGISTRYINDEX - slot) - 1;
        mx_obj_cfn_t *cfn = ls->upvals;
        if (cfn == NULL || up >= cfn->len) {
            return NULL;
        } else {
            return &cfn->values[up];
        }
    }
}

mx_obj_t *mx_std_at(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_std_slot_t abs = mx_std_absindex(ls, slot);
    if (abs <= MX_STD_REGISTRYINDEX) {
        return mx_std_pseudo_at(ls, abs);
    } else if (abs < 1 || abs > ls->top) {
        return NULL;
    } else {
        return &mx_std_slots(ls)[abs - 1];
    }
}

void mx_std_raise(mx_std_state_t *ls) {
    mx_t *vm = ls->vm;
    if (!vm->std_error.is_set) {
        vm->std_error = (mx_std_error_t) { .value = ls->error, .is_set = true };
    }
    ls->error = mx_obj_of_nil();
}

mx_std_slot_t mx_std_top(mx_std_state_t *ls) {
    return ls->top;
}

void mx_std_settop(mx_std_state_t *ls, mx_std_slot_t n) {
    mx_std_slot_t want = n;
    if (want < 0) {
        want = ls->top + n + 1;
    }
    if (want > ls->top) {
        if (!mx_std_grow(ls, want - ls->top)) {
            mx_std_error(ls, "stack overflow");
            return;
        }
        mx_obj_t *slots = mx_std_slots(ls);
        for (mx_std_slot_t i = ls->top; i < want; i++) {
            slots[i] = mx_obj_of_nil();
        }
    }
    ls->top = want;
    mx_std_sync(ls);
}

void mx_std_pop(mx_std_state_t *ls, mx_std_slot_t n) {
    ls->top -= n;
}

void mx_std_push(mx_std_state_t *ls, mx_obj_t value) {
    if (!mx_std_grow(ls, 1)) {
        mx_std_error(ls, "stack overflow");
        return;
    }
    mx_std_slots(ls)[ls->top] = value;
    ls->top += 1;
    mx_std_sync(ls);
}

void mx_std_pushvalue(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_obj_t *src = mx_std_at(ls, slot);
    mx_obj_t v = mx_obj_of_nil();
    if (src != NULL) {
        v = *src;
    }
    mx_std_push(ls, v);
}

void mx_std_rotate(mx_std_state_t *ls, mx_std_slot_t slot, mx_std_slot_t n) {
    mx_std_slot_t abs = mx_std_absindex(ls, slot);
    mx_std_slot_t count = ls->top - abs + 1;
    if (count <= 0) {
        return;
    }
    mx_std_slot_t shift = n % count;
    if (shift < 0) {
        shift += count;
    }
    mx_obj_t *seg = &mx_std_slots(ls)[abs - 1];
    for (mx_std_slot_t i = 0; i < shift; i++) {
        mx_obj_t held = seg[count - 1];
        memmove(seg + 1, seg, sizeof(mx_obj_t) * (size_t)(count - 1));
        seg[0] = held;
    }
}

void mx_std_insert(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_std_rotate(ls, slot, 1);
}

void mx_std_remove(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_std_slot_t abs = mx_std_absindex(ls, slot);
    mx_std_slot_t count = ls->top - abs + 1;
    if (count >= 1) {
        mx_obj_t *seg = &mx_std_slots(ls)[abs - 1];
        memmove(seg, seg + 1, sizeof(mx_obj_t) * (size_t)(count - 1));
    }
    mx_std_pop(ls, 1);
}

void mx_std_copy(mx_std_state_t *ls, mx_std_slot_t from, mx_std_slot_t to) {
    mx_obj_t *src = mx_std_at(ls, from);
    mx_obj_t *dst = mx_std_at(ls, to);
    if (src != NULL && dst != NULL) {
        *dst = *src;
    }
}

void mx_std_replace(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_std_copy(ls, -1, slot);
    mx_std_pop(ls, 1);
}

bool mx_std_checkstack(mx_std_state_t *ls, mx_std_slot_t extra) {
    return mx_std_grow(ls, extra);
}

mx_lua_type_t mx_std_type(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_obj_t *at = mx_std_at(ls, slot);
    if (at == NULL) {
        return MX_LUA_TYPE_NONE;
    } else {
        return mx_meta_lua_type(ls->vm, *at);
    }
}

const char *mx_std_typename(mx_std_state_t *ls, mx_lua_type_t type) {
    return mx_meta_lua_typename(type);
}

bool mx_std_isnil(mx_std_state_t *ls, mx_std_slot_t slot) {
    return mx_std_type(ls, slot) == MX_LUA_TYPE_NIL;
}

bool mx_std_isboolean(mx_std_state_t *ls, mx_std_slot_t slot) {
    return mx_std_type(ls, slot) == MX_LUA_TYPE_BOOLEAN;
}

bool mx_std_isinteger(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_obj_t *at = mx_std_at(ls, slot);
    return at != NULL && mx_obj_is_int(*at);
}

bool mx_std_isfloat(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_obj_t *at = mx_std_at(ls, slot);
    return at != NULL && mx_obj_is_float(*at);
}

bool mx_std_isnumber(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_obj_t *at = mx_std_at(ls, slot);
    if (at == NULL) {
        return false;
    } else if (mx_obj_is_number(*at)) {
        return true;
    } else if (mx_obj_is_string(*at)) {
        mx_str_t *s = mx_obj_get_string(*at);
        return mx_std_str2num(s->buf, s->len).is_num;
    } else {
        return false;
    }
}

bool mx_std_isstring(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_obj_t *at = mx_std_at(ls, slot);
    return at != NULL && (mx_obj_is_string(*at) || mx_obj_is_number(*at));
}

bool mx_std_istable(mx_std_state_t *ls, mx_std_slot_t slot) {
    return mx_std_type(ls, slot) == MX_LUA_TYPE_TABLE;
}

bool mx_std_isfunction(mx_std_state_t *ls, mx_std_slot_t slot) {
    return mx_std_type(ls, slot) == MX_LUA_TYPE_FUNCTION;
}

bool mx_std_isnone(mx_std_state_t *ls, mx_std_slot_t slot) {
    return mx_std_at(ls, slot) == NULL;
}

bool mx_std_isnoneornil(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_obj_t *at = mx_std_at(ls, slot);
    return at == NULL || mx_obj_is_nil(*at);
}

bool mx_std_toboolean(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_obj_t *at = mx_std_at(ls, slot);
    return at != NULL && mx_obj_is_truthy(*at);
}

mx_obj_int_t mx_std_tointeger(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_obj_t *at = mx_std_at(ls, slot);
    if (at != NULL && mx_obj_is_number(*at)) {
        return mx_obj_to_int(*at);
    } else if (at != NULL && mx_obj_is_string(*at)) {
        mx_str_t *s = mx_obj_get_string(*at);
        mx_obj_num_t num = mx_std_str2num(s->buf, s->len);
        if (num.is_num) {
            return mx_obj_to_int(num.value);
        }
    }
    return (mx_obj_int_t) { .value = 0, .is_int = false };
}

mx_obj_float_t mx_std_tofloat(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_obj_t *at = mx_std_at(ls, slot);
    if (at != NULL && mx_obj_is_number(*at)) {
        return (mx_obj_float_t) { .value = mx_obj_get_number(*at), .is_num = true };
    } else if (at != NULL && mx_obj_is_string(*at)) {
        mx_str_t *s = mx_obj_get_string(*at);
        mx_obj_num_t num = mx_std_str2num(s->buf, s->len);
        if (num.is_num) {
            return (mx_obj_float_t) { .value = mx_obj_get_number(num.value), .is_num = true };
        }
    }
    return (mx_obj_float_t) { .value = 0.0, .is_num = false };
}

mx_std_str_t mx_std_tostring(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_obj_t *at = mx_std_at(ls, slot);
    if (at == NULL) {
        return (mx_std_str_t) { .ptr = NULL, .len = 0 };
    } else if (mx_obj_is_string(*at)) {
        mx_str_t *buf = mx_obj_get_string(*at);
        return (mx_std_str_t) { .ptr = buf->buf, .len = buf->len };
    } else if (mx_obj_is_number(*at)) {
        mx_str_t *buf = mx_str_new_gc(ls->vm);
        if (mx_obj_is_int(*at)) {
            mx_str_append_int(buf, mx_obj_get_int(*at));
        } else {
            mx_str_float(buf, mx_obj_get_float(*at));
        }
        *at = mx_obj_of_string(buf);
        return (mx_std_str_t) { .ptr = buf->buf, .len = buf->len };
    } else {
        return (mx_std_str_t) { .ptr = NULL, .len = 0 };
    }
}

uint64_t mx_std_rawlen(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_obj_t *at = mx_std_at(ls, slot);
    if (at == NULL) {
        return 0;
    } else if (mx_obj_is_string(*at)) {
        return mx_obj_get_string(*at)->len;
    } else if (mx_obj_is_table(*at)) {
        return mx_obj_get_table(*at)->len;
    } else {
        return 0;
    }
}

int64_t mx_std_len_meta(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_obj_t *at = mx_std_at(ls, slot);
    if (at == NULL) {
        return 0;
    }
    mx_obj_t got = mx_meta_len(ls->vm, *at);
    if (mx_obj_is_error(got)) {
        ls->error = got;
        mx_std_raise(ls);
        return 0;
    }
    mx_obj_int_t n = mx_obj_to_int(got);
    if (!n.is_int) {
        mx_std_error(ls, "object length is not an integer");
        return 0;
    }
    return n.value;
}

void mx_std_pushnil(mx_std_state_t *ls) {
    mx_std_push(ls, mx_obj_of_nil());
}

void mx_std_pushboolean(mx_std_state_t *ls, bool b) {
    mx_std_push(ls, mx_obj_of_boolean(b));
}

void mx_std_pushinteger(mx_std_state_t *ls, int64_t n) {
    mx_std_push(ls, mx_obj_of_int(n));
}

void mx_std_pushfloat(mx_std_state_t *ls, double n) {
    mx_std_push(ls, mx_obj_of_number(n));
}

void mx_std_pushlstring(mx_std_state_t *ls, const char *str, size_t len) {
    if (len >= 1 && len <= 23) {
        mx_std_push(ls, mx_obj_of_string(mx_str_cache_get(ls->vm, str, (uint32_t)len)));
    } else {
        mx_str_t *buf = mx_str_new_gc_sized(ls->vm, (uint32_t)len);
        mx_str_append(buf, str, (uint32_t)len);
        mx_std_push(ls, mx_obj_of_string(buf));
    }
}

mx_obj_t mx_std_char_str_new(mx_t *vm, char c) {
    mx_obj_t s = mx_obj_intern_strn(vm, &c, 1);
    vm->char_strs[(uint8_t)c] = s;
    return s;
}

void mx_std_pushstring(mx_std_state_t *ls, const char *str) {
    if (str == NULL) {
        mx_std_pushnil(ls);
    } else {
        mx_std_pushlstring(ls, str, strlen(str));
    }
}

void mx_std_pushcfn(mx_std_state_t *ls, mx_std_fn_t *fn, const char *name) {
    mx_std_push(ls, mx_std_cfn_new_std(ls->vm, fn, name, 0, NULL));
}

void mx_std_newtable(mx_std_state_t *ls) {
    mx_std_push(ls, mx_obj_of_table(mx_table_new_short(ls->vm)));
}

void mx_std_createtable(mx_std_state_t *ls, uint32_t narr, uint32_t nhash) {
    mx_std_push(ls, mx_obj_of_table(mx_table_new_sized(ls->vm, narr, nhash)));
}

mx_lua_type_t mx_std_rawget(mx_std_state_t *ls, mx_std_slot_t table) {
    mx_obj_t *t = mx_std_at(ls, table);
    mx_obj_t *slots = mx_std_slots(ls);
    mx_obj_t got = mx_obj_of_nil();
    if (t != NULL && mx_obj_is_table(*t)) {
        got = mx_table_get(mx_obj_get_table(*t), slots[ls->top - 1]);
    }
    slots[ls->top - 1] = got;
    return mx_std_type(ls, -1);
}

void mx_std_rawset(mx_std_state_t *ls, mx_std_slot_t table) {
    mx_obj_t *t = mx_std_at(ls, table);
    mx_obj_t *slots = mx_std_slots(ls);
    mx_obj_t key = slots[ls->top - 2];
    mx_obj_t val = slots[ls->top - 1];
    if (t != NULL && mx_obj_is_table(*t)) {
        if (mx_obj_is_nil(key)) {
            mx_std_pop(ls, 2);
            mx_std_error(ls, "index is nil");
            return;
        } else if (mx_obj_is_float(key) && mx_obj_get_float(key) != mx_obj_get_float(key)) {
            mx_std_pop(ls, 2);
            mx_std_error(ls, "index is NaN");
            return;
        }
        mx_table_set(ls->vm, mx_obj_get_table(*t), key, val);
    }
    mx_std_pop(ls, 2);
}

mx_lua_type_t mx_std_rawgeti(mx_std_state_t *ls, mx_std_slot_t table, int64_t i) {
    mx_obj_t *t = mx_std_at(ls, table);
    mx_obj_t got = mx_obj_of_nil();
    if (t != NULL && mx_obj_is_table(*t)) {
        got = mx_table_get(mx_obj_get_table(*t), mx_obj_of_int(i));
    }
    mx_std_push(ls, got);
    return mx_std_type(ls, -1);
}

void mx_std_rawseti(mx_std_state_t *ls, mx_std_slot_t table, int64_t i) {
    mx_obj_t *t = mx_std_at(ls, table);
    mx_obj_t val = mx_std_slots(ls)[ls->top - 1];
    if (t != NULL && mx_obj_is_table(*t)) {
        mx_table_set(ls->vm, mx_obj_get_table(*t), mx_obj_of_int(i), val);
    }
    mx_std_pop(ls, 1);
}

mx_lua_type_t mx_std_geti(mx_std_state_t *ls, mx_std_slot_t table, int64_t i) {
    mx_std_slot_t abs = mx_std_absindex(ls, table);
    mx_obj_t *t = mx_std_at(ls, abs);
    if (t != NULL && mx_obj_is_table(*t)) {
        mx_obj_table_t *tbl = mx_obj_get_table(*t);
        if (tbl->is_plain && tbl->meta == NULL) {
            mx_std_push(ls, mx_table_get(tbl, mx_obj_of_int(i)));
            return mx_std_type(ls, -1);
        }
    }
    mx_std_pushinteger(ls, i);
    return mx_std_gettable(ls, abs);
}

void mx_std_seti(mx_std_state_t *ls, mx_std_slot_t table, int64_t i) {
    mx_std_slot_t abs = mx_std_absindex(ls, table);
    mx_std_pushinteger(ls, i);
    mx_std_insert(ls, -2);
    mx_std_settable(ls, abs);
}

mx_lua_type_t mx_std_getfield(mx_std_state_t *ls, mx_std_slot_t table, const char *key) {
    mx_std_slot_t abs = mx_std_absindex(ls, table);
    mx_std_push(ls, mx_obj_intern(ls->vm, key));
    return mx_std_gettable(ls, abs);
}

mx_lua_type_t mx_std_getmetakey(mx_std_state_t *ls, mx_std_slot_t table, mx_lua_meta_key_t key) {
    mx_obj_t *t = mx_std_at(ls, table);
    if (t != NULL && mx_obj_is_table(*t)) {
        mx_std_push(ls, mx_table_get(mx_obj_get_table(*t), mx_lua_of(ls->vm)->meta_keys[key]));
    } else {
        mx_std_pushnil(ls);
    }
    return mx_std_type(ls, -1);
}

void mx_std_setfield(mx_std_state_t *ls, mx_std_slot_t table, const char *key) {
    mx_std_slot_t abs = mx_std_absindex(ls, table);
    mx_std_push(ls, mx_obj_intern(ls->vm, key));
    mx_std_insert(ls, -2);
    mx_std_settable(ls, abs);
}

bool mx_std_next(mx_std_state_t *ls, mx_std_slot_t table) {
    mx_obj_t *t = mx_std_at(ls, table);
    if (t == NULL || !mx_obj_is_table(*t)) {
        return false;
    }
    mx_obj_t *slots = mx_std_slots(ls);
    mx_table_pair_t pair = mx_table_pairs_next(ls->vm, mx_obj_get_table(*t), slots[ls->top - 1]);
    if (!pair.is_valid) {
        mx_std_error(ls, "invalid key to 'next'");
        return false;
    } else if (mx_obj_is_nil(pair.key)) {
        mx_std_pop(ls, 1);
        return false;
    } else {
        slots[ls->top - 1] = pair.key;
        mx_std_push(ls, pair.val);
        return true;
    }
}

void mx_std_setglobal(mx_std_state_t *ls, const char *name) {
    mx_obj_t val = mx_std_slots(ls)[ls->top - 1];
    mx_obj_t globals = mx_lua_of(ls->vm)->std;
    mx_table_set(ls->vm, mx_obj_get_table(globals), mx_obj_intern(ls->vm, name), val);
    mx_std_pop(ls, 1);
}

mx_lua_type_t mx_std_getglobal(mx_std_state_t *ls, const char *name) {
    mx_obj_t globals = mx_lua_of(ls->vm)->std;
    mx_std_push(ls, mx_table_get(mx_obj_get_table(globals), mx_obj_intern(ls->vm, name)));
    return mx_std_type(ls, -1);
}

bool mx_std_getmetatable(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_obj_t *at = mx_std_at(ls, slot);
    if (at == NULL) {
        return false;
    }
    mx_obj_table_t *meta = mx_meta_table_of(ls->vm, *at);
    if (meta == NULL) {
        return false;
    } else {
        mx_std_push(ls, mx_obj_of_table(meta));
        return true;
    }
}

void mx_std_setmetatable(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_obj_t *at = mx_std_at(ls, slot);
    mx_obj_t mt = mx_std_slots(ls)[ls->top - 1];
    mx_obj_table_t *nmeta = NULL;
    if (mx_obj_is_table(mt)) {
        nmeta = mx_obj_get_table(mt);
    }
    if (at != NULL && mx_obj_is_table(*at)) {
        mx_obj_table_t *t = mx_obj_get_table(*at);
        if (nmeta != NULL) {
            mx_gc_write_barrier(ls->vm, t, mt);
        }
        t->meta = nmeta;
        if (nmeta != NULL && !mx_obj_is_nil(mx_table_get(nmeta, mx_lua_of(ls->vm)->meta_keys[MX_LUA_META_KEY_GC]))) {
            mx_gc_note_finalizer(ls->vm, t);
        }
    } else if (at != NULL) {
        mx_lua_of(ls->vm)->type_metas[mx_meta_basic_type(ls->vm, *at)] = mt;
    }
    mx_std_pop(ls, 1);
}

mx_lua_type_t mx_std_gettable(mx_std_state_t *ls, mx_std_slot_t table) {
    mx_obj_t *t = mx_std_at(ls, table);
    mx_obj_t tv = mx_obj_of_nil();
    if (t != NULL) {
        tv = *t;
    }
    mx_obj_t key = mx_std_slots(ls)[ls->top - 1];
    mx_obj_t got = mx_meta_index(ls->vm, tv, key);
    if (mx_obj_is_error(got)) {
        mx_error_locate(ls->vm, mx_obj_get_error(got), NULL);
        ls->error = got;
        mx_std_raise(ls);
        got = mx_obj_of_nil();
    }
    mx_std_slots(ls)[ls->top - 1] = got;
    return mx_std_type(ls, -1);
}

void mx_std_settable(mx_std_state_t *ls, mx_std_slot_t table) {
    mx_obj_t *t = mx_std_at(ls, table);
    mx_obj_t tv = mx_obj_of_nil();
    if (t != NULL) {
        tv = *t;
    }
    mx_obj_t *slots = mx_std_slots(ls);
    mx_obj_t key = slots[ls->top - 2];
    mx_obj_t value = slots[ls->top - 1];
    mx_obj_t got = mx_meta_newindex(ls->vm, tv, key, value);
    mx_std_pop(ls, 2);
    if (mx_obj_is_error(got)) {
        mx_error_locate(ls->vm, mx_obj_get_error(got), NULL);
        ls->error = got;
        mx_std_raise(ls);
    }
}

bool mx_std_rawequal(mx_std_state_t *ls, mx_std_slot_t a, mx_std_slot_t b) {
    mx_obj_t *va = mx_std_at(ls, a);
    mx_obj_t *vb = mx_std_at(ls, b);
    return va != NULL && vb != NULL && mx_table_key_eq(*va, *vb);
}

bool mx_std_compare(mx_std_state_t *ls, mx_std_slot_t a, mx_std_slot_t b, mx_op_t op) {
    mx_obj_t *va = mx_std_at(ls, a);
    mx_obj_t *vb = mx_std_at(ls, b);
    if (va == NULL || vb == NULL) {
        return false;
    }
    mx_obj_t got = mx_meta_compare(ls->vm, op, *va, *vb);
    if (mx_obj_is_error(got)) {
        mx_error_locate(ls->vm, mx_obj_get_error(got), NULL);
        ls->error = got;
        mx_std_raise(ls);
        return false;
    } else {
        return mx_obj_is_truthy(got);
    }
}

void mx_std_take_rets(mx_std_state_t *ls, int32_t from, mx_std_slot_t nresults) {
    mx_t *vm = ls->vm;
    ls->top = from;
    uint32_t got = vm->rets.len;
    uint32_t want = (uint32_t)nresults;
    if (nresults == MX_STD_MULTRET) {
        want = got;
    }
    if (!mx_std_grow(ls, (int32_t)want)) {
        mx_std_error(ls, "too many results");
        return;
    }
    mx_obj_t *slots = mx_std_slots(ls);
    for (uint32_t i = 0; i < want; i++) {
        if (i < got) {
            slots[ls->top + (int32_t)i] = vm->rets.vals[i];
        } else {
            slots[ls->top + (int32_t)i] = mx_obj_of_nil();
        }
    }
    ls->top += (int32_t)want;
    mx_std_sync(ls);
}

void mx_std_call(mx_std_state_t *ls, mx_std_slot_t nargs, mx_std_slot_t nresults) {
    int32_t fn_slot = ls->top - nargs - 1;
    mx_std_sync(ls);
    mx_obj_t *slots = mx_std_slots(ls);
    mx_obj_t got = mx_call(ls->vm, slots[fn_slot], (uint32_t)nargs, slots + fn_slot + 1);
    if (mx_obj_is_error(got)) {
        ls->error = got;
        mx_std_raise(ls);
        ls->vm->rets.len = 0;
    }
    mx_std_take_rets(ls, fn_slot, nresults);
}

mx_std_status_t mx_std_pcall(mx_std_state_t *ls, mx_std_slot_t nargs, mx_std_slot_t nresults, mx_std_slot_t msgh) {
    mx_obj_t handler = mx_obj_of_nil();
    if (msgh != 0) {
        mx_obj_t *at = mx_std_at(ls, msgh);
        if (at != NULL) {
            handler = *at;
        }
    }
    int32_t fn_slot = ls->top - nargs - 1;
    mx_std_sync(ls);
    mx_obj_t *slots = mx_std_slots(ls);
    mx_obj_t got = mx_pcall(ls->vm, slots[fn_slot], (uint32_t)nargs, slots + fn_slot + 1, handler);
    if (mx_obj_is_error(got)) {
        ls->top = fn_slot;
        mx_std_push(ls, mx_error_payload(got));
        return MX_STD_ERR_RUNTIME;
    } else {
        mx_std_take_rets(ls, fn_slot, nresults);
        return MX_STD_OK;
    }
}
