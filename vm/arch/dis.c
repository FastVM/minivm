#include "../lib.h"
#include "../vm.h"

enum vm_jump_scanned {
  VM_JUMP_IN = 1,
  VM_JUMP_OUT = 2, 
  VM_JUMP_INIT = 4,
  VM_JUMP_REACH = 8,
};

void vm_scan_reachable_from(size_t index, size_t nops, const vm_opcode_t *ops, uint8_t *jumps) {
  if (jumps[index] & VM_JUMP_REACH) {
    return;
  }
  while (index < nops) {
    jumps[index] |= VM_JUMP_REACH;
    switch (ops[index++]) {
    case VM_OPCODE_EXIT: {
      break;
    }
    case VM_OPCODE_REG: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t inreg = ops[index++];
      break;
    }
    case VM_OPCODE_INT: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t num = ops[index++];
      break;
    }
    case VM_OPCODE_RETURN: {
      vm_opcode_t outreg = ops[index++];
      return;
    }
    case VM_OPCODE_ADD: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_SUB: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_MUL: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_DIV: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_MOD: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_BB: {
      vm_opcode_t inreg = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      vm_scan_reachable_from(jfalse, nops, ops, jumps);
      vm_scan_reachable_from(jtrue, nops, ops, jumps);
      return;
    }
    case VM_OPCODE_CALL: {
      vm_opcode_t rreg = ops[index++];
      vm_opcode_t func = ops[index++];
      vm_opcode_t nargs = ops[index++];
      index += nargs;
      vm_scan_reachable_from(func, nops, ops, jumps);
      break;
    }
    case VM_OPCODE_DCALL: {
      vm_opcode_t rreg = ops[index++];
      vm_opcode_t func = ops[index++];
      vm_opcode_t nargs = ops[index++];
      index += nargs;
      break;
    }
    case VM_OPCODE_TCALL: {
      vm_opcode_t nargs = ops[index++];
      index += nargs;
      break;
    }
    case VM_OPCODE_INTF: {
      vm_opcode_t reg = ops[index++];
      vm_opcode_t func = ops[index++];
      vm_scan_reachable_from(func, nops, ops, jumps);
      break;
    }
    case VM_OPCODE_PUTCHAR: {
      vm_opcode_t inreg = ops[index++];
      break;
    }
    case VM_OPCODE_JUMP: {
      vm_opcode_t over = ops[index++];
      vm_scan_reachable_from(over, nops, ops, jumps);
      return;
    }
    case VM_OPCODE_FUNC: {
      vm_opcode_t over = ops[index++];
      vm_opcode_t nargs = ops[index++];
      vm_opcode_t nregs = ops[index++];
      vm_scan_reachable_from(index, nops, ops, jumps);
      vm_scan_reachable_from(over, nops, ops, jumps);
      return;
    }
    case VM_OPCODE_ADDI: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_SUBI: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_BEQ: {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      vm_scan_reachable_from(jfalse, nops, ops, jumps);
      vm_scan_reachable_from(jtrue, nops, ops, jumps);
      return;
    }
    case VM_OPCODE_BLT: {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      vm_scan_reachable_from(jfalse, nops, ops, jumps);
      vm_scan_reachable_from(jtrue, nops, ops, jumps);
      return;
    }
    case VM_OPCODE_RETI: {
      vm_opcode_t num = ops[index++];
      return;
    }
    case VM_OPCODE_BEQI: {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      vm_scan_reachable_from(jfalse, nops, ops, jumps);
      vm_scan_reachable_from(jtrue, nops, ops, jumps);
      return;
    }
    case VM_OPCODE_BLTI: {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      vm_scan_reachable_from(jfalse, nops, ops, jumps);
      vm_scan_reachable_from(jtrue, nops, ops, jumps);
      return;
    }
    case VM_OPCODE_MULI: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_DIVI: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_MODI: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    default:
      return;
    }
  }
}

void vm_scan_reachable(size_t nops, const vm_opcode_t *ops, uint8_t *jumps) {
vm_scan_reachable_from(0, nops, ops, jumps);
}

