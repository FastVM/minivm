
#if !defined(VM_HEADER_LIB)
#define VM_HEADER_LIB

#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef double vm_number_t;
typedef int64_t vm_t;

#if defined(VM_XGC)

void *vm_malloc(size_t size);
void *vm_alloc0(size_t size);
void *vm_realloc(void *ptr, size_t size);
void vm_free(void *ptr);

#elif defined(VM_MIMALLOC)

#include <mimalloc.h>
#define vm_malloc(size) (mi_malloc(size))
#define vm_alloc0(size) (mi_calloc(size, 1))
#define vm_realloc(ptr, size) (mi_realloc(ptr, size))
#define vm_free(ptr) (mi_free((void *)ptr))

#else

#define vm_malloc(size) (malloc(size))
#define vm_alloc0(size) (calloc(size, 1))
#define vm_realloc(ptr, size) (realloc(ptr, size))
#define vm_free(ptr) (free((void *)ptr))

#endif

#endif
