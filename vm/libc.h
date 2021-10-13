
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define VM_FRAMES_UNITS (1 << 16)
#define VM_LOCALS_UNITS (VM_FRAMES_UNITS * 16)

#define VM_MEM_UNITS (1 << 21)

int putchar(int chr);
double fmod(double a, double b);

void *vm_mem_grow(size_t size);
void vm_mem_reset(void);

#define vm_putchar(chr) (putchar(chr))
#define vm_fmod(lhs, rhs) (fmod(lhs, rhs))