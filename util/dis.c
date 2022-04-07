#include "../vm/lib.h"
#include "../vm/vm.h"
#include "../vm/jump.h"
#include "../vm/io.h"

int vm_run_dis(size_t nops, const vm_opcode_t *ops) {
  uint8_t *jumps = vm_jump_all(nops, ops);
  if (jumps == NULL) {
    return 1;
  }
  size_t index = 0;
  size_t cend = 0;
  size_t depth = 0;
  while (index < nops) {
    uint8_t reachable = (jumps[index] & VM_JUMP_REACH) != 0;
    if (index+1 >= cend && depth != 0) {
      if (reachable) printf("}\n");
      depth = 0;
    }
    else if ((jumps[index] & VM_JUMP_IN) || (jumps[index] & VM_JUMP_OUT)) {
      if (reachable) printf("  #%zu:\n", (size_t) index);
    }
    if (depth != 0) {
      if (reachable) printf("    ");
    }
    vm_opcode_t op = ops[index];
    index += 1;
    switch(op) {
    case VM_OPCODE_EXIT: {
      if (reachable) printf("exit\n");
      break;
    }
    case VM_OPCODE_REG: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t inreg = ops[index++];
      if (reachable) printf("r%zu <- reg r%zu\n", (size_t) outreg, (size_t) inreg);
      break;
    }
    case VM_OPCODE_INT: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t num = ops[index++];
      if (reachable) printf("r%zu <- int %zu\n", (size_t) outreg, (size_t) num);
      break;
    }
    case VM_OPCODE_RET: {
      vm_opcode_t outreg = ops[index++];
      if (reachable) printf("ret r%zu\n", (size_t) outreg);
      break;
    }
    case VM_OPCODE_ADD: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (reachable) printf("r%zu <- add r%zu r%zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
      break;
    }
    case VM_OPCODE_SUB: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (reachable) printf("r%zu <- sub r%zu r%zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
      break;
    }
    case VM_OPCODE_MUL: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (reachable) printf("r%zu <- mul r%zu r%zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
      break;
    }
    case VM_OPCODE_DIV: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (reachable) printf("r%zu <- div r%zu r%zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
      break;
    }
    case VM_OPCODE_MOD: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (reachable) printf("r%zu <- mod r%zu r%zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
      break;
    }
    case VM_OPCODE_BB: {
      vm_opcode_t inreg = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      if (reachable) printf("bb r%zu #%zu #%zu\n", (size_t) inreg, (size_t) jfalse, (size_t) jtrue);
      break;
    }
    case VM_OPCODE_CALL: {
      vm_opcode_t rreg = ops[index++];
      vm_opcode_t func = ops[index++];
      vm_opcode_t nargs = ops[index++];
      if (reachable) printf("r%zu <- f%zu(", (size_t) rreg, (size_t) func);
      for (int i = index; i < index + nargs; i++) {
        if (i != index) {
          if (reachable) printf(" ");
        }
        if (reachable) printf("r%zu", (size_t) ops[i]);
      }
      if (reachable) printf(")\n");
      index += nargs;
      break;
    }
    case VM_OPCODE_DCALL: {
      vm_opcode_t rreg = ops[index++];
      vm_opcode_t func = ops[index++];
      vm_opcode_t nargs = ops[index++];
      if (reachable) printf("r%zu <- r%zu(", (size_t) rreg, (size_t) func);
      for (int i = index; i < index + nargs; i++) {
        if (i != index) {
          if (reachable) printf(" ");
        }
        if (reachable) printf("r%zu", (size_t) ops[i]);
      }
      if (reachable) printf(")\n");
      index += nargs;
      break;
    }
    case VM_OPCODE_TCALL: {
      vm_opcode_t nargs = ops[index++];
      if (reachable) printf("ret this(");
      for (int i = index; i < index + nargs; i++) {
        if (i != index) {
          if (reachable) printf(" ");
        }
        if (reachable) printf("r%zu", (size_t) ops[i]);
      }
      if (reachable) printf(")\n");
      index += nargs;
      break;
    }
    case VM_OPCODE_INTF: {
      vm_opcode_t reg = ops[index++];
      vm_opcode_t func = ops[index++];
      if (reachable) printf("r%zu <- f%zu\n", (size_t) reg, (size_t) func);
      break;
    }
    case VM_OPCODE_PUTCHAR: {
      vm_opcode_t inreg = ops[index++];
      if (reachable) printf("putchar r%zu\n", (size_t) inreg);
      break;
    }
    case VM_OPCODE_JUMP: {
      vm_opcode_t over = ops[index++];
      if (reachable) printf("jump #%zu\n", (size_t) over);
      break;
    }
    case VM_OPCODE_FUNC: {
      vm_opcode_t over = ops[index++];
      vm_opcode_t nargs = ops[index++];
      vm_opcode_t nregs = ops[index++];
      cend = over;
      depth += 1;
      if (reachable) printf("func f%zu(", (size_t) index);
      for (int i = 1; i <= nargs; i++) {
        if (i != 1) {
          if (reachable) printf(" ");
        }
        if (reachable) printf("r%zu", (size_t) i);
      }
      if (reachable) printf(") {\n");
      break;
    }
    case VM_OPCODE_ADDI: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (reachable) printf("r%zu <- add r%zu %zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
      break;
    }
    case VM_OPCODE_SUBI: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (reachable) printf("r%zu <- sub r%zu %zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
      break;
    }
    case VM_OPCODE_BEQ: {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      if (reachable) printf("beq r%zu r%zu #%zu #%zu\n", (size_t) lhs, (size_t) rhs, (size_t) jfalse, (size_t) jtrue);
      break;
    }
    case VM_OPCODE_BLT: {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      if (reachable) printf("blt r%zu r%zu #%zu #%zu\n", (size_t) lhs, (size_t) rhs, (size_t) jfalse, (size_t) jtrue);
      break;
    }
    case VM_OPCODE_RETI: {
      vm_opcode_t num = ops[index++];
      if (reachable) printf("ret %zu\n", (size_t) num);
      break;
    }
    case VM_OPCODE_BEQI: {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      if (reachable) printf("beq r%zu %zu #%zu #%zu\n", (size_t) lhs, (size_t) rhs, (size_t) jfalse, (size_t) jtrue);
      break;
    }
    case VM_OPCODE_BLTI: {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      if (reachable) printf("blt r%zu %zu #%zu #%zu\n", (size_t) lhs, (size_t) rhs, (size_t) jfalse, (size_t) jtrue);
      break;
    }
    case VM_OPCODE_MULI: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (reachable) printf("r%zu <- mul r%zu %zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
      break;
    }
    case VM_OPCODE_DIVI: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (reachable) printf("r%zu <- div r%zu %zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
      break;
    }
    case VM_OPCODE_MODI: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      if (reachable) printf("r%zu <- mod r%zu %zu\n", (size_t) outreg, (size_t) lhs, (size_t) rhs);
      break;
    }
    default:
      break;
    }
  }
  free(jumps);
  return 0;
}

int main(int argc, const char **argv) {
  if (argc < 2) {
    printf("need a file argument");
    return 1;
  }
  vm_io_res_t ops = vm_io_read(argv[1]);
  if (ops.err != NULL) {
    printf("%s\n", ops.err);
    return 1;
  }
  int res = vm_run_dis(ops.nops, ops.ops);
  vm_free(ops.ops);
  return res;
}
