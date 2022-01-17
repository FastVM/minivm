import vm.ctfelib;

import vm.dislib;

import std.stdio;
import std.array;
import std.algorithm;
import std.ascii;
import std.range;
import std.conv;

int readInt(ref string src) {
    int ival = (src[3] << 24) + (src[2] << 16) + (src[1] << 8) + (src[0] << 0);
    src = src[4..$];
    return ival;
}

int[] toInts(string src) {
    int[] ret;
    while (src.length >= 4) {
        ret ~= src.readInt;
    }
    return ret;
}

Instr[] parseStr(string src) {
    return src.toInts.parse;
}

string convert(const Arg arg) {
    if (Num num = cast(Num) arg) {
        return num.to!string;
    }
    if (Reg reg = cast(Reg) arg) {
        return reg.to!string;
    }
    if (Loc loc = cast(Loc) arg) {
        return loc.to!string;
    }
    if (Nums nums = cast(Nums) arg) {
        return nums.to!string;
    }
    if (Regs regs = cast(Regs) arg) {
        return regs.to!string;
    }
    if (Func func = cast(Func) arg) {
        string regv = iota(1, func.nregs + 1).map!(nreg => "Value r" ~ nreg.to!string ~ "=Value.init").join(',');
        string fbody = func.instrs.map!convert.array.join("");
        string doAlias;
        if (func.name.map!(x => x.isAlphaNum || x == '_').all) {
            doAlias = "alias " ~ func.name ~ "=l" ~ func.loc.to!string ~ ";";
        }
        return "private Value l" ~ func.loc.to!string ~ "(" ~ regv ~ "){Value r0;" ~ fbody ~ "}" ~ doAlias;
    }
    throw new Exception("cannot arg");
}

string mconvert(const Instr instr) {
    final switch (instr.op) {
        case Opcode.exit:
            return "exit(1);assert(false);";
        case Opcode.reg:
            return instr.data[0].convert ~ "=" ~ instr.data[1].convert ~ ";";
        case Opcode.bb:
            return "if(" ~ instr.data[0].convert ~ ".num){goto " ~ instr.data[2].convert ~ ";}else{goto " ~ instr.data[1].convert ~ ";}";
        case Opcode.num:
            return instr.data[0].convert ~ "=Value(" ~ instr.data[1].convert ~ ");";
        case Opcode.jump:
            return "goto " ~ instr.data[0].convert ~ ";";
        case Opcode.func:
            return instr.data[0].convert;
        case Opcode.add:
            return instr.data[0].convert ~ "=" ~ instr.data[1].convert ~ "+" ~ instr.data[2].convert ~ ";";
        case Opcode.sub:
            return instr.data[0].convert ~ "=" ~ instr.data[1].convert ~ "-" ~ instr.data[2].convert ~ ";";
        case Opcode.mul:
            return instr.data[0].convert ~ "=" ~ instr.data[1].convert ~ "*" ~ instr.data[2].convert ~ ";";
        case Opcode.div:
            return instr.data[0].convert ~ "=" ~ instr.data[1].convert ~ "/" ~ instr.data[2].convert ~ ";";
        case Opcode.mod:
            return instr.data[0].convert ~ "=" ~ instr.data[1].convert ~ "%" ~ instr.data[2].convert ~ ";";
        case Opcode.blte:
            return "if(" ~ instr.data[0].convert ~ ".num<=" ~ instr.data[1].convert ~ ".num){goto " ~ instr.data[3].convert ~ ";}else{goto " ~ instr.data[2].convert ~ ";}";
        case Opcode.call:
            Regs regs = cast(Regs) instr.data[2];
            return instr.data[0].convert ~ "=" ~ instr.data[1].convert ~ "(" ~ regs.regs.map!(to!string).join(',') ~ ");";
        case Opcode.ret:
            return "return " ~ instr.data[0].convert ~ ";";
        case Opcode.putchar:
            return "std.stdio.write(cast(char)" ~ instr.data[0].convert ~".num);";
        case Opcode.string:
            Nums nums = cast(Nums) instr.data[1];
            return instr.data[0].convert ~ "=Value.arr([" ~ nums.nums.map!(num => "Value(" ~ num.to!string ~ ")").join(',') ~ "]);";
        case Opcode.length:
            return instr.data[0].convert ~ "=" ~ instr.data[1].convert ~ ".length;";
        case Opcode.get:
            return instr.data[0].convert ~ "=" ~ instr.data[1].convert ~ "[" ~ instr.data[2].convert ~ ".num];";
        case Opcode.set:
            return instr.data[0].convert ~ "[" ~ instr.data[1].convert ~ ".num]=" ~ instr.data[2].convert ~ ";";
        case Opcode.dump:
            return "std.file.write(" ~ instr.data[0].convert ~ ".str," ~ instr.data[1].convert ~ ".ints);";
        case Opcode.read:
            return instr.data[0].convert ~ "=Value(std.file.readText(" ~ instr.data[1].convert ~ ".str));";
        case Opcode.write:
            return "std.file.write(" ~ instr.data[0].convert ~ ".str," ~ instr.data[1].convert ~ ".str);";
        case Opcode.array:
            Regs regs = cast(Regs) instr.data[1];
            return instr.data[0].convert ~ "=Value.arr([" ~ regs.regs.map!(to!string).join(',') ~ "]);";
        case Opcode.cat:
            return instr.data[0].convert ~ "=" ~ instr.data[1].convert ~ "~" ~ instr.data[2].convert ~ ";";
        case Opcode.beq:
            return "if (" ~ instr.data[0].convert ~ ".num==" ~ instr.data[1].convert ~ ".num){goto " ~ instr.data[3].convert ~ ";}else{goto " ~ instr.data[2].convert ~ ";}";
        case Opcode.blt:
            return "if (" ~ instr.data[0].convert ~ ".num<" ~ instr.data[1].convert ~ ".num){goto " ~ instr.data[3].convert ~ ";}else{goto " ~ instr.data[2].convert ~ ";}";
    }
}

