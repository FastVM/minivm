
#include "std.h"
#include "../../../value/error.h"
#include "../../../value/obj.h"
#include "../../../value/string.h"

int32_t mx_std_error(mx_std_state_t *ls, const char *fmt, ...) {
    mx_str_t *msg = mx_str_new_gc(ls->vm);
    mx_std_where(ls, 1, msg);
    va_list ap;
    va_start(ap, fmt);
    mx_str_vformat(msg, fmt, ap);
    va_end(ap);
    ls->error = mx_error_value(ls->vm, mx_obj_of_string(msg));
    mx_std_raise(ls);
    return 0;
}

int32_t mx_std_error_noloc(mx_std_state_t *ls, const char *fmt, ...) {
    mx_str_t *msg = mx_str_new_gc(ls->vm);
    va_list ap;
    va_start(ap, fmt);
    mx_str_vformat(msg, fmt, ap);
    va_end(ap);
    ls->error = mx_error_value(ls->vm, mx_obj_of_string(msg));
    mx_std_raise(ls);
    return 0;
}

const char *mx_std_libname(mx_std_fn_t *fn) {
    const mx_std_reg_t *const regs[] = {
        mx_std_string_regs,
        mx_std_utf8_regs,
        mx_std_table_regs,
        mx_std_math_regs,
        mx_std_io_regs,
        mx_std_os_regs,
        mx_std_debug_regs,
        mx_std_package_regs,
        mx_std_coroutine_regs,
    };
    const char *const names[] = {"string", "utf8", "table", "math", "io", "os", "debug", "package", "coroutine"};
    for (size_t lib = 0; lib < sizeof(regs) / sizeof(regs[0]); lib++) {
        for (size_t i = 0; regs[lib][i].name != NULL; i++) {
            if (regs[lib][i].fn == fn) {
                return names[lib];
            }
        }
    }
    return NULL;
}

mx_std_qualname_t mx_std_qualname(mx_std_state_t *ls) {
    mx_std_qualname_t out = { .name = {'\0'}, .is_method = false };
    mx_varname_t site = mx_std_site_name(ls);
    const char *lib = NULL;
    if (ls->fn != NULL) {
        lib = mx_std_libname(ls->fn);
    }
    if (site.kind != MX_VARHINT_NONE && site.name != NULL) {
        snprintf(out.name, sizeof(out.name), "%s", site.name);
        out.is_method = site.kind == MX_VARHINT_METHOD;
    } else if (lib != NULL) {
        snprintf(out.name, sizeof(out.name), "%s.%s", lib, ls->fnname);
    } else if (ls->fnname != NULL) {
        snprintf(out.name, sizeof(out.name), "%s", ls->fnname);
    } else {
        snprintf(out.name, sizeof(out.name), "?");
    }
    return out;
}

int32_t mx_std_argerror(mx_std_state_t *ls, mx_std_slot_t arg, const char *msg) {
    mx_std_qualname_t qual = mx_std_qualname(ls);
    int32_t shown = (int32_t)arg;
    if (qual.is_method) {
        shown -= 1;
    }
    if (qual.is_method && shown == 0) {
        return mx_std_error(ls, "calling '%s' on bad self (%s)", qual.name, msg);
    } else {
        return mx_std_error(ls, "bad argument #%d to '%s' (%s)", shown, qual.name, msg);
    }
}

int32_t mx_std_typeerror(mx_std_state_t *ls, mx_std_slot_t arg, const char *expected) {
    const char *got;
    mx_obj_t *at = mx_std_at(ls, arg);
    mx_obj_t name = mx_obj_of_nil();
    if (at != NULL && mx_obj_is_table(*at) && mx_obj_get_table(*at)->meta != NULL) {
        name = mx_table_get(mx_obj_get_table(*at)->meta, mx_lua_of(ls->vm)->meta_keys[MX_LUA_META_KEY_NAME]);
    }
    if (mx_obj_is_string(name)) {
        got = mx_obj_get_string(name)->buf;
    } else if (at != NULL && mx_obj_is_lightuserdata(*at)) {
        got = "light userdata";
    } else {
        got = mx_std_typename(ls, mx_std_type(ls, arg));
    }
    mx_str_t *msg = mx_str_new();
    mx_str_format(msg, "%s expected, got %s", expected, got);
    int32_t ret = mx_std_argerror(ls, arg, msg->buf);
    mx_str_free(msg);
    return ret;
}

