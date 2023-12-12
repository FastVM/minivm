// If you're trying to port TB on to a new platform you'll need to fill in these
// functions with their correct behavior.
#pragma once
#include <stddef.h>
#include <setjmp.h>

#if defined(TB_USE_MIMALLOC)
#include <mimalloc.h>
#define tb_platform_heap_alloc(size)        mi_malloc(size)
#define tb_platform_heap_realloc(ptr, size) mi_realloc(ptr, size)
#define tb_platform_heap_free(ptr)          mi_free(ptr)
#else
void *GC_malloc(size_t size);
void *GC_realloc(void *ptr, size_t size);
void *GC_free(void *ptr);
#define tb_platform_heap_alloc(size)        GC_malloc(size)
#define tb_platform_heap_free(ptr)          ((void)(ptr))
#define tb_platform_heap_realloc(ptr, size) GC_realloc(ptr, size)
#endif

////////////////////////////////
// Virtual memory management
////////////////////////////////
typedef enum {
    TB_PAGE_RO,
    TB_PAGE_RW,
    TB_PAGE_RX,
    TB_PAGE_RXW,
} TB_MemProtect;

// This is used for JIT compiler pages or any large scale memory allocations.
void* tb_platform_valloc(size_t size);
void  tb_platform_vfree(void* ptr, size_t size);
bool  tb_platform_vprotect(void* ptr, size_t size, TB_MemProtect prot);
