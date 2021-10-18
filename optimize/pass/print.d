module optimize.pass.print;

import std.algorithm;
import std.stdio;

import optimize.instr;
import optimize.locs;
import optimize.opt;
import optimize.bytecode;

static this() {
	"print".set!Print;
}

class Print : Optimizer {
	this(Instr[] instrs) {
		super(instrs);
	}

	override void impl() {
		if (toplevel) {
			writeln(program);
		}
	}
}