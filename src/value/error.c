
#include "error.h"
#include "obj.h"
#include "string.h"
#include "../vm/gc.h"

const char *const mx_varhint_names[] = {
    [MX_VARHINT_NONE] = "",
    [MX_VARHINT_LOCAL] = "local",
    [MX_VARHINT_GLOBAL] = "global",
    [MX_VARHINT_FIELD] = "field",
    [MX_VARHINT_METHOD] = "method",
    [MX_VARHINT_UPVALUE] = "upvalue",
    [MX_VARHINT_CONSTANT] = "constant",
    [MX_VARHINT_FOR_ITERATOR] = "for iterator",
};

uint32_t mx_source_scan(const char *src, uint32_t len, uint32_t *lines) {
    if (memchr(src, '\r', len) == NULL) {
        uint32_t n = 1;
        const char *end = src + len;
        const char *at = memchr(src, '\n', len);
        while (at != NULL) {
            at += 1;
            if (lines != NULL) {
                lines[n] = (uint32_t)(at - src);
            }
            n += 1;
            at = memchr(at, '\n', (size_t)(end - at));
        }
        return n;
    }
    uint32_t n = 1;
    uint32_t i = 0;
    while (i < len) {
        char c = src[i];
        i += 1;
        if (c == '\n' || c == '\r') {
            if (i < len && (src[i] == '\n' || src[i] == '\r') && src[i] != c) {
                i += 1;
            }
            if (lines != NULL) {
                lines[n] = i;
            }
            n += 1;
        }
    }
    return n;
}

const mx_source_t *mx_source_new(mx_t *vm, const char *file, const char *src, uint32_t len) {
    uint32_t nlines = mx_source_scan(src, len, NULL);
    uint32_t *lines = mx_arena_alloc(&vm->arenas.ir, sizeof(uint32_t) * nlines);
    lines[0] = 0;
    mx_source_scan(src, len, lines);
    mx_source_t *source = mx_arena_alloc(&vm->arenas.ir, sizeof(mx_source_t));
    *source = (mx_source_t) {
        .file = file,
        .src = src,
        .lines = lines,
        .len = len,
        .nlines = nlines,
    };
    vm->arenas.ir.sources += (uint32_t)(sizeof(uint32_t) * nlines + sizeof(mx_source_t));
    return source;
}

