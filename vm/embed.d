import vm.ctfelib;

void main(string[] args) {
    ImportVM!"boot.bc"(args[1..$]);
}
