
#pragma once

#if !defined(MX_HEADER_VALUE_NUM)
#define MX_HEADER_VALUE_NUM

#include "val.h"

#define MX_NUM_INT_BITS 64

double mx_num_fmod(double a, double b);
int64_t mx_num_idiv(int64_t a, int64_t b);
int64_t mx_num_imod(int64_t a, int64_t b);
int64_t mx_num_shl(int64_t a, int64_t b);
int64_t mx_num_shr(int64_t a, int64_t b);

#endif
