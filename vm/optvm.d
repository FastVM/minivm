
module vm.run;

import core.stdc.stdio;
import std.array;
import std.algorithm;
import std.conv;
import std.stdio;
import vm.dislib;

class Exit: Exception {
  this() {
    super(null);
  }
}

union Value {
  private int num;
  private Value* ptr;

  this(int num_) {
    num = num_;
  }

  this(typeof(null) n) {
    ptr = [Value(0)].ptr;
  }

  this(Value* ptr_) {
    ptr = ptr_;
  }

  this(char[] src) {
    this(cast(string) src);
  }

  this(string src) {
    Value[] va = [Value(cast(int) src.length)];
    foreach (chr; src) {
      va ~= Value(cast(int) chr);
    }
    ptr = va.ptr;
  }

  Value opBinary(string op)(Value other) {
    return Value(mixin("num"~op~"other.num"));
  }

  Value length() {
    return ptr[0];
  }

  ref Value opIndex(int n) {
    return ptr[n + 1];
  }

  string str() {
    string ret;
    foreach (i; 0..length.num) {
      ret ~= cast(char) this[i].num;
    }
    return ret;
  }

  const(char)* cstr() {
    char[] ret = new char[length.num + 1];
    foreach (i, ref c; ret) {
      c = cast(char) this[cast(int) i].num;
    }
    ret[length.num] = '\0';
    return ret.ptr;
  }
}


Block[] markAlive(ref int[] found, Block marking) {
  if (found.canFind(marking.start)) {
    return [];
  }
  found ~= marking.start;      
  Block[] ret = [marking];
  foreach (target; marking.targets) {
    ret ~= markAlive(found, target);
  }
  return ret;
}

Block[int] dce(Block start) {
  int[] found;
  Block[] blocks = markAlive(found, start);
  Block[int] ret;
  foreach (block; blocks) {
    ret[block.start] = block;
  }
  return ret;
}

size_t regUnalias(Instr[] input) {
  size_t count = 0;
  int[int] vals;
  foreach (instr; input) {
    if (instr.op.hasOut) {
      Reg to = cast(Reg) instr.data[0];
      if (to.reg in vals) {
        vals.remove(to.reg);
      }
    }
    foreach (ref arg; instr.data) {
      if (Reg reg = cast(Reg) arg) {
        if (int* regno = reg.reg in vals) {
          arg = new Reg(*regno);
          count += 1;
        }
      }
    }
    if (instr.op == Opcode.reg) {
      Reg to = cast(Reg) instr.data[0];
      Reg from = cast(Reg) instr.data[1];
      vals[to.reg] = from.reg;
    }
  }
  return count;
}

