
#include "../api/std.h"
#include "../../../value/obj.h"

enum {
    MX_STD_PACK_LONG = 8,
    MX_STD_PACK_INTSIZE = 8,
    MX_STD_PACK_SIZET = 8,
    MX_STD_PACK_INT_DEFAULT = 4,
    MX_STD_PACK_MAXINTSIZE = 16,
    MX_STD_PACK_MAXALIGN = 8,
};

bool mx_std_pack_digit(char c) {
    return c >= '0' && c <= '9';
}

size_t mx_std_pack_getnum(mx_std_pack_t *h, size_t df) {
    if (!mx_std_pack_digit(*h->fmt)) {
        return df;
    } else {
        size_t a = 0;
        bool has_more = true;
        while (has_more) {
            a = a * 10 + (size_t)(*h->fmt - '0');
            h->fmt += 1;
            has_more = mx_std_pack_digit(*h->fmt) && a <= ((size_t)INT64_MAX - 9) / 10;
        }
        return a;
    }
}

int32_t mx_std_pack_getnumlimit(mx_std_pack_t *h, int32_t df) {
    size_t sz = mx_std_pack_getnum(h, (size_t)df);
    if (sz > MX_STD_PACK_MAXINTSIZE || sz == 0) {
        return mx_std_error(h->ls, "integral size (%d) out of limits [1,%d]", (int32_t)sz, MX_STD_PACK_MAXINTSIZE);
    } else {
        return (int32_t)sz;
    }
}

mx_std_pack_res_t mx_std_pack_getoption(mx_std_pack_t *h) {
    char opt = *h->fmt;
    h->fmt += 1;
    switch (opt) {
        case 'b': {
            return (mx_std_pack_res_t) { .size = 1, .opt = MX_STD_PACK_INT };
        }
        case 'B': {
            return (mx_std_pack_res_t) { .size = 1, .opt = MX_STD_PACK_UINT };
        }
        case 'h': {
            return (mx_std_pack_res_t) { .size = 2, .opt = MX_STD_PACK_INT };
        }
        case 'H': {
            return (mx_std_pack_res_t) { .size = 2, .opt = MX_STD_PACK_UINT };
        }
        case 'l': {
            return (mx_std_pack_res_t) { .size = MX_STD_PACK_LONG, .opt = MX_STD_PACK_INT };
        }
        case 'L': {
            return (mx_std_pack_res_t) { .size = MX_STD_PACK_LONG, .opt = MX_STD_PACK_UINT };
        }
        case 'j': {
            return (mx_std_pack_res_t) { .size = MX_STD_PACK_INTSIZE, .opt = MX_STD_PACK_INT };
        }
        case 'J': {
            return (mx_std_pack_res_t) { .size = MX_STD_PACK_INTSIZE, .opt = MX_STD_PACK_UINT };
        }
        case 'T': {
            return (mx_std_pack_res_t) { .size = MX_STD_PACK_SIZET, .opt = MX_STD_PACK_UINT };
        }
        case 'f': {
            return (mx_std_pack_res_t) { .size = sizeof(float), .opt = MX_STD_PACK_FLOAT };
        }
        case 'n': {
            return (mx_std_pack_res_t) { .size = sizeof(double), .opt = MX_STD_PACK_NUMBER };
        }
        case 'd': {
            return (mx_std_pack_res_t) { .size = sizeof(double), .opt = MX_STD_PACK_DOUBLE };
        }
        case 'i': {
            return (mx_std_pack_res_t) { .size = mx_std_pack_getnumlimit(h, MX_STD_PACK_INT_DEFAULT), .opt = MX_STD_PACK_INT };
        }
        case 'I': {
            return (mx_std_pack_res_t) { .size = mx_std_pack_getnumlimit(h, MX_STD_PACK_INT_DEFAULT), .opt = MX_STD_PACK_UINT };
        }
        case 's': {
            return (mx_std_pack_res_t) { .size = mx_std_pack_getnumlimit(h, MX_STD_PACK_SIZET), .opt = MX_STD_PACK_STRING };
        }
        case 'c': {
            size_t csize = mx_std_pack_getnum(h, SIZE_MAX);
            if (csize == SIZE_MAX) {
                mx_std_error(h->ls, "missing size for format option 'c'");
                return (mx_std_pack_res_t) { .size = 0, .opt = MX_STD_PACK_CHAR };
            } else {
                return (mx_std_pack_res_t) { .size = csize, .opt = MX_STD_PACK_CHAR };
            }
        }
        case 'z': {
            return (mx_std_pack_res_t) { .size = 0, .opt = MX_STD_PACK_ZSTR };
        }
        case 'x': {
            return (mx_std_pack_res_t) { .size = 1, .opt = MX_STD_PACK_PADDING };
        }
        case 'X': {
            return (mx_std_pack_res_t) { .size = 0, .opt = MX_STD_PACK_PADDALIGN };
        }
        case ' ': {
            break;
        }
        case '<': {
            h->is_little = true;
            break;
        }
        case '>': {
            h->is_little = false;
            break;
        }
        case '=': {
            h->is_little = true;
            break;
        }
        case '!': {
            h->max_align = mx_std_pack_getnumlimit(h, MX_STD_PACK_MAXALIGN);
            break;
        }
        default: {
            mx_std_error(h->ls, "invalid format option '%c'", opt);
            break;
        }
    }
    return (mx_std_pack_res_t) { .size = 0, .opt = MX_STD_PACK_NOP };
}

