
#pragma once

#if !defined(MX_HEADER_VALUE_VAL)
#define MX_HEADER_VALUE_VAL

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <locale.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct mx_ref_t mx_ref_t;
typedef union mx_val_t mx_val_t;

struct mx_ref_t {
    uint64_t h;
};

union mx_val_t {
    uint64_t bits;
    int64_t i64;
    double f64;
    mx_ref_t ref;
};

static inline mx_val_t mx_val_i64(int64_t v) {
    return (mx_val_t) { .i64 = v };
}

static inline mx_val_t mx_val_f64(double v) {
    return (mx_val_t) { .f64 = v };
}

static inline mx_val_t mx_val_bool(bool b) {
    return (mx_val_t) { .bits = (uint64_t)b };
}

static inline mx_val_t mx_val_nil(void) {
    return (mx_val_t) { .bits = 0 };
}

static inline mx_val_t mx_val_ref(mx_ref_t r) {
    return (mx_val_t) { .ref = r };
}

static inline int64_t mx_val_as_i64(mx_val_t v) {
    return v.i64;
}

static inline double mx_val_as_f64(mx_val_t v) {
    return v.f64;
}

static inline bool mx_val_as_bool(mx_val_t v) {
    return v.bits != 0;
}

static inline mx_ref_t mx_val_as_ref(mx_val_t v) {
    return v.ref;
}

static inline mx_ref_t mx_ref_pin(void *obj) {
    return (mx_ref_t) { .h = (uint64_t)(uintptr_t)obj };
}

static inline void *mx_ref_obj(mx_ref_t r) {
    return (void *)(uintptr_t)r.h;
}

#endif
