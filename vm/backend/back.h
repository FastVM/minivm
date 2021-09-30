#pragma once

#include <vm/vm.h>
#include <vm/libc.h>
#include <vm/obj.h>
#include <stdarg.h>

#define SPACES "    "

static inline void vm_internal_bufout(int *buflen, int *bufalloc, char **bufptr, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);
	if (*buflen + n + 4 > *bufalloc)
	{
		*bufalloc = (*buflen + n + 4) * 2;
		*bufptr = realloc(*bufptr, *bufalloc + 1);
	}
	va_start(ap, fmt);
	n = vsnprintf(*bufptr + *buflen, *bufalloc - *buflen, fmt, ap);
	va_end(ap);
	*buflen += n;
}

#define BUFOUT(fmt, ...) (vm_internal_bufout(&buflen, &bufalloc, &bufptr, fmt, __VA_ARGS__))

#define OUTLN(fmt, ...)             \
	for (int i = 0; i < depth; i++) \
		BUFOUT("%s", SPACES);       \
	BUFOUT(fmt "\n", __VA_ARGS__)

#define OUT(fmt, ...) (BUFOUT(fmt, __VA_ARGS__))

#define PUTLN(fmt)                  \
	for (int i = 0; i < depth; i++) \
		BUFOUT("%s", SPACES);       \
	BUFOUT("%s\n", fmt)

#define PUT(fmt) (BUFOUT("%s", fmt))

#define cur_bytecode_next(Type)                       \
	(                                                 \
		{                                             \
			Type ret = *(Type *)&basefunc[cur_index]; \
			cur_index += sizeof(Type);                \
			ret;                                      \
		})

#define read_instr (cur_bytecode_next(opcode_t))
#define read_byte (cur_bytecode_next(unsigned char))
#define read_reg (cur_bytecode_next(unsigned char))
#define read_int (cur_bytecode_next(int))
#define read_loc (cur_bytecode_next(int))
