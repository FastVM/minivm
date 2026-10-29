
#include "../api/std.h"
#include "../lua.h"
#include "../pattern.h"
#include "../../front/dump.h"
#include "../../../value/error.h"
#include "../../../value/obj.h"
#include "../../../value/string.h"
#include "../../../value/table.h"

enum {
    MX_STD_FMT_SPEC_CAP = 32,
    MX_STD_GSUB_PATTERN_INLINE = 128,
};

int64_t mx_std_str_relat(int64_t pos, size_t len) {
    if (pos >= 0) {
        return pos;
    } else if ((uint64_t)(0 - pos) > len) {
        return 0;
    } else {
        return (int64_t)len + pos + 1;
    }
}

int32_t mx_std_string_len(mx_std_state_t *ls) {
    mx_std_str_t s = mx_std_checkstring(ls, 1);
    mx_std_pushinteger(ls, (int64_t)s.len);
    return 1;
}

int32_t mx_std_string_sub(mx_std_state_t *ls) {
    mx_std_str_t str = mx_std_checkstring(ls, 1);
    size_t len = str.len;
    const char *s = str.ptr;
    int64_t i = mx_std_str_relat(mx_std_checkinteger(ls, 2), len);
    int64_t j = mx_std_str_relat(mx_std_optinteger(ls, 3, -1), len);
    if (i < 1) {
        i = 1;
    }
    if (j > (int64_t)len) {
        j = (int64_t)len;
    }
    if (i <= j) {
        mx_std_pushlstring(ls, s + i - 1, (size_t)(j - i + 1));
    } else {
        mx_std_pushlstring(ls, "", 0);
    }
    return 1;
}

int32_t mx_std_string_upper(mx_std_state_t *ls) {
    mx_std_str_t str = mx_std_checkstring(ls, 1);
    mx_std_buf_t buf;
    mx_std_buf_init(ls, &buf);
    char *dst = mx_std_buf_prep(&buf, str.len);
    if (dst == NULL) {
        return 0;
    }
    for (size_t i = 0; i < str.len; i++) {
        dst[i] = (char)toupper((uint8_t)str.ptr[i]);
    }
    buf.len = str.len;
    mx_std_buf_push(&buf);
    return 1;
}

int32_t mx_std_string_lower(mx_std_state_t *ls) {
    mx_std_str_t str = mx_std_checkstring(ls, 1);
    mx_std_buf_t buf;
    mx_std_buf_init(ls, &buf);
    char *dst = mx_std_buf_prep(&buf, str.len);
    if (dst == NULL) {
        return 0;
    }
    for (size_t i = 0; i < str.len; i++) {
        dst[i] = (char)tolower((uint8_t)str.ptr[i]);
    }
    buf.len = str.len;
    mx_std_buf_push(&buf);
    return 1;
}

int32_t mx_std_string_reverse(mx_std_state_t *ls) {
    mx_std_str_t str = mx_std_checkstring(ls, 1);
    mx_std_buf_t buf;
    mx_std_buf_init(ls, &buf);
    char *dst = mx_std_buf_prep(&buf, str.len);
    if (dst == NULL) {
        return 0;
    }
    for (size_t i = 0; i < str.len; i++) {
        dst[i] = str.ptr[str.len - 1 - i];
    }
    buf.len = str.len;
    mx_std_buf_push(&buf);
    return 1;
}

int32_t mx_std_string_rep(mx_std_state_t *ls) {
    mx_std_str_t str = mx_std_checkstring(ls, 1);
    size_t len = str.len;
    const char *s = str.ptr;
    int64_t n = mx_std_checkinteger(ls, 2);
    mx_std_str_t sepstr = mx_std_optstring(ls, 3, "");
    size_t seplen = sepstr.len;
    const char *sep = sepstr.ptr;
    size_t unit = len + seplen;
    if (n <= 0 || unit == 0) {
        mx_std_pushlstring(ls, "", 0);
        return 1;
    } else {
        if ((uint64_t)n > MX_STD_MAX_STRING / unit) {
            return mx_std_error(ls, "resulting string too large");
        }
        size_t total = (size_t)n * len + (size_t)(n - 1) * seplen;
        if (!mx_mem_fits(sizeof(mx_str_t) + total + 1)) {
            return mx_std_error_noloc(ls, "not enough memory");
        } else if (total <= MX_INTERN_MAX_LEN) {
            char tmp[MX_INTERN_MAX_LEN];
            size_t at = 0;
            for (int64_t i = 0; i < n; i++) {
                if (i > 0) {
                    memcpy(tmp + at, sep, seplen);
                    at += seplen;
                }
                memcpy(tmp + at, s, len);
                at += len;
            }
            mx_std_push(ls, mx_obj_intern_strn(ls->vm, tmp, (uint32_t)total));
            return 1;
        }
        mx_str_t *out = mx_str_new_gc_sized(ls->vm, (uint32_t)total);
        for (int64_t i = 0; i < n; i++) {
            if (i > 0 && seplen > 0) {
                mx_str_append(out, sep, (uint32_t)seplen);
            }
            mx_str_append(out, s, (uint32_t)len);
        }
        mx_std_push(ls, mx_obj_of_string(out));
        return 1;
    }
}