size_t constFold(ref Instr[] input) {
  Instr[] instrs;
  int[int] vals;
  size_t count = 0;
  void nextInstr(Instr next) {
    instrs ~= next;
    count += 1;
  }
  foreach (instr; input) {
    if (instr.op == Opcode.num) {
      Reg reg = cast(Reg) instr.data[0];
      Num num = cast(Num) instr.data[1];
      vals[reg.reg] = num.num;
      instrs ~= instr;
    } else if (instr.op == Opcode.blt) {
      Reg lhs = cast(Reg) instr.data[0];
      Reg rhs = cast(Reg) instr.data[1];
      int* lref = lhs.reg in vals;
      int* rref = rhs.reg in vals;
      if (lref is null) {
        if (rref is null) {
          instrs ~= instr;
        } else {
          nextInstr = new Instr(Opcode.blti, instr.data[0], new Num(*rref), instr.data[2], instr.data[3]);
        }
      } else {
        if (rref is null) {
          nextInstr = new Instr(Opcode.blti, instr.data[0], new Num(*lref), instr.data[3], instr.data[2]);
        } else {
          if (*lref < *rref) {
            nextInstr = new Instr(Opcode.jump, instr.data[3]);
          } else {
            nextInstr = new Instr(Opcode.jump, instr.data[2]);
          }
        }
      }
    } else if (instr.op == Opcode.reg) {
      Reg ireg = cast(Reg) instr.data[1];
      if (int* val = ireg.reg in vals) {
        vals[ireg.reg] = *val;
        nextInstr = new Instr(Opcode.num, instr.data[0], new Num(*val));
      } else {
        instrs ~= instr;
      }
    } else if (instr.op == Opcode.blte) {
      Reg lhs = cast(Reg) instr.data[0];
      Reg rhs = cast(Reg) instr.data[1];
      int* lref = lhs.reg in vals;
      int* rref = rhs.reg in vals;
      if (lref is null) {
        if (rref is null) {
          instrs ~= instr;
        } else {
          nextInstr = new Instr(Opcode.bltei, instr.data[0], new Num(*rref), instr.data[2], instr.data[3]);
        }
      } else {
        if (rref is null) {
          nextInstr = new Instr(Opcode.bltei, instr.data[0], new Num(*lref), instr.data[3], instr.data[2]);
        } else {
          if (*lref <= *rref) {
            nextInstr = new Instr(Opcode.jump, instr.data[3]);
          } else {
            nextInstr = new Instr(Opcode.jump, instr.data[2]);
          }
        }
      }
    } else if (instr.op == Opcode.beq) {
      Reg lhs = cast(Reg) instr.data[0];
      Reg rhs = cast(Reg) instr.data[1];
      int* lref = lhs.reg in vals;
      int* rref = rhs.reg in vals;
      if (lref is null) {
        if (rref is null) {
          instrs ~= instr;
        } else {
          nextInstr = new Instr(Opcode.beqi, instr.data[0], new Num(*rref), instr.data[2], instr.data[3]);
        }
      } else {
        if (rref is null) {
          nextInstr = new Instr(Opcode.beqi, instr.data[0], new Num(*lref), instr.data[3], instr.data[2]);
        } else {
          if (*lref == *rref) {
            nextInstr = new Instr(Opcode.jump, instr.data[3]);
          } else {
            nextInstr = new Instr(Opcode.jump, instr.data[2]);
          }
        }
      }
    } else if (instr.op == Opcode.add) {
      Reg lhs = cast(Reg) instr.data[1];
      Reg rhs = cast(Reg) instr.data[2];
      int* lref = lhs.reg in vals;
      int* rref = rhs.reg in vals;
      if (lref is null) {
        if (rref is null) {
          instrs ~= instr;
        } else {
          nextInstr = new Instr(Opcode.addi, instr.data[0], instr.data[1], new Num(*rref));
        }
      } else {
        if (rref is null) {
          nextInstr = new Instr(Opcode.addi, instr.data[0], instr.data[2], new Num(*lref));
        } else {
          nextInstr = new Instr(Opcode.num, instr.data[0], new Num(*lref + *rref));
        }
      }
    } else if (instr.op == Opcode.mul) {
      Reg lhs = cast(Reg) instr.data[1];
      Reg rhs = cast(Reg) instr.data[2];
      int* lref = lhs.reg in vals;
      int* rref = rhs.reg in vals;
      if (lref is null) {
        if (rref is null) {
          instrs ~= instr;
        } else {
          nextInstr = new Instr(Opcode.muli, instr.data[0], instr.data[1], new Num(*rref));
        }
      } else {
        if (rref is null) {
          nextInstr = new Instr(Opcode.muli, instr.data[0], instr.data[2], new Num(*lref));
        } else {
          nextInstr = new Instr(Opcode.num, instr.data[0], new Num(*lref * *rref));
        }
      }
    } else if (instr.op == Opcode.sub) {
      Reg lhs = cast(Reg) instr.data[1];
      Reg rhs = cast(Reg) instr.data[2];
      int* lref = lhs.reg in vals;
      int* rref = rhs.reg in vals;
      if (lref is null) {
        if (rref is null) {
          instrs ~= instr;
        } else {
          nextInstr = new Instr(Opcode.subi, instr.data[0], instr.data[1], new Num(*rref));
        }
      } else {
        if (rref is null) {
          instrs ~= instr;
        } else {
          nextInstr = new Instr(Opcode.num, instr.data[0], new Num(*lref - *rref));
        }
      }
    }  else if (instr.op == Opcode.div) {
      Reg lhs = cast(Reg) instr.data[1];
      Reg rhs = cast(Reg) instr.data[2];
      int* lref = lhs.reg in vals;
      int* rref = rhs.reg in vals;
      if (lref is null) {
        if (rref is null) {
          instrs ~= instr;
        } else {
          nextInstr = new Instr(Opcode.divi, instr.data[0], instr.data[1], new Num(*rref));
        }
      } else {
        if (rref is null) {
          instrs ~= instr;
        } else {
          nextInstr = new Instr(Opcode.num, instr.data[0], new Num(*lref / *rref));
        }
      }
    }  else if (instr.op == Opcode.mod) {
      Reg lhs = cast(Reg) instr.data[1];
      Reg rhs = cast(Reg) instr.data[2];
      int* lref = lhs.reg in vals;
      int* rref = rhs.reg in vals;
      if (lref is null) {
        if (rref is null) {
          instrs ~= instr;
        } else {
          nextInstr = new Instr(Opcode.modi, instr.data[0], instr.data[1], new Num(*rref));
        }
      } else {
        if (rref is null) {
          instrs ~= instr;
        } else {
          nextInstr = new Instr(Opcode.num, instr.data[0], new Num(*lref % *rref));
        }
      }
    } else {
      instrs ~= instr;
    }
  }
  input = instrs;
  return count;
}



