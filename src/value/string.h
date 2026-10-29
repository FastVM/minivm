
#pragma once

#if !defined(MX_HEADER_VALUE_STRING)
#define MX_HEADER_VALUE_STRING

#include "../vm/vm.h"

#define MX_STR_INT_DIGITS 24
#define MX_STR_ROPE_STACK 64
#define MX_STR_SSO_CAP 15
#define MX_STR_POOL_MIN_SHIFT 4
#define MX_STR_POOL_CLASSES 9
#define MX_STR_FLOAT_CAP 48
#define MX_STR_FORMAT_STAGE 256
#define MX_STR_DEBUG_KEY_CAP 64
#define MX_STR_EXACT_FLOAT_MAX 9007199254740992.0
#define MX_STR_CACHE_LEN_MULT ((uint32_t)131)

typedef struct mx_str_rope_t mx_str_rope_t;
typedef struct mx_str_debug_t mx_str_debug_t;
typedef struct mx_str_payload_t mx_str_payload_t;

struct mx_str_payload_t {
    char *data;
    uint32_t cap;
};

struct mx_str_rope_t {
    mx_str_t *left;
    mx_str_t *right;
};

struct mx_str_t {
    mx_obj_head_t head;
    uint32_t cap;
    uint32_t len;
    char *buf;
    uint32_t hash;
    bool is_pooled: 1;
    bool is_rope: 1;
    bool is_interned: 1;
    bool is_sso: 1;
    bool is_gc: 1;
    bool is_tail: 1;
    bool is_kept: 1;
    union {
        mx_str_rope_t rope;
        char small[MX_STR_SSO_CAP + 1];
    };
};

#define MX_STR_TAIL_HEADER offsetof(mx_str_t, small)

struct mx_str_debug_t {
    mx_str_debug_t *next;
    mx_obj_t value;
};

static inline uint32_t mx_str_hashn(const char *bytes, size_t len) {
    uint32_t ret = MX_VM_FNV_OFFSET;
    for (size_t i = 0; i < len; i++) {
        ret *= MX_VM_FNV_PRIME;
        ret ^= (uint8_t)bytes[i];
    }
    if (ret == 0) {
        ret = 1;
    }
    return ret;
}

static inline uint32_t mx_str_content_hash(mx_str_t *buf) {
    if (buf->hash == 0) {
        buf->hash = mx_str_hashn(buf->buf, buf->len);
    }
    return buf->hash;
}

mx_str_t *mx_str_new(void);
mx_str_t *mx_str_from_cstr(const char *str);
void mx_str_free(mx_str_t *buf);
char *mx_str_release(mx_str_t *buf);
int32_t mx_str_pool_class(size_t bytes);
mx_str_payload_t mx_str_payload_alloc(size_t bytes);
void mx_str_payload_free(char *payload);
void mx_str_reserve(mx_str_t *buf, uint32_t need);

mx_str_t *mx_str_new_gc(mx_t *vm);
mx_str_t *mx_str_new_gc_sized(mx_t *vm, uint32_t len);
mx_str_t *mx_str_new_gc_vformat(mx_t *vm, const char *fmt, va_list ap);
mx_str_t *mx_str_cache_get(mx_t *vm, const char *bytes, uint32_t len);
mx_str_t *mx_str_from_cstr_gc(mx_t *vm, const char *str);
mx_str_t *mx_str_new_rope_gc(mx_t *vm, mx_str_t *l, mx_str_t *r);

char *mx_str_sprintf(const char *fmt, ...);
char *mx_str_vsprintf(const char *fmt, va_list ap);
void mx_str_print(mx_str_t *buf, FILE *out);
void mx_str_flatten(mx_str_t *buf);

void mx_str_format(mx_str_t *buf, const char *fmt, ...);
void mx_str_vformat(mx_str_t *buf, const char *fmt, va_list ap);
void mx_str_append(mx_str_t *buf, const char *data, uint32_t len);
void mx_str_append_byte(mx_str_t *buf, char c);
uint32_t mx_str_int_digits(char *tmp, int64_t v);
void mx_str_append_int(mx_str_t *buf, int64_t v);
void mx_str_obj_debug(mx_str_t *out, size_t indent, const char *prefix, mx_obj_t value, mx_str_debug_t *link);
void mx_str_obj_debug_table(mx_str_t *out, size_t indent, mx_obj_table_t *tbl, mx_str_debug_t *next);
void mx_str_object_tostring(mx_t *vm, mx_str_t *buf, mx_obj_t value);
int32_t mx_str_float_digits(char *tmp, double n);
void mx_str_number(mx_str_t *out, double n);
void mx_str_float(mx_str_t *out, double n);
void mx_str_indent(mx_str_t *out, size_t indent, const char *prefix);
void mx_str_print_lit(mx_str_t *out, mx_obj_t value);

static inline mx_str_t *mx_str_flat(mx_str_t *buf) {
    if (buf->is_rope) {
        mx_str_flatten(buf);
    }
    return buf;
}

#endif
