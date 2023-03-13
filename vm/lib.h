
#if !defined(VM_HEADER_LIB)
#define VM_HEADER_LIB

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(VM_XGC)

void *vm_malloc(size_t size);
void *vm_realloc(void *ptr, size_t size);
void vm_free(void *ptr);

#elif defined(VM_MIMALLOC)

#include <mimalloc.h>
#define vm_malloc(size) (mi_malloc(size))
#define vm_realloc(ptr, size) (mi_realloc(ptr, size))
#define vm_free(ptr) (mi_free((void *)ptr))

#else

#define vm_malloc(size_) (malloc((size_)))
#define vm_realloc(ptr_, size_) (realloc((ptr_), (size_)))
#define vm_free(ptr_) (free((void *)(ptr_)))

#endif

#if defined(__TINYC__)
#define __builtin_trap() exit(1)
#define __builtin_unreachable() exit(1)
#endif

#endif
