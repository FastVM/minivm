
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct FILE;
typedef struct FILE FILE;

void *malloc(size_t n);
void *calloc(size_t n1, size_t n2);
void *realloc(void *ptr, size_t n);
void free(void *ptr);

FILE *fopen(const char *file, const char *mode);
int fclose(FILE *file);
size_t fwrite(const void *ptr, size_t size, size_t n, FILE *file);
size_t fread(void *ptr, size_t size, size_t n, FILE *file);
double fmod(double lhs, double rhs);

int printf(const char *fmt, ...);

static inline size_t vm_strlen(const char *str)
{
  size_t len = 0;
  while (str[len] != '\0')
  {
    len += 1;
  }
  return len;
}

static inline int vm_streq(const char *str1, const char *str2)
{
  for (;;)
  {
    if (*str1 != *str2)
    {
      return 0;
    }
    if (*str1 == '\0')
    {
      return 1;
    }
    str1 += 1;
    str2 += 1;
  }
}

#define vm_malloc(size) (malloc(size))
#define vm_alloc0(size) (calloc(size, 1))
#define vm_realloc(ptr, size) (realloc(ptr, size))
#define vm_free(ptr) (free(ptr))
