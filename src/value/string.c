
#include "string.h"
#include "kind.h"
#include "num.h"
#include "obj.h"
#include "table.h"
#include "../vm/gc.h"

int32_t mx_str_pool_class(size_t bytes) {
    size_t sz = (size_t)1 << MX_STR_POOL_MIN_SHIFT;
    int32_t cls = 0;
    while (sz < bytes) {
        sz <<= 1;
        cls += 1;
        if (cls >= MX_STR_POOL_CLASSES) {
            return -1;
        }
    }
    return cls;
}

mx_str_payload_t mx_str_payload_alloc(size_t bytes) {
    int32_t cls = mx_str_pool_class(bytes);
    if (cls < 0) {
        return (mx_str_payload_t) { .data = mx_mem_alloc(bytes), .cap = (uint32_t)bytes };
    } else {
        size_t sz = (size_t)1 << (cls + MX_STR_POOL_MIN_SHIFT);
        return (mx_str_payload_t) { .data = mx_mem_alloc(sz), .cap = (uint32_t)sz };
    }
}

void mx_str_payload_free(char *payload) {
    mx_mem_free(payload);
}

void mx_str_reserve(mx_str_t *buf, uint32_t need) {
    bool have = need <= buf->cap && buf->buf != NULL;
    if (!have && need <= MX_STR_SSO_CAP && !buf->is_rope && (buf->buf == NULL || buf->is_sso)) {
        buf->buf = buf->small;
        buf->cap = MX_STR_SSO_CAP;
        buf->is_sso = true;
    } else if (!have) {
        if (buf->is_gc) {
            mx_gc_note_final_owned(&buf->head);
        }
        uint64_t ncap = 16;
        while (ncap < (uint64_t)need) {
            ncap <<= 1;
        }
        if (ncap > UINT32_MAX) {
            mx_fatal("string: buffer too large");
        }
        if (buf->is_tail) {
            mx_str_payload_t got = mx_str_payload_alloc((size_t)ncap);
            memcpy(got.data, buf->buf, (size_t)buf->len + 1);
            buf->buf = got.data;
            buf->cap = got.cap;
            buf->is_tail = false;
        } else if (buf->is_sso) {
            mx_str_payload_t got = mx_str_payload_alloc((size_t)ncap);
            size_t copy = MX_STR_SSO_CAP + 1;
            if (buf->len < MX_STR_SSO_CAP) {
                copy = buf->len + 1;
            }
            memcpy(got.data, buf->small, copy);
            buf->buf = got.data;
            buf->is_sso = false;
            buf->cap = got.cap;
        } else if (buf->buf != NULL
            && mx_str_pool_class((size_t)ncap) < 0
            && mx_str_pool_class((size_t)buf->cap) < 0) {
            buf->buf = mx_mem_realloc(buf->buf, (size_t)ncap);
            buf->cap = (uint32_t)ncap;
        } else {
            mx_str_payload_t got = mx_str_payload_alloc((size_t)ncap);
            if (buf->buf != NULL) {
                size_t copy = buf->len;
                if (copy < buf->cap) {
                    copy = (size_t)buf->len + 1;
                }
                memcpy(got.data, buf->buf, copy);
                if ((size_t)buf->len < (size_t)got.cap) {
                    got.data[buf->len] = '\0';
                }
                mx_str_payload_free(buf->buf);
            }
            buf->buf = got.data;
            buf->cap = got.cap;
        }
    }
}

void mx_str_free(mx_str_t *buf) {
    if (buf->buf != NULL && !buf->is_sso) {
        mx_str_payload_free(buf->buf);
    }
    mx_mem_free(buf);
}

void mx_str_print(mx_str_t *buf, FILE *out) {
    mx_str_flat(buf);
    fwrite(buf->buf, 1, buf->len, out);
}

char *mx_str_release(mx_str_t *buf) {
    mx_str_flat(buf);
    char *data = mx_mem_alloc(buf->len + 1);
    memcpy(data, buf->buf, buf->len);
    data[buf->len] = '\0';
    mx_str_free(buf);
    return data;
}

