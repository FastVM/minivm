#include "../jump.h"
#include "../lib.h"
#include "../vm.h"

#if !defined(VM_CHECK_TRACE)
#define VM_CHECK_TRACE 2
#endif

struct vm_check_print_data_t;
typedef struct vm_check_print_data_t vm_check_print_data_t;

typedef int vm_check_print_func_t(vm_check_print_data_t *state, const char *fmt, ...);

struct vm_check_print_callback_t;
typedef struct vm_check_print_callback_t vm_check_print_callback_t;

struct vm_check_print_callback_t {
  vm_check_print_data_t *data;
  vm_check_print_func_t *func;
};

struct vm_check_config_t {
  size_t trace;
  vm_check_print_callback_t print;
};

const char *vm_check_opcode_name(vm_opcode_t op) {
  switch (op) {
  case VM_OPCODE_EXIT: return "exit";
  case VM_OPCODE_REG: return "reg";
  case VM_OPCODE_INT: return "int";
  case VM_OPCODE_JUMP: return "jump";
  case VM_OPCODE_FUNC: return "func";
  case VM_OPCODE_ADD: return "add";
  case VM_OPCODE_SUB: return "sub";
  case VM_OPCODE_MUL: return "mul";
  case VM_OPCODE_DIV: return "div";
  case VM_OPCODE_MOD: return "mod";
  case VM_OPCODE_CALL: return "call";
  case VM_OPCODE_RET: return "ret";
  case VM_OPCODE_PUTCHAR: return "putchar";
  case VM_OPCODE_BB: return "bb";
  case VM_OPCODE_DCALL: return "dcall";
  case VM_OPCODE_INTF: return "intf";
  case VM_OPCODE_BEQ: return "beq";
  case VM_OPCODE_BLT: return "blt";
  case VM_OPCODE_ADDI: return "addi";
  case VM_OPCODE_SUBI: return "subi";
  case VM_OPCODE_TCALL: return "tcall";
  case VM_OPCODE_RETI: return "reti";
  case VM_OPCODE_BEQI: return "beqi";
  case VM_OPCODE_BLTI: return "blti";
  case VM_OPCODE_MULI: return "muli";
  case VM_OPCODE_DIVI: return "divi";
  case VM_OPCODE_MODI: return "modi";
  default: return NULL;
  }
}