uint32_t mx_source_line(const mx_source_t *source, uint32_t byte) {
    uint32_t lo = 0;
    uint32_t hi = source->nlines;
    while (hi - lo > 1) {
        uint32_t mid = lo + (hi - lo) / 2;
        if (source->lines[mid] <= byte) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    return lo + 1;
}

mx_error_chunkid_t mx_error_chunkid(const char *file) {
    mx_error_chunkid_t got;
    size_t cap = sizeof(got.id);
    if (file == NULL) {
        memcpy(got.id, "?", 2);
    } else if (file[0] == '=' || file[0] == '@') {
        size_t len = strlen(file + 1);
        if (file[0] == '@' && len >= cap) {
            memcpy(got.id, "...", 3);
            memcpy(got.id + 3, file + 1 + len - (cap - 4), cap - 4);
            got.id[cap - 1] = '\0';
        } else {
            if (len > cap - 1) {
                len = cap - 1;
            }
            memcpy(got.id, file + 1, len);
            got.id[len] = '\0';
        }
    } else {
        const char *nl = strchr(file, '\n');
        size_t len = strlen(file);
        size_t room = cap - sizeof("[string \"...\"]");
        if (nl == NULL && len <= room) {
            snprintf(got.id, cap, "[string \"%s\"]", file);
        } else {
            if (nl != NULL) {
                len = (size_t)(nl - file);
            }
            if (len > room) {
                len = room;
            }
            snprintf(got.id, cap, "[string \"%.*s...\"]", (int32_t)len, file);
        }
    }
    return got;
}

mx_error_t *mx_error_new(mx_t *vm, mx_obj_t payload) {
    mx_error_t *err = mx_gc_alloc(vm, sizeof(mx_error_t));
    err->head.kind = MX_KIND_ERROR;
    err->payload_kind = payload.kind;
    err->payload = payload.val;
    err->operand = MX_ERROR_OPERAND_NONE;
    err->is_raw = false;
    err->is_handled = false;
    err->is_hint_inline = false;
    return err;
}

mx_obj_t mx_error_raw_vformat(mx_t *vm, mx_error_operand_t operand, const char *fmt, va_list ap) {
    mx_str_t *msg = mx_str_new_gc_vformat(vm, fmt, ap);
    mx_error_t *err = mx_error_new(vm, mx_obj_of_string(msg));
    err->is_raw = true;
    err->operand = operand;
    return mx_obj_of_error(err);
}

mx_obj_t mx_error_raw(mx_t *vm, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    mx_obj_t err = mx_error_raw_vformat(vm, MX_ERROR_OPERAND_NONE, fmt, ap);
    va_end(ap);
    return err;
}

mx_obj_t mx_error_raw_operand(mx_t *vm, mx_error_operand_t operand, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    mx_obj_t err = mx_error_raw_vformat(vm, operand, fmt, ap);
    va_end(ap);
    return err;
}

mx_obj_t mx_error_value(mx_t *vm, mx_obj_t payload) {
    return mx_obj_of_error(mx_error_new(vm, payload));
}

void mx_error_where(mx_str_t *out, const mx_location_range_t *range) {
    if (range != NULL && range->source != NULL) {
        mx_error_chunkid_t id = mx_error_chunkid(range->source->file);
        mx_str_append(out, id.id, (uint32_t)strlen(id.id));
        mx_str_append_byte(out, ':');
        mx_str_append_int(out, mx_source_line(range->source, range->start));
        mx_str_append(out, ": ", 2);
    }
}

mx_obj_t mx_error_message(mx_t *vm, const mx_location_range_t *range, const char *fmt, ...) {
    mx_str_t *msg = mx_str_new_gc(vm);
    mx_error_where(msg, range);
    va_list ap;
    va_start(ap, fmt);
    mx_str_vformat(msg, fmt, ap);
    va_end(ap);
    return mx_error_value(vm, mx_obj_of_string(msg));
}

void mx_error_locate(mx_t *vm, mx_error_t *err, const mx_location_range_t *range) {
    if (err->is_raw) {
        mx_str_t *raw = mx_str_flat((mx_str_t *)mx_ref_obj(err->payload.ref));
        mx_varname_t name = {0};
        if (err->operand != MX_ERROR_OPERAND_NONE && range != NULL && range->names[err->operand - MX_ERROR_OPERAND_LHS].name != NULL) {
            name = range->names[err->operand - MX_ERROR_OPERAND_LHS];
        }
        size_t hint_len = 0;
        if (name.kind != MX_VARHINT_NONE) {
            hint_len = strlen(mx_varhint_names[name.kind]) + strlen(name.name) + 6;
        }
        size_t where_len = sizeof(mx_error_chunkid_t) + MX_STR_INT_DIGITS + 3;
        mx_str_t *msg = mx_str_new_gc_sized(vm, (uint32_t)(where_len + raw->len + hint_len));
        mx_error_where(msg, range);
        uint32_t split = raw->len;
        const char *space = memchr(raw->buf, ' ', raw->len);
        if (err->is_hint_inline && space != NULL) {
            split = (uint32_t)(space - raw->buf);
        }
        mx_str_append(msg, raw->buf, split);
        if (name.kind != MX_VARHINT_NONE) {
            const char *hint = mx_varhint_names[name.kind];
            mx_str_append(msg, " (", 2);
            mx_str_append(msg, hint, (uint32_t)strlen(hint));
            mx_str_append(msg, " '", 2);
            mx_str_append(msg, name.name, (uint32_t)strlen(name.name));
            mx_str_append(msg, "')", 2);
        }
        mx_str_append(msg, raw->buf + split, raw->len - split);
        err->payload_kind = MX_KIND_STRING;
        err->payload = mx_obj_of_string(msg).val;
        err->is_raw = false;
    }
}

mx_obj_t mx_error_payload(mx_obj_t err) {
    mx_error_t *e = mx_obj_get_error(err);
    return mx_obj_of_raw(e->payload_kind, e->payload);
}