void mx_str_vformat(mx_str_t *buf, const char *fmt, va_list ap) {
    if (buf->buf == NULL) {
        mx_str_reserve(buf, 16);
    }
    char stage[MX_STR_FORMAT_STAGE];
    va_list ap_copy;
    va_copy(ap_copy, ap);
    int32_t written = vsnprintf(stage, sizeof(stage), fmt, ap_copy);
    va_end(ap_copy);
    if (written < 0) {
        return;
    } else if ((size_t)written < sizeof(stage)) {
        mx_str_append(buf, stage, (uint32_t)written);
    } else {
        mx_str_reserve(buf, buf->len + (uint32_t)written + 1);
        vsnprintf(&buf->buf[buf->len], (size_t)written + 1, fmt, ap);
        buf->len += (uint32_t)written;
    }
}

void mx_str_flatten(mx_str_t *buf) {
    if (buf->is_rope) {
        char sso_stage[MX_STR_SSO_CAP + 1];
        bool to_sso = buf->len <= MX_STR_SSO_CAP;
        char *data;
        uint32_t data_cap;
        if (to_sso) {
            data = sso_stage;
            data_cap = 0;
        } else {
            if (buf->is_gc) {
                mx_gc_note_final_owned(&buf->head);
            }
            mx_str_payload_t got = mx_str_payload_alloc((size_t)buf->len + 1);
            data = got.data;
            data_cap = got.cap;
        }
        size_t w = 0;
        mx_str_t *small_stack[MX_STR_ROPE_STACK];
        mx_str_t **stack = small_stack;
        size_t stack_cap = MX_STR_ROPE_STACK;
        size_t depth = 0;
        mx_str_t *node = buf;
        bool done = false;
        while (!done) {
            if (node->is_rope) {
                if (depth == stack_cap) {
                    if (stack == small_stack) {
                        stack = mx_mem_alloc(sizeof(small_stack) * 2);
                        memcpy(stack, small_stack, sizeof(small_stack));
                    } else {
                        stack = mx_mem_realloc(stack, sizeof(mx_str_t *) * stack_cap * 2);
                    }
                    stack_cap *= 2;
                }
                stack[depth] = node->rope.right;
                depth += 1;
                node = node->rope.left;
            } else {
                memcpy(&data[w], node->buf, node->len);
                w += node->len;
                if (depth == 0) {
                    done = true;
                } else {
                    depth -= 1;
                    node = stack[depth];
                }
            }
        }
        if (stack != small_stack) {
            mx_mem_free(stack);
        }
        data[buf->len] = '\0';
        if (to_sso) {
            buf->is_rope = false;
            memcpy(buf->small, sso_stage, (size_t)buf->len + 1);
            buf->buf = buf->small;
            buf->cap = MX_STR_SSO_CAP;
            buf->is_sso = true;
            buf->is_pooled = false;
        } else {
            buf->buf = data;
            buf->cap = data_cap;
            buf->is_pooled = false;
            buf->is_rope = false;
            buf->rope.left = NULL;
            buf->rope.right = NULL;
        }
    }
}

mx_str_t *mx_str_new(void) {
    mx_str_t *ret = mx_mem_alloc(sizeof(mx_str_t));
    *ret = (mx_str_t) {
        .head = {
            .kind = MX_KIND_STRING,
        },
    };
    mx_str_reserve(ret, 1);
    ret->buf[0] = '\0';
    return ret;
}

void mx_str_format(mx_str_t *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    mx_str_vformat(buf, fmt, ap);
    va_end(ap);
}

void mx_str_append(mx_str_t *buf, const char *data, uint32_t len) {
    uint64_t need = (uint64_t)buf->len + (uint64_t)len + 1;
    if (need > UINT32_MAX) {
        mx_fatal("string: buffer too large");
    }
    if (need > buf->cap || buf->buf == NULL) {
        mx_str_reserve(buf, (uint32_t)need);
    }
    memcpy(&buf->buf[buf->len], data, len);
    buf->len += len;
    buf->buf[buf->len] = '\0';
}

