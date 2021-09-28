#include <vm/libc.h>

#include <vm/libc.h>
#include <vm/vm.h>

typedef void vm_putchar_func_t(int chr);

int vm_xhead = 0;
opcode_t vm_xprogram[1 << 16];
int vm_xhp = 0;
uint8_t vm_xh[(1 << 30)];
int vm_use_the_putchar = 0;

typedef struct
{
	void *ptr;
	size_t len;
} vm_iovs_t;

__attribute__((import_module("wasi_unstable"))) size_t fd_write(size_t fd, vm_iovs_t *iovs, size_t len, size_t *nout);
__attribute__((import_module("wasi_unstable"))) size_t fd_read(size_t fd, vm_iovs_t *iovs, size_t len, size_t *nout);

int putchar(int i)
{
	if (vm_use_the_putchar)
	{
		fd_write(i, NULL, 0, NULL);
	}
	else
	{
		char c = i;
		vm_iovs_t iovs;
		iovs.ptr = &c;
		iovs.len = 1;
		size_t out;
		fd_write(1, &iovs, 1, &out);
	}
	return 0;
}

void *memset(void *a, int b, size_t c)
{
	uint8_t *o = a;
	while (c > 0)
	{
		*o = c;
		o++;
		c--;
	}
	return a;
}

double fmod(double a, double b)
{
	return (double)((long)a % (long)b);
}

void vm_xadd(opcode_t o)
{
	vm_xprogram[vm_xhead++] = o;
}

void vm_xrun(void)
{
	vm_xhp = 0;
	vm_xhead = 0;
	vm_run_no_xinstrs(vm_xprogram);
}

void *malloc(size_t size)
{
	uint8_t *ret = &vm_xh[vm_xhp];
	vm_xhp += size;
	return ret;
}

void vm_xset_putchar()
{
	vm_use_the_putchar = 1;
}

void free(void *ptr) {}

void _start()
{
	while (1)
	{
		opcode_t op;
		vm_iovs_t iovs;
		iovs.ptr = &op;
		iovs.len = 1;
		size_t out;
		fd_read(0, &iovs, 1, &out);
		if (out < 1)
		{
			break;
		}
		vm_xadd(op);
	}
	vm_xrun();
}