
#include "vm.h"
#include "lib.h"
#include <assert.h>

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
    if (x == 0) {
        return NULL;
    }
    void *ret = malloc(x);
    assert(ret != NULL);
    return ret;
}

void *vm_calloc(size_t x) {
    if (x == 0) {
        return NULL;
    }
    void *ret = calloc(x, 1);
    assert(ret != NULL);
    return ret;
}

void *vm_realloc(void *x, size_t y) {
    if (y == 0) {
        vm_free(x);
    }
    void *ret = realloc(x, y);
    assert(ret != NULL);
    return ret;
}

void vm_free(const void *x) {
    if (x != NULL) {
        free((void *) (x));
    }
}

char *vm_strdup(const char *x) {
    assert(x != NULL);
    char *ret = strdup(x);
    assert(ret != NULL);
    return ret;
}
#endif