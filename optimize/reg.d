module optimize.reg;

import std.algorithm;

import optimize.bytecode;
import optimize.instr;

int[] usedRegs(Instr[] instrs) {
	int[] regs;
	int depth = 0;
	Instr[] sub;
	foreach (ref instr; instrs) {
		if (depth == 0) {
			foreach (ref arg; instr.args) {
				if (arg.type == Argument.type.register) {
				    Register reg  = arg.value.register;
					if (!regs.canFind(reg.reg)) {
						regs ~= reg.reg;
					}
				}
			}
		}
		if (instr.op == Opcode.store_fun) {
			depth += 1;
		}
		if (instr.op == Opcode.fun_done) {
			depth -= 1;
		}
	}
	return regs;
}

int[][] allUsedRegs(Instr[] instrs) {
	int[][] allRegs = [usedRegs(instrs)];
	int depth = 0;
	Instr[] sub;
	foreach (ref instr; instrs) {
		if (instr.op == Opcode.fun_done) {
			depth -= 1;
			if (depth == 0) {
				allRegs ~= usedRegs(sub);
			}
		}
		if (depth > 0) {
			sub ~= instr;
		}
		if (instr.op == Opcode.store_fun) {
			depth += 1;
		}
	}
	return allRegs;
}
