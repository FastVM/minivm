
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void free(void *ptr);
void *calloc(size_t one, size_t size);

#define vm_mem_free(ptr) (free(ptr))
#define vm_mem_alloc(len) (calloc(1, len))

#if defined(__linux__)
long syscall(long number, ...);
#define vm_putchar(chr) (         \
    {                             \
        char local = chr;         \
        syscall(1, 1, &local, 1); \
        (void)0;                  \
    })
#else
void putchar(int chr);
#define vm_putchar(chr) (putchar(chr));
#endif

#if defined(__GNUC__) || defined(__clang__)
#define fmod(lhs, rhs) (__builtin_fmod(lhs, rhs))
#else
double fmod(double lhs, double rhs);
#endif