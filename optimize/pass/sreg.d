module optimize.pass.sreg;

import std.algorithm;
import std.stdio;
import std.math;

import optimize.instr;
import optimize.locs;
import optimize.opt;
import optimize.bytecode;

static this() {
	"sreg".set!StoreReg;
}

class StoreReg : Optimizer {
	this(Instr[] instrs) {
		super(instrs);
	}

	void removeDead(ubyte[] usedRegs, Block block) {
		int[ubyte] over;
		foreach (index, ref instr; block.instrs) {
			if (instr.op == Opcode.store_int || instr.op == Opcode.store_byte || instr.op == Opcode.store_reg) {
				Register outReg = instr.args[0].value.register;
				if (!usedRegs.canFind(outReg.reg)) {
					instr.keep = false;				
				}
				if (int* refIndex = outReg.reg in over) {
					block.instrs[*refIndex].keep = false;
					*refIndex = cast(int) index;
				} else {
					over[outReg.reg] = cast(int) index;
				}
				foreach (arg; instr.args[1..$]) {
					if (arg.type == Argument.type.register) {
						Register reg = arg.value.register;
						if (reg.reg in over) {
							over.remove(reg.reg);
						}
					}
				}
			} else {
				foreach (arg; instr.args) {
					if (arg.type == Argument.type.register) {
						Register reg = arg.value.register;
						if (reg.reg in over) {
							over.remove(reg.reg);
						}
					}
				}
			}
		}
	}

	void markInstr(ref ubyte[] usedRegs, Instr instr) {
		int start = 0;
		if (instr.op == Opcode.store_int || instr.op == Opcode.store_byte || instr.op == Opcode.store_reg) {
			start = 1;
		}
		foreach (arg; instr.args[start..$]) {
			if (arg.type == Argument.type.register) {
			    Register reg = arg.value.register;
				usedRegs ~= reg.reg;
			}
			if (arg.type == Argument.type.call) {
			    Call call = arg.value.call;
				foreach (reg; call.regs) {
					usedRegs ~= reg;
				}
			}
		}
	}

	override void impl() {
		ubyte[] usedRegs;
		foreach (block; program.blocks) {
			foreach (instr; block.instrs) {
				markInstr(usedRegs, instr);
			}
		}
		foreach (block; program.blocks) {
			removeDead(usedRegs, block);
		}
	}
}