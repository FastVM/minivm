#include "../vm/io.h"
#include "../vm/jump.h"
#include "../vm/lib.h"
#include "../vm/reguse.h"
#include "../vm/vm.h"

#define VM_COLOR_BLACK() ({ printf("\033[0;30m"); })
#define VM_COLOR_RED() ({ printf("\033[0;31m"); })
#define VM_COLOR_GREEN() ({ printf("\033[0;32m"); })
#define VM_COLOR_YELLOW() ({ printf("\033[0;33m"); })
#define VM_COLOR_BLUE() ({ printf("\033[0;34m"); })
#define VM_COLOR_PURPLE() ({ printf("\033[0;35m"); })
#define VM_COLOR_CYAN() ({ printf("\033[0;36m"); })
#define VM_COLOR_WHITE() ({ printf("\033[0;37m"); })

#define VM_PRINT_REG(r_) vm_run_dis_print_reg(nops, ops, jumps, index, r_)
void vm_run_dis_print_reg(size_t nops, const vm_opcode_t *ops, uint8_t *jumps, size_t index, vm_opcode_t reg)
{
  if (vm_reg_is_used(nops, ops, jumps, index, reg))
  {
    VM_COLOR_BLUE();
  }
  else
  {
    VM_COLOR_CYAN();
  }
  printf("r%zu", (size_t)reg);
  VM_COLOR_WHITE();
}

#define VM_PRINT_OUTREG(r_) vm_run_dis_print_outreg(nops, ops, jumps, index, r_)
void vm_run_dis_print_outreg(size_t nops, const vm_opcode_t *ops, uint8_t *jumps, size_t index, vm_opcode_t reg)
{
  if (vm_reg_is_used(nops, ops, jumps, index, reg))
  {
    VM_COLOR_BLUE();
  }
  else
  {
    VM_COLOR_RED();
  }
  printf("r%zu", (size_t)reg);
  VM_COLOR_WHITE();
}