void mx_str_append_byte(mx_str_t *buf, char c) {
    mx_str_reserve(buf, buf->len + 2);
    buf->buf[buf->len] = c;
    buf->len += 1;
    buf->buf[buf->len] = '\0';
}

uint32_t mx_str_int_digits(char *tmp, int64_t v) {
    uint32_t at = MX_STR_INT_DIGITS;
    uint64_t u = (uint64_t)v;
    if (v < 0) {
        u = (uint64_t)0 - u;
    }
    bool has_more = true;
    while (has_more) {
        at -= 1;
        tmp[at] = (char)('0' + (u % 10));
        u /= 10;
        has_more = u != 0;
    }
    if (v < 0) {
        at -= 1;
        tmp[at] = '-';
    }
    return at;
}

void mx_str_append_int(mx_str_t *buf, int64_t v) {
    char tmp[MX_STR_INT_DIGITS];
    uint32_t at = mx_str_int_digits(tmp, v);
    mx_str_append(buf, &tmp[at], MX_STR_INT_DIGITS - at);
}

char *mx_str_vsprintf(const char *fmt, va_list ap) {
    mx_str_t buf = {0};
    mx_str_vformat(&buf, fmt, ap);
    return buf.buf;
}

char *mx_str_sprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *r = mx_str_vsprintf(fmt, ap);
    va_end(ap);
    return r;
}

void mx_str_indent(mx_str_t *out, size_t indent, const char *prefix) {
    for (size_t i = 0; i < indent; i++) {
        mx_str_format(out, "    ");
    }
    mx_str_format(out, "%s", prefix);
}

int32_t mx_str_float_digits(char *tmp, double n) {
    int32_t written = snprintf(tmp, MX_STR_FLOAT_CAP, "%.15g", n);
    if (n == n && strtod(tmp, NULL) != n) {
        written = snprintf(tmp, MX_STR_FLOAT_CAP, "%.17g", n);
    }
    return written;
}

void mx_str_number(mx_str_t *out, double n) {
    if (n == (double)(int64_t)n && -MX_STR_EXACT_FLOAT_MAX <= n && n <= MX_STR_EXACT_FLOAT_MAX) {
        mx_str_append_int(out, (int64_t)n);
    } else {
        char tmp[MX_STR_FLOAT_CAP];
        int32_t written = mx_str_float_digits(tmp, n);
        mx_str_append(out, tmp, (uint32_t)written);
    }
}

void mx_str_float(mx_str_t *out, double n) {
    char tmp[MX_STR_FLOAT_CAP];
    int32_t written = mx_str_float_digits(tmp, n);
    bool looks_int = strspn(tmp, "-0123456789") == (size_t)written;
    if (looks_int && written + 2 < (int32_t)sizeof(tmp)) {
        tmp[written] = localeconv()->decimal_point[0];
        tmp[written + 1] = '0';
        written += 2;
    }
    mx_str_append(out, tmp, (uint32_t)written);
}

void mx_str_print_lit(mx_str_t *out, mx_obj_t value) {
    if (mx_obj_is_nil(value)) {
        mx_str_format(out, "nil");
    } else if (mx_obj_is_boolean(value)) {
        const char *text = "false";
        if (mx_obj_get_boolean(value)) {
            text = "true";
        }
        mx_str_format(out, "%s", text);
    } else if (mx_obj_is_number(value)) {
        mx_str_number(out, mx_obj_get_number(value));
    } else if (mx_obj_is_ffi(value)) {
        mx_str_format(out, "<function: %p>", mx_obj_get_ffi(value));
    } else if (mx_obj_is_string(value)) {
        mx_str_format(out, "\"%s\"", mx_obj_get_string(value)->buf);
    } else if (mx_obj_is_table(value)) {
        mx_str_format(out, "<table: %p>", mx_obj_get_table(value));
    } else if (mx_obj_is_closure(value)) {
        mx_str_format(out, "<closure: %p>", mx_obj_get_closure(value));
    } else {
        mx_str_format(out, "<object>");
    }
}

