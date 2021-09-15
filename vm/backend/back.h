#pragma once

#include <vm/vm.h>
#include <vm/libc.h>
#include <vm/obj.h>

#define cur_bytecode_next(Type)                       \
	(                                                 \
		{                                             \
			Type ret = *(Type *)&basefunc[cur_index]; \
			cur_index += sizeof(Type);                \
			ret;                                      \
		})

#define read_instr (cur_bytecode_next(opcode_t))
#define read_reg (cur_bytecode_next(int))
#define read_int (cur_bytecode_next(int))
#define read_loc (cur_bytecode_next(int))
