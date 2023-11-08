
#include "gc.h"

tgc_t vm_gc_global;

void *GC_malloc(size_t size) {
    return tgc_alloc(&vm_gc_global, size);
}

void *GC_realloc(void *ptr, size_t size) {
    return tgc_realloc(&vm_gc_global, ptr, size);
}

void GC_free(void *ptr) {
    tgc_free(&vm_gc_global, ptr);
}

void GC_init(void) {
    size_t dummy = 0;
    tgc_start(&vm_gc_global, (uint8_t *)&dummy + 1024);
}
