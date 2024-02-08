#pragma once
#include <stdint.h>

typedef struct TB_MultiplyResult {
    uint64_t lo;
    uint64_t hi;
} TB_MultiplyResult;

#if defined(_MSC_VER) && !defined(__clang__)
static int tb_clz64(uint64_t x) {
    return _lzcnt_u64(x);
}

static int tb_ffs(uint32_t x) {
    unsigned long index;
    return _BitScanForward(&index, x) ? 1 + index : 0;
}

static int tb_ffs64(uint64_t x) {
    unsigned long index;
    return _BitScanForward64(&index, x) ? 1 + index : 0;
}

static int tb_popcount(uint32_t x) {
    return __popcnt(x);
}

static int tb_popcount64(uint64_t x) {
    return __popcnt64(x);
}

static uint64_t tb_next_pow2(uint64_t x) {
    return x == 1 ? 1 : 1 << (64 - _lzcnt_u64(x - 1));
}

static bool tb_add_overflow(uint64_t a, uint64_t b, uint64_t* result) {
    uint64_t c = a + b;
    *result = c;
    return c < a;
}

static bool tb_sub_overflow(uint64_t a, uint64_t b, uint64_t* result) {
    uint64_t c = a - b;
    *result = c;
    return c > a;
}

#pragma intrinsic(_udiv128)
static uint64_t tb_div128(uint64_t ahi, uint64_t alo, uint64_t b) {
    uint64_t rem;
    return _udiv128(ahi, alo, b, &rem);
}

#pragma intrinsic(_umul128)
static TB_MultiplyResult tb_mul64x128(uint64_t a, uint64_t b) {
    uint64_t hi;
    uint64_t lo = _umul128(a, b, &hi);
    return (TB_MultiplyResult) { lo, hi };
}
#else
static int tb_clz64(uint64_t x) {
    return __builtin_clzll(x);
}

static int tb_ffs(uint32_t x) {
    return __builtin_ffs(x);
}

static int tb_ffs64(uint64_t x) {
    return __builtin_ffsll(x);
}

static int tb_popcount(uint32_t x) {
    return __builtin_popcount(x);
}

static int tb_popcount64(uint64_t x) {
    return __builtin_popcountll(x);
}

static uint64_t tb_next_pow2(uint64_t x) {
    return x == 1 ? 1 : 1 << (64 - __builtin_clzll(x - 1));
}

static bool tb_add_overflow(uint64_t a, uint64_t b, uint64_t* result) {
    return __builtin_add_overflow(a, b, result);
}

static bool tb_sub_overflow(uint64_t a, uint64_t b, uint64_t* result) {
    return __builtin_sub_overflow(a, b, result);
}

static TB_MultiplyResult tb_mul64x128(uint64_t a, uint64_t b) {
    __uint128_t product = (__uint128_t)a * (__uint128_t)b;

    return (TB_MultiplyResult) { (uint64_t)(product & 0xFFFFFFFFFFFFFFFF), (uint64_t)(product >> 64) };
}

static uint64_t tb_div128(uint64_t ahi, uint64_t alo, uint64_t b) {
    // // We don't want 128 bit software division
    // uint64_t d, e;
    // __asm__("divq %[b]"
    //     : "=a"(d), "=d"(e)
    //     : [b] "r"(b), "a"(alo), "d"(ahi)
    // );
    // We want 128 bit software division 
    __uint128_t x = 0;
    x += alo;
    x <<= 64;
    x += ahi;
    x /= b;
    return x;
}
#endif
