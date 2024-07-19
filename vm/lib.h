
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

#include "./vm.h"

#if 0
#define __builtin_trap()                                       \
    printf("file %s, line %zu\n", __FILE__, (size_t)__LINE__); \
    exit(1);
#endif

#if 1
#define VM_MAYBE_INLINE
#else
#define VM_MAYBE_INLINE inline
#endif

void *mi_malloc(size_t size);
void *mi_realloc(void *ptr, size_t size);
void mi_free(const void *ptr);
char *mi_strdup(const char *str);

#define vm_malloc(x) mi_malloc(x)
#define vm_realloc(x, y) mi_realloc(x, y)
#define vm_free(x) mi_free(x)
#define vm_strdup(x) mi_strdup(x)

#endif
