
#if !defined(VM_HEADER_LIB)
#define VM_HEADER_LIB

#if !defined(VM_COSMO)
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include "../bin/cosmopolitan.h"
#endif

#if defined(__TINYC__)
#define __builtin_trap() exit(1)
#define __builtin_unreachable() exit(1)
#endif

void GC_init(void);
void *GC_malloc(size_t size);
void *GC_realloc(void *ptr, size_t size);
void GC_free(void *ptr);

#define vm_init_mem() (GC_init())
#define vm_malloc(x) (GC_malloc((x)))
#define vm_realloc(x,y) (GC_realloc((x),(y)))
#define vm_free(x) (GC_free((x)))

#endif
