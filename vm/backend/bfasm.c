#include <vm/backend/back.h>

void vm_backend_bfasm(opcode_t *basefunc)
{
	int n = 0;
	int cur_index = 0;
	int rec = 0;
	int nregs = 0;
	printf("  stk %i\n", 1 << 12);
	printf("  [bits 8]\n");
	for (int i = 0; i < 16; i++)
	{
		printf("  push 0\n");
	}

	while (true)
	{
		int i = n++;
		printf("@loc_%i\n", cur_index);
		opcode_t op = read_instr;
		switch (op)
		{
		case OPCODE_STORE_INT:
		{
			reg_t to = read_reg;
			reg_t from = read_int;
			printf("  mov r1, %i\n", from);
			printf("  spt r1, %i\n", to + 1);
			break;
		}
		case OPCODE_STORE_REG:
		{
			reg_t to = read_reg;
			int from = read_int;
			printf("  sgt r1, %i\n", from + 1);
			printf("  spt r1, %i\n", to + 1);
			break;
		}
		case OPCODE_STORE_FUN:
		{
			reg_t to = read_reg;
			int end = read_int;
			printf("  mov r1, %i\n", cur_index);
			printf("  spt r1, %i\n", to + 1);
			printf("  jmp %%loc_%i\n", end);
			printf("@fun_%i\n", cur_index);
			rec = cur_index;
			nregs = read_int + 2;
			for (int i = 2; i < nregs; i++)
			{
				printf("  push 0\n");
			}
			if (nregs >= 2)
			{
				printf("  push r2\n");
			}
			if (nregs >= 1)
			{
				printf("  push r1\n");
			}
			break;
		}
		case OPCODE_FUN_DONE:
		{
			break;
		}
		case OPCODE_STATIC_CALL0:
		{
			reg_t outreg = read_reg;
			int next_func = read_loc;
			printf("#call(\"fun_%i\")\n", next_func);
			printf("  spt r1, %i\n", outreg + 1);
			break;
		}
		case OPCODE_STATIC_CALL1:
		{
			reg_t outreg = read_reg;
			int next_func = read_loc;
			reg_t r1arg = read_reg;
			printf("  sgt r1, %i\n", r1arg + 1);
			printf("#call(\"fun_%i\")\n", next_func);
			printf("  spt r1, %i\n", outreg + 1);
			break;
		}
		case OPCODE_STATIC_CALL2:
		{
			reg_t outreg = read_reg;
			int next_func = read_loc;
			reg_t r1arg = read_reg;
			reg_t r2arg = read_reg;
			printf("  sgt r1, %i\n", r1arg + 1);
			printf("  sgt r2, %i\n", r2arg + 1);
			printf("#call(\"fun_%i\")\n", next_func);
			printf("  spt r1, %i\n", outreg + 1);
			break;
		}
		case OPCODE_REC0:
		{
			reg_t outreg = read_reg;
			printf("#call(\"fun_%i\")\n", rec);
			printf("  spt r1, %i\n", outreg + 1);
			break;
		}
		case OPCODE_REC1:
		{
			reg_t outreg = read_reg;
			reg_t r1arg = read_reg;
			printf("  sgt r1, %i\n", r1arg + 1);
			printf("#call(\"fun_%i\")\n", rec);
			printf("  spt r1, %i\n", outreg + 1);
			break;
		}
		case OPCODE_REC2:
		{
			reg_t outreg = read_reg;
			reg_t r1arg = read_reg;
			reg_t r2arg = read_reg;
			printf("  sgt r1, %i\n", r1arg + 1);
			printf("  sgt r2, %i\n", r2arg + 1);
			printf("#call(\"fun_%i\")\n", rec);
			printf("  spt r1, %i\n", outreg + 1);
			break;
		}
		case OPCODE_RETURN:
		{
			reg_t retreg = read_reg;
			printf("  sgt r1, %i\n", retreg + 1);
			for (int i = 0; i < nregs; i++)
			{
				printf("  pop\n");
			}
			printf("  ret\n");
			break;
		}
		case OPCODE_PRINTLN:
		{
			reg_t from = read_reg;
			printf("  sgt r1, %i\n", from + 1);
			printf("#call(\"print_%i\")\n", i);
			printf("  out 10\n");
			printf("  jmp %%println_done_%i\n", i);
			printf("@print_%i\n", i);
			printf("  jz r1, %%print_done_%i\n", i);
			printf("  mov r2, r1\n");
			printf("  mod r2, 10\n");
			printf("  push r2\n");
			printf("  div r1, 10\n");
			printf("#call(\"print_%i\")\n", i);
			printf("  pop r2\n");
			printf("  add r2, 48\n");
			printf("  out r2\n");
			printf("@print_done_%i\n", i);
			printf("  ret\n");
			printf("@println_done_%i\n", i);
			break;
		}
		case OPCODE_EQUAL:
		{
			reg_t to = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  sgt r2, %i\n", rhs + 1);
			printf("  eq r1, r2\n");
			printf("  spt r1, %i\n", to + 1);
			break;
		}
		case OPCODE_EQUAL_NUM:
		{
			reg_t to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  eq r1, %i\n", rhs);
			printf("  spt r1, %i\n", to + 1);
			break;
		}
		case OPCODE_NOT_EQUAL:
		{
			reg_t to = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  sgt r2, %i\n", rhs + 1);
			printf("  neq r1, r2\n");
			printf("  spt r1, %i\n", to + 1);
			break;
		}
		case OPCODE_LESS:
		{
			reg_t to = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  sgt r2, %i\n", rhs + 1);
			printf("  lt r1, r2\n");
			printf("  spt r1, %i\n", to + 1);
			break;
		}
		case OPCODE_LESS_NUM:
		{
			reg_t to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  lt r1, %i\n", rhs);
			printf("  spt r1, %i\n", to + 1);
			break;
		}
		case OPCODE_NOT_EQUAL_NUM:
		{
			reg_t to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_int;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  neq r1, %i\n", rhs);
			printf("  spt r1, %i\n", to + 1);
			break;
		}
		case OPCODE_JUMP_ALWAYS:
		{
			int to = read_loc;
			printf("  jmp %%loc_%i\n", to);
			break;
		}
		case OPCODE_JUMP_IF_TRUE:
		{
			int to = read_loc;
			reg_t from = read_reg;
			printf("  sgt r1, %i\n", from + 1);
			printf("  jnz r1, %%loc_%i\n", to);
			break;
		}
		case OPCODE_JUMP_IF_FALSE:
		{
			int to = read_loc;
			reg_t from = read_reg;
			printf("  sgt r1, %i\n", from + 1);
			printf("  jz r1, %%loc_%i\n", to);
			break;
		}
		case OPCODE_JUMP_IF_EQUAL:
		{
			int to = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  sgt r2, %i\n", rhs + 1);
			printf("  eq r1, r2\n");
			printf("  jnz r1, %%loc_%i\n", to);
			break;
		}
		case OPCODE_JUMP_IF_EQUAL_NUM:
		{
			int to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  eq r1, %i\n", rhs);
			printf("  jnz r1, %%loc_%i\n", to);
			break;
		}
		case OPCODE_JUMP_IF_NOT_EQUAL:
		{
			int to = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  sgt r2, %i\n", rhs + 1);
			printf("  neq r1, r2\n");
			printf("  jnz r1, %%loc_%i\n", to);
			break;
		}
		case OPCODE_JUMP_IF_NOT_EQUAL_NUM:
		{
			int to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  neq r1, %i\n", rhs);
			printf("  jnz r1, %%loc_%i\n", to);
			break;
		}
		case OPCODE_JUMP_IF_LESS:
		{
			int to = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  sgt r2, %i\n", rhs + 1);
			printf("  lt r1, r2\n");
			printf("  jnz r1, %%loc_%i\n", to);
			break;
		}
		case OPCODE_JUMP_IF_LESS_NUM:
		{
			int to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  lt r1, %i\n", rhs);
			printf("  jnz r1, %%loc_%i\n", to);
			break;
		}
		case OPCODE_JUMP_IF_GREATER:
		{
			int to = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  sgt r2, %i\n", rhs + 1);
			printf("  gt r1, r2\n");
			printf("  jnz r1, %%loc_%i\n", to);
			break;
		}
		case OPCODE_JUMP_IF_GREATER_NUM:
		{
			int to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  gt r1, %i\n", rhs);
			printf("  jnz r1, %%loc_%i\n", to);
			break;
		}
		case OPCODE_JUMP_IF_LESS_THAN_EQUAL:
		{
			int to = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  sgt r2, %i\n", rhs + 1);
			printf("  le r1, r2\n");
			printf("  jnz r1, %%loc_%i\n", to);
			break;
		}
		case OPCODE_JUMP_IF_LESS_THAN_EQUAL_NUM:
		{
			int to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  le r1, %i\n", rhs);
			printf("  jnz r1, %%loc_%i\n", to);
			break;
		}
		case OPCODE_JUMP_IF_GREATER_THAN_EQUAL:
		{
			int to = read_loc;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  sgt r2, %i\n", rhs + 1);
			printf("  ge r1, r2\n");
			printf("  jnz r1, %%loc_%i\n", to);
			break;
		}
		case OPCODE_JUMP_IF_GREATER_THAN_EQUAL_NUM:
		{
			int to = read_loc;
			reg_t lhs = read_reg;
			int rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  ge r1, %i\n", rhs);
			printf("  jnz r1, %%loc_%i\n", to);
			break;
		}
		case OPCODE_INC:
		{
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  sgt r2, %i\n", rhs + 1);
			printf("  add r1, r2\n");
			printf("  spt r1, %i\n", lhs + 1);
			break;
		}
		case OPCODE_INC_NUM:
		{
			reg_t lhs = read_reg;
			int rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  add r1, %i\n", rhs);
			printf("  spt r1, %i\n", lhs + 1);
			break;
		}
		case OPCODE_DEC:
		{
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  sgt r2, %i\n", rhs + 1);
			printf("  sub r1, r2\n");
			printf("  spt r1, %i\n", lhs + 1);
			break;
		}
		case OPCODE_DEC_NUM:
		{
			reg_t lhs = read_reg;
			int rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  sub r1, %i\n", rhs);
			printf("  spt r1, %i\n", lhs + 1);
			break;
		}
		case OPCODE_ADD:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  sgt r2, %i\n", rhs + 1);
			printf("  add r1, r2\n");
			printf("  spt r1, %i\n", to + 1);
			break;
		}
		case OPCODE_ADD_NUM:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			int rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  add r1, %i\n", rhs);
			printf("  spt r1, %i\n", to + 1);
			break;
		}
		case OPCODE_SUB:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  sgt r2, %i\n", rhs + 1);
			printf("  sub r1, r2\n");
			printf("  spt r1, %i\n", to + 1);
			break;
		}
		case OPCODE_SUB_NUM:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			int rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  sub r1, %i\n", rhs);
			printf("  spt r1, %i\n", to + 1);
			break;
		}
		case OPCODE_MUL:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  sgt r2, %i\n", rhs + 1);
			printf("  mul r1, r2\n");
			printf("  spt r1, %i\n", to + 1);
			break;
		}
		case OPCODE_MUL_NUM:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			int rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  mul r1, %i\n", rhs);
			printf("  spt r1, %i\n", to + 1);
			break;
		}
		case OPCODE_DIV:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  sgt r2, %i\n", rhs + 1);
			printf("  div r1, r2\n");
			printf("  spt r1, %i\n", to + 1);
			break;
		}
		case OPCODE_DIV_NUM:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			int rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  div r1, %i\n", rhs);
			printf("  spt r1, %i\n", to + 1);
			break;
		}
		case OPCODE_MOD:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			reg_t rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  sgt r2, %i\n", rhs + 1);
			printf("  mod r1, r2\n");
			printf("  spt r1, %i\n", to + 1);
			break;
		}
		case OPCODE_MOD_NUM:
		{
			reg_t to = read_reg;
			reg_t lhs = read_reg;
			int rhs = read_reg;
			printf("  sgt r1, %i\n", lhs + 1);
			printf("  mod r1, %i\n", rhs);
			printf("  spt r1, %i\n", to + 1);
			break;
		}
		case OPCODE_PUTCHAR:
		{
			reg_t from = read_reg;
			printf("  sgt r1, %i\n", from + 1);
			printf("  out r1\n");
			break;
		}
		case OPCODE_EXIT:
		{
			return;
		}
		default:
		{
			fprintf(stderr, "unhandle opcode: %i\n", (int)op);
			return;
		}
		}
	}
}