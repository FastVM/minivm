module optimize.opt;

import std.stdio;
import std.conv;

import optimize.parser;
import optimize.writer;
import optimize.instr;
import optimize.locs;
import optimize.reg;
import optimize.bytecode;

Instr readInstr(ref Instr[] instrs) {
	Instr ret = instrs[0];
	instrs = instrs[1..$];
	return ret;
}

class Blocks {
	Block[] blocks;
	
	this() {}

	static Blocks from(Instr[] instrs) {
		return Blocks.parse(instrs);
	}

	static Blocks parse(ref Instr[] instrs) {
		Blocks func = new Blocks;
		while (true) {
			Block block = Block.parse(instrs);
			func.blocks ~= block;
			if (instrs.length == 0) {
				break;
			}
		}
		foreach (index, block; func.blocks[0..$-1]) {
			if (block.usesNext) {
				block.next = func.blocks[index + 1];
			}
		}
		if (func.blocks[$-1].instrs[$-1].op != Opcode.exit) {
			func.blocks[0].startOffset = 1;
		}
		return func;
	}
	
	Instr[] instrs() {
		Instr[] instrs;
		getInstrs(instrs);
		return instrs;
	}

	void getInstrs(ref Instr[] outInstrs) {
		foreach (index, block; blocks[0..$-1]) {
			Block next = blocks[index + 1];
			foreach (ref instr; block.instrs) {
				outInstrs ~= instr;
			}
			if (block.next !is null && next != block.next) {
				outInstrs ~= Instr(Opcode.jump_always, Location(next.firstOffset));
			}
		}
		foreach (ref instr; blocks[$-1].instrs) {
			outInstrs ~= instr;
		}
	}

	override string toString() {
		Instr[] instrs;
		getInstrs(instrs);
		return instrsToString(instrs);
	}
}

class Block {
	Instr[] instrs;
	Block next;
	int startOffset = 0;

	this() {}

	static Block parse(ref Instr[] instrs) {
		Block block = new Block;
		Instr first = instrs.readInstr;
		block.instrs ~= first;
		if (first.outJump) {
			return block;
		}
		while (true) {
			if (instrs.length == 0) {
				break;
			}
			if (instrs[0].inJump) {
				break;
			}
			Instr instr = instrs.readInstr;
			block.instrs ~= instr;
			if (instr.outJump) {
				break;
			}
		}
		return block;
	}

	int firstOffset() {
		if (instrs.length != 0) {
			return instrs[0].offset + startOffset;
		}
		if (next !is null) {
			return next.firstOffset;
		}
		throw new Exception("basic block has nowhere to go");
		assert(false);
	}

	bool usesNext() {
		return instrs[$-1].op != Opcode.exit && instrs[$-1].op != Opcode.ret;
	}

	override string toString() {
		return '@' ~ firstOffset.to!string;
	}
}

class Optimizer {
	bool toplevel;
	ubyte nregs;
	Blocks program;

	this(Instr[] instrs) {
		toplevel = false;
		program = Blocks.parse(instrs);
	}

	Instr[] instrs() {
		return program.instrs;
	}

	void opt(string pass) {
		foreach (block; program.blocks) {
			foreach (ref instr; block.instrs) {
				foreach (ref arg; instr.args) {
					if (arg.type == Argument.type.function_) {
					    Function func = arg.value.function_;
						Optimizer subOpt = passes[pass](func.instrs);
						subOpt.nregs = func.nregs;
						subOpt.opt(pass);
						func.instrs = subOpt.instrs;
						// writeln(func.nregs, " -> ", subOpt.nregs);
						func.nregs = subOpt.nregs;
						arg.value.function_ = func;
					}
				}
			}
		}
		impl();
	}

	void impl() {
		assert(false);	
	}
}

Optimizer delegate(Instr[])[string] passes;

void set(Type)(string name) {
	passes[name] = (Instr[] instrs) {
		return new Type(instrs);
	};
}

void[] bcOpt(void[] code, string pass) {
	if (pass !in passes) {
		throw new Exception("unknown pass: " ~ pass);
	}
	Optimizer opt = passes[pass](code.parse);
	opt.toplevel = true;
	opt.opt(pass);
	void[] ret = opt.instrs.toBytecode;
	return ret;
}

void[] validate(void[] code) {
	return code.parse.toBytecode;
}
