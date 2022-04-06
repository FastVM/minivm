
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>

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

#define vm_malloc(size) (malloc(size))
#define vm_realloc(ptr, size) (realloc(ptr, size))
#define vm_free(ptr) (free(ptr))
