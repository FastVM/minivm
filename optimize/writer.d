module optimize.writer;

import std.conv;
import std.stdio;
import std.algorithm;

import optimize.bytecode;
import optimize.instr;

void put(Type)(ref void[] buf, Type val) {
	buf ~= (cast(void*)&val)[0..Type.sizeof];
}

void bufWrite(ref void[] buf, Instr instr, ref int[int] updateOffsets, ref int[] putOffsets) {
	updateOffsets[instr.offset] = cast(int) buf.length;
	if (instr.keep == false) {
		return;
	}
	buf.put(instr.op);
	foreach (arg; instr.args) {
		if (arg.type == Argument.type.register) {
		    Register reg = arg.value.register;
			buf.put(cast(ubyte) reg.reg);
		}
		if (arg.type == Argument.type.byte_) {
			Byte byte_ = arg.value.byte_;
			buf.put(cast(ubyte) byte_.val);
		}
		if (arg.type == Argument.type.integer) {
			Integer int_ = arg.value.integer;
			buf.put(cast(int) int_.val);
		}
		if (arg.type == Argument.type.location) {
		    Location loc = arg.value.location;
			putOffsets ~= cast(int) buf.length;
			buf.put(cast(int) loc.loc);
		}
		if (arg.type == Argument.type.call) {
		    Call call = arg.value.call;
			buf.put(cast(ubyte) call.regs.length);
			foreach (ref reg; call.regs) {
				buf.put(cast(ubyte) reg);
			}
		}
		if (arg.type == Argument.type.function_) {
		    Function func = arg.value.function_;
			updateOffsets[func.instrs[0].offset - 1] = cast(int) buf.length;
			buf.put(cast(ubyte) func.nregs);
			foreach (ref subInstr; func.instrs) {
				buf.bufWrite(subInstr, updateOffsets, putOffsets);
			}
			buf.put(Opcode.fun_done);
		}
	}
}

void[] toBytecode(Instr[] instrs) {
	void[] ret;
	int[int] updateOffsets;
	int[] putOffsets;
	foreach (ref instr; instrs) {
		ret.bufWrite(instr, updateOffsets, putOffsets);
	}
	updateOffsets[0] = 0;
	foreach (ref where; putOffsets) {
		int *ptr = cast(int*) ret[where..where+int.sizeof].ptr;
		*ptr = updateOffsets[*ptr];
	}
	// ret.put(Opcode.exit);
	return ret;
}
