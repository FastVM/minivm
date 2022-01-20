import vm.ctfelib;

import vm.dislib;

import std.stdio;
import std.array;
import std.algorithm;
import std.ascii;
import std.range;
import std.conv;

enum bool wantNames = false;

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
        if (wantNames && func.name.map!(x => x.isAlphaNum || x == '_').all) {
            doAlias = "alias " ~ func.name ~ "=l" ~ func.loc.to!string ~ ";";
        }
        return "private Value l" ~ func.loc.to!string ~ "(" ~ regv ~ "){Value r0;" ~ fbody ~ "}" ~ doAlias;
    }
    throw new Exception("cannot arg");
}

string mconvert(const Instr instr) {
    switch (instr.op) {
        default:
            throw new Exception("cannot opcode");
        case Opcode.exit:
            return "core.stdc.stdlib.exit(1);assert(false);";
        case Opcode.reg:
            return instr.data[0].convert ~ "=" ~ instr.data[1].convert ~ ";";
        case Opcode.bb:
            return "if(" ~ instr.data[0].convert ~ ".num){goto " ~ instr.data[2].convert ~ ";}else{goto " ~ instr.data[1].convert ~ ";}";
        case Opcode.num:
            return instr.data[0].convert ~ "=" ~ instr.data[1].convert ~ ".toValue;";
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
            return "core.stdc.stdio.putchar(cast(int)" ~ instr.data[0].convert ~".num);";
        case Opcode.string:
            Nums nums = cast(Nums) instr.data[1];
            if (nums.nums.length == 0) {
            return instr.data[0].convert ~ "=emptyValuePtr.toValue;";
            }
            return instr.data[0].convert ~ "=toArray([" ~ nums.nums.map!(num => num.to!string ~ ".toValue").join(',') ~ "]);";
        case Opcode.length:
            return instr.data[0].convert ~ "=" ~ instr.data[1].convert ~ ".length;";
        case Opcode.get:
            return instr.data[0].convert ~ "=" ~ instr.data[1].convert ~ "[" ~ instr.data[2].convert ~ ".num];";
        case Opcode.set:
            return instr.data[0].convert ~ "[" ~ instr.data[1].convert ~ ".num]=" ~ instr.data[2].convert ~ ";";
        case Opcode.dump:
            return instr.data[1].convert ~ ".dumpFile(" ~ instr.data[0].convert ~ ");";
        case Opcode.read:
            return instr.data[0].convert ~ "=" ~ instr.data[1].convert ~ ".readFile;";
        case Opcode.write:
            return instr.data[1].convert ~ ".writeFile(" ~ instr.data[0].convert ~ ");";
        case Opcode.array:
            Regs regs = cast(Regs) instr.data[1];
            if (regs.regs.length == 0) {
                return instr.data[0].convert ~ "=emptyValuePtr.toValue;";
            }
            return instr.data[0].convert ~ "=toArray([" ~ regs.regs.map!(to!string).join(',') ~ "]);";
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
}

string genregs(int nregs) {
    return "Value " ~ iota(1, nregs).map!(nreg => "r" ~ nreg.to!string).join(',') ~ ";";
}

static const prelude = q{
static import core.stdc.stdio;
static import core.stdc.stdlib;
static import core.stdc.string;
static import std.file;

alias allocate = core.stdc.stdlib.malloc;
alias reallocate = core.stdc.stdlib.realloc;

pragma(inline, true) Value toArray(size_t n)(Value[n] vals) {
    // Value[] ret = new Value[n + 1];
    Value* ret = cast(Value*) allocate(Value.sizeof * (n + 1));
    ret[0] = toValue(cast(ptrdiff_t) n);
    static foreach (ind; 0..n) {
        ret[ind + 1] = vals[ind];
    }
    return toValue(ret);
}

pragma(inline, true) Value toValue(Arg)(Arg arg) {
    return Value(arg);
}

__gshared Value zeroValue = Value(0);
__gshared Value* emptyValuePtr = &zeroValue;

union Value {    
pragma(inline, true):
    ptrdiff_t num;
    Value* ptr;

    this(ptrdiff_t num_) {
        num = num_;
    }

    this(Value* ptr_) {
        ptr = ptr_ + 1;
    }

    this(const(char)[] src) {
        this(cast(string) src);
    }

    this(string src) {
        Value* ret = cast(Value*) allocate(Value.sizeof * (src.length + 1));
        ret[0] = toValue(cast(ptrdiff_t) src.length);
        foreach (i, chr; src) {
            ret[i+1] = toValue(cast(ptrdiff_t) chr);
        }
        this(ret);
    }

    Value opBinary(string op)(Value other) {
        return toValue(mixin("num"~op~"other.num"));
    }

    Value opBinary(string op: "~")(Value other) {
        size_t lenSelf = length.num;
        size_t lenOther = other.length.num;
        size_t total = lenSelf + lenOther;
        Value* ret = cast(Value*) allocate(Value.sizeof * (total + 1));
        ret[0] = toValue(cast(ptrdiff_t) total);
        foreach (i; 0..lenSelf) {
            ret[i + 1] = this[i];
        }
        foreach (i; 0..lenOther) {
            ret[lenSelf + i + 1] = other[i];
        }
        return toValue(ret);
    }

    Value length() {
        return ptr[-1];
    }

    ref Value opIndex(ptrdiff_t n) {
        return ptr[n];
    }

    Value readFile() {
        core.stdc.stdio.FILE* infile = core.stdc.stdio.fopen(str, "r");
        size_t alloc = 1024; 
        size_t end = 1;
        Value* ret = cast(Value*) allocate(Value.sizeof * alloc);
        while (true) {
            char[256] buf;
            size_t n = core.stdc.stdio.fread(&buf, 1, 256, infile);
            if (end + 260 >= alloc) {
                alloc *= 4;
                ret = cast(Value*) reallocate(ret, Value.sizeof * alloc);
            }
            foreach (chr; buf[0..n]) {
                ret[end++] = toValue(cast(ptrdiff_t) chr);
            }
            if (n != 256) {
                ret[0] = toValue(cast(ptrdiff_t) end);
                return toValue(ret);
            }
        }
    }

    void dumpFile(Value name) {
        char* strname = name.str;
        core.stdc.stdio.FILE* outfile = core.stdc.stdio.fopen(strname, "wb"); 
        foreach (i; 0..length.num) {
            int ival = cast(int) this[i].num;
            core.stdc.stdio.fwrite(&ival, int.sizeof, 1, outfile);
        }
        core.stdc.stdio.fclose(outfile);
    }


    void writeFile(Value name) {
        char* strname = name.str;
        core.stdc.stdio.FILE* outfile = core.stdc.stdio.fopen(strname, "w"); 
        foreach (i; 0..length.num) {
            char ival = cast(char) this[i].num;
            core.stdc.stdio.fwrite(&ival, char.sizeof, 1, outfile);
        }
        core.stdc.stdio.fclose(outfile);
    }

    char* str() {
        char* ret = cast(char*) core.stdc.stdlib.malloc(char.sizeof * (length.num + 1));
        foreach (i; 0..length.num) {
            ret[i] = cast(char) this[i].num;
        }
        ret[length.num] = '\0';
        return ret;
    }
}
};

string convert(Instr[] instrs) {
    return prelude ~ instrs.filter!(x => x.op == Opcode.func).map!mconvert.array.join("") ~ "Value vm_run(Value r0){" ~ genregs(256) ~ instrs.filter!(x => x.op != Opcode.func).map!convert.array.join("") ~ "}";
}

template Convert(string file) {
    static const string bc = import(file);
    static const Instr[] instrs = bc.parseStr;
    static const output = instrs.convert;
}

template ImportVM(string file) {
    static const output = Convert!file.output;
    mixin(output);
    Value vm_global_from(string[] args) {
        Value[] global = [toValue(cast(int) args.length)];
        foreach (arg; args) {
            Value[] ent = [toValue(cast(int) arg.length)];
            foreach (chr; arg) {
                ent ~= toValue(chr);
            }
            global ~= toValue(ent.ptr);
        }
        return toValue(global.ptr);
    }
    Value ImportVM(string[] args) {
        return vm_run(vm_global_from(args));
    }
}
