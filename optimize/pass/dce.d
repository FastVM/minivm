module optimize.pass.dce;

import std.algorithm;

import optimize.instr;
import optimize.locs;
import optimize.opt;
import optimize.bytecode;

static this() {
	"dce".set!DCE;
}

class DCE : Optimizer {
	int[] blockScanned;
	Block[int] blocksByOffset;
	int[Block] blockRefCount;

	this(Instr[] instrs) {
		super(instrs);
	}
	
	void jumpCombineRef(Block block) {
		blockRefCount[block] += 1;
		if (!blockScanned.canFind(block.firstOffset)) {
			blockScanned ~= block.firstOffset;
			foreach (ref instr; block.instrs) {
				if (instr.op == Opcode.store_fun) {
					continue;
				}
				if (!instr.outJump) {
					continue;
				}
				if (instr.op == Opcode.ret) {
					return;
				}
				foreach (ref arg; instr.args) {
					if (arg.type == Argument.type.location) {
					    Location loc = arg.value.location;
						jumpCombineRef(blocksByOffset[loc.loc]);
					}
				}
				if (instr.op == Opcode.jump_always) {
					return;
				}
			}
			if (block.next !is null) {
				jumpCombineRef(block.next);
			}
		}
	}

	void jumpCombine() {
		foreach (ref block; program.blocks) {
			blocksByOffset[block.firstOffset] = block;
			blockRefCount[block] = 0;
		}
		jumpCombineRef(program.blocks[0]);
		Block[] oldBlocks = program.blocks;
		program.blocks = null;
		Block last = null;
		foreach (ref block; oldBlocks) {
			if (blockRefCount[block] != 0) {
				if (last !is null) {
					if (last.usesNext) {
						last.next = block;
					}
				}
				program.blocks ~= block;
				last = block;
			}
		}
		if (last !is null) {
			last.next = null;
		}
	}

	override void impl() {
		jumpCombine();
	}
}