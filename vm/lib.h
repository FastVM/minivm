
#if !defined(VM_HEADER_LIB)
#define VM_HEADER_LIB

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if VM_LIBGC
void *GC_malloc(size_t size);
void *GC_realloc(void *ptr, size_t size);
void GC_free(void *ptr);
void GC_init(void);
#define vm_alloc0(size) GC_malloc(size)
#define vm_malloc(size) GC_malloc(size)
#define vm_realloc(ptr, size) GC_realloc(ptr, size)
#define vm_free(ptr) GC_free(ptr)
#define vm_init() GC_init()
#elif VM_ALLOC
void *vm_malloc(size_t size);
void *vm_alloc0(size_t size);
void *vm_realloc(void *ptr, size_t size);
void vm_free(void *size);
#define vm_init() ((void)0)
#else
#define vm_malloc(size) (malloc(size))
#define vm_alloc0(size) (calloc(size, 1))
#define vm_realloc(ptr, size) (realloc(ptr, size))
#define vm_free(ptr) (free((void *)ptr))
#define vm_init() ((void)0)
#endif

#include "config.h"

#endif
