
#include "io.h"
#include "gc.h"

void vm_putn(long n)
{
	if (n < 0)
	{
		vm_putchar('-');
		vm_putn(-n);
	}
	else
	{
		if (n >= 10)
		{
			vm_putn(n / 10);
		}
		vm_putchar(n % 10 + '0');
	}
}

void vm_puts(const char *ptr)
{
	while (*ptr)
	{
		vm_putchar(*ptr);
		ptr += 1;
	}
}