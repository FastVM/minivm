
#include "../api/std.h"
#include "../../front/lex.h"
#include "../../../value/obj.h"

const char mx_std_utf8_charpattern[] = "[\0-\x7F\xC2-\xFD][\x80-\xBF]*";

const uint32_t mx_std_utf8_limits[] = {0, 0, 0x80, 0x800, 0x10000, 0x200000, 0x4000000};

bool mx_std_utf8_iscont(uint8_t c) {
    return (c & 0xC0) == 0x80;
}

mx_std_utf8_decode_t mx_std_utf8_decode(const char *s, size_t avail, bool strict) {
    mx_std_utf8_decode_t bad = { .code = 0, .len = 0 };
    uint8_t lead = (uint8_t)s[0];
    uint32_t n;
    if (lead < 0x80) {
        return (mx_std_utf8_decode_t) { .code = lead, .len = 1 };
    } else if (lead < 0xC0) {
        return bad;
    } else if (lead < 0xE0) {
        n = 2;
    } else if (lead < 0xF0) {
        n = 3;
    } else if (lead < 0xF8) {
        n = 4;
    } else if (lead < 0xFC) {
        n = 5;
    } else if (lead < 0xFE) {
        n = 6;
    } else {
        return bad;
    }
    if (n > avail) {
        return bad;
    }
    uint32_t code = lead & (0x7Fu >> n);
    for (uint32_t i = 1; i < n; i++) {
        uint8_t c = (uint8_t)s[i];
        if (!mx_std_utf8_iscont(c)) {
            return bad;
        }
        code = (code << 6) | (c & 0x3F);
    }
    bool is_surrogate = code >= 0xD800 && code <= 0xDFFF;
    if (code < mx_std_utf8_limits[n] || (strict && (code > 0x10FFFF || is_surrogate))) {
        return bad;
    } else {
        return (mx_std_utf8_decode_t) { .code = code, .len = n };
    }
}

int32_t mx_std_utf8_len(mx_std_state_t *ls) {
    mx_std_str_t s = mx_std_checkstring(ls, 1);
    int64_t from = mx_std_str_relat(mx_std_optinteger(ls, 2, 1), s.len);
    int64_t to = mx_std_str_relat(mx_std_optinteger(ls, 3, -1), s.len);
    bool strict = !mx_std_toboolean(ls, 4);
    if (ls->vm->std_error.is_set) {
        return 0;
    } else if (from < 1 || from - 1 > (int64_t)s.len) {
        return mx_std_argerror(ls, 2, "initial position out of bounds");
    } else if (to > (int64_t)s.len) {
        return mx_std_argerror(ls, 3, "final position out of bounds");
    }
    int64_t count = 0;
    for (int64_t at = from - 1; at < to; count++) {
        mx_std_utf8_decode_t d = mx_std_utf8_decode(s.ptr + at, s.len - (size_t)at, strict);
        if (d.len == 0) {
            mx_std_pushnil(ls);
            mx_std_pushinteger(ls, at + 1);
            return 2;
        }
        at += d.len;
    }
    mx_std_pushinteger(ls, count);
    return 1;
}

int32_t mx_std_utf8_codepoint(mx_std_state_t *ls) {
    mx_std_str_t s = mx_std_checkstring(ls, 1);
    int64_t from = mx_std_str_relat(mx_std_optinteger(ls, 2, 1), s.len);
    int64_t to = mx_std_str_relat(mx_std_optinteger(ls, 3, from), s.len);
    bool strict = !mx_std_toboolean(ls, 4);
    if (ls->vm->std_error.is_set) {
        return 0;
    } else if (from < 1) {
        return mx_std_argerror(ls, 2, "out of bounds");
    } else if (to > (int64_t)s.len) {
        return mx_std_argerror(ls, 3, "out of bounds");
    } else if (from > to) {
        return 0;
    } else if (to - from >= INT32_MAX || !mx_std_checkstack(ls, (int32_t)(to - from) + 1)) {
        return mx_std_error(ls, "string slice too long");
    }
    int32_t n = 0;
    for (int64_t at = from - 1; at < to; n++) {
        mx_std_utf8_decode_t d = mx_std_utf8_decode(s.ptr + at, s.len - (size_t)at, strict);
        if (d.len == 0) {
            return mx_std_error(ls, "invalid UTF-8 code");
        }
        mx_std_pushinteger(ls, d.code);
        at += d.len;
    }
    return n;
}

int32_t mx_std_utf8_char(mx_std_state_t *ls) {
    mx_str_t *out = mx_str_new();
    for (int32_t i = 1; i <= ls->top && !ls->vm->std_error.is_set; i++) {
        int64_t code = mx_std_checkinteger(ls, i);
        if ((uint64_t)code > 0x7FFFFFFF) {
            mx_std_argerror(ls, i, "value out of range");
        } else {
            mx_lex_utf8_encode(out, (uint64_t)code);
        }
    }
    mx_std_pushlstring(ls, out->buf, out->len);
    mx_str_free(out);
    return 1;
}

