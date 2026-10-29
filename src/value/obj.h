
#pragma once

#if !defined(MX_HEADER_VALUE_OBJ)
#define MX_HEADER_VALUE_OBJ

#include "string.h"
#include "../vm/vm.h"

#define MX_INTERN_MAX_LEN 32
#define MX_INTERN_MIN_SLOTS 1024
#define MX_INTERN_LOAD_NUM 3
#define MX_INTERN_LOAD_DEN 4

typedef struct mx_obj_num_t mx_obj_num_t;
typedef struct mx_obj_int_t mx_obj_int_t;
typedef struct mx_obj_float_t mx_obj_float_t;
typedef struct mx_obj_strtod_t mx_obj_strtod_t;

struct mx_obj_num_t {
    mx_obj_t value;
    bool is_num: 1;
};

struct mx_obj_int_t {
    int64_t value;
    bool is_int: 1;
};

struct mx_obj_float_t {
    double value;
    bool is_num: 1;
};

struct mx_obj_strtod_t {
    double value;
    const char *end;
};

struct mx_obj_closure_t {
    mx_obj_head_t head;
    uint32_t len;
    mx_ir_block_t *block;
    void *gc_next;
    mx_obj_upval_t *upvals[];
};

struct mx_obj_upval_t {
    mx_obj_head_t head;
    uint32_t at;
    mx_thread_t *thread;
    void *gc_next;
    mx_obj_upval_t *next;
    uint32_t kind_at;
    mx_obj_t value;
};

struct mx_obj_cfn_t {
    mx_obj_head_t head;
    uint32_t len;
    mx_ffi_t *fn;
    void *gc_next;
    mx_std_fn_t *std_fn;
    const char *std_name;
    mx_obj_t values[];
};

mx_obj_closure_t *mx_obj_closure_new(mx_t *vm, mx_ir_block_t *block, uint32_t len);
mx_obj_upval_t *mx_obj_upval_new_closed(mx_t *vm, mx_obj_t value);

mx_obj_t mx_obj_new_string(mx_t *vm, const char *str);
mx_obj_t mx_obj_intern(mx_t *vm, const char *str);
mx_obj_t mx_obj_intern_strn(mx_t *vm, const char *bytes, uint32_t len);
void mx_intern_remove(mx_t *vm, mx_str_t *buf);
mx_str_t *mx_intern_tomb(void);
void mx_intern_grow(mx_t *vm);
mx_obj_t mx_intern_insert(mx_t *vm, const char *bytes, uint32_t len);

static inline bool mx_obj_is_nil(mx_obj_t o) {
    return o.kind == MX_KIND_NIL;
}

static inline bool mx_obj_is_boolean(mx_obj_t o) {
    return o.kind == MX_KIND_BOOLEAN;
}

static inline bool mx_obj_is_number(mx_obj_t o) {
    return o.kind == MX_KIND_INT || o.kind == MX_KIND_FLOAT;
}

static inline bool mx_obj_is_int(mx_obj_t o) {
    return o.kind == MX_KIND_INT;
}

static inline bool mx_obj_is_float(mx_obj_t o) {
    return o.kind == MX_KIND_FLOAT;
}

static inline int64_t mx_obj_get_int(mx_obj_t o) {
    return mx_val_as_i64(o.val);
}

static inline mx_obj_t mx_obj_of_raw(mx_kind_t kind, mx_val_t val) {
    mx_obj_t out;
    out.kind = kind;
    out.val = val;
    return out;
}

static inline mx_obj_t mx_obj_of_int(int64_t v) {
    mx_obj_t out;
    out.kind = MX_KIND_INT;
    out.val = mx_val_i64(v);
    return out;
}

static inline bool mx_obj_float_fits_int(double d) {
    return d >= -9223372036854775808.0 && d < 9223372036854775808.0;
}

static inline bool mx_obj_is_string(mx_obj_t o) {
    return o.kind == MX_KIND_STRING;
}

static inline bool mx_obj_is_data(mx_obj_t o) {
    return o.kind >= MX_KIND_FIRST_DATA;
}

static inline bool mx_obj_is_table(mx_obj_t o) {
    if (o.kind < MX_KIND_FIRST_DATA) {
        return false;
    } else {
        return ((const mx_obj_head_t *)mx_ref_obj(o.val.ref))->is_container;
    }
}

static inline mx_obj_t mx_obj_of_data(void *o) {
    mx_obj_t out;
    out.kind = ((mx_obj_head_t *)o)->kind;
    out.val = mx_val_ref(mx_ref_pin(o));
    return out;
}

static inline bool mx_obj_is_closure(mx_obj_t o) {
    return o.kind == MX_KIND_CLOSURE;
}

static inline bool mx_obj_is_ffi(mx_obj_t o) {
    return o.kind == MX_KIND_FFI;
}

