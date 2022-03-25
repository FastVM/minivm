
#pragma once

#if defined(__SIZE_TYPE__)
typedef __SIZE_TYPE__ size_t;
#else
#include <stddef.h>
#endif

#if defined(__UINT8_TYPE__)
typedef __UINT8_TYPE__ uint8_t;
#else
#include <stdint.h>
#endif

#if defined(__UINT16_TYPE__)
typedef __UINT16_TYPE__ uint16_t;
#else
#include <stdint.h>
#endif

#define NULL ((void *)0)

// I define libc things myself, this massivly speeds up compilation
struct FILE;
typedef struct FILE FILE;

static inline size_t vm_strlen(const char *str) {
  size_t len = 0;
  while (str[len] != '\0') {
    len += 1;
  }
  return len;
}

static inline int vm_streq(const char *str1, const char *str2) {
  for (;;) {
    if (*str1 != *str2) {
      return 0;
    }
    if (*str1 == '\0') {
      return 1;
    }
    str1 += 1;
    str2 += 1;
  }
}

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t n);
void free(void *ptr);
#define vm_malloc(size) (malloc(size))
#define vm_calloc(size) (calloc(1, size))
#define vm_realloc(ptr, size) (realloc(ptr, size))
#define vm_free(ptr) (free(ptr))

int printf(const char *src, ...);
FILE *fopen(const char *src, const char *name);
int fclose(FILE *);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
