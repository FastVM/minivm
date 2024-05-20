
#if !defined(VM_HEADER_LIB)
#define VM_HEADER_LIB

#if defined(_WIN32)
#define _CRT_SECURE_NO_DEPRECATE
#endif

#if defined(__TINYC__)
#define __builtin_trap() exit(1)
#define __builtin_unreachable() exit(1)
#define __pure2 __attribute__((__const__))
#define __unused __attribute__((__unused__))
#define __used __attribute__((__used__))
#define __packed __attribute__((__packed__))
#define __aligned(x) __attribute__((__aligned__(x)))
#define __section(x) __attribute__((__section__(x)))
#endif

#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wctype.h>

#include "config.h"

#if 0
#define __builtin_trap()                                       \
    printf("file %s, line %zu\n", __FILE__, (size_t)__LINE__); \
    exit(1);
#endif

static inline void *vm_malloc(size_t size) {
    void *ret = malloc(size);
    if (ret == NULL) {
        __builtin_trap();
    }
    return ret;
}

static inline void *vm_realloc(void *ptr, size_t size) {
    void *ret = realloc(ptr, size);
    if (ret == NULL) {
        __builtin_trap();
    }
    return ret;
}

static inline void vm_free(const void *ptr) {
    free((void *) ptr);
}

static inline char *vm_strdup(const char *str) {
    size_t len = strlen(str);
    char *buf = vm_malloc(sizeof(char) * (len + 1));
    memcpy(buf, str, len + 1);
    return buf;
}

static inline void vm_config_add_extern(vm_config_t *config, void *value) {
    vm_externs_t *last = config->externs;
    for (vm_externs_t *cur = last; cur; cur = cur->last) {
        if (cur->value == value) {
            return;
        }
    }
    vm_externs_t *next = vm_malloc(sizeof(vm_externs_t));
    next->id = last == NULL ? 0 : last->id + 1;
    next->value = value;
    next->last = last;
    config->externs = next;
}

#endif