static inline bool mx_obj_is_block(mx_obj_t o) {
    return o.kind == MX_KIND_BLOCK;
}

static inline bool mx_obj_is_lightuserdata(mx_obj_t o) {
    return o.kind == MX_KIND_LIGHTUSERDATA;
}

static inline mx_obj_t mx_obj_of_lightuserdata(void *p) {
    mx_obj_t out;
    out.kind = MX_KIND_LIGHTUSERDATA;
    out.val = mx_val_ref(mx_ref_pin(p));
    return out;
}

static inline void *mx_obj_get_lightuserdata(mx_obj_t o) {
    return mx_ref_obj(o.val.ref);
}

static inline bool mx_obj_is_error(mx_obj_t o) {
    return o.kind == MX_KIND_ERROR;
}

static inline bool mx_obj_get_boolean(mx_obj_t o) {
    return mx_val_as_bool(o.val);
}

static inline bool mx_obj_is_truthy(mx_obj_t o) {
    if (mx_obj_is_nil(o)) {
        return false;
    } else if (mx_obj_is_boolean(o)) {
        return mx_obj_get_boolean(o);
    } else {
        return true;
    }
}

static inline double mx_obj_get_number(mx_obj_t o) {
    if (o.kind == MX_KIND_INT) {
        return (double)mx_val_as_i64(o.val);
    } else {
        return mx_val_as_f64(o.val);
    }
}

static inline double mx_obj_get_float(mx_obj_t o) {
    return mx_val_as_f64(o.val);
}

static inline mx_obj_int_t mx_obj_to_int(mx_obj_t v) {
    if (mx_obj_is_int(v)) {
        return (mx_obj_int_t) { .value = mx_obj_get_int(v), .is_int = true };
    } else if (!mx_obj_is_float(v)) {
        return (mx_obj_int_t) { .value = 0, .is_int = false };
    } else {
        double d = mx_obj_get_float(v);
        if (!mx_obj_float_fits_int(d) || floor(d) != d) {
            return (mx_obj_int_t) { .value = 0, .is_int = false };
        } else {
            return (mx_obj_int_t) { .value = (int64_t)d, .is_int = true };
        }
    }
}

static inline bool mx_obj_int_eq_float(int64_t i, double f) {
    return mx_obj_float_fits_int(f) && floor(f) == f && (int64_t)f == i;
}

static inline bool mx_obj_int_lt_float(int64_t i, double f) {
    if (mx_obj_float_fits_int(f)) {
        return i < (int64_t)ceil(f);
    } else {
        return f > 0.0;
    }
}

static inline bool mx_obj_int_le_float(int64_t i, double f) {
    if (mx_obj_float_fits_int(f)) {
        return i <= (int64_t)floor(f);
    } else {
        return f > 0.0;
    }
}

static inline bool mx_obj_float_lt_int(double f, int64_t i) {
    if (mx_obj_float_fits_int(f)) {
        return (int64_t)floor(f) < i;
    } else {
        return f < 0.0;
    }
}

static inline bool mx_obj_float_le_int(double f, int64_t i) {
    if (mx_obj_float_fits_int(f)) {
        return (int64_t)ceil(f) <= i;
    } else {
        return f < 0.0;
    }
}

static inline bool mx_obj_num_eq(mx_obj_t a, mx_obj_t b) {
    if (mx_obj_is_int(a) && mx_obj_is_int(b)) {
        return mx_obj_get_int(a) == mx_obj_get_int(b);
    } else if (mx_obj_is_int(a)) {
        return mx_obj_int_eq_float(mx_obj_get_int(a), mx_obj_get_float(b));
    } else if (mx_obj_is_int(b)) {
        return mx_obj_int_eq_float(mx_obj_get_int(b), mx_obj_get_float(a));
    } else {
        return mx_obj_get_float(a) == mx_obj_get_float(b);
    }
}

static inline bool mx_obj_num_lt(mx_obj_t a, mx_obj_t b) {
    if (mx_obj_is_int(a) && mx_obj_is_int(b)) {
        return mx_obj_get_int(a) < mx_obj_get_int(b);
    } else if (mx_obj_is_int(a)) {
        return mx_obj_int_lt_float(mx_obj_get_int(a), mx_obj_get_float(b));
    } else if (mx_obj_is_int(b)) {
        return mx_obj_float_lt_int(mx_obj_get_float(a), mx_obj_get_int(b));
    } else {
        return mx_obj_get_float(a) < mx_obj_get_float(b);
    }
}