void vm_check_print_instr(const vm_opcode_t *ops, size_t index) {
  switch(ops[index++]) {
  case VM_OPCODE_EXIT: {
    printf("exit\n");
    break;
  }
  case VM_OPCODE_REG: {
    vm_opcode_t outreg = ops[index++];
    vm_opcode_t inreg = ops[index++];
    printf("r%zu <- reg r%zu\n", (size_t) outreg, (size_t) inreg);
    break;
  }
  case VM_OPCODE_INT: {
    vm_opcode_t outreg = ops[index++];
    vm_opcode_t num = ops[index++];
    printf("r%zu <- int %zu\n", (size_t) outreg, (size_t) num);
    break;
  }
  case VM_OPCODE_RET: {
    vm_opcode_t outreg = ops[index++];
    printf("ret r%zu\n", (size_t) outreg);
    break;
  }
  case VM_OPCODE_ADD: {
    vm_opcode_t outreg = ops[index++];
    vm_opcode_t lhs = ops[index++];
    vm_opcode_t rhs = ops[index++];
    printf("r%zu <- add r%zu r%zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
    break;
  }
  case VM_OPCODE_SUB: {
    vm_opcode_t outreg = ops[index++];
    vm_opcode_t lhs = ops[index++];
    vm_opcode_t rhs = ops[index++];
    printf("r%zu <- sub r%zu r%zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
    break;
  }
  case VM_OPCODE_MUL: {
    vm_opcode_t outreg = ops[index++];
    vm_opcode_t lhs = ops[index++];
    vm_opcode_t rhs = ops[index++];
    printf("r%zu <- mul r%zu r%zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
    break;
  }
  case VM_OPCODE_DIV: {
    vm_opcode_t outreg = ops[index++];
    vm_opcode_t lhs = ops[index++];
    vm_opcode_t rhs = ops[index++];
    printf("r%zu <- div r%zu r%zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
    break;
  }
  case VM_OPCODE_MOD: {
    vm_opcode_t outreg = ops[index++];
    vm_opcode_t lhs = ops[index++];
    vm_opcode_t rhs = ops[index++];
    printf("r%zu <- mod r%zu r%zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
    break;
  }
  case VM_OPCODE_BB: {
    vm_opcode_t inreg = ops[index++];
    vm_opcode_t jfalse = ops[index++];
    vm_opcode_t jtrue = ops[index++];
    printf("bb r%zu #%zu #%zu\n", (size_t) inreg, (size_t) jfalse, (size_t) jtrue);
    break;
  }
  case VM_OPCODE_CALL: {
    vm_opcode_t rreg = ops[index++];
    vm_opcode_t func = ops[index++];
    vm_opcode_t nargs = ops[index++];
    printf("r%zu <- f%zu(", (size_t) rreg, (size_t) func);
    for (int i = index; i < index + nargs; i++) {
      if (i != index) {
        printf(" ");
      }
      printf("r%zu", (size_t) ops[i]);
    }
    printf(")\n");
    index += nargs;
    break;
  }
  case VM_OPCODE_DCALL: {
    vm_opcode_t rreg = ops[index++];
    vm_opcode_t func = ops[index++];
    vm_opcode_t nargs = ops[index++];
    printf("r%zu <- r%zu(", (size_t) rreg, (size_t) func);
    for (int i = index; i < index + nargs; i++) {
      if (i != index) {
        printf(" ");
      }
      printf("r%zu", (size_t) ops[i]);
    }
    printf(")\n");
    index += nargs;
    break;
  }
  case VM_OPCODE_TCALL: {
    vm_opcode_t nargs = ops[index++];
    printf("ret this(");
    for (int i = index; i < index + nargs; i++) {
      if (i != index) {
        printf(" ");
      }
      printf("r%zu", (size_t) ops[i]);
    }
    printf(")\n");
    index += nargs;
    break;
  }
  case VM_OPCODE_INTF: {
    vm_opcode_t reg = ops[index++];
    vm_opcode_t func = ops[index++];
    printf("r%zu <- f%zu\n", (size_t) reg, (size_t) func);
    break;
  }
  case VM_OPCODE_PUTCHAR: {
    vm_opcode_t inreg = ops[index++];
    printf("putchar r%zu\n", (size_t) inreg);
    break;
  }
  case VM_OPCODE_JUMP: {
    vm_opcode_t over = ops[index++];
    printf("jump #%zu\n", (size_t) over);
    break;
  }
  case VM_OPCODE_FUNC: {
    vm_opcode_t over = ops[index++];
    vm_opcode_t nargs = ops[index++];
    vm_opcode_t nregs = ops[index++];
    printf("func f%zu(", (size_t) index);
    for (int i = 1; i <= nargs; i++) {
      if (i != 1) {
        printf(" ");
      }
      printf("r%zu", (size_t) i);
    }
    printf("):\n");
    break;
  }
  case VM_OPCODE_ADDI: {
    vm_opcode_t outreg = ops[index++];
    vm_opcode_t lhs = ops[index++];
    vm_opcode_t rhs = ops[index++];
    printf("r%zu <- add r%zu %zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
    break;
  }
  case VM_OPCODE_SUBI: {
    vm_opcode_t outreg = ops[index++];
    vm_opcode_t lhs = ops[index++];
    vm_opcode_t rhs = ops[index++];
    printf("r%zu <- sub r%zu %zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
    break;
  }
  case VM_OPCODE_BEQ: {
    vm_opcode_t lhs = ops[index++];
    vm_opcode_t rhs = ops[index++];
    vm_opcode_t jfalse = ops[index++];
    vm_opcode_t jtrue = ops[index++];
    printf("beq r%zu r%zu #%zu #%zu\n", (size_t) lhs, (size_t) rhs, (size_t) jfalse, (size_t) jtrue);
    break;
  }
  case VM_OPCODE_BLT: {
    vm_opcode_t lhs = ops[index++];
    vm_opcode_t rhs = ops[index++];
    vm_opcode_t jfalse = ops[index++];
    vm_opcode_t jtrue = ops[index++];
    printf("blt r%zu r%zu #%zu #%zu\n", (size_t) lhs, (size_t) rhs, (size_t) jfalse, (size_t) jtrue);
    break;
  }
  case VM_OPCODE_RETI: {
    vm_opcode_t num = ops[index++];
    printf("ret %zu\n", (size_t) num);
    break;
  }
  case VM_OPCODE_BEQI: {
    vm_opcode_t lhs = ops[index++];
    vm_opcode_t rhs = ops[index++];
    vm_opcode_t jfalse = ops[index++];
    vm_opcode_t jtrue = ops[index++];
    printf("beq r%zu %zu #%zu #%zu\n", (size_t) lhs, (size_t) rhs, (size_t) jfalse, (size_t) jtrue);
    break;
  }
  case VM_OPCODE_BLTI: {
    vm_opcode_t lhs = ops[index++];
    vm_opcode_t rhs = ops[index++];
    vm_opcode_t jfalse = ops[index++];
    vm_opcode_t jtrue = ops[index++];
    printf("blt r%zu %zu #%zu #%zu\n", (size_t) lhs, (size_t) rhs, (size_t) jfalse, (size_t) jtrue);
    break;
  }
  case VM_OPCODE_MULI: {
    vm_opcode_t outreg = ops[index++];
    vm_opcode_t lhs = ops[index++];
    vm_opcode_t rhs = ops[index++];
    printf("r%zu <- mul r%zu %zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
    break;
  }
  case VM_OPCODE_DIVI: {
    vm_opcode_t outreg = ops[index++];
    vm_opcode_t lhs = ops[index++];
    vm_opcode_t rhs = ops[index++];
    printf("r%zu <- div r%zu %zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
    break;
  }
  case VM_OPCODE_MODI: {
    vm_opcode_t outreg = ops[index++];
    vm_opcode_t lhs = ops[index++];
    vm_opcode_t rhs = ops[index++];
    printf("r%zu <- mod r%zu %zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
    break;
  }
  default:
    break;
  }
}

#define vm_read_safe() ({ \
  if (index >= nops)      \
  {                       \
    goto bounds;          \
  }                       \
  ops[index++];           \
})

int vm_check_base(size_t nops, const vm_opcode_t *ops)
{
  size_t index = 0;
  size_t last[VM_CHECK_TRACE] = {0};
  while (index < nops)
  {
    for (int i = VM_CHECK_TRACE-2; i >= 0; i--) {
      last[i+1] = last[i]; 
    }
    last[0] = index;
    switch (vm_read_safe())
    {
    case VM_OPCODE_EXIT:
    {
      break;
    }
    case VM_OPCODE_REG:
    {
      vm_opcode_t outreg = vm_read_safe();
      vm_opcode_t inreg = vm_read_safe();
      break;
    }
    case VM_OPCODE_INT:
    {
      vm_opcode_t outreg = vm_read_safe();
      vm_opcode_t num = vm_read_safe();
      break;
    }
    case VM_OPCODE_RET:
    {
      vm_opcode_t outreg = vm_read_safe();
      break;
    }
    case VM_OPCODE_ADD:
    {
      vm_opcode_t outreg = vm_read_safe();
      vm_opcode_t lhs = vm_read_safe();
      vm_opcode_t rhs = vm_read_safe();
      break;
    }
    case VM_OPCODE_SUB:
    {
      vm_opcode_t outreg = vm_read_safe();
      vm_opcode_t lhs = vm_read_safe();
      vm_opcode_t rhs = vm_read_safe();
      break;
    }
    case VM_OPCODE_MUL:
    {
      vm_opcode_t outreg = vm_read_safe();
      vm_opcode_t lhs = vm_read_safe();
      vm_opcode_t rhs = vm_read_safe();
      break;
    }
    case VM_OPCODE_DIV:
    {
      vm_opcode_t outreg = vm_read_safe();
      vm_opcode_t lhs = vm_read_safe();
      vm_opcode_t rhs = vm_read_safe();
      break;
    }
    case VM_OPCODE_MOD:
    {
      vm_opcode_t outreg = vm_read_safe();
      vm_opcode_t lhs = vm_read_safe();
      vm_opcode_t rhs = vm_read_safe();
      break;
    }
    case VM_OPCODE_BB:
    {
      vm_opcode_t inreg = vm_read_safe();
      vm_opcode_t jfalse = vm_read_safe();
      vm_opcode_t jtrue = vm_read_safe();
      break;
    }
    case VM_OPCODE_CALL:
    {
      vm_opcode_t rreg = vm_read_safe();
      vm_opcode_t func = vm_read_safe();
      vm_opcode_t nargs = vm_read_safe();
      index += nargs;
      break;
    }
    case VM_OPCODE_DCALL:
    {
      vm_opcode_t rreg = vm_read_safe();
      vm_opcode_t func = vm_read_safe();
      vm_opcode_t nargs = vm_read_safe();
      index += nargs;
      break;
    }
    case VM_OPCODE_TCALL:
    {
      vm_opcode_t nargs = vm_read_safe();
      index += nargs;
      break;
    }
    case VM_OPCODE_INTF:
    {
      vm_opcode_t reg = vm_read_safe();
      vm_opcode_t func = vm_read_safe();
      break;
    }
    case VM_OPCODE_PUTCHAR:
    {
      vm_opcode_t inreg = vm_read_safe();
      break;
    }
    case VM_OPCODE_JUMP:
    {
      vm_opcode_t over = vm_read_safe();
      break;
    }
    case VM_OPCODE_FUNC:
    {
      vm_opcode_t over = vm_read_safe();
      vm_opcode_t nargs = vm_read_safe();
      vm_opcode_t nregs = vm_read_safe();
      break;
    }
    case VM_OPCODE_ADDI:
    {
      vm_opcode_t outreg = vm_read_safe();
      vm_opcode_t lhs = vm_read_safe();
      vm_opcode_t rhs = vm_read_safe();
      break;
    }
    case VM_OPCODE_SUBI:
    {
      vm_opcode_t outreg = vm_read_safe();
      vm_opcode_t lhs = vm_read_safe();
      vm_opcode_t rhs = vm_read_safe();
      break;
    }
    case VM_OPCODE_BEQ:
    {
      vm_opcode_t lhs = vm_read_safe();
      vm_opcode_t rhs = vm_read_safe();
      vm_opcode_t jfalse = vm_read_safe();
      vm_opcode_t jtrue = vm_read_safe();
      break;
    }
    case VM_OPCODE_BLT:
    {
      vm_opcode_t lhs = vm_read_safe();
      vm_opcode_t rhs = vm_read_safe();
      vm_opcode_t jfalse = vm_read_safe();
      vm_opcode_t jtrue = vm_read_safe();
      break;
    }
    case VM_OPCODE_RETI:
    {
      vm_opcode_t num = vm_read_safe();
      break;
    }
    case VM_OPCODE_BEQI:
    {
      vm_opcode_t lhs = vm_read_safe();
      vm_opcode_t rhs = vm_read_safe();
      vm_opcode_t jfalse = vm_read_safe();
      vm_opcode_t jtrue = vm_read_safe();
      break;
    }
    case VM_OPCODE_BLTI:
    {
      vm_opcode_t lhs = vm_read_safe();
      vm_opcode_t rhs = vm_read_safe();
      vm_opcode_t jfalse = vm_read_safe();
      vm_opcode_t jtrue = vm_read_safe();
      break;
    }
    case VM_OPCODE_MULI:
    {
      vm_opcode_t outreg = vm_read_safe();
      vm_opcode_t lhs = vm_read_safe();
      vm_opcode_t rhs = vm_read_safe();
      break;
    }
    case VM_OPCODE_DIVI:
    {
      vm_opcode_t outreg = vm_read_safe();
      vm_opcode_t lhs = vm_read_safe();
      vm_opcode_t rhs = vm_read_safe();
      break;
    }
    case VM_OPCODE_MODI:
    {
      vm_opcode_t outreg = vm_read_safe();
      vm_opcode_t lhs = vm_read_safe();
      vm_opcode_t rhs = vm_read_safe();
      break;
    }
    default:
      printf("error: unknown opcode: %zu\n", (size_t)ops[index - 1]);
      printf("  bad instruction was #%zu\n", (size_t) last[0]);
      size_t max = 0;
      for (size_t i = 1; i < VM_CHECK_TRACE; i++) {
        max = i;
        if (last[i] == 0) {
          break;
        }
      }
      if (max != 0) {
        printf("  most recent instruction(s):\n");
        for (size_t i = max; i > 0; i--) {
          printf("    ");
          vm_check_print_instr(ops, last[i]);
        }
      }
      return 1;
    }
  }
  return 0;
bounds:
  printf("error: instruction \"%s\" needs more arguments\n", vm_check_opcode_name(ops[last[0]]));
  printf("  bad instruction was #%zu\n", (size_t) last[0]);
  size_t max = 0;
  for (size_t i = 1; i < VM_CHECK_TRACE; i++) {
    max = i;
    if (last[i] == 0) {
      break;
    }
  }
  if (max != 0) {
    printf("  most recent instruction(s):\n");
    for (size_t i = max; i > 0; i--) {
      printf("    ");
      vm_check_print_instr(ops, last[i]);
    }
  }
  return 1;
}

#define vm_check_jump(loc) ({                                                                             \
  if (loc > nops || loc < 0)                                                                              \
  {                                                                                                       \
    printf("error: jump off end of instruction buffer at #%zu\n", (size_t)nops); \
    goto err_debug;                                                                                             \
  }                                                                                                       \
  if ((jumps[loc] & VM_JUMP_INSTR) == 0)                                                                  \
  {                                                                                                       \
    printf("error: jump to #%zu is not at head of an instruction\n", (size_t)loc);    \
    goto err_debug;                                                                                             \
  }                                                                                                       \
})

int vm_check_jump_targets(size_t nops, const vm_opcode_t *ops)
{
  uint8_t *jumps = vm_jump_base(nops, ops);
  size_t index = 0;
  size_t last[VM_CHECK_TRACE] = {0};
  while (index < nops)
  {
    for (int i = VM_CHECK_TRACE-2; i >= 0; i--) {
      last[i+1] = last[i]; 
    }
    last[0] = index;
    switch (ops[index++])
    {
    case VM_OPCODE_EXIT:
    {
      break;
    }
    case VM_OPCODE_REG:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t inreg = ops[index++];
      break;
    }
    case VM_OPCODE_INT:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t num = ops[index++];
      break;
    }
    case VM_OPCODE_RET:
    {
      vm_opcode_t outreg = ops[index++];
      break;
    }
    case VM_OPCODE_ADD:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_SUB:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_MUL:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_DIV:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_MOD:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_BB:
    {
      vm_opcode_t inreg = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      break;
    }
    case VM_OPCODE_CALL:
    {
      vm_opcode_t rreg = ops[index++];
      vm_opcode_t func = ops[index++];
      vm_opcode_t nargs = ops[index++];
      index += nargs;
      break;
    }
    case VM_OPCODE_DCALL:
    {
      vm_opcode_t rreg = ops[index++];
      vm_opcode_t func = ops[index++];
      vm_opcode_t nargs = ops[index++];
      index += nargs;
      break;
    }
    case VM_OPCODE_TCALL:
    {
      vm_opcode_t nargs = ops[index++];
      index += nargs;
      break;
    }
    case VM_OPCODE_INTF:
    {
      vm_opcode_t reg = ops[index++];
      vm_opcode_t func = ops[index++];
      break;
    }
    case VM_OPCODE_PUTCHAR:
    {
      vm_opcode_t inreg = ops[index++];
      break;
    }
    case VM_OPCODE_JUMP:
    {
      vm_opcode_t over = ops[index++];
      vm_check_jump(over);
      break;
    }
    case VM_OPCODE_FUNC:
    {
      vm_opcode_t over = ops[index++];
      vm_opcode_t nargs = ops[index++];
      vm_opcode_t nregs = ops[index++];
      vm_check_jump(over);
      break;
    }
    case VM_OPCODE_ADDI:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_SUBI:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_BEQ:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      vm_check_jump(jfalse);
      vm_check_jump(jtrue);
      break;
    }
    case VM_OPCODE_BLT:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      vm_check_jump(jfalse);
      vm_check_jump(jtrue);
      break;
    }
    case VM_OPCODE_RETI:
    {
      vm_opcode_t num = ops[index++];
      break;
    }
    case VM_OPCODE_BEQI:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      vm_check_jump(jfalse);
      vm_check_jump(jtrue);
      break;
    }
    case VM_OPCODE_BLTI:
    {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      vm_check_jump(jfalse);
      vm_check_jump(jtrue);
      break;
    }
    case VM_OPCODE_MULI:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_DIVI:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_MODI:
    {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    default:
      return 1;
    }
    continue;
  err_debug:
    printf("  bad instruction was #%zu\n", (size_t) last[0]);
    int max = 0;
    for (int i = 1; i < VM_CHECK_TRACE; i++) {
      max = i;
      if (last[i] == 0) {
        break;
      }
    }
    if (max != 0) {
      printf("  most recent instruction(s):\n");
      for (int i = max; i >= 0; i--) {
          printf("    ");
          vm_check_print_instr(ops, last[i]);
        if (i == 0) {
          printf("    ^ error was in this instruction\n");
        }
      }
    }
    return 1;
  }
  return 0;
}

int vm_run(size_t nops, const vm_opcode_t *ops)
{
  int base_res = vm_check_base(nops, ops);
  if (base_res != 0)
  {
    return 1;
  }
  int jump_res = vm_check_jump_targets(nops, ops);
  if (jump_res != 0)
  {
    return 1;
  }
  return 0;
}