int32_t mx_std_string_byte(mx_std_state_t *ls) {
    mx_std_str_t str = mx_std_checkstring(ls, 1);
    size_t len = str.len;
    const char *s = str.ptr;
    int64_t i = mx_std_str_relat(mx_std_optinteger(ls, 2, 1), len);
    int64_t j = mx_std_str_relat(mx_std_optinteger(ls, 3, i), len);
    if (i < 1) {
        i = 1;
    }
    if (j > (int64_t)len) {
        j = (int64_t)len;
    }
    if (i > j) {
        return 0;
    } else if (j - i >= INT32_MAX) {
        return mx_std_error(ls, "string slice too long");
    }
    int32_t n = (int32_t)(j - i + 1);
    for (int32_t k = 0; k < n; k++) {
        mx_std_pushinteger(ls, (uint8_t)s[i - 1 + k]);
    }
    return n;
}

int32_t mx_std_string_char(mx_std_state_t *ls) {
    int32_t n = mx_std_top(ls);
    mx_std_buf_t buf;
    mx_std_buf_init(ls, &buf);
    char *dst = mx_std_buf_prep(&buf, (size_t)n);
    if (dst == NULL) {
        return 0;
    }
    for (int32_t i = 1; i <= n && !ls->vm->std_error.is_set; i++) {
        int64_t c = mx_std_checkinteger(ls, i);
        if (c < 0 || c > UINT8_MAX) {
            mx_std_argerror(ls, i, "value out of range");
        } else {
            dst[buf.len] = (char)c;
            buf.len += 1;
        }
    }
    mx_std_buf_push(&buf);
    return 1;
}

void mx_std_str_addquoted(mx_std_buf_t *buf, const char *s, size_t len) {
    mx_std_buf_addchar(buf, '"');
    for (size_t i = 0; i < len; i++) {
        uint8_t c = (uint8_t)s[i];
        if (c == '"' || c == '\\' || c == '\n') {
            mx_std_buf_addchar(buf, '\\');
            mx_std_buf_addchar(buf, (char)c);
        } else if (c == '\0') {
            if (i + 1 < len && isdigit((uint8_t)s[i + 1]) != 0) {
                mx_std_buf_addstring(buf, "\\000");
            } else {
                mx_std_buf_addstring(buf, "\\0");
            }
        } else if (iscntrl(c) != 0) {
            if (i + 1 < len && isdigit((uint8_t)s[i + 1]) != 0) {
                mx_std_buf_addfmt(buf, "\\%03d", (int32_t)c);
            } else {
                mx_std_buf_addfmt(buf, "\\%d", (int32_t)c);
            }
        } else {
            mx_std_buf_addchar(buf, (char)c);
        }
    }
    mx_std_buf_addchar(buf, '"');
}

void mx_std_str_addliteral(mx_std_state_t *ls, mx_std_buf_t *buf, int32_t arg) {
    mx_obj_t *at = mx_std_at(ls, arg);
    if (at != NULL && mx_obj_is_string(*at)) {
        mx_str_t *s = mx_obj_get_string(*at);
        mx_std_str_addquoted(buf, s->buf, s->len);
    } else if (at != NULL && mx_obj_is_int(*at)) {
        int64_t v = mx_obj_get_int(*at);
        if (v == INT64_MIN) {
            mx_std_buf_addfmt(buf, "0x%llx", (unsigned long long)v);
        } else {
            mx_std_buf_addint(buf, v);
        }
    } else if (at != NULL && mx_obj_is_float(*at)) {
        double d = mx_obj_get_float(*at);
        if (d == (double)HUGE_VAL) {
            mx_std_buf_addstring(buf, "1e9999");
        } else if (d == -(double)HUGE_VAL) {
            mx_std_buf_addstring(buf, "-1e9999");
        } else if (d != d) {
            mx_std_buf_addstring(buf, "(0/0)");
        } else {
            char tmp[MX_STR_FLOAT_CAP];
            int32_t n = snprintf(tmp, sizeof(tmp), "%a", d);
            char *point = memchr(tmp, localeconv()->decimal_point[0], (size_t)n);
            if (point != NULL) {
                *point = '.';
            }
            mx_std_buf_addlstring(buf, tmp, (size_t)n);
        }
    } else if (at != NULL && (mx_obj_is_nil(*at) || mx_obj_is_boolean(*at))) {
        mx_std_tostring_meta(ls, arg);
        mx_std_buf_addvalue(buf, -1);
        mx_std_pop(ls, 1);
    } else {
        mx_std_argerror(ls, arg, "value has no literal form");
    }
}

