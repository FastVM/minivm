module optimize.pass.regalloc;

import std.algorithm;
import std.stdio;

import optimize.instr;
import optimize.locs;
import optimize.opt;
import optimize.bytecode;

static this() {
	"regalloc".set!RegAlloc;
}

class RegAlloc : Optimizer {
	int[ubyte] firstSeen;
	int[ubyte] lastSeen;
	ubyte[] pinnedRegs;

	this(Instr[] instrs) {
		super(instrs);
	}

	void calcBaseRanges() {
		foreach (ref block; program.blocks) {
			foreach (ref instr; block.instrs) {
				foreach (ref arg; instr.args) {
					if (arg.type == Argument.type.register) {
					    Register reg = arg.value.register;
						if (reg.reg !in firstSeen) {
							firstSeen[reg.reg] = instr.offset;
						}
						if (reg.reg !in lastSeen || instr.offset > lastSeen[reg.reg]) {
							lastSeen[reg.reg] = instr.offset;
						}
					}
					if (arg.type == Argument.type.call) {
					    Call call = arg.value.call;
						foreach (ref reg; call.regs) {
							if (reg !in firstSeen) {
								firstSeen[reg] = instr.offset;
							}
							if (reg !in lastSeen || instr.offset > lastSeen[reg]) {
								lastSeen[reg] = instr.offset;
							}
						}
					}
				}
			}
		}
		foreach (ref block; program.blocks) {
			foreach (ref instr; block.instrs) {
				foreach (ref arg; instr.args) {
					if (arg.type == Argument.type.location) {
					    Location loc = arg.value.location;
						if (loc.loc < instr.offset) {
							foreach (index, ref ls; lastSeen) {
								int fs = firstSeen[index];
								if (fs < loc.loc && loc.loc <= ls && ls < instr.offset) {
									ls = instr.offset;
								} 
							}
						}
					}
				}
			}
		}
	}

	void regAlloc() {
		bool[256] used;
		ubyte[ubyte] regs;
		foreach (ref arg; pinnedRegs) {
			used[arg] = true;
			regs[arg] = arg;
		}
		foreach (ref block; program.blocks) {
			foreach (ref instr; block.instrs) {
				foreach (oldReg, ref newReg; regs) {
					if (!pinnedRegs.canFind(oldReg) && lastSeen[oldReg] == instr.offset) {
						used[regs[cast(ubyte) oldReg]] = false;
					}
				}
				foreach (ref arg; instr.args) {
					if (arg.type == Argument.type.register) {
					    Register reg = arg.value.register;
						if (pinnedRegs.canFind(reg.reg)) {
							continue;
						}
						if (reg.reg !in regs) {
							foreach (ref index, ref isUsed; used) {
								if (!isUsed) {
									regs[reg.reg] = cast(ubyte) (index);
									isUsed = true;
									if (index >= nregs) {
										nregs = cast(ubyte) (index + 1);
									}
									break;
								}
							}
						}
						reg.reg = regs[reg.reg];
						arg.value.register = reg;
					}
					if (arg.type == Argument.type.call) {
					    Call call = arg.value.call;
						foreach (argno, reg; call.regs) {
							if (pinnedRegs.canFind(reg)) {
								continue;
							}
							if (reg !in regs) {
								foreach (index, ref isUsed; used) {
									if (!isUsed) {
										regs[reg] = cast(ubyte) (index);
										isUsed = true;
										if (index >= nregs) {
											nregs = cast(ubyte) (index + 1);
										}
										break;
									}
								}
							}
							call.regs[argno] = regs[reg];
						}
						arg.value.call = call;
					}
				}
				foreach (oldReg, newReg; regs) {
					if (lastSeen[oldReg] == instr.offset) {
						regs.remove(cast(ubyte) oldReg);
					}
				}
			}
		}
	}

	void calcNumArgs() {
		int[] seen;
		foreach (block; program.blocks) {
			foreach (ref instr; block.instrs) {
				int start = 0;
				if (!instr.op.noOutputs) {
					start = 1;
					if (instr.args[0].type == Argument.type.register) {
					    Register reg = instr.args[0].value.register;
						if (!seen.canFind(reg.reg)) {
							seen ~= reg.reg;
						}
					}
				}
				foreach (arg; instr.args[start..$]) {
					if (arg.type == Argument.type.register) {
					    Register reg = arg.value.register;
						if (!seen.canFind(reg.reg) && !pinnedRegs.canFind(reg.reg)) {
							pinnedRegs ~= reg.reg;
						}
					}
					if (arg.type == Argument.type.call) {
					    Call call = arg.value.call;
						foreach (reg; call.regs) {
							if (!seen.canFind(reg) && !pinnedRegs.canFind(reg)) {
								pinnedRegs ~= reg;
							}
						}
					}
				}
			}
		}
	}

	override void impl() {
		nregs = 0;
		calcNumArgs();
		calcBaseRanges();
		regAlloc();
	}
}