
#ifndef VM_COSMO
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mimalloc/include/mimalloc.h>
#else
#include <cosmopolitan.h>
#endif

#define vm_mem_free(ptr) (mi_free(ptr))
#define vm_mem_alloc0(len) (mi_calloc(1, len))
#define vm_mem_alloc(len) (mi_malloc(len))
#define vm_mem_realloc(ptr, len) (mi_realloc(ptr, len))
#define vm_putchar(chr) (putchar(chr))

#if defined(__GNUC__) || defined(__clang__)
#define vm_fmod(lhs, rhs) (__builtin_fmod((lhs), (rhs)))
#else
#include <math.h>
#define vm_fmod(lhs, rhs) fmod((lhs), (rhs))
#endif