int vm_run_dis_all(size_t nops, const vm_opcode_t *ops, uint8_t *jumps)
{
  size_t index = 0;
  size_t cend = 0;
  size_t depth = 0;
  while (index < nops)
  {
    while (index < cend && (jumps[index] & VM_JUMP_REACH) == 0)
    {
      index += 1;
    }
    VM_COLOR_WHITE();
    if (index + 1 >= cend && depth != 0)
    {
      printf("}\n");
      depth = 0;
    }
    else if ((jumps[index] & VM_JUMP_IN) || (jumps[index] & VM_JUMP_OUT))
    {
      printf("  #%zu:\n", (size_t)index);
    }
    if (depth != 0)
    {
      printf("    ");
    }
    vm_opcode_t op = ops[index];
    index += 1;
    switch (op)
    {
    case VM_OPCODE_EXIT:
    {
      printf("exit\n");
      break;
    }
    case VM_OPCODE_REG:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t inreg = ops[index++];
      VM_PRINT_OUTREG(outreg);
      printf(" <- reg ");
      VM_PRINT_REG(inreg);
      printf("\n");
      break;
    }
    case VM_OPCODE_INT:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t num = ops[index++];
      VM_PRINT_OUTREG(outreg);
      printf(" <- %zu\n", (size_t)num);
      break;
    }
    case VM_OPCODE_RET:
    {
      vm_opcode_t inreg = ops[index++];
      printf("ret ");
      VM_COLOR_CYAN();
      printf("r%zu\n", (size_t)inreg);
      VM_COLOR_WHITE();
      break;
    }
    case VM_OPCODE_ADD:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      VM_PRINT_OUTREG(outreg);
      printf(" <- add ");
      VM_PRINT_REG(lhs);
      printf(" ");
      VM_PRINT_REG(rhs);
      printf("\n");
      break;
    }
    case VM_OPCODE_SUB:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      VM_PRINT_OUTREG(outreg);
      printf(" <- sub ");
      VM_PRINT_REG(lhs);
      printf(" ");
      VM_PRINT_REG(rhs);
      printf("\n");
      break;
    }
    case VM_OPCODE_MUL:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      VM_PRINT_OUTREG(outreg);
      printf(" <- mul ");
      VM_PRINT_REG(lhs);
      printf(" ");
      VM_PRINT_REG(rhs);
      printf("\n");
      break;
    }
    case VM_OPCODE_DIV:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      VM_PRINT_OUTREG(outreg);
      printf(" <- div ");
      VM_PRINT_REG(lhs);
      printf(" ");
      VM_PRINT_REG(rhs);
      printf("\n");
      break;
    }
    case VM_OPCODE_MOD:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      VM_PRINT_OUTREG(outreg);
      printf(" <- mod ");
      VM_PRINT_REG(lhs);
      printf(" ");
      VM_PRINT_REG(rhs);
      printf("\n");
      break;
    }
    case VM_OPCODE_BB:
    {
      vm_opcode_t inreg = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      printf("bb ");
      VM_PRINT_REG(inreg);
      printf(" #%zu #%zu\n", (size_t)jfalse, (size_t)jtrue);
      break;
    }
    case VM_OPCODE_CALL:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t func = ops[index++];
      int nargs = (int)ops[index++];
      index += nargs;
      VM_PRINT_OUTREG(outreg);
      printf(" <- ");
      printf("f%zu(", (size_t)func);
      for (int i = 0; i < nargs; i++)
      {
        if (i != 0)
        {
          printf(" ");
        }
        VM_PRINT_REG(ops[index + i - nargs]);
      }
      printf(")\n");
      break;
    }
    case VM_OPCODE_DCALL:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t func = ops[index++];
      int nargs = ops[index++];
      index += nargs;
      VM_PRINT_OUTREG(outreg);
      printf(" <- ");
      VM_PRINT_REG(func);
      printf("(");
      for (int i = 0; i < nargs; i++)
      {
        if (i != 0)
        {
          printf(" ");
        }
        VM_PRINT_REG(ops[index + i - nargs]);
      }
      printf(")\n");
      break;
    }
    case VM_OPCODE_TCALL:
    {
      int nargs = ops[index++];
      index += nargs;
      printf("ret this(");
      for (int i = 0; i < nargs; i++)
      {
        if (i != 0)
        {
          printf(" ");
        }
        VM_COLOR_CYAN();
        printf("r%zu", (size_t)ops[index + i - nargs]);
      }
      VM_COLOR_WHITE();
      printf(")\n");
      break;
    }
    case VM_OPCODE_INTF:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t func = ops[index++];
      VM_PRINT_OUTREG(outreg);
      printf(" <- ");
      printf("f%zu\n", (size_t)func);
      break;
    }
    case VM_OPCODE_PUTCHAR:
    {
      vm_opcode_t inreg = ops[index++];
      printf("putchar ");
      VM_PRINT_REG(inreg);
      printf("\n");
      break;
    }
    case VM_OPCODE_JUMP:
    {
      vm_opcode_t over = ops[index++];
      printf("jump #%zu\n", (size_t)over);
      break;
    }
    case VM_OPCODE_FUNC:
    {
      vm_opcode_t over = ops[index++];
      vm_opcode_t nargs = ops[index++];
      vm_opcode_t nregs = ops[index++];
      cend = over;
      depth += 1;
      printf("func f%zu(", (size_t)index);
      for (int i = 1; i <= nargs; i++)
      {
        if (i != 1)
        {
          printf(" ");
        }
        VM_PRINT_REG(i);
      }
      printf(") {\n");
      break;
    }
    case VM_OPCODE_BEQ:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      printf("beq ");
      VM_PRINT_REG(lhs);
      printf(" ");
      VM_PRINT_REG(rhs);
      printf(" #%zu #%zu\n", (size_t)jfalse, (size_t)jtrue);
      break;
    }
    case VM_OPCODE_BLT:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      printf("blt ");
      VM_PRINT_REG(lhs);
      printf(" ");
      VM_PRINT_REG(rhs);
      printf(" #%zu #%zu\n", (size_t)jfalse, (size_t)jtrue);
      break;
    }
    case VM_OPCODE_PAIR:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t car = ops[index++];
      vm_opcode_t cdr = ops[index++];
      VM_PRINT_OUTREG(outreg);
      printf(" <- ");
      printf("pair ");
      VM_PRINT_REG(car);
      printf(" ");
      VM_PRINT_REG(cdr);
      printf("\n");
      break;
    }
    case VM_OPCODE_FIRST:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t pair = ops[index++];
      VM_PRINT_OUTREG(outreg);
      printf(" <- ");
      printf("first ");
      VM_PRINT_REG(pair);
      printf("\n");
      break;
    }
    case VM_OPCODE_SECOND:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t pair = ops[index++];
      VM_PRINT_OUTREG(outreg);
      printf(" <- ");
      printf("second ");
      VM_PRINT_REG(pair);
      printf("\n");
      break;
    }
    default:
      break;
    }
  }
  return 0;
}

int vm_run_dis(size_t nops, const vm_opcode_t *ops)
{
  uint8_t *jumps = vm_jump_all(nops, ops);
  if (jumps == NULL)
  {
    return 1;
  }
  int ret = vm_run_dis_all(nops, ops, jumps);
  free(jumps);
  if (ret != 0)
  {
    return 1;
  }
  return 0;
}

int main(int argc, const char **argv)
{
  if (argc < 2)
  {
    printf("need a file argument");
    return 1;
  }
  vm_io_res_t ops = vm_io_read(argv[1]);
  if (ops.err != NULL)
  {
    printf("%s\n", ops.err);
    return 1;
  }
  int res = vm_run_dis(ops.nops, ops.ops);
  vm_free(ops.ops);
  return res;
}