int32_t mx_std_string_format(mx_std_state_t *ls) {
    mx_std_str_t fmtstr = mx_std_checkstring(ls, 1);
    const char *fmt = fmtstr.ptr;
    int32_t arg = 1;
    mx_std_buf_t buf;
    mx_std_buf_init(ls, &buf);
    const char *end = fmt + fmtstr.len;
    for (const char *p = fmt; p < end && !ls->vm->std_error.is_set;) {
        if (*p != '%') {
            mx_std_buf_addchar(&buf, *p);
            p += 1;
        } else if (p + 1 < end && p[1] == '%') {
            mx_std_buf_addchar(&buf, '%');
            p += 2;
        } else {
            mx_std_fmt_step_t step = mx_std_fmt_one(ls, &buf, p + 1, end, arg);
            if (step.next == NULL) {
                if (buf.data != buf.init) {
                    mx_mem_free(buf.data);
                }
                return step.ret_count;
            }
            p = step.next;
            arg = step.arg;
        }
    }
    mx_std_buf_push(&buf);
    return 1;
}

mx_std_fmt_step_t mx_std_fmt_fail(mx_std_state_t *ls, int32_t nret) {
    return (mx_std_fmt_step_t) { .next = NULL, .arg = 0, .ret_count = nret };
}

const char *mx_std_fmt_digits(const char *s) {
    for (int32_t i = 0; i < 2 && isdigit((uint8_t)*s) != 0; i++) {
        s += 1;
    }
    return s;
}

bool mx_std_fmt_valid(const char *spec, const char *flags, bool has_precision) {
    const char *s = spec + 1;
    s += strspn(s, flags);
    if (*s != '0') {
        s = mx_std_fmt_digits(s);
        if (*s == '.' && has_precision) {
            s = mx_std_fmt_digits(s + 1);
        }
    }
    return isalpha((uint8_t)*s) != 0;
}

mx_std_fmt_step_t mx_std_fmt_badspec(mx_std_state_t *ls, const char *spec) {
    return mx_std_fmt_fail(ls, mx_std_error(ls, "invalid conversion specification: '%s'", spec));
}

void mx_std_fmt_addlenmod(char *spec, const char *mod) {
    size_t len = strlen(spec);
    size_t modlen = strlen(mod);
    char conv = spec[len - 1];
    memcpy(spec + len - 1, mod, modlen);
    spec[len + modlen - 1] = conv;
    spec[len + modlen] = '\0';
}

mx_std_fmt_step_t mx_std_fmt_one(mx_std_state_t *ls, mx_std_buf_t *buf, const char *p, const char *end, int32_t arg) {
    arg += 1;
    if (arg > mx_std_top(ls)) {
        return mx_std_fmt_fail(ls, mx_std_argerror(ls, arg, "no value"));
    }
    const char *start = p;
    while (p < end && *p != '\0' && strchr("-+ #0123456789.", *p) != NULL) {
        p += 1;
    }
    size_t speclen = (size_t)(p - start) + 1;
    if (speclen >= MX_STD_FMT_SPEC_CAP - 10) {
        return mx_std_fmt_fail(ls, mx_std_error(ls, "invalid format (too long)"));
    }
    char conv;
    if (p < end) {
        conv = *p;
        p += 1;
    } else {
        conv = '\0';
    }
    char spec[MX_STD_FMT_SPEC_CAP];
    spec[0] = '%';
    memcpy(spec + 1, start, speclen - 1);
    spec[speclen] = conv;
    spec[speclen + 1] = '\0';
    bool has_mods = speclen > 1;
    switch (conv) {
        case 'c': {
            if (!mx_std_fmt_valid(spec, "-", false)) {
                return mx_std_fmt_badspec(ls, spec);
            }
            mx_std_buf_addfmt(buf, spec, (int32_t)mx_std_checkinteger(ls, arg));
            break;
        }
        case 'd':
        case 'i':
        case 'u':
        case 'o':
        case 'x':
        case 'X': {
            int64_t v = mx_std_checkinteger(ls, arg);
            const char *flags;
            if (conv == 'd' || conv == 'i') {
                flags = "-+0 ";
            } else if (conv == 'u') {
                flags = "-0";
            } else {
                flags = "-#0";
            }
            if (!mx_std_fmt_valid(spec, flags, true)) {
                return mx_std_fmt_badspec(ls, spec);
            }
            mx_std_fmt_addlenmod(spec, "ll");
            if (conv == 'd' || conv == 'i') {
                mx_std_buf_addfmt(buf, spec, (long long)v);
            } else {
                mx_std_buf_addfmt(buf, spec, (unsigned long long)v);
            }
            break;
        }
        case 'a':
        case 'A': {
            if (!mx_std_fmt_valid(spec, "-+ #0", true)) {
                return mx_std_fmt_badspec(ls, spec);
            }
            mx_std_buf_addfmt(buf, spec, mx_std_checkfloat(ls, arg));
            break;
        }
        case 'e':
        case 'E':
        case 'f':
        case 'g':
        case 'G': {
            double d = mx_std_checkfloat(ls, arg);
            if (!mx_std_fmt_valid(spec, "-+ #0", true)) {
                return mx_std_fmt_badspec(ls, spec);
            }
            mx_std_buf_addfmt(buf, spec, d);
            break;
        }
        case 'p': {
            if (!mx_std_fmt_valid(spec, "-", false)) {
                return mx_std_fmt_badspec(ls, spec);
            }
            mx_obj_t *at = mx_std_at(ls, arg);
            const void *ptr = NULL;
            if (at != NULL && (mx_obj_kind(*at) >= MX_KIND_FIRST_DATA || mx_obj_is_string(*at) || mx_obj_is_closure(*at) || mx_obj_is_ffi(*at))) {
                ptr = mx_ref_obj(at->val.ref);
            }
            if (ptr == NULL) {
                spec[speclen] = 's';
                mx_std_buf_addfmt(buf, spec, "(null)");
            } else {
                mx_std_buf_addfmt(buf, spec, ptr);
            }
            break;
        }
        case 'q': {
            if (has_mods) {
                return mx_std_fmt_fail(ls, mx_std_error(ls, "specifier '%%q' cannot have modifiers"));
            }
            mx_std_str_addliteral(ls, buf, arg);
            break;
        }
        case 's': {
            mx_std_tostring_meta(ls, arg);
            mx_std_str_t s = mx_std_tostring(ls, -1);
            if (s.ptr != NULL) {
                if (!has_mods) {
                    mx_std_buf_addlstring(buf, s.ptr, s.len);
                } else if (s.len != strlen(s.ptr)) {
                    return mx_std_fmt_fail(ls, mx_std_argerror(ls, arg, "string contains zeros"));
                } else if (!mx_std_fmt_valid(spec, "-", true)) {
                    return mx_std_fmt_badspec(ls, spec);
                } else if (strchr(spec, '.') == NULL && s.len >= 100) {
                    mx_std_buf_addlstring(buf, s.ptr, s.len);
                } else {
                    mx_std_buf_addfmt(buf, spec, s.ptr);
                }
            }
            mx_std_pop(ls, 1);
            break;
        }
        default: {
            return mx_std_fmt_fail(ls, mx_std_error(ls, "invalid conversion '%s' to 'format'", spec));
        }
    }
    return (mx_std_fmt_step_t) { .next = p, .arg = arg, .ret_count = 0 };
}

