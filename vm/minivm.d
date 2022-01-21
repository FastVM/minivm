
module vm.run;

import core.stdc.stdio;

class Exit: Exception {
  this() {
    super(null);
  }
}

enum Opcode: int {
  exit = 0,
  store_reg = 1,
  branch_bool = 2,
  store_int = 3,
  jump = 4,
  func = 5,
  add = 6,
  sub = 7,
  mul = 8,
  div = 9,
  mod = 10,
  blte = 11,
  call = 12,
  return_ = 13,
  putchar = 14,
  string = 15,
  length = 16,
  get = 17,
  set = 18,
  dump = 19,
  read = 20,
  write = 21,
  array = 22,
  cat = 23,
  beq = 24,
  blt = 25,
  // NEW OPS!!! 
  addi = 26,
  subi = 27,
  muli = 28,
  divi = 29,
  modi = 30,
  geti = 31,
  seti = 32,
  bltei = 33,
  beqi = 34,
  blti = 35,
  call0 = 36,
  call1 = 37,
  call2 = 38,
  call3 = 39,
}

union Value {
  private ptrdiff_t num;
  private Value* ptr;

  this(ptrdiff_t num_) {
    num = num_;
  }

  this(typeof(null) n) {
    ptr = [Value(0)].ptr;
  }

  this(Value* ptr_) {
    ptr = ptr_;
  }

  this(ubyte[] src) {
    Value[] va = [Value(cast(ptrdiff_t) src.length)];
    foreach (chr; src) {
      va ~= Value(cast(ptrdiff_t) chr);
    }
    this(va.ptr);
  }

  this(char[] src) {
    this(cast(string) src);
  }

  this(string src) {
    Value[] va = [Value(cast(ptrdiff_t) src.length)];
    foreach (chr; src) {
      va ~= Value(cast(ptrdiff_t) chr);
    }
    this(va.ptr);
  }

  Value opBinary(string op)(Value other) {
    return Value(mixin("num"~op~"other.num"));
  }

  Value length() {
    return ptr[0];
  }

  ref Value opIndex(ptrdiff_t n) {
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
      c = cast(char) this[cast(ptrdiff_t) i].num;
    }
    ret[length.num] = '\0';
    return ret.ptr;
  }
}

Value vm_global_from(string[] args) {
  Value[] global = [Value(cast(ptrdiff_t) args.length)];
  foreach (arg; args) {
    Value[] ent = [Value(cast(ptrdiff_t) arg.length)];
    foreach (chr; arg) {
      ent ~= Value(chr);
    }
    global ~= Value(ent.ptr);
  }
  return Value(global.ptr);
}

