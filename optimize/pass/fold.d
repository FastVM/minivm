module optimize.pass.fold;

import std.algorithm;
import std.stdio;
import std.math;

import optimize.instr;
import optimize.locs;
import optimize.opt;
import optimize.bytecode;

static this() {
	"fold".set!Fold;
}

class Fold : Optimizer {
	this(Instr[] instrs) {
		super(instrs);
	}

	Instr foldMathInstr(string op)(ref int[ubyte] constRegs, Argument outRegArg, Argument inRegArg, Argument valArg) {
		Register outReg = outRegArg.value.register;
		Register argReg = inRegArg.value.register;
		int num;
		if (valArg.type == Argument.type.integer) {
		    Integer inum = valArg.value.integer;
			num = inum.val;
		}
		if (valArg.type == Argument.type.byte_) {
		    Byte bnum = valArg.value.byte_;
			num = cast(int) bnum.val;
		}
		if (valArg.type == Argument.type.register) {
		    Register reg = valArg.value.register;
			if (int *refValue = reg.reg in constRegs) {
				num = *refValue;
			} else {
				return Instr.noKeep;
			}
		}
		if (int *refValue = argReg.reg in constRegs) {
			double res = mixin(`cast(double) *refValue` ~ op ~ `cast(double) num`);
			if (res % 1 == 0 && 0 <= res && res < 256) {
				return Instr(Opcode.store_byte, outReg, Byte(cast(ubyte) res));
			}
			if (res % 1 == 0 && res.abs < 2L ^^ 31) {
				return Instr(Opcode.store_int, outReg, Integer(cast(int) res));
			}
		}
		return Instr.noKeep;
	}

	void foldMath(Block block) {
		redoPass: while (true) {
			int[ubyte] constRegs;
			storeHere: foreach (ref instr; block.instrs) {
				if (instr.op == Opcode.store_int) {
					Register reg = instr.args[0].value.register;
					Integer num = instr.args[1].value.integer;
					constRegs[reg.reg] = num.val;
					continue storeHere;
				}
				if (instr.op == Opcode.store_byte) {
					Register reg = instr.args[0].value.register;
					Byte num = instr.args[1].value.byte_;
					constRegs[reg.reg] = num.val;
					continue storeHere;
				}
				if (instr.op == Opcode.store_reg) {
					Register outReg = instr.args[0].value.register;
					Register inReg = instr.args[1].value.register;
					if (int *refValue = inReg.reg in constRegs) {
						if (0 <= *refValue && *refValue < 256) {
							instr = Instr(Opcode.store_byte, outReg, Byte(cast(ubyte) *refValue));
						} else {
							instr = Instr(Opcode.store_int, outReg, Integer(*refValue));
						}
						continue redoPass;
					}
				}
				bool redo = false;
				void fold(string op, Args...)(Args args) {
					if (!redo) {
						Instr res = foldMathInstr!op(constRegs, args);
						if (res.keep) {
							redo = true;
							instr = res;
						}
					}
				}
				if (instr.op == Opcode.inc_num || instr.op == Opcode.inc) {
					fold!"+"(instr.args[0], instr.args[0], instr.args[1]);
				}
				if (instr.op == Opcode.dec_num || instr.op == Opcode.dec) {
					fold!"-"(instr.args[0], instr.args[0], instr.args[1]);
				}
				if (instr.op == Opcode.add_num || instr.op == Opcode.add) {
					fold!"+"(instr.args[0], instr.args[1], instr.args[2]);
				}
				if (instr.op == Opcode.sub_num || instr.op == Opcode.sub) {
					fold!"-"(instr.args[0], instr.args[1], instr.args[2]);
				}
				if (instr.op == Opcode.mul_num || instr.op == Opcode.mul) {
					fold!"*"(instr.args[0], instr.args[1], instr.args[2]);
				}
				if (instr.op == Opcode.div_num || instr.op == Opcode.div) {
					fold!"/"(instr.args[0], instr.args[1], instr.args[2]);
				}
				if (instr.op == Opcode.mod_num || instr.op == Opcode.mod) {
					fold!"%"(instr.args[0], instr.args[1], instr.args[2]);
				}
				if (redo) {
					continue redoPass;
				}
			}
			break redoPass;
		}
	}

