
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#if defined(VM_NO_STD)
int putchar(int a);
double fmod(double a, double b);
#elif !defined(VM_COSMO)
#include <stdio.h>
#include <math.h>
#else
#include <cosmopolitan.h>
#endif

#define VM_FRAME_BYTES (sizeof(vm_stack_frame_t) * (1 << 16))
#define VM_LOCALS_BYTES (sizeof(vm_obj_t) * ((1 << 16) * (1 << 4)))

#if defined(VM_GC_NO_MALLOC)
#define VM_MEM_UNITS_LOG2 ((24))
#else
#define VM_MEM_UNITS_LOG2 ((22))
void *calloc(size_t n, size_t size);
void free(void *ptr);
#endif
#define VM_MEM_UNITS ((1 << VM_MEM_UNITS_LOG2))

void *vm_mem_grow(size_t size);
void vm_mem_reset(void);

#define vm_putchar(chr) (putchar(chr))
#define vm_fmod(lhs, rhs) (fmod(lhs, rhs))