Value vm_run_from(immutable(Opcode*) ops, int index, Value* locals,
                     Value* next_locals) {
  next:
  // printf("%i: %i\n", index, ops[index]);
  final switch (ops[index++]) {
case Opcode.exit:
 throw new Exit;
case Opcode.return_:
 return locals[ops[index]];
case Opcode.branch_bool:
  int from = ops[index++];
  if (locals[from].num) {
    index = ops[index + 1];
    goto next;
  } else {
    index = ops[index];
    goto next;
  }
case Opcode.store_reg:
  int to = ops[index++];
  int from = ops[index++];
  locals[to] = locals[from];
  goto next;
case Opcode.store_int:
  int to = ops[index++];
  int from = ops[index++];
  locals[to] = Value(from);
  goto next;
case Opcode.jump:
case Opcode.func:
  index = ops[index];
  goto next;
case Opcode.add:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = locals[lhs] + locals[rhs];
  goto next;
case Opcode.sub:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = locals[lhs] - locals[rhs];
  goto next;
case Opcode.mul:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = locals[lhs] * locals[rhs];
  goto next;
case Opcode.div:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = locals[lhs] / locals[rhs];
  goto next;
case Opcode.mod:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = locals[lhs] % locals[rhs];
  goto next;
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
  goto next;
case Opcode.putchar:
  int from = ops[index++];
  ptrdiff_t val = locals[from].num;
  printf("%c", cast(char)val);
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
  goto next;
case Opcode.length:
  int outreg = ops[index++];
  Value vec = locals[ops[index++]];
  locals[outreg] = vec.length;
  goto next;
case Opcode.get:
  int outreg = ops[index++];
  Value vec = locals[ops[index++]];
  Value oindex = locals[ops[index++]];
  locals[outreg] = vec[oindex.num];
  goto next;
case Opcode.set:
  Value vec = locals[ops[index++]];
  Value oindex = locals[ops[index++]];
  Value value = locals[ops[index++]];
  vec[oindex.num] = value;
  goto next;
case Opcode.dump:
  int namreg = ops[index++];
  Value* sname = locals[namreg].ptr;
  ptrdiff_t slen = sname[0].num;
  char[] name = new char[slen + 1];
  for (int i = 0; i < slen; i++) {
    name[i] = cast(char) sname[i+1].num;
  }
  name[slen] = '\0';
  Value* ent = locals[ops[index++]].ptr;
  ptrdiff_t xlen = ent[0].num;
  FILE *rout = fopen(name.ptr, "wb");
  for (int i = 0; i < xlen; i++) {
    Value obj = ent[i+1];
    int op = cast(int) obj.num;
    fwrite(&op, int.sizeof, 1, rout);
  }
  fclose(rout);
  goto next;
case Opcode.read:
  int outreg = ops[index++];
  int namreg = ops[index++];
  int where = 0;
  FILE *fin = fopen(locals[namreg].cstr, "rb");
  if (fin is null) {
    locals[outreg] = Value(null);
    goto next;
  }
  ubyte[] str = null;
  while (true) {
    ubyte[2048] buf;
    size_t n = fread(buf.ptr, 1, 2048, fin);
    for (int i = 0; i < n; i++) {
      str ~= cast(ubyte) buf[i];
    }
    if (n < 2048) {
      break;
    }
  }
  fclose(fin);
  locals[outreg] = Value(str);
  goto next;
case Opcode.write:
  int outreg = ops[index++];
  int namreg = ops[index++];
  Value* sname = locals[namreg].ptr;
  ptrdiff_t slen = sname[0].num;
  char[] name = new char[slen + 1];
  for (int i = 0; i < slen; i++) {
    name[i] = cast(char) sname[i+1].num;
  }
  name[slen] = '\0';
  Value* ent = locals[ops[index++]].ptr;
  ptrdiff_t xlen = ent[0].num;
  FILE *rout = fopen(name.ptr, "wb");
  for (int i = 0; i < xlen; i++) {
    char op = cast(char) ent[i + 1].num;
    fwrite(&op, 1, char.sizeof, rout);
  }
  fclose(rout);
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
  goto next;
case Opcode.beq:
  Value lhs = locals[ops[index++]];
  Value rhs = locals[ops[index++]];
  if (lhs.num == rhs.num) {
    index = ops[index + 1];

    goto next;
  } else {
    index = ops[index];
    goto next;
  }
case Opcode.blt:
  Value lhs = locals[ops[index++]];
  Value rhs = locals[ops[index++]];
  if (lhs.num < rhs.num) {
    index = ops[index + 1];
    goto next;
  } else {
    index = ops[index];
    goto next;
  }
case Opcode.blte:
  Value lhs = locals[ops[index++]];
  Value rhs = locals[ops[index++]];
  if (lhs.num <= rhs.num) {
    index = ops[index + 1];
    goto next;
  } else {
    index = ops[index];
    goto next;
  }
case Opcode.addi:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = Value(locals[lhs].num + rhs);
  goto next;
case Opcode.subi:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = Value(locals[lhs].num - rhs);
  goto next;
case Opcode.muli:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = Value(locals[lhs].num * rhs);
  goto next;
case Opcode.divi:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = Value(locals[lhs].num / rhs);
  goto next;
case Opcode.modi:
  int to = ops[index++];
  int lhs = ops[index++];
  int rhs = ops[index++];
  locals[to] = Value(locals[lhs].num % rhs);
  goto next;
case Opcode.geti:
  int outreg = ops[index++];
  Value vec = locals[ops[index++]];
  int iindex = ops[index++];
  locals[outreg] = vec[iindex];
  goto next;
case Opcode.seti:
  Value vec = locals[ops[index++]];
  int oiindex = ops[index++];
  Value value = locals[ops[index++]];
  vec[oiindex] = value;
  goto next;
case Opcode.bltei:
  Value lhs = locals[ops[index++]];
  if (lhs.num <= ops[index++]) {
    index = ops[index + 1];
    goto next;
  } else {
    index = ops[index];
    goto next;
  }
case Opcode.beqi:
  Value lhs = locals[ops[index++]];
  if (lhs.num == ops[index++]) {
    index = ops[index + 1];
    goto next;
  } else {
    index = ops[index];
    goto next;
  }
case Opcode.blti:
  Value lhs = locals[ops[index++]];
  if (lhs.num < ops[index++]) {
    index = ops[index + 1];
    goto next;
  } else {
    index = ops[index];
    goto next;
  }
case Opcode.call0:
  int outreg = ops[index++];
  int next_func = ops[index++];
  locals[outreg] = vm_run_from(ops, next_func, next_locals,
                               next_locals + ops[next_func - 1]);
  goto next;
case Opcode.call1:
  int outreg = ops[index++];
  int next_func = ops[index++];
  next_locals[1] = locals[ops[index++]];
  locals[outreg] = vm_run_from(ops, next_func, next_locals,
                               next_locals + ops[next_func - 1]);
  goto next;
case Opcode.call2:
  int outreg = ops[index++];
  int next_func = ops[index++];
  next_locals[1] = locals[ops[index++]];
  next_locals[2] = locals[ops[index++]];
  locals[outreg] = vm_run_from(ops, next_func, next_locals,
                               next_locals + ops[next_func - 1]);
  goto next;
case Opcode.call3:
  int outreg = ops[index++];
  int next_func = ops[index++];
  next_locals[1] = locals[ops[index++]];
  next_locals[2] = locals[ops[index++]];
  next_locals[3] = locals[ops[index++]];
  locals[outreg] = vm_run_from(ops, next_func, next_locals,
                               next_locals + ops[next_func - 1]);
  goto next;
}
}

int vm_run(Opcode[] ops, string[] args) {
  try {
    Value[] locals = new Value[1 << 16];
    locals[0] = vm_global_from(args);
    vm_run_from(ops.idup.ptr, 0, locals.ptr, locals.ptr + 256);
    return 0;
  } catch (Exit e) {
    return 1;
  }
}

int main(string[] args) {
  if (args.length < 2) {
    stderr.fprintf("cannot run vm: not enough args\n");
    return 1;
  }
  FILE *file = fopen((args[1] ~ "\0").ptr, "rb");
  if (file is null) {
    stderr.fprintf("cannot run vm: file to run could not be read\n");
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