void mx_str_obj_debug_table(mx_str_t *out, size_t indent, mx_obj_table_t *tbl, mx_str_debug_t *next) {
    mx_str_format(out, "table(%p) {\n", tbl);
    for (size_t i = 0; i < tbl->array_len; i++) {
        if (!mx_obj_is_nil(tbl->array[i])) {
            char key[MX_STR_DEBUG_KEY_CAP];
            snprintf(key, sizeof(key), "[[" "%.14g" "]] = ", (double)(i + 1));
            mx_str_obj_debug(out, indent + 1, key, tbl->array[i], next);
        }
    }
    size_t len = 0;
    if (tbl->entries != NULL) {
        len = mx_table_hash_cap(tbl);
    }
    for (size_t i = 0; i < len; i++) {
        mx_obj_t key = mx_table_entry_key(&tbl->entries[i]);
        mx_obj_t val = mx_table_entry_val(&tbl->entries[i]);
        if (mx_obj_is_nil(val) || mx_obj_is_nil(key)) {
            continue;
        }
        if (mx_obj_is_boolean(key)) {
            if (mx_obj_get_boolean(key)) {
                mx_str_obj_debug(out, indent + 1, "[[true]] = ", val, next);
            } else {
                mx_str_obj_debug(out, indent + 1, "[[false]] = ", val, next);
            }
        } else if (mx_obj_is_number(key)) {
            char text[MX_STR_DEBUG_KEY_CAP];
            snprintf(text, sizeof(text), "[[" "%.14g" "]] = ", mx_obj_get_number(key));
            mx_str_obj_debug(out, indent + 1, text, val, next);
        } else if (mx_obj_is_string(key)) {
            mx_str_t *text = mx_str_new();
            mx_str_format(text, "%s = ", mx_obj_get_string(key)->buf);
            mx_str_obj_debug(out, indent + 1, text->buf, val, next);
            mx_str_free(text);
        } else {
            mx_str_indent(out, indent + 1, "");
            mx_str_format(out, "pair {\n");
            mx_str_obj_debug(out, indent + 2, "key = ", key, next);
            mx_str_obj_debug(out, indent + 2, "val = ", val, next);
            mx_str_indent(out, indent + 1, "");
            mx_str_format(out, "}\n");
        }
    }
    mx_str_indent(out, indent, "");
    mx_str_format(out, "}\n");
}

void mx_str_obj_debug(mx_str_t *out, size_t indent, const char *prefix, mx_obj_t value, mx_str_debug_t *link) {
    size_t up = 1;
    size_t seen_at = 0;
    for (mx_str_debug_t *cur = link; cur != NULL && seen_at == 0; cur = cur->next) {
        if (value.kind == cur->value.kind && value.val.bits == cur->value.val.bits) {
            seen_at = up;
        }
        up += 1;
    }
    if (seen_at != 0) {
        mx_str_indent(out, indent, prefix);
        mx_str_format(out, "<ref %zu>\n", seen_at);
    } else {
        mx_str_debug_t next = {
            .next = link,
            .value = value,
        };
        mx_str_indent(out, indent, prefix);
        if (mx_obj_is_nil(value)) {
            mx_str_format(out, "nil\n");
        } else if (mx_obj_is_boolean(value)) {
            if (mx_obj_get_boolean(value)) {
                mx_str_format(out, "true\n");
            } else {
                mx_str_format(out, "false\n");
            }
        } else if (mx_obj_is_number(value)) {
            mx_str_number(out, mx_obj_get_number(value));
            mx_str_format(out, "\n");
        } else if (mx_obj_is_string(value)) {
            mx_str_format(out, "\"%s\"\n", mx_obj_get_string(value)->buf);
        } else if (mx_obj_is_closure(value)) {
            mx_str_format(out, "<function: %p>\n", mx_obj_get_closure(value));
        } else if (mx_obj_is_block(value)) {
            mx_str_format(out, "<code: %p>\n", mx_obj_get_block(value));
        } else if (mx_obj_is_table(value)) {
            mx_str_obj_debug_table(out, indent, mx_obj_get_table(value), &next);
        } else if (mx_obj_is_ffi(value)) {
            mx_str_format(out, "<function: %p>\n", mx_obj_get_ffi(value));
        } else {
            mx_str_format(out, "<object>\n");
        }
    }
}