size_t inlineJumps(Block[int] refs, ref Instr[] input) {
  size_t count = 0;
  while (true) {
    if (input.length == 0) {
      return count;
    }
    Instr last = input[$-1];
    if (last.op != Opcode.jump) {
      return count;
    }
    Loc jumpTo = cast(Loc) last.data[0];
    Instr[] other = refs[jumpTo.loc].instrs;
    input.length -= 1;
    input ~= other;
    count += 1;
  }
}

int[int] countRegUses(const Block[int] blocks) {
  int[int] uses;
  void use(const Arg arg) {
    if (Reg reg = cast(Reg) arg) {
      if (int* iptr = reg.reg in uses) {
        *iptr += 1;
      } else {
        uses[reg.reg] = 1;
      }
    }
    if (Regs regs = cast(Regs) arg) {
      foreach (reg; regs.regs) {
        if (int* iptr = reg.reg in uses) {
          *iptr += 1;
        } else {
          uses[reg.reg] = 1;
        }
      }
    }
  }
  foreach (block; blocks) {
    foreach (instr; block.instrs) {
      if (instr.data.length == 0) {
        continue;
      }
      if (!instr.op.hasOut) {
        use(instr.data[0]);
      } 
      foreach (arg; instr.data[1..$]) {
        use(arg);
      }
    }
  }
  return uses;
}

size_t removeUnusedRegs(const int[int] counts, ref Instr[] instrs) {
  size_t count = 0;
  Instr[] ret;
  foreach (instr; instrs) {
    if (instr.op.hasOut && !instr.op.isCall) {
      Reg reg = cast(Reg) instr.data[0];
      if (reg is null) {
        writeln(instr);
      }
      if (reg.reg !in counts) {
        count += 1;
        continue;
      }
    }
    ret ~= instr;
  }
  instrs = ret;
  return count;
}

class Block {
  int start;
  Instr[] instrs;
  Block[int] refs;
  this(){}

  Block[] targets() {
    if (instrs.length == 0) {
      return [];
    }
    Block[] ret;
    foreach (arg; instrs[$-1].data) {
      if (Loc loc = cast(Loc) arg) {
        if (Block* next = loc.loc in refs) {
          ret ~= *next;
        }
      }
    }
    return ret;
  }

