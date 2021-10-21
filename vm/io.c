
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

void vm_print(vm_obj_t val)
{
	if (vm_obj_is_num(val))
	{
		vm_number_t num = vm_obj_to_num(val);
		vm_putn(num);
	}
	else if (vm_obj_is_ptr(val))
	{
		bool first = true;
		vm_putchar('[');
		size_t len = vm_gc_sizeof(vm_obj_to_ptr(val));
		if (len >= 256)
		{
			__builtin_trap();
		}
		if (len != 0)
		{
			while (true)
			{
				for (int i = 0; i < len - 1; i++)
				{
					if (i != 0)
					{
						vm_putchar(',');
						vm_putchar(' ');
					}
					vm_print(vm_gc_get_index(vm_obj_to_ptr(val), i));
				}
				vm_obj_t cur = vm_gc_get_index(vm_obj_to_ptr(val), len - 1);
				if (vm_obj_is_ptr(cur))
				{
					vm_putchar(';');
					val = cur;
					len = vm_gc_sizeof(vm_obj_to_ptr(val));
					if (len == 0)
					{
						break;
					}
					else
					{
						vm_putchar(' ');
					}
				}
				else
				{
					if (len != 1)
					{
						vm_putchar(',');
						vm_putchar(' ');
					}
					vm_print(cur);
					break;
				}
			}
		}
		vm_putchar(']');
	}
	else
	{
		__builtin_trap();
	}
}