bool mx_std_pat_has_special(const char *p, size_t plen) {
    for (size_t i = 0; i < plen; i++) {
        if (strchr("^$*+?.([%-", p[i]) != NULL) {
            return true;
        }
    }
    return false;
}

const char *mx_std_pat_lmemfind(const char *s1, size_t l1, const char *s2, size_t l2) {
    if (l2 == 0) {
        return s1;
    } else if (l2 > l1) {
        return NULL;
    } else {
        l2 -= 1;
        l1 = l1 - l2;
        while (l1 > 0) {
            const char *init = memchr(s1, *s2, l1);
            if (init == NULL) {
                return NULL;
            }
            init += 1;
            if (memcmp(init, s2 + 1, l2) == 0) {
                return init - 1;
            }
            l1 -= (size_t)(init - s1);
            s1 = init;
        }
        return NULL;
    }
}

mx_pattern_result_t mx_std_pat_match(mx_std_state_t *ls, mx_pattern_captures_t *scratch, const char *s, size_t len, const char *p, size_t lp, size_t init) {
    mx_pattern_result_t res = mx_pattern_match(&mx_lua_of(ls->vm)->patterns, scratch, s, len, p, lp, init);
    if (res.status == MX_PATTERN_ERROR) {
        mx_std_error(ls, "%s", res.error);
        res.status = MX_PATTERN_NOMATCH;
    }
    return res;
}

void mx_std_pat_push_one(mx_std_state_t *ls, const char *s, const mx_pattern_match_t *m, size_t idx) {
    if (idx >= m->len) {
        if (idx != 0) {
            mx_std_error(ls, "invalid capture index %%%d", (int32_t)idx + 1);
        } else {
            mx_std_pushlstring(ls, s + m->start, m->stop - m->start);
        }
    } else {
        const mx_pattern_capture_t *cap = &m->captures[idx];
        if (cap->is_position) {
            mx_std_pushinteger(ls, (int64_t)cap->start + 1);
        } else {
            mx_std_pushlstring(ls, s + cap->start, cap->len);
        }
    }
}

int32_t mx_std_pat_push_captures(mx_std_state_t *ls, const char *s, const mx_pattern_match_t *m, bool whole) {
    if (m->is_unfinished) {
        return mx_std_error(ls, "unfinished capture");
    }
    size_t n = m->len;
    if (n == 0 && whole) {
        n = 1;
    }
    for (size_t i = 0; i < n; i++) {
        mx_std_pat_push_one(ls, s, m, i);
    }
    return (int32_t)n;
}

