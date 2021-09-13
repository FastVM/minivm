
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

#if !defined(VM_MEM_NO_MI) && !defined(VM_COSMO)
#define vm_mem_free(ptr) (mi_free(ptr))
#define vm_mem_alloc0(len) (mi_calloc(1, len))
#define vm_mem_alloc(len) (mi_malloc(len))
#define vm_mem_realloc(ptr, len) (mi_realloc(ptr, len))
#else
#define vm_mem_free(ptr) (free(ptr))
#define vm_mem_alloc0(len) (calloc(1, len))
#define vm_mem_alloc(len) (malloc(len))
#define vm_mem_realloc(ptr, len) (realloc(ptr, len))
#endif

#define vm_putchar(chr) (putchar(chr))