mx_std_pack_det_t mx_std_pack_getdetails(mx_std_pack_t *h, size_t totalsize) {
    mx_std_pack_res_t got = mx_std_pack_getoption(h);
    size_t align;
    if (got.opt != MX_STD_PACK_PADDALIGN) {
        align = got.size;
    } else if (*h->fmt == '\0') {
        mx_std_argerror(h->ls, 1, "invalid next option for option 'X'");
        align = got.size;
    } else {
        mx_std_pack_res_t next = mx_std_pack_getoption(h);
        if (next.opt == MX_STD_PACK_CHAR || next.size == 0) {
            mx_std_argerror(h->ls, 1, "invalid next option for option 'X'");
        }
        align = next.size;
    }
    int32_t ntoalign;
    if (align <= 1 || got.opt == MX_STD_PACK_CHAR) {
        ntoalign = 0;
    } else {
        size_t capped;
        if (align > (size_t)h->max_align) {
            capped = (size_t)h->max_align;
        } else {
            capped = align;
        }
        if ((capped & (capped - 1)) != 0) {
            mx_std_argerror(h->ls, 1, "format asks for alignment not power of 2");
        }
        ntoalign = (int32_t)((capped - (totalsize & (capped - 1))) & (capped - 1));
    }
    return (mx_std_pack_det_t) { .size = got.size, .pad = ntoalign, .opt = got.opt };
}

int32_t mx_std_pack_index(int32_t i, int32_t size, bool is_little) {
    if (is_little) {
        return i;
    } else {
        return size - 1 - i;
    }
}

void mx_std_pack_packint(mx_std_buf_t *b, uint64_t n, bool is_little, int32_t size, bool neg) {
    char *out = mx_std_buf_prep(b, (size_t)size);
    if (out != NULL) {
        for (int32_t i = 0; i < size; i++) {
            uint8_t byte;
            if (i < MX_STD_PACK_INTSIZE) {
                byte = (uint8_t)(n >> (i * 8));
            } else if (neg) {
                byte = 0xff;
            } else {
                byte = 0;
            }
            out[mx_std_pack_index(i, size, is_little)] = (char)byte;
        }
        b->len += (size_t)size;
    }
}