int32_t mx_std_str_find_aux(mx_std_state_t *ls, bool find) {
    mx_std_str_t sv = mx_std_checkstring(ls, 1);
    mx_std_str_t pv = mx_std_checkstring(ls, 2);
    const char *s = sv.ptr;
    const char *p = pv.ptr;
    size_t ls_len = sv.len;
    size_t lp = pv.len;
    int64_t init = mx_std_str_relat(mx_std_optinteger(ls, 3, 1), ls_len);
    if (init < 1) {
        init = 1;
    }
    if (init > (int64_t)ls_len + 1) {
        mx_std_pushnil(ls);
        return 1;
    } else if (find && (mx_std_toboolean(ls, 4) || !mx_std_pat_has_special(p, lp))) {
        const char *s2 = mx_std_pat_lmemfind(s + init - 1, ls_len - (size_t)init + 1, p, lp);
        if (s2 != NULL) {
            mx_std_pushinteger(ls, (s2 - s) + 1);
            mx_std_pushinteger(ls, (s2 - s) + (int64_t)lp);
            return 2;
        } else {
            mx_std_pushnil(ls);
            return 1;
        }
    } else {
        mx_pattern_captures_t scratch;
        mx_pattern_result_t res = mx_std_pat_match(ls, &scratch, s, ls_len, p, lp, (size_t)init - 1);
        if (res.status != MX_PATTERN_MATCHED) {
            mx_std_pushnil(ls);
            return 1;
        } else if (find) {
            mx_std_pushinteger(ls, (int64_t)res.match.start + 1);
            mx_std_pushinteger(ls, (int64_t)res.match.stop);
            return mx_std_pat_push_captures(ls, s, &res.match, false) + 2;
        } else {
            return mx_std_pat_push_captures(ls, s, &res.match, true);
        }
    }
}

int32_t mx_std_string_find(mx_std_state_t *ls) {
    return mx_std_str_find_aux(ls, true);
}

int32_t mx_std_string_match(mx_std_state_t *ls) {
    return mx_std_str_find_aux(ls, false);
}

int32_t mx_std_string_gmatch_iter(mx_std_state_t *ls) {
    mx_obj_t sobj = mx_std_upval(ls, 0);
    mx_obj_t pobj = mx_std_upval(ls, 1);
    mx_str_t *sb = mx_obj_get_string(sobj);
    mx_str_t *pb = mx_obj_get_string(pobj);
    const char *s = sb->buf;
    size_t ls_len = sb->len;
    const char *p = pb->buf;
    size_t lp = pb->len;
    int64_t pos = mx_obj_get_int(mx_std_upval(ls, 2));
    int64_t lastmatch = mx_obj_get_int(mx_std_upval(ls, 3));
    while (pos <= (int64_t)ls_len) {
        mx_pattern_captures_t scratch;
        mx_pattern_result_t res = mx_pattern_match_unanchored(&mx_lua_of(ls->vm)->patterns, &scratch, s, ls_len, p, lp, (size_t)pos);
        if (res.status == MX_PATTERN_ERROR) {
            return mx_std_error(ls, "%s", res.error);
        } else if (res.status != MX_PATTERN_MATCHED) {
            return 0;
        } else if ((int64_t)res.match.stop == lastmatch && res.match.stop == res.match.start) {
            pos = (int64_t)res.match.start + 1;
        } else {
            mx_std_setupval(ls, 2, mx_obj_of_int((int64_t)res.match.stop));
            mx_std_setupval(ls, 3, mx_obj_of_int((int64_t)res.match.stop));
            return mx_std_pat_push_captures(ls, s, &res.match, true);
        }
    }
    return 0;
}

int32_t mx_std_string_gmatch(mx_std_state_t *ls) {
    size_t ls_len = mx_std_checkstring(ls, 1).len;
    mx_std_checkstring(ls, 2);
    int64_t init = mx_std_str_relat(mx_std_optinteger(ls, 3, 1), ls_len);
    if (init < 1) {
        init = 1;
    }
    if (init > (int64_t)ls_len + 1) {
        init = (int64_t)ls_len + 2;
    }
    mx_std_settop(ls, 2);
    mx_std_pushinteger(ls, init - 1);
    mx_std_pushinteger(ls, -1);
    mx_std_pushcfn_up(ls, mx_std_string_gmatch_iter, "gmatch", 4);
    return 1;
}

void mx_std_gsub_append_rep(mx_std_state_t *ls, mx_std_buf_t *buf, const char *s, const mx_pattern_match_t *m) {
    mx_std_str_t rep = mx_std_tostring(ls, 3);
    const char *news = rep.ptr;
    size_t l = rep.len;
    for (size_t i = 0; i < l && !ls->vm->std_error.is_set; i++) {
        if (news[i] != '%') {
            mx_std_buf_addchar(buf, news[i]);
        } else {
            i += 1;
            if (i >= l) {
                mx_std_error(ls, "invalid use of '%c' in replacement string", '%');
            } else if (news[i] == '%') {
                mx_std_buf_addchar(buf, news[i]);
            } else if (isdigit((uint8_t)news[i]) == 0) {
                mx_std_error(ls, "invalid use of '%c' in replacement string", '%');
            } else if (news[i] == '0') {
                mx_std_buf_addlstring(buf, s + m->start, m->stop - m->start);
            } else {
                mx_std_pat_push_one(ls, s, m, (size_t)(news[i] - '1'));
                if (!ls->vm->std_error.is_set) {
                    mx_std_tostring_meta(ls, -1);
                    mx_std_remove(ls, -2);
                    mx_std_buf_addvalue(buf, -1);
                    mx_std_pop(ls, 1);
                }
            }
        }
    }
}

