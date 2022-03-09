
#pragma once

// MiniVM needs these three at all times
typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;

typedef __UINT8_TYPE__ uint8_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;

typedef __INT8_TYPE__ int8_t;
typedef __INT16_TYPE__ int16_t;
typedef __INT32_TYPE__ int32_t;
typedef __INT64_TYPE__ int64_t;

// I define libc things myself, this massivly speeds up compilation
struct FILE;
typedef struct FILE FILE;

#if defined(VM_IMPL_STRLEN)
static inline size_t strlen(const char *str) {
  size_t len = 0;
  while (str[len] != '\0') {
    len += 1;
  }
  return len;
}
#else
size_t strlen(const char *str);
#endif

void *malloc(size_t size);
void *realloc(void *ptr, size_t n);
void free(void *ptr);
#define vm_malloc(size) (malloc(size))
#define vm_realloc(ptr, size) (realloc(ptr, size))
#define vm_free(ptr) (free(ptr))

int printf(const char *src, ...);
FILE *fopen(const char *src, const char *name);
int fclose(FILE *);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
