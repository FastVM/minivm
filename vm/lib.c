
#include "lib.h"

#if defined(__TINYC__)
void *end;
#endif

void *dasm_mem_realloc(void *ptr, size_t size) {
    return vm_realloc(ptr, size);
}

void dasm_mem_free(void *ptr) {
    vm_free(ptr);
}