int64_t mx_std_checkinteger(mx_std_state_t *ls, mx_std_slot_t arg) {
    mx_obj_int_t v = mx_std_tointeger(ls, arg);
    if (!v.is_int && mx_std_isnumber(ls, arg)) {
        mx_std_argerror(ls, arg, "number has no integer representation");
    } else if (!v.is_int) {
        mx_std_typeerror(ls, arg, "number");
    }
    return v.value;
}

double mx_std_checkfloat(mx_std_state_t *ls, mx_std_slot_t arg) {
    mx_obj_float_t v = mx_std_tofloat(ls, arg);
    if (!v.is_num) {
        mx_std_typeerror(ls, arg, "number");
    }
    return v.value;
}

mx_std_str_t mx_std_checkstring(mx_std_state_t *ls, mx_std_slot_t arg) {
    mx_std_str_t s = mx_std_tostring(ls, arg);
    if (s.ptr == NULL) {
        mx_std_typeerror(ls, arg, "string");
        return (mx_std_str_t) { .ptr = "", .len = 0 };
    } else {
        return s;
    }
}

void mx_std_checktype(mx_std_state_t *ls, mx_std_slot_t arg, mx_lua_type_t type) {
    mx_lua_type_t got = mx_std_type(ls, arg);
    if (got == MX_LUA_TYPE_FLOAT) {
        got = MX_LUA_TYPE_INTEGER;
    }
    if (got != type) {
        mx_std_typeerror(ls, arg, mx_std_typename(ls, type));
    }
}

void mx_std_checkany(mx_std_state_t *ls, mx_std_slot_t arg) {
    if (mx_std_isnone(ls, arg)) {
        mx_std_argerror(ls, arg, "value expected");
    }
}

int64_t mx_std_optinteger(mx_std_state_t *ls, mx_std_slot_t arg, int64_t def) {
    if (mx_std_isnoneornil(ls, arg)) {
        return def;
    } else {
        return mx_std_checkinteger(ls, arg);
    }
}

double mx_std_optfloat(mx_std_state_t *ls, mx_std_slot_t arg, double def) {
    if (mx_std_isnoneornil(ls, arg)) {
        return def;
    } else {
        return mx_std_checkfloat(ls, arg);
    }
}

mx_std_str_t mx_std_optstring(mx_std_state_t *ls, mx_std_slot_t arg, const char *def) {
    if (!mx_std_isnoneornil(ls, arg)) {
        return mx_std_checkstring(ls, arg);
    } else if (def == NULL) {
        return (mx_std_str_t) { .ptr = NULL, .len = 0 };
    } else {
        return (mx_std_str_t) { .ptr = def, .len = strlen(def) };
    }
}

int32_t mx_std_checkoption_at(mx_std_state_t *ls, mx_std_slot_t slot, mx_std_slot_t argnum, const char *def, const char *const opts[]) {
    const char *name;
    if (def == NULL || !mx_std_isnoneornil(ls, slot)) {
        name = mx_std_checkstring(ls, slot).ptr;
    } else {
        name = def;
    }
    for (int32_t i = 0; opts[i] != NULL; i++) {
        if (strcmp(opts[i], name) == 0) {
            return i;
        }
    }
    mx_str_t *msg = mx_str_new();
    mx_str_format(msg, "invalid option '%s'", name);
    mx_std_argerror(ls, argnum, msg->buf);
    mx_str_free(msg);
    return 0;
}

int32_t mx_std_checkoption(mx_std_state_t *ls, mx_std_slot_t arg, const char *def, const char *const opts[]) {
    return mx_std_checkoption_at(ls, arg, arg, def, opts);
}

