module optimize.locs;

import optimize.instr;

int[][int] branches(Instr[] instrs) {
	int[][int] ret;
	foreach (ref instr; instrs) {
		foreach (arg; instr.args) {
			if (arg.type == Argument.type.location) {
			    Location loc = arg.value.location;
				int to = loc.loc;
				if (int[]* val = instr.offset in ret) {
					*val ~= to;
				} else {
					ret[instr.offset] = [to];
				}
			}
		}
	}
	return ret;
}

int[] indexToOffset(Instr[] instrs) {
	int[] ret;
	foreach (ref instr; instrs) {
		ret ~= instr.offset;
	}
	return ret;
}

int[int] offsetToIndex(Instr[] instrs) {
	int[int] ret;
	foreach (index, ref instr; instrs) {
		ret[instr.offset - 1] = cast(int) index;
		ret[instr.offset] = cast(int) index;
	}
	return ret;
}
