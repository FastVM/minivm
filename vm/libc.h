#pragma once

typedef __SIZE_TYPE__ size_t;
typedef __INT8_TYPE__ int8_t;

typedef __UINT8_TYPE__ uint8_t;
typedef __INT16_TYPE__ int16_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __INT32_TYPE__ int32_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __INT64_TYPE__ int64_t;
typedef __UINT64_TYPE__ uint64_t;

typedef _Bool bool;
#define true ((bool)1)
#define false ((bool)0)

#define NULL ((void *)0)

#if defined(VM_EMCC)
#include <emscripten.h>
char *emscripten_run_script_string(const char *script);
size_t strlen(const char *src);

int printf(const char *fmt, ...);
#define vm_putchar(chr_)                                                       \
  ({                                                                           \
    char chr = chr_;                                                           \
    printf("%c", (chr));                                                       \
  })
#else
struct FILE;
typedef struct FILE FILE;

#if defined(VM_USE_FP)
double fmod(double lhs, double rhs);
#else
#define fmod(lhs, rhs) ((lhs) % (rhs))
#endif
int putchar(int chr);
#define vm_putchar(chr) (putchar((chr)))
#endif

void *malloc(size_t size);
void *realloc(void *ptr, size_t n);
void free(void *ptr);
#define vm_malloc(size) (malloc((size)))
#define vm_realloc(ptr, size) (realloc((ptr), (size)))
#define vm_free(ptr) (free((ptr)))

FILE *fopen(const char *src, const char *name);
int fclose(FILE *);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

#if defined(VM_EMCC)
#define VM_API EMSCRIPTEN_KEEPALIVE
#else
#define VM_API
#endif