bool mx_std_gsub_add_value(mx_std_state_t *ls, mx_std_buf_t *buf, const char *s, const mx_pattern_match_t *m, mx_lua_type_t tr) {
    if (tr != MX_LUA_TYPE_FUNCTION && tr != MX_LUA_TYPE_TABLE) {
        mx_std_gsub_append_rep(ls, buf, s, m);
        return true;
    } else {
        if (tr == MX_LUA_TYPE_FUNCTION) {
            mx_std_pushvalue(ls, 3);
            int32_t n = mx_std_pat_push_captures(ls, s, m, true);
            mx_std_call(ls, n, 1);
            if (ls->vm->std_error.is_set) {
                return false;
            }
        } else {
            mx_std_pat_push_one(ls, s, m, 0);
            mx_std_gettable(ls, 3);
        }
        if (!mx_std_toboolean(ls, -1)) {
            mx_std_pop(ls, 1);
            mx_std_buf_addlstring(buf, s + m->start, m->stop - m->start);
            return false;
        } else if (!mx_std_isstring(ls, -1)) {
            mx_std_error(ls, "invalid replacement value (a %s)", mx_std_typename(ls, mx_std_type(ls, -1)));
            return false;
        } else {
            mx_std_buf_addvalue(buf, -1);
            mx_std_pop(ls, 1);
            return true;
        }
    }
}

int32_t mx_std_string_gsub(mx_std_state_t *ls) {
    mx_std_str_t sv = mx_std_checkstring(ls, 1);
    mx_std_str_t pv = mx_std_checkstring(ls, 2);
    const char *src = sv.ptr;
    const char *p = pv.ptr;
    size_t srcl = sv.len;
    size_t lp = pv.len;
    mx_lua_type_t tr = mx_std_type(ls, 3);
    int64_t max_s = mx_std_optinteger(ls, 4, (int64_t)srcl + 1);
    bool anchor = lp > 0 && *p == '^';
    if (tr != MX_LUA_TYPE_INTEGER && tr != MX_LUA_TYPE_FLOAT && tr != MX_LUA_TYPE_STRING
        && tr != MX_LUA_TYPE_TABLE && tr != MX_LUA_TYPE_FUNCTION) {
        return mx_std_typeerror(ls, 3, "string/function/table");
    }
    char ap_buf[MX_STD_GSUB_PATTERN_INLINE];
    char *ap = ap_buf;
    if (lp + 2 > sizeof(ap_buf)) {
        ap = mx_mem_alloc(lp + 2);
    }
    size_t aplen = lp;
    if (anchor) {
        memcpy(ap, p, lp);
    } else {
        ap[0] = '^';
        memcpy(ap + 1, p, lp);
        aplen = lp + 1;
    }
    ap[aplen] = '\0';
    mx_std_buf_t b;
    mx_std_buf_init(ls, &b);
    size_t pos = 0;
    int64_t n = 0;
    bool changed = false;
    size_t lastmatch = SIZE_MAX;
    const char *sp = p;
    size_t splen = lp;
    if (anchor) {
        sp = ap;
        splen = aplen;
    }
    mx_pattern_captures_t scratch;
    bool is_anchored = splen > 0 && sp[0] == '^';
    if (is_anchored) {
        sp += 1;
        splen -= 1;
    }
    mx_pattern_cache_t *cache = &mx_lua_of(ls->vm)->patterns;
    mx_pattern_prog_t own;
    const mx_pattern_prog_t *prog;
    if (tr == MX_LUA_TYPE_FUNCTION || tr == MX_LUA_TYPE_TABLE) {
        mx_pattern_compile(cache, &own, sp, splen);
        prog = &own;
    } else {
        prog = mx_pattern_prog(cache, &own, sp, splen);
    }
    while (n < max_s && !ls->vm->std_error.is_set) {
        mx_pattern_result_t res = mx_pattern_search(&scratch, prog, src, srcl, pos, is_anchored);
        if (res.status == MX_PATTERN_ERROR) {
            mx_std_error(ls, "%s", res.error);
            break;
        } else if (res.status != MX_PATTERN_MATCHED) {
            break;
        }
        mx_pattern_match_t m = res.match;
        if (m.start > pos) {
            mx_std_buf_addlstring(&b, src + pos, m.start - pos);
            pos = m.start;
        }
        if (m.stop != lastmatch) {
            n += 1;
            bool c = mx_std_gsub_add_value(ls, &b, src, &m, tr);
            changed = changed || c;
            pos = m.stop;
            lastmatch = m.stop;
        } else if (pos < srcl) {
            mx_std_buf_addchar(&b, src[pos]);
            pos += 1;
        } else {
            break;
        }
        if (anchor) {
            break;
        }
    }
    if (prog == &own) {
        mx_pattern_prog_free(&own);
    }
    if (ap != ap_buf) {
        mx_mem_free(ap);
    }
    if (!changed) {
        if (b.data != b.init) {
            mx_mem_free(b.data);
        }
        mx_std_pushvalue(ls, 1);
    } else {
        mx_std_buf_addlstring(&b, src + pos, srcl - pos);
        mx_std_buf_push(&b);
    }
    mx_std_pushinteger(ls, n);
    return 2;
}

