
#if !defined(VM_HEADER_LIB)
#define VM_HEADER_LIB

#if defined(_WIN32)
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <stdlib.h>

void *vm_malloc(size_t size);
void *vm_calloc(size_t size);
void *vm_realloc(void *ptr, size_t size);
void vm_free(const void *ptr);
char *vm_strdup(const char *str);


#endif