  override string toString() {
    return "  l" ~ start.to!string ~ ":" ~ instrs.map!(x => "\n    " ~ x.to!string).join("");
  }
}

class Blocks {
  string name;
  int first;
  int argc;
  Block[int] blocks;
  this(string name_, int argc_, Instr[] instrs) {
    name = name_;
    argc = argc_;
    int[] pre;
    foreach (i, instr; instrs) {
      if (!instr.op.isBranch) {
        continue;
      }
      if (i + 1 < instrs.length) {
        pre ~= instrs[i+1].loc;
      }
      foreach (arg; instr.data) {
        if (Loc loc = cast(Loc) arg) {
          pre ~= loc.loc;
        }
      }
    }
    first = instrs[0].loc;
    Block cur = new Block;
    int last = first;
    foreach (instr; instrs) {
      if (pre.canFind(instr.loc)) {
        if (cur.instrs.length != 0) {
          blocks[last] = cur;
          Block next = new Block;
          if (!cur.instrs[$-1].op.isBranch) {
            cur.instrs ~= new Instr(Opcode.jump, new Loc(instr.loc));
          }
          cur = next;
        }
        last = instr.loc;
      } 
      cur.instrs ~= instr;
    }
    if (cur.instrs.length != 0) {
      blocks[last] = cur;
    }
    Block[int] newb;
    foreach (block; blocks) {
      block.refs = blocks;
      if (block.instrs.length != 0) {
        block.start = block.instrs[0].loc;
        newb[block.start] = block;
      }
    }
    blocks = newb;
  }

  void opt() {
    while (true) {
      blocks = blocks[first].dce;
      size_t count = 0;
      foreach (block; blocks) {
        count += regUnalias(block.instrs);
      }
      foreach (block; blocks) {
        count += constFold(block.instrs);
      }
      foreach (block; blocks) {
        count += inlineJumps(block.refs, block.instrs);
      }
      int[int] counts = countRegUses(blocks);
      foreach (block; blocks) {
        count += removeUnusedRegs(counts, block.instrs);
      }
      if (count == 0) {
        break;
      }
    }
  }

  Block[] orderedBlocks() {
    return blocks.keys.sort.map!(x => blocks[x]).array;
  }

  override string toString() {
    string ret;
    ret ~= name;
    ret ~= "(";
    ret ~= argc.to!string;
    ret ~= ") {";
    foreach (block; orderedBlocks) {
      ret ~= "\n";
      ret ~= block.to!string;
    }
    ret ~= "\n}";
    return ret;
  }
}

Instr[] optSingle(Instr[] func, Instr self) {
  return [self];
}

Instr[] optimize(Instr[] func) {
  Instr[] ret;
  foreach (instr; func) {
    if (instr.op == Opcode.func) {
      Func newfunc = cast(Func) instr.data[0];
      Blocks blocks = new Blocks(newfunc.name, newfunc.argc, newfunc.instrs);
      // writeln(blocks);
      blocks.opt;
      writeln(blocks);
    }
    ret ~= optSingle(func, instr);
  }
  return ret;
}

Opcode[] optimize(Opcode[] args) {
  const(int)[] ops = cast(const(int)[]) args.idup;
  Instr[] instrs = ops.parse;
  Instr[] faster = instrs.optimize;
  Blocks blocks = new Blocks("", 1, instrs);
  // writeln(blocks);
  return args;
}

Value vm_global_from(string[] args) {
  Value[] global = [Value(cast(int) args.length)];
  foreach (arg; args) {
    Value[] ent = [Value(cast(int) arg.length)];
    foreach (chr; arg) {
      ent ~= Value(chr);
    }
    global ~= Value(ent.ptr);
  }
  return Value(global.ptr);
}

