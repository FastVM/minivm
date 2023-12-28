
#include "lib.h"

#if defined(__TINYC__)
void *end;
#endif

#if defined(_WIN32)
void vm_hack_chkstk(void) {}
#endif

#if 1
void GC_init() {
}

void *GC_malloc(size_t size) {
    // void *ptr = calloc(1, size);
    return calloc(1, size);
    // printf("%p: %zu\n", ptr, size);
    // return ptr;
}

void *GC_realloc(void *ptr, size_t size) {
    return realloc(ptr, size);
}

void GC_free(void *ptr) {
    free(ptr);
}
#endif

char *vm_strdup(const char *str) {
    if (str == NULL) {
        return NULL;
    }
    int len = strlen(str);
    char *buf = vm_malloc(sizeof(char) * (len + 1));
    memcpy(buf, str, len + 1);
    return buf;
}
