#include <vm/backend/back.h>

char *vm_backend_lua(opcode_t *basefunc)
{
	int buflen = 0;
	int bufalloc = 16;
	char *bufptr = vm_mem_alloc(bufalloc);
	int n = 0;
	int cur_index = 0;
	int depth = 0;
	int *nregs = alloca(sizeof(int) * 256);
	int *rec = alloca(sizeof(int) * 256);
	int *freg = alloca(sizeof(int) * 256);
	int *base = rec;
	*rec = 0;
	*nregs = 256;
	PUTLN("local funcs = {}");
	while (true)
	{
		OUTLN("::op%i::", cur_index);
	nocase:;
		opcode_t op = read_instr;
		switch (op)
		{
		case OPCODE_STORE_REG:
		{
			reg_t reg = read_reg;
			reg_t from = read_reg;
			OUTLN("reg%i = reg%i", reg, from);
			break;
		}
		case OPCODE_STORE_INT:
		{
			reg_t reg = read_reg;
			int n = read_int;
			OUTLN("reg%i = %i", reg, n);
			break;
		}
		case OPCODE_STORE_FUN:
		{
			reg_t to = read_reg;
			int end = read_int;
			if (rec - base > 250)
			{
				fprintf(stderr, "too much nesting\n");
				free(bufptr);
				return NULL;
			}
			*(++rec) = cur_index;
			OUTLN("::op%i::", cur_index);
			PUTLN("do");
			depth++;
			*(++freg) = to;
			PUTLN("local function rec(reg0, reg1, reg2, reg3, reg4, reg5, reg6, reg7)");
			int cnregs = read_int;
			depth++;
			*(++nregs) = cnregs;
			for (int i = 8; i < cnregs; i++)
			{
				OUTLN("local reg%i", i);
			}
			goto nocase;
		}
		case OPCODE_FUN_DONE:
		{
			depth--;
			PUTLN("end");
			OUTLN("reg%i = rec", *freg);
			OUTLN("funcs[%i] = rec", *rec);
			depth--;
			PUTLN("end");
			freg--;
			rec--;
			nregs--;
			break;
		}
		case OPCODE_RETURN:
		{
			reg_t reg = read_reg;

			OUTLN("do return reg%i end", reg);

			break;
		}
		case OPCODE_CALL0:
		{
			reg_t outreg = read_reg;
			reg_t func = read_reg;
			OUTLN("if type(reg%i) ~= 'function' then", func);
			depth++;
			OUTLN("reg%i = reg%i[1](reg%i)", outreg, func, func);
			depth--;
			PUTLN("else");
			depth++;
			OUTLN("reg%i = reg%i()", outreg, func);
			depth--;
			PUTLN("end");
			break;
		}
		case OPCODE_CALL1:
		{
			reg_t outreg = read_reg;
			reg_t func = read_loc;
			reg_t r1arg = read_reg;
			OUTLN("if type(reg%i) ~= 'function' then", func);
			depth++;
			OUTLN("reg%i = reg%i[1](reg%i, reg%i)", outreg, func, r1arg, func);
			depth--;
			PUTLN("else");
			depth++;
			OUTLN("reg%i = reg%i(reg%i)", outreg, func, r1arg);
			depth--;
			PUTLN("end");
			break;
		}
		case OPCODE_CALL2:
		{
			reg_t outreg = read_reg;
			reg_t func = read_loc;
			reg_t r1arg = read_reg;
			reg_t r2arg = read_reg;
			OUTLN("if type(reg%i) ~= 'function' then", func);
			depth++;
			OUTLN("reg%i = reg%i[1](reg%i, reg%i, reg%i)", outreg, func, r1arg, r2arg, func);
			depth--;
			PUTLN("else");
			depth++;
			OUTLN("reg%i = reg%i(reg%i, reg%i)", outreg, func, r1arg, r2arg);
			depth--;
			PUTLN("end");
			break;
		}
		case OPCODE_CALL:
		{
			reg_t outreg = read_reg;
			reg_t func = read_loc;
			int nargs = read_int;

			for (int i = 0; i < nargs; i++)
			{
				if (i != 0)
				{
				}
			}

			break;
		}
		case OPCODE_STATIC_CALL0:
		{
			reg_t outreg = read_reg;
			int next_func = read_loc;
			OUTLN("reg%i = funcs[%i]()", outreg, next_func);
			break;
		}
		case OPCODE_STATIC_CALL1:
		{
			reg_t outreg = read_reg;
			int next_func = read_loc;
			reg_t r1arg = read_reg;
			OUTLN("reg%i = funcs[%i](reg%i)", outreg, next_func, r1arg);
			break;
		}
		case OPCODE_STATIC_CALL2:
		{
			reg_t outreg = read_reg;
			int next_func = read_loc;
			reg_t r1arg = read_reg;
			reg_t r2arg = read_reg;
			OUTLN("reg%i = funcs[%i](reg%i, reg%i)", outreg, next_func, r1arg, r2arg);
			break;
		}
		case OPCODE_STATIC_CALL:
		{
			reg_t outreg = read_reg;
			int next_func = read_loc;
			int nargs = read_int;

			for (int i = 0; i < nargs; i++)
			{
				if (i != 0)
				{
				}
			}

			break;
		}
		case OPCODE_REC0:
		{
			reg_t outreg = read_reg;
			OUTLN("reg%i = rec()", outreg);
			break;
		}
		case OPCODE_REC1:
		{
			reg_t outreg = read_reg;
			reg_t r1arg = read_reg;
			OUTLN("reg%i = rec(reg%i)", outreg, r1arg);
			break;
		}
		case OPCODE_REC2:
		{
			reg_t outreg = read_reg;
			reg_t r1arg = read_reg;
			reg_t r2arg = read_reg;
			OUTLN("reg%i = rec(reg%i, reg%i)", outreg, r1arg, r2arg);
			break;
		}
		case OPCODE_REC:
		{
			reg_t outreg = read_reg;
			int nargs = read_int;

			for (int i = 0; i < nargs; i++)
			{
				if (i != 0)
				{
				}
			}

			break;
		}
		case OPCODE_EQUAL:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("if reg%i == reg%i then reg%i = 1 else reg%i = 0 end", lhs, rhs, to, to);
			break;
		}
		case OPCODE_EQUAL_NUM:
		{
			reg_t to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUTLN("if reg%i == %i then reg%i = 1 else reg%i = 0 end", lhs, rhs, to, to);
			break;
		}
		case OPCODE_NOT_EQUAL:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("if reg%i ~= reg%i then reg%i = 1 else reg%i = 0 end", lhs, rhs, to, to);
			break;
		}
		case OPCODE_NOT_EQUAL_NUM:
		{
			reg_t to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUTLN("if reg%i ~= %i then reg%i = 1 else reg%i = 0 end", lhs, rhs, to, to);
			break;
		}
		case OPCODE_LESS:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("if reg%i < reg%i then reg%i = 1 else reg%i = 0 end", lhs, rhs, to, to);
			break;
		}
		case OPCODE_LESS_NUM:
		{
			reg_t to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUTLN("if reg%i < %i then reg%i = 1 else reg%i = 0 end", lhs, rhs, to, to);
			break;
		}
		case OPCODE_GREATER:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("if reg%i > reg%i then reg%i = 1 else reg%i = 0 end", lhs, rhs, to, to);
			break;
		}
		case OPCODE_GREATER_NUM:
		{
			reg_t to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUTLN("if reg%i > %i then reg%i = 1 else reg%i = 0 end", lhs, rhs, to, to);
			break;
		}
		case OPCODE_LESS_THAN_EQUAL:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("if reg%i <= reg%i then reg%i = 1 else reg%i = 0 end", lhs, rhs, to, to);
			break;
		}
		case OPCODE_LESS_THAN_EQUAL_NUM:
		{
			reg_t to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUTLN("if reg%i <= %i then reg%i = 1 else reg%i = 0 end", lhs, rhs, to, to);
			break;
		}
		case OPCODE_GREATER_THAN_EQUAL:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("if reg%i >= reg%i then reg%i = 1 else reg%i = 0 end", lhs, rhs, to, to);
			break;
		}
		case OPCODE_GREATER_THAN_EQUAL_NUM:
		{
			reg_t to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUTLN("if reg%i >= %i then reg%i = 1 else reg%i = 0 end", lhs, rhs, to, to);
			break;
		}
		case OPCODE_JUMP_ALWAYS:
		{
			int loc = read_loc;
			OUTLN("goto op%i", loc);
			break;
		}
		case OPCODE_JUMP_IF_FALSE:
		{
			int loc = read_loc;
			reg_t reg = read_reg;
			OUTLN("if reg%i == 0 then goto op%i end", reg, loc);
			break;
		}
		case OPCODE_JUMP_IF_TRUE:
		{
			int loc = read_loc;
			reg_t reg = read_reg;
			OUTLN("if reg%i ~= 0 then goto op%i end", reg, loc);
			break;
		}
		case OPCODE_JUMP_IF_EQUAL:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("if reg%i == reg%i then goto op%i end", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_EQUAL_NUM:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUTLN("if reg%i == %i then goto op%i end", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_NOT_EQUAL:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("if reg%i ~= reg%i then goto op%i end", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_NOT_EQUAL_NUM:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUTLN("if reg%i ~= %i then goto op%i end", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_LESS:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("if reg%i < reg%i then goto op%i end", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_LESS_NUM:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUTLN("if reg%i < %i then goto op%i end", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_GREATER:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("if reg%i > reg%i then goto op%i end", lhs, rhs, loc);

			break;
		}
		case OPCODE_JUMP_IF_GREATER_NUM:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUTLN("if reg%i > %i then goto op%i end", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_LESS_THAN_EQUAL:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("if reg%i <= reg%i then goto op%i end", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_LESS_THAN_EQUAL_NUM:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUTLN("if reg%i <= %i then goto op%i end", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_GREATER_THAN_EQUAL:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("if reg%i >= reg%i then goto op%i end", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_GREATER_THAN_EQUAL_NUM:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUTLN("if reg%i >= %i then goto op%i end", lhs, rhs, loc);
			break;
		}
		case OPCODE_INC:
		{
			reg_t reg = read_reg;
			reg_t from = read_reg;
			OUTLN("reg%i = reg%i + reg%i", reg, reg, from);
			break;
		}
		case OPCODE_INC_NUM:
		{
			reg_t reg = read_reg;
			int n = read_int;
			OUTLN("reg%i = reg%i + %i", reg, reg, n);
			break;
		}
		case OPCODE_DEC:
		{
			reg_t reg = read_reg;
			reg_t from = read_reg;
			OUTLN("reg%i = reg%i - reg%i", reg, reg, from);
			break;
		}
		case OPCODE_DEC_NUM:
		{
			reg_t reg = read_reg;
			int n = read_int;
			OUTLN("reg%i = reg%i - %i", reg, reg, n);
			break;
		}
		case OPCODE_ADD:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("reg%i = reg%i + reg%i", reg, lhs, rhs);
			break;
		}
		case OPCODE_ADD_NUM:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("reg%i = reg%i + %i", reg, lhs, rhs);
			break;
		}
		case OPCODE_SUB:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("reg%i = reg%i - reg%i", reg, lhs, rhs);
			break;
		}
		case OPCODE_SUB_NUM:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("reg%i = reg%i - %i", reg, lhs, rhs);
			break;
		}
		case OPCODE_MUL:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("reg%i = reg%i * reg%i", reg, lhs, rhs);
			break;
		}
		case OPCODE_MUL_NUM:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("reg%i = reg%i * %i", reg, lhs, rhs);
			break;
		}
		case OPCODE_DIV:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("reg%i = reg%i / reg%i", reg, lhs, rhs);
			break;
		}
		case OPCODE_DIV_NUM:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("reg%i = reg%i / %i", reg, lhs, rhs);
			break;
		}
		case OPCODE_MOD:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("reg%i = reg%i %% reg%i", reg, lhs, rhs);
			break;
		}
		case OPCODE_MOD_NUM:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUTLN("reg%i = reg%i %% %i", reg, lhs, rhs);
			break;
		}
		case OPCODE_ARRAY:
		{
			reg_t outreg = read_reg;
			int nargs = read_int;
			OUTLN("reg%i = {", outreg);
			depth++;
			for (int i = 0; i < nargs; i++)
			{
				reg_t reg = read_reg;
				OUTLN("reg%i,", reg);
			}
			depth--;
			PUTLN("}");
			break;
		}
		case OPCODE_LENGTH:
		{
			reg_t outreg = read_reg;
			reg_t reg = read_reg;
			OUTLN("reg%i = #reg%i", outreg, reg);
			break;
		}
		case OPCODE_INDEX:
		{
			reg_t outreg = read_reg;
			reg_t reg = read_reg;
			reg_t ind = read_reg;
			OUTLN("reg%i = reg%i[reg%i+1]", outreg, reg, ind);
			break;
		}
		case OPCODE_INDEX_NUM:
		{
			reg_t outreg = read_reg;
			reg_t reg = read_reg;
			int index = read_reg;
			OUTLN("reg%i = reg%i[%i]", outreg, reg, index + 1);
			break;
		}
		case OPCODE_PRINTLN:
		{
			reg_t reg = read_reg;
			OUTLN("print(reg%i)", reg);
			break;
		}
		case OPCODE_PUTCHAR:
		{
			reg_t reg = read_reg;
			OUTLN("io.write(string.char(reg%i))", reg);
			break;
		}
		case OPCODE_EXIT:
		{
			goto done;
		}
		default:
		{
			fprintf(stderr, "unhandle opcode: %i\n", (int)op);
			free(bufptr);
			return NULL;
		}
		}
	}
done:
	return bufptr;
}