	Instr foldJumpInstr(string op)(ref int[ubyte] constRegs, Block block, Argument jumpToArg, Argument lhsArg, Argument rhsArg) {
		Location jumpTo = jumpToArg.value.location;
		Register lhsReg = lhsArg.value.register;
		int num;
		if (rhsArg.type == Argument.type.integer) {
		    Integer inum = rhsArg.value.integer;
			num = inum.val;
		}
		if (rhsArg.type == Argument.type.byte_) {
		    Byte bnum = rhsArg.value.byte_;
			num = cast(int) bnum.val;
		}
		if (rhsArg.type == Argument.type.register) {
		    Register reg = rhsArg.value.register;
			if (int *refValue = reg.reg in constRegs) {
				num = *refValue;
			} else {
				return Instr.noKeep;
			}
		}
		if (int *refValue = lhsReg.reg in constRegs) {
			bool res = mixin(`*refValue` ~ op ~ `num`);
			if (res) {
				return Instr(Opcode.jump_always, jumpTo);
			} else if (block.next !is null) {
				return Instr(Opcode.jump_always, Location(block.next.firstOffset));
			}
		}
		return Instr.noKeep;
	}

	void foldJumps(Block block) {
		redoPass: while (true) {
			int[ubyte] constRegs;
			storeHere: foreach (ref instr; block.instrs) {
				if (instr.op == Opcode.store_int) {
					Register reg = instr.args[0].value.register;
					Integer num = instr.args[1].value.integer;
					constRegs[reg.reg] = num.val;
					continue storeHere;
				}
				if (instr.op == Opcode.store_byte) {
					Register reg = instr.args[0].value.register;
					Byte num = instr.args[1].value.byte_;
					constRegs[reg.reg] = num.val;
					continue storeHere;
				}
				if (instr.op == Opcode.store_reg) {
					Register outReg = instr.args[0].value.register;
					Register inReg = instr.args[1].value.register;
					if (int *refValue = inReg.reg in constRegs) {
						if (0 <= *refValue && *refValue < 256) {
							instr = Instr(Opcode.store_byte, outReg, Byte(cast(ubyte) *refValue));
						} else {
							instr = Instr(Opcode.store_int, outReg, Integer(*refValue));
						}
						continue redoPass;
					}
				}
				bool redo = false;
				void fold(string op, Args...)(Args args) {
					if (!redo) {
						Instr res = foldJumpInstr!op(constRegs, block, args);
						if (res.keep) {
							redo = true;
							instr = res;
						}
					}
				}
				if (instr.op == Opcode.jump_if_false) {
					fold!"=="(instr.args[0], instr.args[1], Argument(Integer(0)));
				}
				if (instr.op == Opcode.jump_if_true) {
					fold!"!="(instr.args[0], instr.args[1], Argument(Integer(0)));
				}
				if (instr.op == Opcode.jump_if_equal_num || instr.op == Opcode.jump_if_equal) {
					fold!"=="(instr.args[0], instr.args[1], instr.args[2]);
				}
				if (instr.op == Opcode.jump_if_not_equal_num || instr.op == Opcode.jump_if_not_equal) {
					fold!"!="(instr.args[0], instr.args[1], instr.args[2]);
				}
				if (instr.op == Opcode.jump_if_less_num || instr.op == Opcode.jump_if_less) {
					fold!"<"(instr.args[0], instr.args[1], instr.args[2]);
				}
				if (instr.op == Opcode.jump_if_greater_num || instr.op == Opcode.jump_if_greater) {
					fold!">"(instr.args[0], instr.args[1], instr.args[2]);
				}
				if (instr.op == Opcode.jump_if_less_than_equal_num || instr.op == Opcode.jump_if_less_than_equal) {
					fold!"<="(instr.args[0], instr.args[1], instr.args[2]);
				}
				if (instr.op == Opcode.jump_if_greater_than_equal_num || instr.op == Opcode.jump_if_greater_than_equal) {
					fold!">="(instr.args[0], instr.args[1], instr.args[2]);
				}
				if (redo) {
					continue redoPass;
				}
			}
			break redoPass;
		}
	}

	void foldMathBlocks() {
		foreach (ref block; program.blocks) {
			foldMath(block);
			foldJumps(block);
		}
	}

	override void impl() {
		foldMathBlocks();
	}
}