int32_t mx_std_utf8_offset(mx_std_state_t *ls) {
    mx_std_str_t s = mx_std_checkstring(ls, 1);
    int64_t n = mx_std_checkinteger(ls, 2);
    int64_t def = 1;
    if (n < 0) {
        def = (int64_t)s.len + 1;
    }
    int64_t at = mx_std_str_relat(mx_std_optinteger(ls, 3, def), s.len) - 1;
    if (ls->vm->std_error.is_set) {
        return 0;
    } else if (at < 0 || at > (int64_t)s.len) {
        return mx_std_argerror(ls, 3, "position out of bounds");
    }
    const uint8_t *b = (const uint8_t *)s.ptr;
    if (n == 0) {
        while (at > 0 && mx_std_utf8_iscont(b[at])) {
            at -= 1;
        }
    } else if (mx_std_utf8_iscont(b[at])) {
        return mx_std_error(ls, "initial position is a continuation byte");
    } else if (n < 0) {
        for (; n < 0 && at > 0; n++) {
            at -= 1;
            while (at > 0 && mx_std_utf8_iscont(b[at])) {
                at -= 1;
            }
        }
    } else {
        for (n -= 1; n > 0 && at < (int64_t)s.len; n--) {
            at += 1;
            while (at < (int64_t)s.len && mx_std_utf8_iscont(b[at])) {
                at += 1;
            }
        }
    }
    if (n != 0) {
        mx_std_pushnil(ls);
        return 1;
    }
    int64_t last = at;
    if ((b[at] & 0x80) != 0 && mx_std_utf8_iscont(b[at])) {
        return mx_std_error(ls, "initial position is a continuation byte");
    } else if ((b[at] & 0x80) != 0) {
        while (last + 1 < (int64_t)s.len && mx_std_utf8_iscont(b[last + 1])) {
            last += 1;
        }
    }
    mx_std_pushinteger(ls, at + 1);
    mx_std_pushinteger(ls, last + 1);
    return 2;
}

int32_t mx_std_utf8_codes_iter(mx_std_state_t *ls) {
    bool strict = mx_obj_is_truthy(mx_std_upval(ls, 0));
    mx_std_str_t s = mx_std_checkstring(ls, 1);
    uint64_t at = (uint64_t)mx_std_checkinteger(ls, 2);
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    while (at < s.len && mx_std_utf8_iscont((uint8_t)s.ptr[at])) {
        at += 1;
    }
    if (at >= s.len) {
        return 0;
    }
    mx_std_utf8_decode_t d = mx_std_utf8_decode(s.ptr + at, s.len - at, strict);
    if (d.len == 0 || (at + d.len < s.len && mx_std_utf8_iscont((uint8_t)s.ptr[at + d.len]))) {
        return mx_std_error(ls, "invalid UTF-8 code");
    }
    mx_std_pushinteger(ls, (int64_t)at + 1);
    mx_std_pushinteger(ls, d.code);
    return 2;
}

int32_t mx_std_utf8_codes(mx_std_state_t *ls) {
    bool strict = !mx_std_toboolean(ls, 2);
    mx_std_str_t s = mx_std_checkstring(ls, 1);
    if (ls->vm->std_error.is_set) {
        return 0;
    } else if (s.len > 0 && mx_std_utf8_iscont((uint8_t)s.ptr[0])) {
        return mx_std_argerror(ls, 1, "invalid UTF-8 code");
    }
    mx_std_pushboolean(ls, strict);
    mx_std_pushcfn_up(ls, mx_std_utf8_codes_iter, "codes", 1);
    mx_std_pushvalue(ls, 1);
    mx_std_pushinteger(ls, 0);
    return 3;
}

const mx_std_reg_t mx_std_utf8_regs[] = {
    {"offset", mx_std_utf8_offset, NULL},
    {"codepoint", mx_std_utf8_codepoint, NULL},
    {"char", mx_std_utf8_char, NULL},
    {"len", mx_std_utf8_len, NULL},
    {"codes", mx_std_utf8_codes, NULL},
    {NULL, NULL, NULL},
};

void mx_std_open_utf8(mx_std_state_t *ls) {
    mx_std_newlib(ls, "utf8", mx_std_utf8_regs);
    mx_std_pushlstring(ls, mx_std_utf8_charpattern, sizeof(mx_std_utf8_charpattern) - 1);
    mx_std_setfield(ls, -2, "charpattern");
    mx_std_pop(ls, 1);
}
