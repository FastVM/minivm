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

int[] offsetToIndex(Instr[] instrs) {
	int[] ret;
	int last = 0;
	foreach (index, ref instr; instrs) {
		while (ret.length < instr.offset) {
			ret ~= last;
		}
		last = cast(int) index;
		ret ~= last;
	}
	return ret;
}
