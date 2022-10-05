
#if !defined(VM_HEADER_LIB)
#define VM_HEADER_LIB

#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__MINIVM__)
const char *__minivm__ferror = NULL;
#define __attribute__(...) 
#define VM_SPALL_LEVEL 1
#define fflush(...) (0)
#define fgetc(...) (getchar())
#define ferror(...) (__minivm__ferror)
#define fseek(...) (0)
#endif

#if defined(VM_WASM)

struct FILE;
typedef struct FILE FILE;
typedef double time_t;

FILE *vm_stdin(void);
FILE *vm_stdout(void);
FILE *vm_stderr(void);
int vm_errno(void);

FILE *fopen(const char *path, const char *mode);
void fclose(FILE *file);
size_t fread(void *ptr, size_t size, size_t n, FILE *file);
size_t fwrite(const void *ptr, size_t size, size_t n, FILE *file);

int fprintf(FILE *file, const char *fmt, ...);
int printf(const char *fmt, ...);

int feof(FILE *file);
void exit(int code);

size_t strlen(const char *str);

int putchar(int c);
int fgetc(FILE *file);

int strcmp(const char *lhs, const char *rhs);
int strncmp(const char *lhs, const char *rhs, size_t n);
char *strdup(const char *lhs);
int strcasecmp(const char *lhs, const char *rhs);
int strncasecmp(const char *lhs, const char *rhs, size_t n);

void *malloc(size_t size);
void *calloc(size_t a, size_t b);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

char *dirname(const char *name);

int isdigit(int chr);
int isxdigit(int chr);
int isalpha(int chr);
int isalnum(int chr);
int isprint(int chr);
int atoi(const char *str);
unsigned long strtoul(const char *str, char **end_out, int base);
double strtod(const char *str, char **end_out);

char *strerror(int errno);
char *strchr(const char *str, int i);

void *memcpy(void *dest, const void *src, size_t n);

char *strpbrk(const char *str1, const char *str2);

int vsnprintf(char *s, size_t n, const char *format, va_list arg);
int vfprintf(FILE *F, const char *format, va_list arg);

const char *getcwd(char *cwd, size_t path_max);

#define stdin vm_stdin()
#define stdout vm_stderr()
#define stderr vm_stderr()

#define errno vm_errno()

#define assert(x) ((void)(x))

#define EOF ((char)-1)
#define PATH_MAX 2048

#else

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#endif

#if defined(VM_XGC)

void *vm_malloc(size_t size);
void *vm_alloc0(size_t size);
void *vm_realloc(void *ptr, size_t size);
void vm_free(void *ptr);

#elif defined(VM_MIMALLOC)

#include <mimalloc.h>
#define vm_malloc(size) (mi_malloc(size))
#define vm_alloc0(size) (mi_calloc(size, 1))
#define vm_realloc(ptr, size) (mi_realloc(ptr, size))
#define vm_free(ptr) (mi_free((void *)ptr))

#else

#define vm_malloc(size) (malloc(size))
#define vm_alloc0(size) (calloc(size, 1))
#define vm_realloc(ptr, size) (realloc(ptr, size))
#define vm_free(ptr) (free((void *)ptr))

#endif

#include "config.h"

#endif
