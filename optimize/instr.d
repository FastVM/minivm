module optimize.instr;

import std.conv;
import std.array;
import std.algorithm;
import optimize.bytecode;

struct Argument {
	enum Type {
		register,
		byte_,
		integer,
		location,
		call,
		function_,
	}
	union Value {
		Register register;
		Byte byte_;
		Integer integer;
		Location location;
		Call call;
		Function function_;
	}
	Type type;
	Value value;

	this(Register val) {
		value.register = val;
		type = Type.register;
	}

	this(Byte val) {
		value.byte_ = val;
		type = Type.byte_;
	}

	this(Integer val) {
		value.integer = val;
		type = Type.integer;
	}

	this(Location val) {
		value.location = val;
		type = Type.location;
	}

	this(Call val) {
		value.call = val;
		type = Type.call;
	}

	this(Function val) {
		value.function_ = val;
		type = Type.function_;
	}

	string toString() {
		final switch (type) {
		case Type.register:
			return value.register.to!string;
		case Type.byte_:
			return value.byte_.to!string;
		case Type.integer:
			return value.integer.to!string;
		case Type.location:
			return value.location.to!string;
		case Type.call:
			return value.call.to!string;
		case Type.function_:
			return value.function_.to!string;
		}
	}
}

struct Register {
	ubyte reg;

	this(ubyte reg_) {
		reg = reg_;
	}

	string toString() {
		return "r" ~ reg.to!string;
	}
}

struct Byte {
	ubyte val;

	this(ubyte val_) {
		val = val_;
	}

	string toString() {
		return val.to!string;
	}
}

struct Integer {
	int val;

	this(int val_) {
		val = val_;
	}

	string toString() {
		return val.to!string;
	}
}

struct Location {
	int loc;

	this(int loc_) {
		loc = loc_;
	}

	string toString() {
		return "@" ~ loc.to!string;
	}
}

struct Call {
	ubyte[] regs;
	
	this(ubyte[] regs_) {
		regs = regs_;
	}

	string toString() {
		return "(" ~ regs.map!(x => 'r' ~ x.to!string).join(", ") ~ ")";
	}
}

string indent(string src) {
	string ret = "  ";
	foreach (c; src) {
		if (c == '\n') {
			ret ~= "\n  ";
		} else {
			ret ~= c;
		}
	}
	return ret;
}

struct Function {
	Instr[] instrs;
	ubyte nregs;

	this(ubyte nregs_, Instr[] instrs_) {
		nregs = nregs_;
		instrs = instrs_;
	}

	string toString() {
		return "{\n" ~ instrs.instrsToString.indent ~ "}";
	}
}

struct Instr {
	int offset = -1;
	Opcode op;
	Argument[] args;
	bool outJump;
	bool inJump;
	bool keep = true;

	this(Args...)(Opcode op_, Args args_) {
		op = op_;
		static foreach (arg; args_) {
			static if (is(typeof(arg) == Argument[])) {
				args ~= arg;
			} else {
				args ~= Argument(arg);
			}
		}
	} 

	Instr copy() {
		Instr ret = Instr(op, args);
		ret.outJump = outJump;
		ret.inJump = inJump;
		ret.keep = true;
		return ret;
	}

	void opOpAssign(string op: "~", Type)(Type val) {
		args ~= Argument(val);
	}

	string toString() {
		if (args.length == 0) {
			return op.to!string;
		}
		return op.to!string ~ " " ~ args.to!string[1..$-1];
	}

	static Instr noKeep() {
		Instr ret = void;
		ret.keep = false;
		return ret;
	}
}

string instrsToString(Instr[] instrs) {
	string ret;
	bool last = false;
	foreach (ref instr; instrs) {
		if (instr.inJump || last) {
			ret ~= instr.offset.to!string;
			ret ~= ":\n";
		}
		ret ~= "  ";
		ret ~= instr.to!string;
		ret ~= '\n';
		last = instr.outJump;
		// last = false;
	}
	return ret;
}
