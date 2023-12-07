
#if !defined(VM_HEADER_LIB)
#define VM_HEADER_LIB

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
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>
#include <math.h>
#include <time.h>

#include "config.h"

// this is only used for clock()

#if 0
#define __builtin_trap()                                       \
    printf("file %s, line %zu\n", __FILE__, (size_t)__LINE__); \
    exit(1);
#endif

#if 0
void GC_init();
void *GC_malloc(size_t size);
void *GC_realloc(void *ptr, size_t size);
void GC_free(void *ptr);

#define vm_init_mem() (GC_init())
#define vm_malloc(x) (GC_malloc(x))
#define vm_realloc(x, y) (GC_realloc(x, y))
#endif

#if 1
#define vm_init_mem() ((void)0)
#define vm_malloc(x) (malloc((x)))
#define vm_realloc(x, y) (realloc((x), (y)))
#endif
#endif

#if defined(_WIN32)
#define VM_CDECL __attribute__((cdecl))
#else
#define VM_CDECL
#endif
