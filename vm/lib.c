
#include "vm.h"

#if VM_MALLOC_MI
#include "../vendor/mimalloc/include/mimalloc.h"

void *vm_malloc(size_t x) {
    return mi_malloc(x);
}

void *vm_calloc(size_t x) {
    return mi_calloc(x, 1);
}

void *vm_realloc(void *x, size_t y) {
    return mi_realloc(x, y);
}

void vm_free(const void *x) {
    mi_free((void *) (x));
}

char *vm_strdup(const char *x) {
    return mi_strdup(x);
}
#endif

#if VM_MALLOC_SYS
#include <stdlib.h>

void *vm_malloc(size_t x) {
    return malloc(x);
}

void *vm_calloc(size_t x) {
    return calloc(x, 1);
}

void *vm_realloc(void *x, size_t y) {
    return realloc(x, y);
}

void vm_free(const void *x) {
    free((void *) (x));
}

char *vm_strdup(const char *x) {
    return strdup(x);
}
#endif