int32_t mx_std_string_pack(mx_std_state_t *ls) {
    mx_std_buf_t b;
    mx_std_pack_t h = (mx_std_pack_t) { .ls = ls, .fmt = mx_std_checkstring(ls, 1).ptr, .max_align = 1, .is_little = true };
    int32_t arg = 1;
    size_t totalsize = 0;
    mx_std_pushnil(ls);
    mx_std_buf_init(ls, &b);
    while (*h.fmt != '\0' && !ls->vm->std_error.is_set) {
        mx_std_pack_det_t det = mx_std_pack_getdetails(&h, totalsize);
        if (ls->vm->std_error.is_set) {
            break;
        }
        size_t size = det.size;
        int32_t ntoalign = det.pad;
        mx_std_pack_opt_t opt = det.opt;
        if (size + (size_t)ntoalign > (size_t)INT64_MAX - totalsize) {
            mx_std_argerror(ls, arg, "result too long");
            break;
        }
        totalsize += (size_t)ntoalign + size;
        for (int32_t i = 0; i < ntoalign; i++) {
            mx_std_buf_addchar(&b, 0);
        }
        arg += 1;
        switch (opt) {
            case MX_STD_PACK_INT: {
                int64_t n = mx_std_checkinteger(ls, arg);
                bool fits;
                if (size > 0 && size < MX_STD_PACK_INTSIZE) {
                    int64_t lim = (int64_t)1 << ((size * 8) - 1);
                    fits = -lim <= n && n < lim;
                } else {
                    fits = true;
                }
                if (!fits) {
                    mx_std_argerror(ls, arg, "integer overflow");
                } else {
                    mx_std_pack_packint(&b, (uint64_t)n, h.is_little, (int32_t)size, n < 0);
                }
                break;
            }
            case MX_STD_PACK_UINT: {
                int64_t n = mx_std_checkinteger(ls, arg);
                if (size < MX_STD_PACK_INTSIZE && (uint64_t)n >= ((uint64_t)1 << (size * 8))) {
                    mx_std_argerror(ls, arg, "unsigned overflow");
                } else {
                    mx_std_pack_packint(&b, (uint64_t)n, h.is_little, (int32_t)size, false);
                }
                break;
            }
            case MX_STD_PACK_FLOAT: {
                mx_std_pack_f32_t f = (mx_std_pack_f32_t) { .f32 = (float)mx_std_checkfloat(ls, arg) };
                mx_std_pack_packint(&b, f.bits, h.is_little, (int32_t)size, false);
                break;
            }
            case MX_STD_PACK_NUMBER:
            case MX_STD_PACK_DOUBLE: {
                mx_val_t f = mx_val_f64(mx_std_checkfloat(ls, arg));
                mx_std_pack_packint(&b, f.bits, h.is_little, (int32_t)size, false);
                break;
            }
            case MX_STD_PACK_CHAR: {
                mx_std_str_t sv = mx_std_checkstring(ls, arg);
                const char *s = sv.ptr;
                size_t len = sv.len;
                if (len > (size_t)size) {
                    mx_std_argerror(ls, arg, "string longer than given size");
                } else {
                    mx_std_buf_addlstring(&b, s, len);
                    char *pad = mx_std_buf_prep(&b, size - len);
                    if (pad != NULL) {
                        memset(pad, 0, size - len);
                        b.len += size - len;
                    }
                }
                break;
            }
            case MX_STD_PACK_STRING: {
                mx_std_str_t sv = mx_std_checkstring(ls, arg);
                const char *s = sv.ptr;
                size_t len = sv.len;
                if (size < MX_STD_PACK_SIZET && len >= ((size_t)1 << (size * 8))) {
                    mx_std_argerror(ls, arg, "string length does not fit in given size");
                } else {
                    mx_std_pack_packint(&b, (uint64_t)len, h.is_little, (int32_t)size, false);
                    mx_std_buf_addlstring(&b, s, len);
                    totalsize += len;
                }
                break;
            }
            case MX_STD_PACK_ZSTR: {
                mx_std_str_t sv = mx_std_checkstring(ls, arg);
                const char *s = sv.ptr;
                size_t len = sv.len;
                if (strlen(s) != len) {
                    mx_std_argerror(ls, arg, "string contains zeros");
                } else {
                    mx_std_buf_addlstring(&b, s, len);
                    mx_std_buf_addchar(&b, '\0');
                    totalsize += len + 1;
                }
                break;
            }
            case MX_STD_PACK_PADDING: {
                mx_std_buf_addchar(&b, 0);
                arg -= 1;
                break;
            }
            case MX_STD_PACK_PADDALIGN:
            case MX_STD_PACK_NOP: {
                arg -= 1;
                break;
            }
        }
    }
    mx_std_buf_push(&b);
    return 1;
}

int32_t mx_std_string_packsize(mx_std_state_t *ls) {
    mx_std_pack_t h = (mx_std_pack_t) { .ls = ls, .fmt = mx_std_checkstring(ls, 1).ptr, .max_align = 1, .is_little = true };
    size_t totalsize = 0;
    while (*h.fmt != '\0' && !ls->vm->std_error.is_set) {
        mx_std_pack_det_t det = mx_std_pack_getdetails(&h, totalsize);
        if (det.opt == MX_STD_PACK_STRING || det.opt == MX_STD_PACK_ZSTR) {
            return mx_std_argerror(ls, 1, "variable-length format");
        }
        size_t size = det.size + (size_t)det.pad;
        if (!(totalsize <= (size_t)INT64_MAX - size)) {
            return mx_std_argerror(ls, 1, "format result too large");
        }
        totalsize += size;
    }
    mx_std_pushinteger(ls, (int64_t)totalsize);
    return 1;
}

