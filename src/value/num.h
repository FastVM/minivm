
#pragma once

#if !defined(MX_HEADER_VALUE_NUM)
#define MX_HEADER_VALUE_NUM

#include "val.h"

#define MX_NUM_INT_BITS 64

double mx_num_fmod(double a, double b);

static inline int64_t mx_num_idiv(int64_t a, int64_t b) {
    if (b == -1) {
        return (int64_t)(0 - (uint64_t)a);
    } else {
        int64_t q = a / b;
        if ((a % b != 0) && ((a < 0) != (b < 0))) {
            return q - 1;
        } else {
            return q;
        }
    }
}

static inline int64_t mx_num_imod(int64_t a, int64_t b) {
    if (b == -1) {
        return 0;
    } else {
        int64_t r = a % b;
        if (r != 0 && (r < 0) != (b < 0)) {
            return r + b;
        } else {
            return r;
        }
    }
}

static inline int64_t mx_num_shl(int64_t a, int64_t b) {
    if (b <= -MX_NUM_INT_BITS) {
        return 0;
    } else if (b < 0) {
        return (int64_t)((uint64_t)a >> -b);
    } else if (b >= MX_NUM_INT_BITS) {
        return 0;
    } else {
        return (int64_t)((uint64_t)a << b);
    }
}

static inline int64_t mx_num_shr(int64_t a, int64_t b) {
    if (b <= -MX_NUM_INT_BITS) {
        return 0;
    } else if (b < 0) {
        return (int64_t)((uint64_t)a << -b);
    } else if (b >= MX_NUM_INT_BITS) {
        return 0;
    } else {
        return (int64_t)((uint64_t)a >> b);
    }
}

#endif