uint8_t *vm_scan_jumps(size_t nops, const vm_opcode_t *ops) {
  uint8_t *ret = malloc(sizeof(uint8_t) * nops);
  for (size_t i = 0; i < nops; i++) {
    ret[i] = 0;
  }
  size_t index = 0;
  while (index < nops) {
    switch (ops[index++]) {
    case VM_OPCODE_EXIT: {
      break;
    }
    case VM_OPCODE_REG: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t inreg = ops[index++];
      break;
    }
    case VM_OPCODE_INT: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t num = ops[index++];
      break;
    }
    case VM_OPCODE_RETURN: {
      vm_opcode_t outreg = ops[index++];
      ret[index] |= VM_JUMP_OUT;
      break;
    }
    case VM_OPCODE_ADD: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_SUB: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_MUL: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_DIV: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_MOD: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_BB: {
      vm_opcode_t inreg = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      ret[jfalse] |= VM_JUMP_IN;
      ret[jtrue] |= VM_JUMP_IN;
      ret[index] |= VM_JUMP_OUT;
      break;
    }
    case VM_OPCODE_CALL: {
      vm_opcode_t rreg = ops[index++];
      vm_opcode_t func = ops[index++];
      vm_opcode_t nargs = ops[index++];
      index += nargs;
      break;
    }
    case VM_OPCODE_DCALL: {
      vm_opcode_t rreg = ops[index++];
      vm_opcode_t func = ops[index++];
      vm_opcode_t nargs = ops[index++];
      index += nargs;
      break;
    }
    case VM_OPCODE_TCALL: {
      vm_opcode_t nargs = ops[index++];
      index += nargs;
      break;
    }
    case VM_OPCODE_INTF: {
      vm_opcode_t reg = ops[index++];
      vm_opcode_t func = ops[index++];
      break;
    }
    case VM_OPCODE_PUTCHAR: {
      vm_opcode_t inreg = ops[index++];
      break;
    }
    case VM_OPCODE_JUMP: {
      vm_opcode_t over = ops[index++];
      ret[over] |= VM_JUMP_IN;
      ret[index] |= VM_JUMP_OUT;
      break;
    }
    case VM_OPCODE_FUNC: {
      vm_opcode_t over = ops[index++];
      vm_opcode_t nargs = ops[index++];
      vm_opcode_t nregs = ops[index++];
      ret[index] = VM_JUMP_INIT;
      break;
    }
    case VM_OPCODE_ADDI: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_SUBI: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_BEQ: {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      ret[jfalse] |= VM_JUMP_IN;
      ret[jtrue] |= VM_JUMP_IN;
      ret[index] |= VM_JUMP_OUT;
      break;
    }
    case VM_OPCODE_BLT: {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      ret[jfalse] |= VM_JUMP_IN;
      ret[jtrue] |= VM_JUMP_IN;
      ret[index] |= VM_JUMP_OUT;
      break;
    }
    case VM_OPCODE_RETI: {
      vm_opcode_t num = ops[index++];
      ret[index] |= VM_JUMP_OUT;
      break;
    }
    case VM_OPCODE_BEQI: {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      ret[jfalse] |= VM_JUMP_IN;
      ret[jtrue] |= VM_JUMP_IN;
      ret[index] |= VM_JUMP_OUT;
      break;
    }
    case VM_OPCODE_BLTI: {
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      vm_opcode_t jfalse = ops[index++];
      vm_opcode_t jtrue = ops[index++];
      ret[jfalse] |= VM_JUMP_IN;
      ret[jtrue] |= VM_JUMP_IN;
      ret[index] |= VM_JUMP_OUT;
      break;
    }
    case VM_OPCODE_MULI: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_DIVI: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    case VM_OPCODE_MODI: {
      vm_opcode_t outreg = ops[index++];
      vm_opcode_t lhs = ops[index++];
      vm_opcode_t rhs = ops[index++];
      break;
    }
    default:
      printf("unknown opcode: %zu\n", (size_t) ops[index-1]);
      return NULL;
    }
  }
  return ret;
}

int vm_run(size_t nops, const vm_opcode_t *ops) {
  uint8_t *jumps = vm_scan_jumps(nops, ops);
  if (jumps == NULL) {
    return 1;
  }
  vm_scan_reachable(nops, ops, jumps);
  size_t index = 0;
  size_t cfunc = 0;
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
    case VM_OPCODE_RETURN: {
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
      if (reachable) printf("ret f%zu(", (size_t) cfunc);
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
      cfunc = index;
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
    // if (jumps[index] && VM_JUMP_OUT) {
    //   if (reachable) printf("#%zu:\n", (size_t) index);
    // }
  }
  free(jumps);
  return 0;
}