void mx_str_object_tostring(mx_t *vm, mx_str_t *buf, mx_obj_t value) {
    if (mx_obj_is_nil(value)) {
        mx_str_format(buf, "nil");
    } else if (mx_obj_is_boolean(value)) {
        const char *text = "false";
        if (mx_obj_get_boolean(value)) {
            text = "true";
        }
        mx_str_format(buf, "%s", text);
    } else if (mx_obj_is_number(value)) {
        mx_str_number(buf, mx_obj_get_number(value));
    } else if (mx_obj_is_string(value)) {
        mx_str_t *src = mx_obj_get_string(value);
        mx_str_append(buf, src->buf, src->len);
    } else if (mx_obj_is_closure(value)) {
        mx_str_format(buf, "function: %p", mx_obj_get_closure(value));
    } else if (mx_obj_is_block(value)) {
        mx_str_format(buf, "code: %p", mx_obj_get_block(value));
    } else if (mx_obj_is_data(value)) {
        const char *name = "data";
        if (vm != NULL) {
            name = mx_kind_name(vm, mx_obj_kind(value));
        }
        mx_str_format(buf, "%s: %p", name, mx_obj_data_head(value));
    } else if (mx_obj_is_ffi(value)) {
        mx_str_format(buf, "function: %p", mx_obj_get_ffi(value));
    }
}

mx_str_t *mx_str_from_cstr(const char *str) {
    mx_str_t *buf = mx_str_new();
    mx_str_append(buf, str, (uint32_t)strlen(str));
    return buf;
}

mx_str_t *mx_str_new_gc(mx_t *vm) {
    mx_str_t *ret = mx_gc_alloc(vm, sizeof(mx_str_t));
    *ret = (mx_str_t) {
        .head = {
            .kind = MX_KIND_STRING,
        },
        .is_gc = true,
    };
    mx_str_reserve(ret, 1);
    ret->buf[0] = '\0';
    return ret;
}

mx_str_t *mx_str_from_cstr_gc(mx_t *vm, const char *str) {
    mx_str_t *buf = mx_str_new_gc(vm);
    mx_str_append(buf, str, (uint32_t)strlen(str));
    return buf;
}

mx_str_t *mx_str_new_gc_sized(mx_t *vm, uint32_t len) {
    if (len >= MX_STR_SSO_CAP && MX_STR_TAIL_HEADER + (size_t)len + 1 <= MX_GC_MEDIUM_MAX) {
        mx_str_t *ret = mx_gc_alloc(vm, MX_STR_TAIL_HEADER + (size_t)len + 1);
        *ret = (mx_str_t) {
            .head = {
                .kind = MX_KIND_STRING,
            },
            .cap = len + 1,
            .buf = (char *)ret + MX_STR_TAIL_HEADER,
            .is_gc = true,
            .is_tail = true,
        };
        ret->buf[0] = '\0';
        return ret;
    } else {
        mx_str_t *ret = mx_gc_alloc(vm, sizeof(mx_str_t));
        *ret = (mx_str_t) {
            .head = {
                .kind = MX_KIND_STRING,
            },
            .is_gc = true,
        };
        if (len < MX_STR_SSO_CAP) {
            ret->buf = ret->small;
            ret->cap = MX_STR_SSO_CAP;
            ret->is_sso = true;
        } else {
            mx_gc_note_final_owned(&ret->head);
            mx_str_payload_t got = mx_str_payload_alloc((size_t)len + 1);
            mx_gc_note_bytes(vm, got.cap);
            ret->buf = got.data;
            ret->cap = got.cap;
        }
        ret->buf[0] = '\0';
        return ret;
    }
}

