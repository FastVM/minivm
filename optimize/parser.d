module optimize.parse;

import std.conv;
import std.stdio;
import std.algorithm;

import optimize.bytecode;
import optimize.reg;
import optimize.locs;
import optimize.instr;
import optimize.format;

struct Code {
	void[] code;
	int offset;
}

Type read(Type)(ref Code code) {
	if (code.code.length < Type.sizeof) {
		throw new Exception("invalid bytecode: missing data");
	}
	Type ret = *cast(Type*) code.code.ptr;
	code.code = code.code[Type.sizeof..$];
	code.offset += Type.sizeof;
	return ret;
}

Argument readFmt(ref Code code, char c) {
	final switch (c) {
	case 'r':
		return Argument(Register(code.read!ubyte));
	case 'b': 
		return Argument(Byte(code.read!ubyte));
	case 'i':
		return Argument(Integer(code.read!int));
	case 'j':
		return Argument(Location(code.read!int));
	case 'f':
		return Argument(Function(code.read!ubyte, code.parse([Opcode.fun_done])));
	case 'c':
		ubyte num = code.read!ubyte;
		ubyte[] regs;
		foreach (i; 0..num) {
			regs ~= code.read!ubyte;
		}
		return Argument(Call(regs));
	}
}

Instr readInstr(ref Code code) {
	Opcode op = code.read!Opcode;
	Argument[] args;
	if (op !in format) {
		throw new Exception("invalid bytecode: opcode not found: " ~ op.to!string);
	}
	Format opFmt = format[op];
	foreach (spec; opFmt) {
		args ~= code.readFmt(spec);
	}
	return Instr(op, args);
}

Instr[] parse(ref Code code, Opcode[] end) {
	Instr[] instrs;
	while (code.code.length != 0) {
		int offset =  code.offset;
		Instr instr = code.readInstr;
		instr.offset = offset;
		if (end.canFind(instr.op)) {
			break;
		}
		instrs ~= instr;
	}
	int[][int] branches = instrs.branches;
	int[] o2i = instrs.offsetToIndex;
	int[] i2o = instrs.indexToOffset;
	foreach (key, values; branches) {
		instrs[o2i[key]].outJump = true;
		foreach (value; values) {
			instrs[o2i[value]].inJump = true;
		}
	}
	instrs[0].inJump = true;
	foreach (ref instr; instrs) {
		if (instr.op == Opcode.ret || instr.op == Opcode.exit) {
			instr.outJump = true;
		}
		if (instr.op == Opcode.store_fun) {
			instr.outJump = false;
		}
		if (instr.op == Opcode.static_call0 || instr.op == Opcode.static_call1 || instr.op == Opcode.static_call2 || instr.op == Opcode.static_call) {
			instr.outJump = false;
		}
	}
	return instrs;
}

Instr[] parse(void[] bc) {
	Code code = Code(bc, 0);
	Instr[] instrs = code.parse([]);
	// instrs ~= code.readInstr;
	return instrs;
}