static inline bool mx_obj_num_le(mx_obj_t a, mx_obj_t b) {
    if (mx_obj_is_int(a) && mx_obj_is_int(b)) {
        return mx_obj_get_int(a) <= mx_obj_get_int(b);
    } else if (mx_obj_is_int(a)) {
        return mx_obj_int_le_float(mx_obj_get_int(a), mx_obj_get_float(b));
    } else if (mx_obj_is_int(b)) {
        return mx_obj_float_le_int(mx_obj_get_float(a), mx_obj_get_int(b));
    } else {
        return mx_obj_get_float(a) <= mx_obj_get_float(b);
    }
}

static inline int32_t mx_obj_str_cmp(const mx_str_t *a, const mx_str_t *b) {
    const char *s1 = a->buf;
    const char *s2 = b->buf;
    size_t rest1 = a->len;
    size_t rest2 = b->len;
    while (true) {
        int32_t c = strcoll(s1, s2);
        if (c != 0) {
            return c;
        }
        size_t seg1 = strlen(s1);
        size_t seg2 = strlen(s2);
        if (seg2 == rest2 && seg1 == rest1) {
            return 0;
        } else if (seg2 == rest2) {
            return 1;
        } else if (seg1 == rest1) {
            return -1;
        }
        s1 += seg1 + 1;
        s2 += seg2 + 1;
        rest1 -= seg1 + 1;
        rest2 -= seg2 + 1;
    }
}

static inline mx_str_t *mx_obj_get_string_raw(mx_obj_t o) {
    return (mx_str_t *)mx_ref_obj(o.val.ref);
}

static inline mx_str_t *mx_obj_get_string(mx_obj_t o) {
    mx_str_t *buf = mx_obj_get_string_raw(o);
    if (buf->is_rope) {
        mx_str_flatten(buf);
    }
    return buf;
}

static inline mx_obj_table_t *mx_obj_get_table(mx_obj_t o) {
    return (mx_obj_table_t *)mx_ref_obj(o.val.ref);
}

static inline mx_obj_closure_t *mx_obj_get_closure(mx_obj_t o) {
    return (mx_obj_closure_t *)mx_ref_obj(o.val.ref);
}

static inline mx_ffi_t *mx_obj_get_ffi(mx_obj_t o) {
    return (mx_ffi_t *)mx_ref_obj(o.val.ref);
}

static inline mx_ir_block_t *mx_obj_get_block(mx_obj_t o) {
    return (mx_ir_block_t *)mx_ref_obj(o.val.ref);
}

static inline mx_error_t *mx_obj_get_error(mx_obj_t o) {
    return (mx_error_t *)mx_ref_obj(o.val.ref);
}

static inline mx_obj_head_t *mx_obj_data_head(mx_obj_t o) {
    return (mx_obj_head_t *)mx_ref_obj(o.val.ref);
}

static inline mx_kind_t mx_obj_kind(mx_obj_t o) {
    return o.kind;
}

static inline mx_obj_t mx_obj_of_nil(void) {
    mx_obj_t out;
    out.kind = MX_KIND_NIL;
    out.val = mx_val_nil();
    return out;
}

static inline mx_obj_t mx_obj_of_boolean(bool b) {
    mx_obj_t out;
    out.kind = MX_KIND_BOOLEAN;
    out.val = mx_val_bool(b);
    return out;
}

static inline mx_obj_t mx_obj_of_number(double n) {
    mx_obj_t out;
    out.kind = MX_KIND_FLOAT;
    out.val = mx_val_f64(n);
    return out;
}

static inline mx_obj_t mx_obj_of_string(mx_str_t *o) {
    mx_obj_t out;
    out.kind = MX_KIND_STRING;
    out.val = mx_val_ref(mx_ref_pin(o));
    return out;
}

static inline mx_obj_t mx_obj_of_table(mx_obj_table_t *o) {
    mx_obj_t out;
    out.kind = ((mx_obj_head_t *)o)->kind;
    out.val = mx_val_ref(mx_ref_pin(o));
    return out;
}

static inline mx_obj_t mx_obj_of_closure(mx_obj_closure_t *o) {
    mx_obj_t out;
    out.kind = MX_KIND_CLOSURE;
    out.val = mx_val_ref(mx_ref_pin(o));
    return out;
}

static inline mx_obj_t mx_obj_of_ffi(mx_ffi_t *o) {
    mx_obj_t out;
    out.kind = MX_KIND_FFI;
    out.val = mx_val_ref(mx_ref_pin(o));
    return out;
}

static inline mx_obj_t mx_obj_of_block(mx_ir_block_t *o) {
    mx_obj_t out;
    out.kind = MX_KIND_BLOCK;
    out.val = mx_val_ref(mx_ref_pin(o));
    return out;
}

static inline mx_obj_t mx_obj_of_error(mx_error_t *o) {
    mx_obj_t out;
    out.kind = MX_KIND_ERROR;
    out.val = mx_val_ref(mx_ref_pin(o));
    return out;
}

#endif
