#pragma once


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

struct FILE;
typedef struct FILE FILE;

FILE *fopen(const char *src, const char *name);
int fclose(FILE *);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

#if defined(VM_DEBUG_OPCODE)
int printf(const char *fmt, ...);
#endif

#define VM_FRAMES_UNITS (1 << 12)
#define VM_LOCALS_UNITS (VM_FRAMES_UNITS * 16)

#if !defined(NULL)
#define NULL ((void*)0)
#endif

int putchar(int chr);
double fmod(double a, double b);

#if defined(VM_DEBUG_PUTCHAR)
int printf(const char *fmt, ...);
#define vm_putchar(chr) (printf("[%hhu]\n", chr))
#else
#define vm_putchar(chr) (putchar(chr))
#endif
#define vm_fmod(lhs, rhs) (fmod(lhs, rhs))

#if defined(VM_USE_MIMALLOC)
void *mi_malloc(size_t size);
void *mi_calloc(size_t n, size_t size);
void *mi_realloc(void *ptr, size_t n);
void mi_free(void *ptr);
#define vm_malloc(size) (mi_malloc((size)))
#define vm_calloc(size) (mi_calloc((1),(size)))
#define vm_realloc(ptr, size) (mi_realloc((ptr),(size)))
#define vm_free(ptr) (mi_free((ptr)))
#else
void *malloc(size_t size);
void *calloc(size_t n, size_t size);
void *realloc(void *ptr, size_t n);
void free(void *ptr);
#define vm_malloc(size) (malloc((size)))
#define vm_calloc(size) (calloc((1),(size)))
#define vm_realloc(ptr, size) (realloc((ptr),(size)))
#define vm_free(ptr) (free((ptr)))
#endif