Value vm_run_from(immutable(Opcode*) ops, int index, Value* locals,
                     Value* next_locals) {

  Opcode op = ops[index++];
  next:
  final switch (op) {
case Opcode.exit:
 throw new Exit;
case Opcode.reg:
  int to = ops[index++];
  int from = ops[index++];
  locals[to] = locals[from];
  op = ops[index++];
  goto next;
case Opcode.bb:
  int from = ops[index++];
  if (locals[from].num) {
    index = ops[index + 1];
    op = ops[index++];
    goto next;
  } else {
    index = ops[index];
    op = ops[index++];
    goto next;
  }
case Opcode.num:
  int to = ops[index++];
  int from = ops[index++];
  locals[to] = Value(from);
  op = ops[index++];
  goto next;
case Opcode.jump:
  index = ops[index];
  op = ops[index++];
  goto next;
case Opcode.func:
  index = ops[index];
  op = ops[index++];
  goto next;
case Opcode.add:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = locals[lhs] + locals[rhs];
  op = ops[index++];
  goto next;
case Opcode.sub:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = locals[lhs] - locals[rhs];
  op = ops[index++];
  goto next;
case Opcode.mul:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = locals[lhs] * locals[rhs];
  op = ops[index++];
  goto next;
case Opcode.div:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = locals[lhs] / locals[rhs];
  op = ops[index++];
  goto next;
case Opcode.mod:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = locals[lhs] % locals[rhs];
  op = ops[index++];
  goto next;
case Opcode.blte:
  Value lhs = locals[ops[index++]];
  Value rhs = locals[ops[index++]];
  if (lhs.num <= rhs.num) {
    index = ops[index + 1];
    op = ops[index++];
    goto next;
  } else {
    index = ops[index];
    op = ops[index++];
    goto next;
  }
case Opcode.call:
  int outreg = ops[index++];
  int next_func = ops[index++];
  int nargs = ops[index++];
  for (int argno = 1; argno <= nargs; argno++) {
    int regno = ops[index++];
    next_locals[argno] = locals[regno];
  }
  locals[outreg] = vm_run_from(ops, next_func, next_locals,
                               next_locals + ops[next_func - 1]);
  op = ops[index++];
  goto next;
case Opcode.ret:
  return locals[ops[index]];
case Opcode.putchar:
  int from = ops[index++];
  int val = locals[from].num;
  printf("%c", cast(char)val);
  op = ops[index++];
  goto next;
case Opcode.string:
  int outreg = ops[index++];
  int nargs = ops[index++];
  Value[] str = [Value(nargs)];
  for (size_t i = 0; i < nargs; i++) {
    int num = ops[index++];
    str ~= Value(num);
  }
  locals[outreg] = Value(str.ptr);
  op = ops[index++];
  goto next;
case Opcode.length:
  int outreg = ops[index++];
  Value vec = locals[ops[index++]];
  locals[outreg] = vec.length;
  op = ops[index++];
  goto next;
case Opcode.get:
  int outreg = ops[index++];
  Value vec = locals[ops[index++]];
  Value oindex = locals[ops[index++]];
  locals[outreg] = vec[oindex.num];
  op = ops[index++];
  goto next;
case Opcode.set:
  Value vec = locals[ops[index++]];
  Value oindex = locals[ops[index++]];
  Value value = locals[ops[index++]];
  vec[oindex.num] = value;
  op = ops[index++];
  goto next;
case Opcode.dump:
  int namreg = ops[index++];
  Value* sname = locals[namreg].ptr;
  int slen = sname[0].num;
  char[] name = new char[slen + 1];
  for (int i = 0; i < slen; i++) {
    name[i] = cast(char) sname[i+1].num;
  }
  name[slen] = '\0';
  Value* ent = locals[ops[index++]].ptr;
  int xlen = ent[0].num;
  FILE *rout = fopen(name.ptr, "wb");
  for (int i = 0; i < xlen; i++) {
    Value obj = ent[i+1];
    int opv = obj.num;
    fwrite(&opv, int.sizeof, 1, rout);
  }
  fclose(rout);
  op = ops[index++];
  goto next;
case Opcode.read:
  int outreg = ops[index++];
  int namreg = ops[index++];
  int where = 0;
  FILE *fin = fopen(locals[namreg].cstr, "rb");
  if (fin is null) {
    locals[outreg] = Value(null);
    op = ops[index++];
    goto next;
  }
  char[] str = null;
  while (true) {
    char[2048] buf;
    size_t n = fread(buf.ptr, 1, 2048, fin);
    for (int i = 0; i < n; i++) {
      str ~= cast(char) buf[i];
    }
    if (n < 2048) {
      break;
    }
  }
  fclose(fin);
  locals[outreg] = Value(str);
  op = ops[index++];
  goto next;
case Opcode.write:
  int outreg = ops[index++];
  int namreg = ops[index++];
  Value* sname = locals[namreg].ptr;
  int slen = sname[0].num;
  char[] name = new char[slen + 1];
  for (int i = 0; i < slen; i++) {
    name[i] = cast(char) sname[i+1].num;
  }
  name[slen] = '\0';
  Value* ent = locals[ops[index++]].ptr;
  int xlen = ent[0].num;
  FILE *rout = fopen(name.ptr, "wb");
  for (int i = 0; i < xlen; i++) {
    char chrv = cast(char) ent[i + 1].num;
    fwrite(&chrv, char.sizeof, 1, rout);
  }
  fclose(rout);
  op = ops[index++];
  goto next;
case Opcode.array:
  int outreg = ops[index++];
  int nargs = ops[index++];
  Value[] vec = [Value(nargs)];
  for (int i = 0; i < nargs; i++) {
    int vreg = ops[index++];
    vec ~= locals[vreg];
  }
  locals[outreg] = Value(vec.ptr);
  op = ops[index++];
  goto next;
case Opcode.cat:
  int to = ops[index++];
  Value* left = locals[ops[index++]].ptr;
  Value* right = locals[ops[index++]].ptr;
  Value[] ent = [Value(left[0].num + right[0].num)];
  for (int i = 0; i < left[0].num; i++) {
    ent ~= left[i+1];
  }
  for (int i = 0; i < right[0].num; i++) {
    ent ~= right[i+1];
  }
  locals[to] = Value(ent.ptr);
  op = ops[index++];
  goto next;
case Opcode.beq:
  Value lhs = locals[ops[index++]];
  Value rhs = locals[ops[index++]];
  if (lhs.num == rhs.num) {
    index = ops[index + 1];
    op = ops[index++];
    goto next;
  } else {
    index = ops[index];
    op = ops[index++];
    goto next;
  }
case Opcode.blt:
  Value lhs = locals[ops[index++]];
  Value rhs = locals[ops[index++]];
  if (lhs.num < rhs.num) {
    index = ops[index + 1];
    op = ops[index++];
    goto next;
  } else {
    index = ops[index];
    op = ops[index++];
    goto next;
  }
case Opcode.call0:
  int outreg = ops[index++];
  int next_func = ops[index++];
  locals[outreg] = vm_run_from(ops, next_func, next_locals, next_locals + ops[next_func - 1]);
  op = ops[index++];
  goto next;
case Opcode.call1:
  int outreg = ops[index++];
  int next_func = ops[index++];
  next_locals[1] = locals[ops[index++]];
  locals[outreg] = vm_run_from(ops, next_func, next_locals, next_locals + ops[next_func - 1]);
  op = ops[index++];
  goto next;
case Opcode.call2:
  int outreg = ops[index++];
  int next_func = ops[index++];
  next_locals[1] = locals[ops[index++]];
  next_locals[2] = locals[ops[index++]];
  locals[outreg] = vm_run_from(ops, next_func, next_locals, next_locals + ops[next_func - 1]);
  op = ops[index++];
  goto next;
case Opcode.call3:
  int outreg = ops[index++];
  int next_func = ops[index++];
  next_locals[1] = locals[ops[index++]];
  next_locals[2] = locals[ops[index++]];
  next_locals[3] = locals[ops[index++]];
  locals[outreg] = vm_run_from(ops, next_func, next_locals, next_locals + ops[next_func - 1]);
  op = ops[index++];
  goto next;
case Opcode.call4:
  int outreg = ops[index++];
  int next_func = ops[index++];
  next_locals[1] = locals[ops[index++]];
  next_locals[2] = locals[ops[index++]];
  next_locals[3] = locals[ops[index++]];
  next_locals[4] = locals[ops[index++]];
  locals[outreg] = vm_run_from(ops, next_func, next_locals, next_locals + ops[next_func - 1]);
  op = ops[index++];
  goto next;
case Opcode.call5:
  int outreg = ops[index++];
  int next_func = ops[index++];
  next_locals[1] = locals[ops[index++]];
  next_locals[2] = locals[ops[index++]];
  next_locals[3] = locals[ops[index++]];
  next_locals[4] = locals[ops[index++]];
  next_locals[5] = locals[ops[index++]];
  locals[outreg] = vm_run_from(ops, next_func, next_locals, next_locals + ops[next_func - 1]);
  op = ops[index++];
  goto next;
case Opcode.beqi:
  Value lhs = locals[ops[index++]];
  if (lhs.num < ops[index++]) {
    index = ops[index + 1];
    op = ops[index++];
    goto next;
  } else {
    index = ops[index];
    op = ops[index++];
    goto next;
  } 
case Opcode.blti:
  Value lhs = locals[ops[index++]];
  if (lhs.num < ops[index++]) {
    index = ops[index + 1];
    op = ops[index++];
    goto next;
  } else {
    index = ops[index];
    op = ops[index++];
    goto next;
  } 
case Opcode.bltei:
  Value lhs = locals[ops[index++]];
  if (lhs.num < ops[index++]) {
    index = ops[index + 1];
    op = ops[index++];
    goto next;
  } else {
    index = ops[index];
    op = ops[index++];
    goto next;
  } 
case Opcode.addi:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = locals[lhs] + Value(rhs);
  op = ops[index++];
  goto next;
case Opcode.subi:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = locals[lhs] - Value(rhs);
  op = ops[index++];
  goto next;
case Opcode.muli:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = locals[lhs] * Value(rhs);
  op = ops[index++];
  goto next;
case Opcode.divi:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = locals[lhs] / Value(rhs);
  op = ops[index++];
  goto next;
case Opcode.modi:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = locals[lhs] % Value(rhs);
  op = ops[index++];
  goto next;
case Opcode.geti:
  int outreg = ops[index++];
  Value vec = locals[ops[index++]];
  locals[outreg] = vec[ops[index++]];
  op = ops[index++];
  goto next;
case Opcode.seti:
  Value vec = locals[ops[index++]];
  int oindex = ops[index++];
  Value value = locals[ops[index++]];
  vec[oindex] = value;
  op = ops[index++];
  goto next;
case Opcode.nop:
  op = ops[index++];
  goto next;
}
}

int vm_run(Opcode[] ops, string[] args) {
  try {
    Value[] locals = new Value[1 << 16];
    locals[0] = vm_global_from(args);
    vm_run_from(ops.optimize.idup.ptr, 0, locals.ptr, locals.ptr + 256);
    return 0;
  } catch (Exit e) {
    return 1;
  }
}

int main(string[] args) {
  if (args.length < 2) {
    printf("cannot run vm: not enough args\n");
    return 1;
  }
  FILE *file = fopen((args[1] ~ "\0").ptr, "rb");
  if (file is null) {
    printf("cannot run vm: file to run could not be read\n");
    return 2;
  }
  int[] ops = null;
  while (true) {
    int op = 0;
    size_t size = fread(&op, int.sizeof, 1, file);
    if (size == 0) {
      break;
    }
    ops ~= op;
  }
  return vm_run(cast(Opcode[]) ops, args[2..$]);
}
