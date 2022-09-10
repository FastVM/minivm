
#if !defined(VM_HEADER_LIB)
#define VM_HEADER_LIB

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 1
#define vm_malloc(size) (malloc(size))
#define vm_alloc0(size) (calloc(size, 1))
#define vm_realloc(ptr, size) (realloc(ptr, size))
#define vm_free(ptr) (free((void *)ptr))
#else
void *vm_malloc(size_t size);
void *vm_alloc0(size_t size);
void *vm_realloc(void *ptr, size_t size);
void vm_free(void *size);
#endif
#include "config.h"

#endif
