#include <vm/backend/back.h>

enum
{
	VM_BACKEND_JS_NODE,
	VM_BACKEND_JS_V8,
};

char * vm_backend_js(opcode_t *basefunc, const char *str)
{
	int jstype = VM_BACKEND_JS_NODE;
	if (str != NULL)
	{
		if (!strcmp(str, "node"))
		{
			jstype = VM_BACKEND_JS_NODE;
		}
		if (!strcmp(str, "v8"))
		{
			jstype = VM_BACKEND_JS_V8;
		}
	}
	int buflen = 0;
	int bufalloc = 256;
	char *bufptr = vm_mem_alloc(bufalloc);
	int cur_index = 0;
	int *nregs = alloca(sizeof(int) * 256);
	int *rec = alloca(sizeof(int) * 256);
	int *base = rec;
	*rec = 0;
	*nregs = 256;
	PUT("var o=0,r=[],a,d=0,s=[0],c=Array.isArray;b:while(1){switch(o|0){");
	while (true)
	{
		OUT("case %i:", cur_index);
	nocase:;
		opcode_t op = read_instr;
		switch (op)
		{
		case OPCODE_STORE_REG:
		{
			reg_t reg = read_reg;
			reg_t from = read_reg;
			OUT("r[%i+d]=r[%i+d];", reg, from);
			break;
		}
		case OPCODE_STORE_BYTE:
		{
			reg_t reg = read_reg;
			int n = read_byte;
			OUT("r[%i+d]=%i;", reg, n);
			break;
		}
		case OPCODE_STORE_INT:
		{
			reg_t reg = read_reg;
			int n = read_int;
			OUT("r[%i+d]=%i;", reg, n);
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
			OUT("r[%i+d]=%i;", to, cur_index);
			OUT("o=%i;continue b;", end);
			OUT("case %i:", cur_index);
			*(++nregs) = read_byte;
			goto nocase;
		}
		case OPCODE_FUN_DONE:
		{
			rec--;
			nregs--;
			break;
		}
		case OPCODE_RETURN:
		{
			reg_t reg = read_reg;
			OUT("a=r[%i+d];", reg);
			PUT("d=s.pop();");
			PUT("r[s.pop()+d]=a;");
			PUT("o=s.pop();continue b;");
			break;
		}
		case OPCODE_CALL0:
		{
			reg_t outreg = read_reg;
			reg_t func = read_reg;
			OUT("s.push(%i,%i);", cur_index, outreg);
			OUT("o=r[%i+d];", func);
			PUT("s.push(d);");
			OUT("d+=%i;if(c(o)){r[d]=o;o=o[0];}continue b;", *nregs);
			break;
		}
		case OPCODE_CALL1:
		{
			reg_t outreg = read_reg;
			reg_t func = read_loc;
			reg_t r1arg = read_reg;
			OUT("s.push(%i,%i);", cur_index, outreg);
			OUT("r[d+%i]=r[%i+d];", *nregs, r1arg);
			OUT("o=r[%i+d];", func);
			PUT("s.push(d);");
			OUT("d+=%i;if(c(o)){r[d+1]=o;o=o[0];}continue b;", *nregs);
			break;
		}
		case OPCODE_CALL2:
		{
			reg_t outreg = read_reg;
			reg_t func = read_loc;
			reg_t r1arg = read_reg;
			reg_t r2arg = read_reg;
			OUT("s.push(%i,%i);", cur_index, outreg);
			OUT("r[d+%i]=r[%i+d];", *nregs, r1arg);
			OUT("r[d+%i]=r[%i+d];", 1 + *nregs, r2arg);
			OUT("o=r[%i+d];", func);
			PUT("s.push(d);");
			OUT("d+=%i;if(c(o)){r[d+2]=o;o=o[0];}continue b;", *nregs);
			break;
		}
		case OPCODE_CALL:
		{
			reg_t outreg = read_reg;
			reg_t func = read_loc;
			int nargs = read_byte;
			OUT("s.push(%i,%i);", cur_index, outreg);
			for (int i = 0; i < nargs; i++)
			{
				if (i != 0)
				{
					PUT(",");
				}
				int argreg = read_reg;
				OUT("r[d+%i]=r[d+%i];", i + *nregs, argreg);
			}
			OUT("o=r[%i+d];", func);
			PUT("s.push(d);");
			OUT("d+=%i;if(c(o)){r[d+%i]=o;o=o[0];}continue b;", *nregs, nargs);
			break;
		}
		case OPCODE_STATIC_CALL0:
		{
			reg_t outreg = read_reg;
			int next_func = read_loc;
			OUT("s.push(%i,%i);", cur_index, outreg);
			OUT("s.push(d);d+=%i;", *nregs);
			OUT("o=%i;continue b;", next_func);
			break;
		}
		case OPCODE_STATIC_CALL1:
		{
			reg_t outreg = read_reg;
			int next_func = read_loc;
			reg_t r1arg = read_reg;
			OUT("s.push(%i,%i);", cur_index, outreg);
			OUT("r[d+%i]=r[%i+d];", *nregs, r1arg);
			OUT("s.push(d);d+=%i;", *nregs);
			OUT("o=%i;continue b;", next_func);
			break;
		}
		case OPCODE_STATIC_CALL2:
		{
			reg_t outreg = read_reg;
			int next_func = read_loc;
			reg_t r1arg = read_reg;
			reg_t r2arg = read_reg;
			OUT("s.push(%i,%i);", cur_index, outreg);
			OUT("r[%i+d]=r[%i+d];", *nregs, r1arg);
			OUT("r[%i+d]=r[%i+d];", 1 + *nregs, r2arg);
			OUT("s.push(d);d+=%i;", *nregs);
			OUT("o=%i;continue b;", next_func);
			break;
		}
		case OPCODE_STATIC_CALL:
		{
			reg_t outreg = read_reg;
			int next_func = read_loc;
			int nargs = read_byte;
			OUT("s.push(%i,%i);", cur_index, outreg);
			for (int i = 0; i < nargs; i++)
			{
				if (i != 0)
				{
					PUT(",");
				}
				int argreg = read_reg;
				OUT("r[d+%i]=r[d+%i];", i + *nregs, argreg);
			}
			OUT("s.push(d);d+=%i;", *nregs);
			OUT("o=%i;continue b;", next_func);
			break;
		}
		case OPCODE_REC0:
		{
			reg_t outreg = read_reg;
			OUT("s.push(%i,%i);", cur_index, outreg);
			OUT("s.push(d);d+=%i;", *nregs);
			OUT("o=%i;continue b;", *rec);
			break;
		}
		case OPCODE_REC1:
		{
			reg_t outreg = read_reg;
			reg_t r1arg = read_reg;
			OUT("s.push(%i,%i);", cur_index, outreg);
			OUT("r[d+%i]=r[%i+d];", *nregs, r1arg);
			OUT("s.push(d);d+=%i;", *nregs);
			OUT("o=%i;continue b;", *rec);
			break;
		}
		case OPCODE_REC2:
		{
			reg_t outreg = read_reg;
			reg_t r1arg = read_reg;
			reg_t r2arg = read_reg;
			OUT("s.push(%i,%i);", cur_index, outreg);
			OUT("r[%i+d]=r[%i+d];", *nregs, r1arg);
			OUT("r[%i+d]=r[%i+d];", 1 + *nregs, r2arg);
			OUT("s.push(d);d+=%i;", *nregs);
			OUT("o=%i;continue b;", *rec);
			break;
		}
		case OPCODE_REC:
		{
			reg_t outreg = read_reg;
			int nargs = read_byte;
			OUT("s.push(%i,%i);", cur_index, outreg);
			for (int i = 0; i < nargs; i++)
			{
				if (i != 0)
				{
					PUT(",");
				}
				int argreg = read_reg;
				OUT("r[d+%i]=r[d+%i];", i + *nregs, argreg);
			}
			OUT("s.push(d);d+=%i;", *nregs);
			OUT("o=%i;continue b;", *rec);
			break;
		}
		case OPCODE_EQUAL:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUT("r[%i+d]=+(r[%i+d]===r[%i+d]);", to, lhs, rhs);
			break;
		}
		case OPCODE_EQUAL_NUM:
		{
			reg_t to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUT("r[%i+d]=+(r[%i+d]===%i);", to, lhs, rhs);
			break;
		}
		case OPCODE_NOT_EQUAL:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUT("r[%i+d]=+(r[%i+d]!==r[%i+d]);", to, lhs, rhs);
			break;
		}
		case OPCODE_NOT_EQUAL_NUM:
		{
			reg_t to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUT("r[%i+d]=+(r[%i+d]!==%i);", to, lhs, rhs);
			break;
		}
		case OPCODE_LESS:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUT("r[%i+d]=+(r[%i+d]<r[%i+d]);", to, lhs, rhs);
			break;
		}
		case OPCODE_LESS_NUM:
		{
			reg_t to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUT("r[%i+d]=+(r[%i+d]<%i);", to, lhs, rhs);
			break;
		}
		case OPCODE_GREATER:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUT("r[%i+d]=+(r[%i+d]>r[%i+d]);", to, lhs, rhs);
			break;
		}
		case OPCODE_GREATER_NUM:
		{
			reg_t to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUT("r[%i+d]=+(r[%i+d]>%i);", to, lhs, rhs);
			break;
		}
		case OPCODE_LESS_THAN_EQUAL:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUT("r[%i+d]=+(r[%i+d]<=r[%i+d]);", to, lhs, rhs);
			break;
		}
		case OPCODE_LESS_THAN_EQUAL_NUM:
		{
			reg_t to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUT("r[%i+d]=+(r[%i+d]<=%i);", to, lhs, rhs);
			break;
		}
		case OPCODE_GREATER_THAN_EQUAL:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUT("r[%i+d]=+(r[%i+d]>=r[%i+d]);", to, lhs, rhs);
			break;
		}
		case OPCODE_GREATER_THAN_EQUAL_NUM:
		{
			reg_t to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUT("r[%i+d]=+(r[%i+d]>=%i);", to, lhs, rhs);
			break;
		}
		case OPCODE_JUMP_ALWAYS:
		{
			int loc = read_loc;
			OUT("o=%i;continue b;", loc);
			break;
		}
		case OPCODE_JUMP_IF_FALSE:
		{
			int loc = read_loc;
			reg_t reg = read_reg;
			OUT("if(!r[%i+d]){o=%i;continue b;}", reg, loc);
			break;
		}
		case OPCODE_JUMP_IF_TRUE:
		{
			int loc = read_loc;
			reg_t reg = read_reg;
			OUT("if(r[%i+d]){o=%i;continue b;}", reg, loc);
			break;
		}
		case OPCODE_JUMP_IF_EQUAL:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUT("if(r[%i+d]===r[%i+d]){o=%i;continue b;}", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_EQUAL_NUM:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUT("if(r[%i+d]===%i){o=%i;continue b;}", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_NOT_EQUAL:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUT("if(r[%i+d]!==r[%i+d]){o=%i;continue b;}", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_NOT_EQUAL_NUM:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUT("if(r[%i+d]!==%i){o=%i;continue b;}", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_LESS:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUT("if(r[%i+d]<r[%i+d]){o=%i;continue b;}", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_LESS_NUM:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUT("if(r[%i+d]<%i){o=%i;continue b;}", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_GREATER:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUT("if(r[%i+d]>r[%i+d]){o=%i;continue b;}", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_GREATER_NUM:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUT("if(r[%i+d]>%i){o=%i;continue b;}", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_LESS_THAN_EQUAL:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUT("if(r[%i+d]<=r[%i+d]){o=%i;continue b;}", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_LESS_THAN_EQUAL_NUM:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUT("if(r[%i+d]<=%i){o=%i;continue b;}", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_GREATER_THAN_EQUAL:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUT("if(r[%i+d]>=r[%i+d]){o=%i;continue b;}", lhs, rhs, loc);
			break;
		}
		case OPCODE_JUMP_IF_GREATER_THAN_EQUAL_NUM:
		{
			int loc = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUT("if(r[%i+d]>=%i){o=%i;continue b;}", lhs, rhs, loc);
			break;
		}
		case OPCODE_INC:
		{
			reg_t reg = read_reg;
			reg_t from = read_reg;
			OUT("r[%i+d]+=r[%i+d];", reg, from);
			break;
		}
		case OPCODE_INC_NUM:
		{
			reg_t reg = read_reg;
			int n = read_int;
			OUT("r[%i+d]+=%i;", reg, n);
			break;
		}
		case OPCODE_DEC:
		{
			reg_t reg = read_reg;
			reg_t from = read_reg;
			OUT("r[%i+d]-=r[%i+d];", reg, from);
			break;
		}
		case OPCODE_DEC_NUM:
		{
			reg_t reg = read_reg;
			int n = read_int;
			OUT("r[%i+d]-=%i;", reg, n);
			break;
		}
		case OPCODE_ADD:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUT("r[%i+d]=r[%i+d]+r[%i+d];", reg, lhs, rhs);
			break;
		}
		case OPCODE_ADD_NUM:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUT("r[%i+d]=r[%i+d]+%i;", reg, lhs, rhs);
			break;
		}
		case OPCODE_SUB:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUT("r[%i+d]=r[%i+d]-r[%i+d];", reg, lhs, rhs);
			break;
		}
		case OPCODE_SUB_NUM:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUT("r[%i+d]=r[%i+d]-%i;", reg, lhs, rhs);
			break;
		}
		case OPCODE_MUL:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUT("r[%i+d]=r[%i+d]*r[%i+d];", reg, lhs, rhs);
			break;
		}
		case OPCODE_MUL_NUM:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUT("r[%i+d]=r[%i+d]*%i;", reg, lhs, rhs);
			break;
		}
		case OPCODE_DIV:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUT("r[%i+d]=r[%i+d]/r[%i+d];", reg, lhs, rhs);
			break;
		}
		case OPCODE_DIV_NUM:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUT("r[%i+d]=r[%i+d]/%i;", reg, lhs, rhs);
			OUT("r[%i+d]=r[%i+d]/%i;", reg, lhs, rhs);
			break;
		}
		case OPCODE_MOD:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			OUT("r[%i+d]=r[%i+d]%%r[%i+d];", reg, lhs, rhs);
			break;
		}
		case OPCODE_MOD_NUM:
		{
			reg_t reg = read_reg;
			reg_t lhs = read_reg;
			int rhs = read_int;
			OUT("r[%i+d]=r[%i+d]%%%i;", reg, lhs, rhs);
			break;
		}
		case OPCODE_ARRAY:
		{
			reg_t outreg = read_reg;
			int nargs = read_byte;
			OUT("r[%i+d]=[", outreg);
			for (int i = 0; i < nargs; i++)
			{
				reg_t reg = read_reg;
				if (i != 0)
				{
					PUT(",");
				}
				OUT("r[%i+d]", reg);
			}
			PUT("];");
			break;
		}
		case OPCODE_LENGTH:
		{
			reg_t outreg = read_reg;
			reg_t reg = read_reg;
			OUT("r[%i+d]=r[%i+d].length;", outreg, reg);
			break;
		}
		case OPCODE_INDEX:
		{
			reg_t outreg = read_reg;
			reg_t reg = read_reg;
			reg_t ind = read_reg;
			OUT("r[%i+d]=r[%i+d][r[%i+d]];", outreg, reg, ind);
			break;
		}
		case OPCODE_INDEX_NUM:
		{
			reg_t outreg = read_reg;
			reg_t reg = read_reg;
			int index = read_int;
			OUT("r[%i+d]=r[%i+d][%i];", outreg, reg, index);
			break;
		}
		case OPCODE_PRINTLN:
		{
			reg_t reg = read_reg;
			if (jstype == VM_BACKEND_JS_NODE)
			{
				OUT("console.log(r[%i+d]);", reg);
			}
			else if (jstype == VM_BACKEND_JS_V8)
			{
				OUT("print(r[%i+d]);", reg);
			}
			break;
		}
		case OPCODE_PUTCHAR:
		{
			reg_t reg = read_reg;
			if (jstype == VM_BACKEND_JS_NODE)
			{
				OUT("process.stdout.write(String.fromCharCode(r[%i+d]));", reg);
			}
			else if (jstype == VM_BACKEND_JS_V8)
			{
				OUT("write(String.fromCharCode(r[%i+d]));", reg);
			}
			break;
		}
		case OPCODE_EXIT:
		{
			PUT("break b;}}");
			goto done;
		}
		default:
		{
			fprintf(stderr, "unhandle opcode: %i\n", (int)op);
			return NULL;
		}
		}
	}
done:
	return bufptr;
}