int32_t mx_std_string_dump(mx_std_state_t *ls) {
    mx_std_checktype(ls, 1, MX_LUA_TYPE_FUNCTION);
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    mx_obj_t fn = *mx_std_at(ls, 1);
    if (!mx_obj_is_closure(fn)) {
        return mx_std_argerror(ls, 1, "Lua function expected");
    }
    mx_str_t *out = mx_dump_function(ls->vm, mx_obj_get_closure(fn)->block, mx_std_toboolean(ls, 2));
    if (out == NULL) {
        return mx_std_error(ls, "unable to dump given function");
    }
    mx_std_pushlstring(ls, out->buf, out->len);
    mx_str_free(out);
    return 1;
}

void mx_ffi_string_find(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_string_find, "find", args, (int32_t)nargs);
}

void mx_ffi_string_format(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_string_format, "format", args, (int32_t)nargs);
}

void mx_ffi_string_gmatch(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_string_gmatch, "gmatch", args, (int32_t)nargs);
}

void mx_ffi_string_gsub(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_string_gsub, "gsub", args, (int32_t)nargs);
}

void mx_ffi_string_len(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_string_len, "len", args, (int32_t)nargs);
}

void mx_ffi_string_lower(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_string_lower, "lower", args, (int32_t)nargs);
}

mx_obj_t mx_std_str_of(mx_t *vm, const char *ptr, size_t len) {
    if (len == 1) {
        return mx_std_char_str(vm, ptr[0]);
    } else if (len >= 2 && len <= 23) {
        return mx_obj_of_string(mx_str_cache_get(vm, ptr, (uint32_t)len));
    } else {
        mx_str_t *out = mx_str_new_gc_sized(vm, (uint32_t)len);
        if (len > 0) {
            mx_str_append(out, ptr, (uint32_t)len);
        }
        return mx_obj_of_string(out);
    }
}

bool mx_ffi_string_match_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs != 2 || args[0].kind != MX_KIND_STRING || args[1].kind != MX_KIND_STRING) {
        return false;
    }
    mx_str_t *sb = mx_obj_get_string(args[0]);
    mx_str_t *pb = mx_obj_get_string(args[1]);
    mx_pattern_captures_t scratch;
    mx_pattern_result_t res = mx_pattern_match(&mx_lua_of(vm)->patterns, &scratch, sb->buf, sb->len, pb->buf, pb->len, 0);
    if (res.status == MX_PATTERN_ERROR || (res.status == MX_PATTERN_MATCHED && res.match.is_unfinished)) {
        return false;
    } else if (res.status != MX_PATTERN_MATCHED) {
        args[0] = mx_obj_of_nil();
        vm->rets.len = 1;
        return true;
    } else if (res.match.len == 0) {
        args[0] = mx_std_str_of(vm, sb->buf + res.match.start, res.match.stop - res.match.start);
        vm->rets.len = 1;
        return true;
    } else {
        uint32_t n = res.match.len;
        args = mx_call_results(vm, n);
        if (args == NULL) {
            return false;
        }
        for (uint32_t i = 0; i < n; i++) {
            const mx_pattern_capture_t *cap = &res.match.captures[i];
            if (cap->is_position) {
                args[i] = mx_obj_of_int((int64_t)cap->start + 1);
            } else {
                args[i] = mx_std_str_of(vm, sb->buf + cap->start, cap->len);
            }
        }
        vm->rets.len = n;
        return true;
    }
}

void mx_ffi_string_match(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_ffi_string_match_fast(vm, nargs, args)) {
        mx_std_dispatch(vm, mx_std_string_match, "match", args, (int32_t)nargs);
    }
}

void mx_ffi_string_reverse(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_string_reverse, "reverse", args, (int32_t)nargs);
}

void mx_ffi_string_upper(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_string_upper, "upper", args, (int32_t)nargs);
}

void mx_ffi_string_pack(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_string_pack, "pack", args, (int32_t)nargs);
}

void mx_ffi_string_unpack(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_string_unpack, "unpack", args, (int32_t)nargs);
}

void mx_ffi_string_packsize(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_string_packsize, "packsize", args, (int32_t)nargs);
}

bool mx_ffi_string_sub_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs >= 2 && args[0].kind == MX_KIND_STRING && mx_obj_is_int(args[1])
        && (nargs < 3 || mx_obj_is_int(args[2]))) {
        mx_str_t *buf = mx_obj_get_string(args[0]);
        size_t len = buf->len;
        int64_t i = mx_std_str_relat(mx_obj_get_int(args[1]), len);
        int64_t j = (int64_t)len;
        if (nargs >= 3) {
            j = mx_std_str_relat(mx_obj_get_int(args[2]), len);
        }
        if (i < 1) {
            i = 1;
        }
        if (j > (int64_t)len) {
            j = (int64_t)len;
        }
        if (i <= j) {
            args[0] = mx_std_str_of(vm, buf->buf + i - 1, (size_t)(j - i + 1));
        } else {
            args[0] = mx_std_str_of(vm, buf->buf, 0);
        }
        vm->rets.len = 1;
        return true;
    } else {
        return false;
    }
}

void mx_ffi_string_sub(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_ffi_string_sub_fast(vm, nargs, args)) {
        mx_std_dispatch(vm, mx_std_string_sub, "sub", args, (int32_t)nargs);
    }
}

