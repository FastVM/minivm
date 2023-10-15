#pragma once
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Windows.h>
#endif

#include "../bdwgc/private/gc/gc.h"

// Cuik currently uses mimalloc so we wrap those calls here
#define cuik_malloc(size)        GC_malloc(size)
#define cuik_calloc(count, size) GC_malloc((count) * (size))
#define cuik_free(ptr)           GC_free(ptr)
#define cuik_realloc(ptr, size)  GC_realloc(ptr, size)
#define cuik_strdup(x)           GC_strdup(x)

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

#ifndef _MSC_VER
#include <signal.h>
#define __debugbreak() raise(5 /* SIGTRAP */)
#endif

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