string convert(const Instr instr) {
    return "l" ~ instr.loc.to!string ~ ": " ~ instr.mconvert;
    // return "l" ~ instr.loc.to!string ~ ": " ~ "writeln(" ~  instr.loc.to!string ~ ");" ~ instr.mconvert;
}

string genregs(int nregs) {
    return "Value " ~ iota(1, nregs).map!(nreg => "r" ~ nreg.to!string).join(',') ~ ";";
}

const prelude = `
import core.stdc.stdlib;
static import std.file;
static import std.stdio;

union Value {
    int num;
    Value* ptr;

    this(int num_) {
        num = num_;
    }

    this(typeof(null) n) {
        ptr = [Value(0)].ptr;
    }

    this(Value* ptr_) {
        ptr = ptr_;
    }

    static Value arr(Value[] vals) {
        return Value((Value(cast(int) vals.length) ~ vals).ptr);
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

    Value opBinary(string op: "~")(Value other) {
        Value[] va = [Value(cast(int) (length.num + other.length.num))];
        foreach (i; 0..length.num) {
            va ~= this[i];
        }
        foreach (i; 0..other.length.num) {
            va ~= other[i];
        }
        return Value(va.ptr);
    }

    Value length() {
        return ptr[0];
    }

    ref Value opIndex(int n) {
        return ptr[n + 1];
    }

    int[] ints() {
        int[] ret;
        foreach (i; 0..length.num) {
            ret ~= this[i].num;
        }
        return ret;
    }

    string str() {
        string ret;
        foreach (i; 0..length.num) {
            ret ~= cast(char) this[i].num;
        }
        return ret;
    }
}
`;

template Convert(string file) {
    static const string bc = import(file);
    static const Instr[] instrs = bc.parseStr;
    static const output = prelude ~ instrs.filter!(x => x.op == Opcode.func).map!mconvert.array.join("") ~ "Value vm_run(Value r0){" ~ genregs(256) ~ instrs.filter!(x => x.op != Opcode.func).map!convert.array.join("") ~ "}";
}

template ImportVM(string file) {
    static const output = Convert!file.output;
    mixin(output);
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
    Value ImportVM(string[] args) {
        return vm_run(vm_global_from(args));
    }
}
