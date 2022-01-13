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

struct FILE;
typedef struct FILE FILE;

#define pow(lhs_arg_, rhs_arg_)                                                \
  ({                                                                           \
    int32_t lhs_ = lhs_arg_;                                                   \
    uint16_t rhs_ = rhs_arg_;                                                  \
    int res_ = 1;                                                              \
    while (rhs_ > 0) {                                                         \
      if (rhs_ % 2 == 1) {                                                     \
        res_ *= lhs_;                                                          \
      }                                                                        \
      rhs_ /= 2;                                                               \
      lhs_ *= lhs_;                                                            \
    }                                                                          \
    res_;                                                                      \
  })
#define fmod(lhs_, rhs_) ((lhs_) % (rhs_))

int putchar(int chr);
#define vm_putchar(chr_) (putchar((chr_)))

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

#define VM_API