int64_t mx_std_pack_unpackint(mx_std_state_t *ls, const char *str, bool is_little, int32_t size, bool issigned) {
    int32_t limit;
    if (size > MX_STD_PACK_INTSIZE) {
        limit = MX_STD_PACK_INTSIZE;
    } else {
        limit = size;
    }
    uint64_t res = 0;
    for (int32_t i = limit - 1; i >= 0; i--) {
        res = (res << 8) | (uint8_t)str[mx_std_pack_index(i, size, is_little)];
    }
    if (size < MX_STD_PACK_INTSIZE && issigned) {
        uint64_t mask = (uint64_t)1 << (size * 8 - 1);
        return (int64_t)((res ^ mask) - mask);
    } else if (size > MX_STD_PACK_INTSIZE) {
        uint8_t mask;
        if (issigned && (int64_t)res < 0) {
            mask = 0xff;
        } else {
            mask = 0;
        }
        for (int32_t i = limit; i < size; i++) {
            if ((uint8_t)str[mx_std_pack_index(i, size, is_little)] != mask) {
                mx_std_error(ls, "%d-byte integer does not fit into Lua Integer", size);
            }
        }
        return (int64_t)res;
    } else {
        return (int64_t)res;
    }
}

int32_t mx_std_string_unpack(mx_std_state_t *ls) {
    const char *fmt_arg = mx_std_checkstring(ls, 1).ptr;
    mx_std_str_t dv = mx_std_checkstring(ls, 2);
    const char *data = dv.ptr;
    size_t ld = dv.len;
    size_t pos = (size_t)mx_std_str_relat(mx_std_optinteger(ls, 3, 1), ld) - 1;
    int32_t n = 0;
    if (!(pos <= ld)) {
        return mx_std_argerror(ls, 3, "initial position out of string");
    }
    mx_std_pack_t h = (mx_std_pack_t) { .ls = ls, .fmt = fmt_arg, .max_align = 1, .is_little = true };
    while (*h.fmt != '\0' && !ls->vm->std_error.is_set) {
        mx_std_pack_det_t det = mx_std_pack_getdetails(&h, pos);
        if (ls->vm->std_error.is_set) {
            break;
        }
        size_t size = det.size;
        int32_t ntoalign = det.pad;
        mx_std_pack_opt_t opt = det.opt;
        if (!((size_t)ntoalign + size <= ld - pos)) {
            return mx_std_argerror(ls, 2, "data string too short");
        }
        pos += (size_t)ntoalign;
        if (!mx_std_checkstack(ls, 2)) {
            return mx_std_error(ls, "too many results");
        }
        n += 1;
        switch (opt) {
            case MX_STD_PACK_INT:
            case MX_STD_PACK_UINT: {
                int64_t res = mx_std_pack_unpackint(ls, data + pos, h.is_little, (int32_t)size, opt == MX_STD_PACK_INT);
                mx_std_pushinteger(ls, res);
                break;
            }
            case MX_STD_PACK_FLOAT: {
                int64_t bits = mx_std_pack_unpackint(ls, data + pos, h.is_little, (int32_t)size, false);
                mx_std_pushfloat(ls, (double)((mx_std_pack_f32_t) { .bits = (uint32_t)bits }).f32);
                break;
            }
            case MX_STD_PACK_NUMBER:
            case MX_STD_PACK_DOUBLE: {
                int64_t bits = mx_std_pack_unpackint(ls, data + pos, h.is_little, (int32_t)size, false);
                mx_std_pushfloat(ls, mx_val_i64(bits).f64);
                break;
            }
            case MX_STD_PACK_CHAR: {
                mx_std_pushlstring(ls, data + pos, size);
                break;
            }
            case MX_STD_PACK_STRING: {
                size_t len = (size_t)mx_std_pack_unpackint(ls, data + pos, h.is_little, (int32_t)size, false);
                if (!(len <= ld - pos - size)) {
                    return mx_std_argerror(ls, 2, "data string too short");
                }
                mx_std_pushlstring(ls, data + pos + size, len);
                pos += len;
                break;
            }
            case MX_STD_PACK_ZSTR: {
                size_t len = strlen(data + pos);
                if (!(pos + len < ld)) {
                    return mx_std_argerror(ls, 2, "unfinished string for format 'z'");
                }
                mx_std_pushlstring(ls, data + pos, len);
                pos += len + 1;
                break;
            }
            case MX_STD_PACK_PADDALIGN:
            case MX_STD_PACK_PADDING:
            case MX_STD_PACK_NOP: {
                n -= 1;
                break;
            }
        }
        pos += size;
    }
    mx_std_pushinteger(ls, (int64_t)pos + 1);
    return n + 1;
}
