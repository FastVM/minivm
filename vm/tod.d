import vm.ctfelib;

import vm.dislib;
import std.file;
import std.stdio;

void main(string[] args) {
    int[] bc = cast(int[]) args[1].read;
    Instr[] instrs = bc.parse;
    string output = instrs.convert;
    writeln(output, q{
    Value vm_global_from(const(char*)[] args) {
        Value* global = cast(Value*) allocate(Value.sizeof * (args.length + 1));
        global[0] = toValue(cast(ptrdiff_t) args.length);
        foreach (ind, arg; args) {
            global[ind + 1] = toValue(arg[0..core.stdc.string.strlen(arg)]);
        }
        return toValue(global);
    }
    extern(C) int main(int argc, const(char*) * argv) {
        return cast(int) vm_run(vm_global_from(argv[1..argc])).num;
    }
});
}
