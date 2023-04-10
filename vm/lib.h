
#if !defined(VM_HEADER_LIB)
#define VM_HEADER_LIB

#include <dlfcn.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(__TINYC__)
#define __builtin_trap() exit(1)
#define __builtin_unreachable() exit(1)
#endif

#if 0
#define __builtin_trap() printf("file %s, line %zu\n", __FILE__, (size_t) __LINE__); exit(1);
#endif

void GC_init(void);
void *GC_malloc(size_t size);
void *GC_realloc(void *ptr, size_t size);
void GC_free(void *ptr);
void GC_add_roots(void *low, void *high);

#define vm_init_mem() (GC_init())
#define vm_malloc(x) (GC_malloc((x)))
#define vm_realloc(x, y) (GC_realloc((x), (y)))
#define vm_free(x) (GC_free((x)))

#endif
