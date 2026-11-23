
#pragma once

#if !defined(MX_HEADER_JIT_BACKEND)
#define MX_HEADER_JIT_BACKEND

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t mx_backend_arch_t;
enum {
    MX_BACKEND_ARCH_NONE = 0,
    MX_BACKEND_ARCH_X64 = 1,
    MX_BACKEND_ARCH_RV64 = 2,
};

#if defined(__x86_64__) || defined(_M_X64)
#define MX_CURRENT_ARCH MX_BACKEND_ARCH_X64
#elif defined(__riscv) && (__riscv_xlen == 64)
#define MX_CURRENT_ARCH MX_BACKEND_ARCH_RV64
#else
#define MX_CURRENT_ARCH MX_BACKEND_ARCH_NONE
#endif

typedef struct mx_backend_t mx_backend_t;

struct mx_backend_t {
    mx_backend_arch_t arch;
    const char *name;
    uint32_t n_gpr;
    uint32_t n_fpr;
    bool has_hardware_div: 1;
    bool has_hardware_fpu: 1;
    bool has_compressed_inst: 1;
};

static inline mx_backend_t mx_backend_current(void) {
#if MX_CURRENT_ARCH == MX_BACKEND_ARCH_X64
    return (mx_backend_t) {
        .arch = MX_BACKEND_ARCH_X64,
        .name = "x86-64",
        .n_gpr = 16,
        .n_fpr = 16,
        .has_hardware_div = true,
        .has_hardware_fpu = true,
        .has_compressed_inst = false,
    };
#elif MX_CURRENT_ARCH == MX_BACKEND_ARCH_RV64
    return (mx_backend_t) {
        .arch = MX_BACKEND_ARCH_RV64,
        .name = "riscv64",
        .n_gpr = 32,
        .n_fpr = 32,
        .has_hardware_div = true,
        .has_hardware_fpu = true,
        .has_compressed_inst = true,
    };
#else
    return (mx_backend_t) {
        .arch = MX_BACKEND_ARCH_NONE,
        .name = "interpreter-fallback",
        .n_gpr = 0,
        .n_fpr = 0,
        .has_hardware_div = false,
        .has_hardware_fpu = false,
        .has_compressed_inst = false,
    };
#endif
}

#endif