mx_obj_strtod_t mx_std_str2d_raw(const char *str) {
    char *e = NULL;
    double v = strtod(str, &e);
    const char *rest = mx_std_skip_space(e);
    if (e == str || *rest != '\0') {
        return (mx_obj_strtod_t) { .value = 0.0, .end = str };
    } else {
        return (mx_obj_strtod_t) { .value = v, .end = rest };
    }
}

mx_obj_strtod_t mx_std_str2d(const char *str) {
    for (const char *p = str; *p != '\0'; p++) {
        if (*p == 'n' || *p == 'N') {
            return (mx_obj_strtod_t) { .value = 0.0, .end = str };
        }
    }
    mx_obj_strtod_t got = mx_std_str2d_raw(str);
    char point = localeconv()->decimal_point[0];
    const char *dot = strchr(str, '.');
    size_t len = strlen(str);
    char copy[200];
    if (got.end != str || point == '.' || dot == NULL || len >= sizeof(copy)) {
        return got;
    }
    memcpy(copy, str, len + 1);
    copy[dot - str] = point;
    mx_obj_strtod_t local = mx_std_str2d_raw(copy);
    if (local.end == copy) {
        return got;
    } else {
        return (mx_obj_strtod_t) { .value = local.value, .end = str + (local.end - copy) };
    }
}

mx_obj_num_t mx_std_str2int(const char *str) {
    const char *s = mx_std_skip_space(str);
    bool neg = *s == '-';
    if (*s == '+' || *s == '-') {
        s += 1;
    }
    uint64_t acc = 0;
    bool any = false;
    bool is_overflow = false;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        for (s += 2; isxdigit((uint8_t)*s) != 0; s++) {
            acc = acc * 16 + (uint64_t)mx_std_base_digit(*s);
            any = true;
        }
    } else {
        uint64_t maxby10 = (uint64_t)INT64_MAX / 10;
        uint64_t maxlast = (uint64_t)INT64_MAX % 10;
        for (; isdigit((uint8_t)*s) != 0; s++) {
            uint64_t d = (uint64_t)(*s - '0');
            if (acc > maxby10 || (acc == maxby10 && d > maxlast + (uint64_t)neg)) {
                is_overflow = true;
            }
            acc = acc * 10 + d;
            any = true;
        }
    }
    s = mx_std_skip_space(s);
    if (!any || is_overflow || *s != '\0') {
        return (mx_obj_num_t) { .value = mx_obj_of_nil(), .is_num = false };
    } else if (neg) {
        return (mx_obj_num_t) { .value = mx_obj_of_int((int64_t)((uint64_t)0 - acc)), .is_num = true };
    } else {
        return (mx_obj_num_t) { .value = mx_obj_of_int((int64_t)acc), .is_num = true };
    }
}

mx_obj_num_t mx_std_str2float(const char *str) {
    mx_obj_strtod_t fv = mx_std_str2d(str);
    if (fv.end != str) {
        return (mx_obj_num_t) { .value = mx_obj_of_number(fv.value), .is_num = true };
    } else {
        return (mx_obj_num_t) { .value = mx_obj_of_nil(), .is_num = false };
    }
}

mx_obj_num_t mx_std_str2num(const char *str, size_t len) {
    mx_obj_num_t got = mx_std_str2int(str);
    if (strlen(str) != len) {
        return (mx_obj_num_t) { .value = mx_obj_of_nil(), .is_num = false };
    } else if (got.is_num) {
        return got;
    } else {
        return mx_std_str2float(str);
    }
}

void mx_std_pushfstring(mx_std_state_t *ls, const char *fmt, ...) {
    mx_str_t *buf = mx_str_new();
    va_list ap;
    va_start(ap, fmt);
    mx_str_vformat(buf, fmt, ap);
    va_end(ap);
    mx_std_pushlstring(ls, buf->buf, buf->len);
    mx_str_free(buf);
}

void mx_std_register(mx_std_state_t *ls, const mx_std_reg_t *fns) {
    for (int32_t i = 0; fns[i].name != NULL; i++) {
        if (fns[i].ffi != NULL) {
            mx_std_push(ls, mx_obj_of_ffi(fns[i].ffi));
        } else {
            mx_std_pushcfn(ls, fns[i].fn, fns[i].name);
        }
        mx_std_setfield(ls, -2, fns[i].name);
    }
}

