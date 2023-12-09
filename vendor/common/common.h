#pragma once
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Cuik currently uses mimalloc so we wrap those calls here
#ifdef CUIK_USE_MIMALLOC
#include <mimalloc.h>

#define cuik_malloc(size)        mi_malloc(size)
#define cuik_calloc(count, size) mi_calloc(count, size)
#define cuik_free(ptr)           mi_free(ptr)
#define cuik_realloc(ptr, size)  mi_realloc(ptr, size)
#define cuik_strdup(x)           mi_strdup(x)
#else
#define cuik_malloc(size)        malloc(size)
#define cuik_calloc(count, size) calloc(count, size)
#define cuik_free(size)          free(size)
#define cuik_realloc(ptr, size)  realloc(ptr, size)

#ifdef _WIN32
#define cuik_strdup(x)           _strdup(x)
#else
#define cuik_strdup(x)           strdup(x)
#endif
#endif

#if defined(__amd64) || defined(__amd64__) || defined(_M_AMD64) || defined(__x86_64__) || defined(__x86_64)
#define CUIK__IS_X64 1
#elif defined(__aarch64__)
#define CUIK__IS_AARCH64 1
#endif

// Cuik doesn't have SIMD intrinsics yet... sadge
#if defined(__CUIK__)
#define USE_INTRIN 0
#else
#define USE_INTRIN 1
#endif

#define STR2(x) #x
#define STR(x) STR2(x)

#ifndef COUNTOF
#define COUNTOF(...) (sizeof(__VA_ARGS__) / sizeof(__VA_ARGS__[0]))
#endif

#ifdef NDEBUG
#define ASSUME(x) ((x) ? 0 : __builtin_unreachable())
#else
#define ASSUME(x) ((x) ? 0 : (fprintf(stderr, __FILE__ ": " STR(__LINE__) ": bad assumption: " #x "\n")))
#endif

#define LIKELY(x)      __builtin_expect(!!(x), 1)
#define UNLIKELY(x)    __builtin_expect(!!(x), 0)

#ifdef NDEBUG
#define TODO() __builtin_unreachable()
#else
#define TODO() (assert(0 && "TODO"), __builtin_unreachable())
#endif

// just because we use a threads fallback layer which can include windows
// and such which is annoying... eventually need to modify that out or something
#ifndef thread_local
#define thread_local _Thread_local
#endif

#define SWAP(T, a, b) \
do {                  \
    T temp = a;       \
    a = b;            \
    b = temp;         \
} while (0)

void  cuik_init_terminal(void);

void  tls_init(void);
void  tls_reset(void);
void* tls_push(size_t size);
void* tls_pop(size_t size);
void* tls_save(void);
void  tls_restore(void* p);

void* cuik__valloc(size_t sz);
void  cuik__vfree(void* p, size_t sz);
