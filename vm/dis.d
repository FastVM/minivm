
import vm.dislib;

import std.stdio;

int main(string[] args) {
  if (args.length < 2) {
    stderr.writeln("cannot dis minivm: not enough args");
    return 1;
  }
  FILE *file = fopen((args[1] ~ "\0").ptr, "rb");
  if (file is null) {
    stderr.writeln("cannot dis vm: file to run could not be read");
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
  Instr[] instrs = ops.parse;
  foreach (instr; instrs) {
    writeln(instr);
  }
  return 0;
}