mx_str_t *mx_str_new_gc_vformat(mx_t *vm, const char *fmt, va_list ap) {
    char stage[MX_STR_FORMAT_STAGE];
    va_list ap_copy;
    va_copy(ap_copy, ap);
    int32_t written = vsnprintf(stage, sizeof(stage), fmt, ap_copy);
    va_end(ap_copy);
    if (written >= 0 && (size_t)written < sizeof(stage)) {
        mx_str_t *out = mx_str_new_gc_sized(vm, (uint32_t)written);
        memcpy(out->buf, stage, (size_t)written + 1);
        out->len = (uint32_t)written;
        return out;
    } else {
        mx_str_t *out = mx_str_new_gc(vm);
        mx_str_vformat(out, fmt, ap);
        return out;
    }
}

mx_str_t *mx_str_cache_get(mx_t *vm, const char *bytes, uint32_t len) {
    uint32_t h = mx_str_hashn(bytes, len);
    size_t slots = sizeof(vm->str_cache) / sizeof(vm->str_cache[0]);
    size_t idx = (h ^ (len * MX_STR_CACHE_LEN_MULT)) & (slots - 1);
    mx_str_t *have = vm->str_cache[idx];
    if (have != NULL && have->len == len && memcmp(have->buf, bytes, len) == 0) {
        return have;
    } else {
        mx_str_t *out = mx_str_new_gc_sized(vm, len);
        memcpy(out->buf, bytes, len);
        out->buf[len] = '\0';
        out->len = len;
        out->hash = h;
        vm->str_cache[idx] = out;
        return out;
    }
}

// s .. v, or v .. s when is_int_first, without making a string for v unless
// the result has to be a rope.
mx_str_t *mx_str_concat_int(mx_t *vm, mx_str_t *s, int64_t v, bool is_int_first) {
    char tmp[MX_STR_INT_DIGITS];
    uint32_t at = mx_str_int_digits(tmp, v);
    uint32_t n = MX_STR_INT_DIGITS - at;
    uint64_t total = (uint64_t)s->len + n;
    if (!s->is_rope && total <= MX_STR_FLAT_CONCAT_MAX) {
        mx_str_t *ret = mx_str_new_gc_sized(vm, (uint32_t)total);
        if (is_int_first) {
            memcpy(ret->buf, &tmp[at], n);
            memcpy(ret->buf + n, s->buf, s->len);
        } else {
            memcpy(ret->buf, s->buf, s->len);
            memcpy(ret->buf + s->len, &tmp[at], n);
        }
        ret->len = (uint32_t)total;
        ret->buf[total] = '\0';
        return ret;
    }
    mx_str_t *num = mx_str_new_gc(vm);
    mx_str_append(num, &tmp[at], n);
    if (is_int_first) {
        return mx_str_new_rope_gc(vm, num, s);
    } else {
        return mx_str_new_rope_gc(vm, s, num);
    }
}

mx_str_t *mx_str_new_rope_gc(mx_t *vm, mx_str_t *l, mx_str_t *r) {
    uint64_t total = (uint64_t)l->len + (uint64_t)r->len;
    if (!l->is_rope && !r->is_rope && total <= MX_STR_FLAT_CONCAT_MAX) {
        mx_str_t *ret = mx_str_new_gc_sized(vm, (uint32_t)total);
        memcpy(ret->buf, l->buf, l->len);
        memcpy(ret->buf + l->len, r->buf, r->len);
        ret->len = (uint32_t)total;
        ret->buf[total] = '\0';
        return ret;
    } else {
        mx_str_t *ret = mx_gc_alloc(vm, sizeof(mx_str_t));
        *ret = (mx_str_t) {
            .head = {
                .kind = MX_KIND_STRING,
            },
            .len = l->len + r->len,
            .is_rope = true,
            .is_gc = true,
            .rope = {
                .left = l,
                .right = r,
            },
        };
        return ret;
    }
}