void mx_std_newlib(mx_std_state_t *ls, const char *name, const mx_std_reg_t *fns) {
    mx_std_newtable(ls);
    mx_std_register(ls, fns);
    if (name != NULL) {
        mx_std_pushvalue(ls, -1);
        mx_std_setglobal(ls, name);
    }
}

void mx_std_buf_init(mx_std_state_t *ls, mx_std_buf_t *buf) {
    buf->ls = ls;
    buf->data = buf->init;
    buf->len = 0;
    buf->cap = sizeof(buf->init);
}

char *mx_std_buf_prep(mx_std_buf_t *buf, size_t need) {
    if (buf->len + need <= buf->cap) {
        return buf->data + buf->len;
    }
    size_t ncap = buf->cap * 2;
    while (ncap < buf->len + need && need <= MX_STD_MAX_STRING) {
        ncap *= 2;
    }
    bool is_large = need > MX_STD_MAX_STRING - buf->len;
    if (is_large || !mx_mem_fits(ncap)) {
        if (buf->data != buf->init) {
            mx_mem_free(buf->data);
        }
        buf->data = buf->init;
        buf->len = 0;
        buf->cap = sizeof(buf->init);
        if (is_large) {
            mx_std_error(buf->ls, "resulting string too large");
        } else {
            mx_std_error_noloc(buf->ls, "not enough memory");
        }
        return NULL;
    } else {
        if (buf->data == buf->init) {
            buf->data = mx_mem_alloc(ncap);
            memcpy(buf->data, buf->init, buf->len);
        } else {
            buf->data = mx_mem_realloc(buf->data, ncap);
        }
        buf->cap = ncap;
        return buf->data + buf->len;
    }
}

void mx_std_buf_addlstring(mx_std_buf_t *buf, const char *data, size_t len) {
    char *dst = mx_std_buf_prep(buf, len);
    if (dst != NULL) {
        memcpy(dst, data, len);
        buf->len += len;
    }
}

void mx_std_buf_addstring(mx_std_buf_t *buf, const char *str) {
    mx_std_buf_addlstring(buf, str, strlen(str));
}

void mx_std_buf_addchar(mx_std_buf_t *buf, char c) {
    char *dst = mx_std_buf_prep(buf, 1);
    if (dst != NULL) {
        *dst = c;
        buf->len += 1;
    }
}

void mx_std_buf_addint(mx_std_buf_t *buf, int64_t v) {
    char tmp[MX_STR_INT_DIGITS];
    uint32_t at = mx_str_int_digits(tmp, v);
    mx_std_buf_addlstring(buf, &tmp[at], MX_STR_INT_DIGITS - at);
}

void mx_std_buf_addvalue(mx_std_buf_t *buf, mx_std_slot_t slot) {
    mx_std_str_t s = mx_std_tostring(buf->ls, slot);
    if (s.ptr != NULL) {
        mx_std_buf_addlstring(buf, s.ptr, s.len);
    }
}

void mx_std_buf_push(mx_std_buf_t *buf) {
    mx_std_pushlstring(buf->ls, buf->data, buf->len);
    if (buf->data != buf->init) {
        mx_mem_free(buf->data);
        buf->data = buf->init;
    }
    buf->len = 0;
    buf->cap = sizeof(buf->init);
}

void mx_std_buf_addfmt(mx_std_buf_t *buf, const char *spec, ...) {
    char out[MX_STD_FMT_OUT_CAP];
    va_list ap;
    va_start(ap, spec);
    int32_t n = vsnprintf(out, sizeof(out), spec, ap);
    va_end(ap);
    if (n >= 0) {
        if ((size_t)n < sizeof(out)) {
            mx_std_buf_addlstring(buf, out, (size_t)n);
        } else {
            char *big = mx_mem_alloc((size_t)n + 1);
            va_start(ap, spec);
            vsnprintf(big, (size_t)n + 1, spec, ap);
            va_end(ap);
            mx_std_buf_addlstring(buf, big, (size_t)n);
            mx_mem_free(big);
        }
    }
}