bool mx_ffi_string_byte_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs >= 1 && nargs <= 2 && args[0].kind == MX_KIND_STRING
        && (nargs < 2 || mx_obj_is_int(args[1]))) {
        mx_str_t *buf = mx_obj_get_string(args[0]);
        size_t len = buf->len;
        int64_t i = 1;
        if (nargs >= 2) {
            i = mx_std_str_relat(mx_obj_get_int(args[1]), len);
        }
        if (i < 1 || i > (int64_t)len) {
            args[0] = mx_obj_of_nil();
            vm->rets.len = 0;
        } else {
            args[0] = mx_obj_of_int((uint8_t)buf->buf[i - 1]);
            vm->rets.len = 1;
        }
        return true;
    } else {
        return false;
    }
}

void mx_ffi_string_byte(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_ffi_string_byte_fast(vm, nargs, args)) {
        mx_std_dispatch(vm, mx_std_string_byte, "byte", args, (int32_t)nargs);
    }
}

bool mx_ffi_string_char_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs > MX_STR_SSO_CAP) {
        return false;
    }
    char tmp[MX_STR_SSO_CAP];
    bool ok = true;
    for (size_t i = 0; i < nargs; i++) {
        if (!mx_obj_is_int(args[i])) {
            ok = false;
            break;
        }
        int64_t c = mx_obj_get_int(args[i]);
        if (c < 0 || c > 255) {
            ok = false;
            break;
        }
        tmp[i] = (char)c;
    }
    if (!ok) {
        return false;
    } else {
        args[0] = mx_std_str_of(vm, tmp, nargs);
        vm->rets.len = 1;
        return true;
    }
}

void mx_ffi_string_char(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_ffi_string_char_fast(vm, nargs, args)) {
        mx_std_dispatch(vm, mx_std_string_char, "char", args, (int32_t)nargs);
    }
}

bool mx_ffi_string_rep_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs != 2 || args[0].kind != MX_KIND_STRING || !mx_obj_is_int(args[1])) {
        return false;
    }
    mx_str_t *buf = mx_obj_get_string(args[0]);
    int64_t n = mx_obj_get_int(args[1]);
    if (n <= 0 || buf->len == 0) {
        args[0] = mx_obj_intern_strn(vm, "", 0);
    } else if ((uint64_t)n > MX_STD_MAX_STRING / buf->len || !mx_mem_fits(sizeof(mx_str_t) + (size_t)n * buf->len + 1)) {
        return false;
    } else if (n > 1 && (uint64_t)n * buf->len <= MX_INTERN_MAX_LEN) {
        char tmp[MX_INTERN_MAX_LEN];
        uint32_t len = 0;
        for (int64_t i = 0; i < n; i++) {
            memcpy(tmp + len, buf->buf, buf->len);
            len += buf->len;
        }
        args[0] = mx_obj_intern_strn(vm, tmp, len);
    } else if (n > 1) {
        mx_str_t *out = mx_str_new_gc_sized(vm, (uint32_t)((uint64_t)buf->len * (uint64_t)n));
        for (int64_t i = 0; i < n; i++) {
            memcpy(&out->buf[out->len], buf->buf, buf->len);
            out->len += buf->len;
        }
        out->buf[out->len] = '\0';
        args[0] = mx_obj_of_string(out);
    }
    vm->rets.len = 1;
    return true;
}

void mx_ffi_string_rep(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_ffi_string_rep_fast(vm, nargs, args)) {
        mx_std_dispatch(vm, mx_std_string_rep, "rep", args, (int32_t)nargs);
    }
}

const mx_std_reg_t mx_std_string_regs[] = {
    {"dump", mx_std_string_dump, NULL},
    {"byte", mx_std_string_byte, mx_ffi_string_byte},
    {"char", mx_std_string_char, mx_ffi_string_char},
    {"find", mx_std_string_find, mx_ffi_string_find},
    {"format", mx_std_string_format, mx_ffi_string_format},
    {"gmatch", mx_std_string_gmatch, mx_ffi_string_gmatch},
    {"gsub", mx_std_string_gsub, mx_ffi_string_gsub},
    {"len", mx_std_string_len, mx_ffi_string_len},
    {"lower", mx_std_string_lower, mx_ffi_string_lower},
    {"match", mx_std_string_match, mx_ffi_string_match},
    {"rep", mx_std_string_rep, mx_ffi_string_rep},
    {"reverse", mx_std_string_reverse, mx_ffi_string_reverse},
    {"sub", mx_std_string_sub, mx_ffi_string_sub},
    {"upper", mx_std_string_upper, mx_ffi_string_upper},
    {"pack", mx_std_string_pack, mx_ffi_string_pack},
    {"unpack", mx_std_string_unpack, mx_ffi_string_unpack},
    {"packsize", mx_std_string_packsize, mx_ffi_string_packsize},
    {NULL, NULL, NULL},
};

void mx_std_open_string(mx_std_state_t *ls) {
    mx_std_newlib(ls, "string", mx_std_string_regs);
    mx_std_pop(ls, 1);
}
