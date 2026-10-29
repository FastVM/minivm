#!/usr/bin/env python3
# Differential fuzzer against a reference Lua. Generates seeded random
# programs whose output is fully deterministic, runs both, reduces any
# disagreement to a minimal program, and groups by failure signature.
#
#   python3 test/fuzz/run.py                  1000 cases from seed 0
#   python3 test/fuzz/run.py -n 5000 -s 7     more cases, different seed
#   python3 test/fuzz/run.py --lua lua5.4     different reference
#
# Reduced repros land in test/fuzz/out/ named after their signature.
import argparse
import os
import random
import re
import subprocess
import sys

base = os.path.dirname(os.path.abspath(__file__))
minivm = os.path.join(base, "..", "..", "build", "bin", "minivm")
out_dir = os.path.join(base, "out")

NAMES = ["a", "b", "c", "d", "e", "f", "g", "h"]
INT_BIN = ["+", "-", "*", "//", "%", "&", "|", "~", "<<", ">>"]
NUM_BIN = ["+", "-", "*", "/", "//", "%", "^"]
CMP = ["<", "<=", ">", ">=", "==", "~="]

INTS = [
    0, 1, 2, 3, 7, 10, 63, 64, 255, 256, 1000, -1, -2, -7, -256,
    2**31 - 1, -(2**31), 2**53, 2**53 + 1, 2**62, -(2**62),
    2**63 - 1, -(2**63), 2**63, -(2**63) - 1,
]

FLOATS = [
    "0.0", "-0.0", "1.0", "-1.0", "0.5", "-0.5", "1e10", "1e-10", "1e308",
    "2.5", "3.14159", "0.1", "1/0", "-1/0", "2^53", "2^63", "-(2^63)",
    "1e15", "1e16", "0/0",
]

STRS = [
    "", "a", "abc", "Hello", "  pad  ", "x\\ty", "0", "10", "3.5", "-7",
    "nan", "inf", "\\n", "%d", "a.b", "\\0mid", "ABC", "z", "0x10", "  12  ",
    "aaa", "aba", "%%", "[]", "()", "1e2", "  -0x1p4  ",
    "\\255\\254", "\\1\\0\\2", "ss", "Ss", "  ", "\\r\\n", "0.0", "-0",
]

PATS = [
    "a", "%a+", "%d", "[abc]", "^a", "b$", "(a)(b)", "%s", ".-", "a*",
    "%w+", "[^a]", "a?", "%b()", "()a", "(.)%1", "%f[%a]%a+", ".*", "[%d%s]",
]


class Gen:
    def __init__(self, rng):
        self.rng = rng
        self.scope = []
        self.funcs = []

    def pick(self, seq):
        return self.rng.choice(seq)

    def int_lit(self):
        return str(self.pick(INTS))

    def num_lit(self):
        return self.pick(FLOATS)

    def str_lit(self):
        return '"' + self.pick(STRS) + '"'

    def var(self):
        if self.scope and self.rng.random() < 0.7:
            return self.pick(self.scope)
        return None

    def int_expr(self, d=0):
        r = self.rng.random()
        v = self.var()
        if d > 2 or r < 0.3:
            if v is not None and r < 0.15:
                return f"m({v})"
            return self.int_lit()
        if r < 0.78:
            op = self.pick(INT_BIN)
            a = self.int_expr(d + 1)
            b = self.int_expr(d + 1)
            if op in ("//", "%"):
                b = f"({b} | 1)"
            if op in ("<<", ">>"):
                b = f"({b} & 63)"
            return f"({a} {op} {b})"
        if r < 0.88:
            return f"(~ {self.int_expr(d + 1)})"
        return f"(- {self.int_expr(d + 1)})"

    def num_expr(self, d=0):
        r = self.rng.random()
        if d > 2 or r < 0.4:
            return self.num_lit()
        if r < 0.9:
            op = self.pick(NUM_BIN)
            return f"({self.num_expr(d+1)} {op} {self.num_expr(d+1)})"
        return f"(- {self.num_expr(d + 1)})"

    def mixed_expr(self, d=0):
        # deliberately crosses int and float: the conversion rules at the
        # int64 boundary are where the two implementations disagree
        a = self.int_expr(d)
        b = self.num_expr(d)
        op = self.pick(NUM_BIN + ["&", "|", "~"])
        if op in ("&", "|", "~"):
            return f"(({a}) {op} math.tointeger({b}) )" if self.rng.random() < 0.5 else f"({a} {op} {a})"
        return f"({a} {op} {b})"

    def str_expr(self, d=0):
        r = self.rng.random()
        if d > 2 or r < 0.5:
            return self.str_lit()
        if r < 0.7:
            return f"({self.str_expr(d+1)} .. {self.str_expr(d+1)})"
        fn = self.pick(["string.upper", "string.lower", "string.reverse"])
        return f"{fn}({self.str_expr(d+1)})"

    def cond(self):
        k = self.rng.random()
        if k < 0.4:
            return f"{self.int_expr()} {self.pick(CMP)} {self.int_expr()}"
        if k < 0.6:
            return f"{self.num_expr()} {self.pick(CMP)} {self.num_expr()}"
        if k < 0.8:
            return f"{self.int_expr()} {self.pick(CMP)} {self.num_expr()}"
        return f"{self.str_expr()} {self.pick(CMP)} {self.str_expr()}"

    # relative weights per statement family. a cumulative if-ladder silently
    # strands any branch placed after a larger threshold, and rebalancing it
    # means recomputing every later bound; the table keeps both honest.
    WEIGHTS = {
        "local": 8,
        "assign": 6,
        "print_int": 6,
        "print_num": 5,
        "print_mixed": 5,
        "print_str": 4,
        "print_cond": 4,
        "if": 7,
        "numfor": 6,
        "table_loop": 5,
        "strlib": 9,
        "mathlib": 8,
        "multi": 6,
        "errors": 7,
        "metatable": 9,
        "coroutine": 7,
        "control": 9,
        "varargs": 7,
        "func": 5,
        "closure": 4,
        "tables": 9,
        "polymorph": 8,
        "nesting": 6,
        "loadchunk": 5,
        "precedence": 7,
        "numedge": 8,
        "gcstress": 7,
        "deep": 5,
        "patterns": 9,
        "formats": 7,
    }

    def stmt(self, ind, d=0):
        pad = "  " * ind
        kinds = list(self.WEIGHTS)
        if d >= 3:
            kinds = [x for x in kinds if x not in ("if", "numfor")]
        if d >= 2:
            kinds = [x for x in kinds if x != "func"]
        weights = [self.WEIGHTS[x] for x in kinds]
        which = self.rng.choices(kinds, weights=weights)[0]
        return self.stmt_of(which, pad, ind, d)

    def stmt_of(self, which, pad, ind, d):
        if which == "local":
            n = self.pick(NAMES)
            self.scope.append(n)
            return f"{pad}local {n} = {self.int_expr()}\n"
        if which == "assign":
            v = self.var()
            if v is None:
                return f"{pad}p({self.int_expr()})\n"
            return f"{pad}{v} = {self.int_expr()}\n"
        if which == "print_int":
            return f"{pad}p({self.int_expr()})\n"
        if which == "print_num":
            return f"{pad}p({self.num_expr()})\n"
        if which == "print_mixed":
            return f"{pad}p({self.mixed_expr()})\n"
        if which == "print_str":
            return f"{pad}p({self.str_expr()})\n"
        if which == "print_cond":
            return f"{pad}p({self.cond()})\n"
        if which == "if":
            return f"{pad}if {self.cond()} then\n{self.block(ind+1, 2, d+1)}{pad}end\n"
        if which == "numfor":
            n = self.pick(["i", "j", "k"])
            lo = self.rng.randint(-2, 3)
            hi = lo + self.rng.randint(0, 4)
            st = self.pick([1, 1, 2, -1])
            if st < 0:
                lo, hi = hi, lo
            self.scope.append(n)
            return f"{pad}for {n} = {lo}, {hi}, {st} do\n{self.block(ind+1, 2, d+1)}{pad}end\n"
        if which == "table_loop":
            return (
                f"{pad}do local t = {{}}\n"
                f"{pad}  for i = 1, {self.rng.randint(1, 5)} do t[i] = {self.int_expr()} end\n"
                f"{pad}  for i = 1, #t do p(t[i]) end\n"
                f"{pad}  p(#t)\n"
                f"{pad}end\n"
            )
        if which == "strlib":
            s = self.str_expr()
            return self.pick([
                f"{pad}p(string.sub({s}, {self.rng.randint(-5,5)}, {self.rng.randint(-5,5)}))\n",
                f"{pad}p(string.rep({s}, {self.rng.randint(0,3)}))\n",
                f"{pad}p(string.rep({s}, {self.rng.randint(0,3)}, \"{self.pick(['', '-', ', '])}\"))\n",
                f"{pad}p(string.byte({s}, {self.rng.randint(1,3)}) or -1)\n",
                f"{pad}p(#{s})\n",
                f"{pad}p(string.format('%d|%s', {self.int_expr()}, {s}))\n",
                f"{pad}p(string.format('%q', {s}))\n",
                f"{pad}p(string.format('%5.2f', {self.num_expr()}))\n",
                f"{pad}p(string.format('%x', ({self.int_expr()}) & 0xffff))\n",
                f"{pad}p(string.format('%g', {self.num_expr()}))\n",
                f"{pad}p(select('#', string.find({s}, \"{self.pick(PATS)}\")))\n",
                f"{pad}p((string.gsub({s}, \"{self.pick(PATS)}\", \"X\")))\n",
                f"{pad}p(select(2, string.gsub({s}, \"{self.pick(PATS)}\", \"X\")))\n",
                f"{pad}p((string.gsub({s}, \"{self.pick(PATS)}\", function(w) return '<' .. w .. '>' end)))\n",
                f"{pad}p(table.concat({{1,2,3}}, {s}))\n",
                f"{pad}p(string.match({s}, \"{self.pick(PATS)}\") or '-')\n",
                f"{pad}do local n = 0 for _ in string.gmatch({s}, \"{self.pick(PATS)}\") do n = n + 1 end p(n) end\n",
                f"{pad}p(({s}):len()) p(({s}):upper())\n",
                f"{pad}p(string.char(({self.int_expr()}) % 128 + 1))\n",
                f"{pad}p(#string.pack('i4', ({self.int_expr()}) % 2147483647))\n",
                f"{pad}p(string.unpack('i4', string.pack('i4', ({self.int_expr()}) % 2147483647)))\n",
                f"{pad}p(utf8.len({s}) or -1)\n",
            ])
        if which == "mathlib":
            return self.pick([
                f"{pad}p(math.type({self.int_expr()}))\n",
                f"{pad}p(tostring({self.int_expr()}))\n",
                f"{pad}p(tostring({self.num_expr()}))\n",
                f"{pad}p(tonumber({self.str_lit()}) or -1)\n",
                f"{pad}p(tonumber({self.str_lit()}, {self.rng.randint(2,16)}) or -1)\n",
                f"{pad}p(math.tointeger({self.num_expr()}) or -1)\n",
                f"{pad}p(math.floor({self.num_expr()}))\n",
                f"{pad}p(math.ceil({self.num_expr()}))\n",
                f"{pad}p(math.max({self.int_expr()}, {self.int_expr()}))\n",
                f"{pad}p(math.abs({self.int_expr()}))\n",
                f"{pad}p(math.fmod({self.int_expr()}, ({self.int_expr()} | 1)))\n",
                f"{pad}p(math.ult({self.int_expr()}, {self.int_expr()}))\n",
                f"{pad}p(({self.int_expr()}) // 1)\n",
            ])
        if which == "multi":
            return self.pick([
                f"{pad}do local function mr() return {self.int_expr()}, {self.int_expr()} end\n"
                f"{pad}  local x, y = mr() p(x) p(y) p(select('#', mr()))\n"
                f"{pad}end\n",
                f"{pad}do local function va(...) return select('#', ...) end\n"
                f"{pad}  p(va()) p(va(1)) p(va(1, nil)) p(va(nil, nil))\n"
                f"{pad}end\n",
                f"{pad}do local t = table.pack({self.int_expr()}, {self.int_expr()})\n"
                f"{pad}  p(t.n) p(t[1]) p(t[2])\n"
                f"{pad}end\n",
                f"{pad}do local t = {{{self.int_expr()}, {self.int_expr()}, {self.int_expr()}}}\n"
                f"{pad}  p(table.unpack(t, 1, 2))\n"
                f"{pad}  table.remove(t, 1) p(#t)\n"
                f"{pad}  table.insert(t, 1, 9) p(t[1]) p(#t)\n"
                f"{pad}end\n",
                # sorted with a total order on (value, type): sort is unstable
                # and equal elements may land either way, so comparing raw
                # sorted output would report unspecified behaviour as a bug.
                # -2^63 as a float and math.mininteger compare equal.
                f"{pad}do local t = {{{self.int_expr()}, {self.int_expr()}, {self.int_expr()}}}\n"
                f"{pad}  table.sort(t, function(x, y)\n"
                f"{pad}    if x ~= y then return x < y end\n"
                f"{pad}    return math.type(x) == 'integer' and math.type(y) == 'float'\n"
                f"{pad}  end)\n"
                f"{pad}  p(table.concat(t, ','))\n"
                f"{pad}end\n",
            ])
        if which == "errors":
            return self.pick([
                f"{pad}do local ok, e = pcall(function() error({self.int_expr()}) end)\n"
                f"{pad}  p(ok) p(e)\n"
                f"{pad}end\n",
                f"{pad}do local ok, e = pcall(function() error({self.str_lit()}, 0) end)\n"
                f"{pad}  p(ok) p(e)\n"
                f"{pad}end\n",
                f"{pad}p(pcall(function() return nil + 1 end))\n",
                f"{pad}p(pcall(function() return ({self.int_expr()}) // 0 end))\n",
                f"{pad}p(pcall(function() return ({self.int_expr()}) % 0 end))\n",
                f"{pad}p(select(2, pcall(function() local t = nil return t.x end)))\n",
                f"{pad}p(pcall(string.rep, {self.str_lit()}, -1))\n",
            ])
        if which == "metatable":
            return self.pick([
                # metatables: dispatch, inheritance chains, and the operators
                # that route through them
                f"{pad}do local mt = {{__index = function(t, k) return k .. '!' end}}\n"
                f"{pad}  local t = setmetatable({{}}, mt)\n"
                f"{pad}  p(t.foo) p(rawget(t, 'foo')) p(rawlen(t))\n"
                f"{pad}end\n",
                f"{pad}do local base = {{x = {self.int_expr()}}}\n"
                f"{pad}  local mid = setmetatable({{}}, {{__index = base}})\n"
                f"{pad}  local top = setmetatable({{}}, {{__index = mid}})\n"
                f"{pad}  p(top.x) p(rawget(top, 'x'))\n"
                f"{pad}end\n",
                f"{pad}do local mt = {{__add = function(a, b) return {self.int_expr()} end,\n"
                f"{pad}    __len = function() return {self.rng.randint(0, 5)} end,\n"
                f"{pad}    __eq = function() return true end,\n"
                f"{pad}    __lt = function() return false end,\n"
                f"{pad}    __concat = function() return 'C' end,\n"
                f"{pad}    __unm = function() return -1 end,\n"
                f"{pad}    __call = function(s, v) return v end}}\n"
                f"{pad}  local a = setmetatable({{}}, mt)\n"
                f"{pad}  local b = setmetatable({{}}, mt)\n"
                f"{pad}  p(a + b) p(#a) p(a == b) p(a < b) p(a .. b) p(-a) p(a({self.int_expr()}))\n"
                f"{pad}end\n",
                f"{pad}do local n = 0\n"
                f"{pad}  local t = setmetatable({{}}, {{__newindex = function(s, k, v) n = n + 1 rawset(s, k, v) end}})\n"
                f"{pad}  t.a = 1 t.a = 2 t.b = 3\n"
                f"{pad}  p(n) p(t.a) p(t.b)\n"
                f"{pad}end\n",
                f"{pad}do local t = setmetatable({{}}, {{__tostring = function() return 'TS' end}})\n"
                f"{pad}  p(tostring(t))\n"
                f"{pad}end\n",
            ])
        if which == "coroutine":
            return self.pick([
                # coroutines: transfer of values and error propagation
                f"{pad}do local co = coroutine.create(function(a)\n"
                f"{pad}    local b = coroutine.yield(a + 1)\n"
                f"{pad}    return b * 2\n"
                f"{pad}  end)\n"
                f"{pad}  p(coroutine.resume(co, {self.rng.randint(0, 5)}))\n"
                f"{pad}  p(coroutine.resume(co, {self.rng.randint(0, 5)}))\n"
                f"{pad}  p(coroutine.status(co))\n"
                f"{pad}  p(coroutine.resume(co))\n"
                f"{pad}end\n",
                f"{pad}do local co = coroutine.wrap(function() for q = 1, 3 do coroutine.yield(q) end end)\n"
                f"{pad}  p(co()) p(co()) p(co())\n"
                f"{pad}end\n",
                f"{pad}do local co = coroutine.create(function() error('boom') end)\n"
                f"{pad}  local ok, e = coroutine.resume(co)\n"
                f"{pad}  p(ok) p(e) p(coroutine.status(co))\n"
                f"{pad}end\n",
                f"{pad}do local co = coroutine.create(function() return coroutine.isyieldable() end)\n"
                f"{pad}  p(coroutine.resume(co)) p(coroutine.isyieldable())\n"
                f"{pad}end\n",
            ])
        if which == "control":
            return self.pick([
                # control flow the compiler lowers specially
                f"{pad}do local n = 0\n"
                f"{pad}  for q = 1, {self.rng.randint(2, 6)} do\n"
                f"{pad}    if q % 2 == 0 then goto cont end\n"
                f"{pad}    n = n + q\n"
                f"{pad}    ::cont::\n"
                f"{pad}  end\n"
                f"{pad}  p(n)\n"
                f"{pad}end\n",
                f"{pad}do local n = 0\n"
                f"{pad}  while true do n = n + 1 if n > {self.rng.randint(1, 4)} then break end end\n"
                f"{pad}  p(n)\n"
                f"{pad}end\n",
                f"{pad}do local n = 0\n"
                f"{pad}  repeat local step = n + 1 n = step until n >= {self.rng.randint(1, 4)}\n"
                f"{pad}  p(n)\n"
                f"{pad}end\n",
                f"{pad}do local function rec(n) if n <= 0 then return 0 end return n + rec(n - 1) end\n"
                f"{pad}  p(rec({self.rng.randint(0, 40)}))\n"
                f"{pad}end\n",
                f"{pad}do local function ev(n) if n == 0 then return true end return od(n - 1) end\n"
                f"{pad}  function od(n) if n == 0 then return false end return ev(n - 1) end\n"
                f"{pad}  p(ev({self.rng.randint(0, 12)}))\n"
                f"{pad}end\n",
                # to-be-closed variables
                f"{pad}do local log = {{}}\n"
                f"{pad}  do local x <close> = setmetatable({{}}, {{__close = function() log[#log+1] = 'c' end}})\n"
                f"{pad}    log[#log+1] = 'b'\n"
                f"{pad}  end\n"
                f"{pad}  p(table.concat(log, ','))\n"
                f"{pad}end\n",
                f"{pad}do local c <const> = {self.int_expr()}\n"
                f"{pad}  p(c)\n"
                f"{pad}end\n",
            ])
        if which == "varargs":
            return self.pick([
                # varargs and multiple-return truncation rules
                f"{pad}do local function v(...) return ... end\n"
                f"{pad}  p(select('#', v())) p(select('#', v(nil)))\n"
                f"{pad}  p(select('#', v(1, nil, 3)))\n"
                f"{pad}  local t = {{v(1, 2, 3)}} p(#t)\n"
                f"{pad}  local u = {{v(1, 2, 3), 9}} p(#u)\n"
                f"{pad}  p((v(1, 2, 3)))\n"
                f"{pad}end\n",
                f"{pad}do local function v(...) local a, b = ... return a, b end\n"
                f"{pad}  p(v({self.int_expr()}))\n"
                f"{pad}  p(select('#', v({self.int_expr()})))\n"
                f"{pad}end\n",
                f"{pad}do local t = table.pack(table.unpack({{1, 2, 3}}, 1, {self.rng.randint(0, 4)}))\n"
                f"{pad}  p(t.n)\n"
                f"{pad}end\n",
                f"{pad}do local function f() return 1, 2 end\n"
                f"{pad}  local a, b, c = f(), f()\n"
                f"{pad}  p(a) p(b) p(c)\n"
                f"{pad}end\n",
            ])
        if which == "tables":
            return self.pick([
                # array/hash split, border semantics, and key coercion
                f"{pad}do local t = {{1, 2, 3, x = 4, [10] = 5, ['y'] = 6}}\n"
                f"{pad}  p(#t) p(t.x) p(t[10]) p(t.y) p(t[1])\n"
                f"{pad}end\n",
                f"{pad}do local t = {{}}\n"
                f"{pad}  t[1] = 'a' t[2] = 'b' t[4] = 'd'\n"
                f"{pad}  p(#t == 2 or #t == 4)\n"
                f"{pad}  t[3] = 'c' p(#t)\n"
                f"{pad}  t[3] = nil p(#t == 2 or #t == 4)\n"
                f"{pad}end\n",
                f"{pad}do local t = {{}}\n"
                f"{pad}  t[1.0] = 'f' p(t[1])\n"
                f"{pad}  t[2] = 'i' p(t[2.0])\n"
                f"{pad}  p(next(t) ~= nil)\n"
                f"{pad}end\n",
                f"{pad}do local t = {{}}\n"
                f"{pad}  for q = 1, {self.rng.randint(4, 20)} do t[q] = q * 2 end\n"
                f"{pad}  local s = 0 for _, v in ipairs(t) do s = s + v end p(s)\n"
                f"{pad}  local c = 0 for _ in pairs(t) do c = c + 1 end p(c)\n"
                f"{pad}end\n",
                f"{pad}do local t = {{}}\n"
                f"{pad}  t['k'] = 1 t[true] = 2 t[2.5] = 3\n"
                f"{pad}  p(t['k']) p(t[true]) p(t[2.5])\n"
                f"{pad}end\n",
                f"{pad}do local t = {{{self.int_expr()}, {self.int_expr()}}}\n"
                f"{pad}  p(table.concat(t, '|'))\n"
                f"{pad}  table.move(t, 1, 2, 2) p(table.concat(t, '|'))\n"
                f"{pad}end\n",
            ])
        if which == "polymorph":
            # the same call site fed different kinds: exercises the rblock
            # version cache and edge resolution
            return self.pick([
                f"{pad}do local function id(x) return x end\n"
                f"{pad}  p(id({self.int_expr()})) p(id({self.num_expr()}))\n"
                f"{pad}  p(id({self.str_lit()})) p(tostring(id(nil))) p(id(true))\n"
                f"{pad}end\n",
                f"{pad}do local function sz(x)\n"
                f"{pad}    if type(x) == 'string' then return #x end\n"
                f"{pad}    if type(x) == 'table' then return #x end\n"
                f"{pad}    return -1\n"
                f"{pad}  end\n"
                f"{pad}  p(sz({self.str_lit()})) p(sz({{1,2}})) p(sz({self.int_expr()}))\n"
                f"{pad}end\n",
                f"{pad}do local acc = {{}}\n"
                f"{pad}  for _, v in ipairs({{1, 'a', 2.5, true}}) do acc[#acc+1] = type(v) end\n"
                f"{pad}  p(table.concat(acc, ','))\n"
                f"{pad}end\n",
                f"{pad}do local function pick(n) if n % 2 == 0 then return {self.int_expr()} end return {self.str_lit()} end\n"
                f"{pad}  for q = 1, 4 do p(type(pick(q))) end\n"
                f"{pad}end\n",
            ])
        if which == "nesting":
            return self.pick([
                f"{pad}do local function outer()\n"
                f"{pad}    local a = {self.int_expr()}\n"
                f"{pad}    local function mid()\n"
                f"{pad}      local function inner() a = a + 1 return a end\n"
                f"{pad}      return inner()\n"
                f"{pad}    end\n"
                f"{pad}    return mid(), mid(), a\n"
                f"{pad}  end\n"
                f"{pad}  p(outer())\n"
                f"{pad}end\n",
                f"{pad}do local fns = {{}}\n"
                f"{pad}  for q = 1, 3 do fns[q] = function() return q end end\n"
                f"{pad}  p(fns[1]()) p(fns[2]()) p(fns[3]())\n"
                f"{pad}end\n",
                f"{pad}do local a, b = {self.int_expr()}, {self.int_expr()}\n"
                f"{pad}  local function swap() a, b = b, a end\n"
                f"{pad}  swap() p(a) p(b) swap() p(a) p(b)\n"
                f"{pad}end\n",
                f"{pad}do local t = {{n = 0}}\n"
                f"{pad}  function t:bump(by) self.n = self.n + by return self.n end\n"
                f"{pad}  p(t:bump(2)) p(t:bump(3)) p(t.n)\n"
                f"{pad}end\n",
            ])
        if which == "loadchunk":
            # the compiler re-entered on a nested chunk, plus its error path
            return self.pick([
                f"{pad}do local f = load('return ' .. {self.int_expr()})\n"
                f"{pad}  p(f and f() or -1)\n"
                f"{pad}end\n",
                f"{pad}do local f, e = load('return +')\n"
                f"{pad}  p(f == nil) p(type(e))\n"
                f"{pad}end\n",
                f"{pad}do local f = load('local a, b = ... return a + b')\n"
                f"{pad}  p(f({self.rng.randint(0, 9)}, {self.rng.randint(0, 9)}))\n"
                f"{pad}end\n",
                f"{pad}do local n = 0\n"
                f"{pad}  local f = load(function() n = n + 1 if n == 1 then return 'return 7' end return nil end)\n"
                f"{pad}  p(f())\n"
                f"{pad}end\n",
                f"{pad}do local env = {{x = {self.int_expr()}}}\n"
                f"{pad}  local f = load('return x', 'c', 't', env)\n"
                f"{pad}  p(f())\n"
                f"{pad}end\n",
                f"{pad}p(pcall(load, 'local x <const> = 1; x = 2'))\n",
            ])
        if which == "precedence":
            # long mixed-operator chains: associativity and the branch-mode
            # parser's and/or short-circuit shapes
            return self.pick([
                f"{pad}p({self.int_expr()} + {self.int_expr()} * {self.int_expr()} - {self.int_expr()})\n",
                f"{pad}p(({self.int_expr()} < {self.int_expr()}) and ({self.int_expr()} > {self.int_expr()}))\n",
                f"{pad}p({self.int_expr()} < {self.int_expr()} or {self.int_expr()} == {self.int_expr()})\n",
                f"{pad}p(not ({self.int_expr()} < {self.int_expr()}) and not not ({self.int_expr()} ~= 0))\n",
                f"{pad}p({self.num_expr()} ^ 2 ^ 1)\n",
                f"{pad}p({self.str_lit()} .. {self.str_lit()} .. {self.str_lit()})\n",
                f"{pad}p(({self.int_expr()} | {self.int_expr()}) & ~{self.int_expr()})\n",
                f"{pad}p(#{self.str_lit()} + 1)\n",
                f"{pad}do local a = {self.int_expr()}\n"
                f"{pad}  local r = a > 0 and 'pos' or a < 0 and 'neg' or 'zero'\n"
                f"{pad}  p(r)\n"
                f"{pad}end\n",
                f"{pad}p((({self.int_expr()})))\n",
            ])
        if which == "numedge":
            # int/float boundary behaviour, where the two implementations have
            # already disagreed more than once
            return self.pick([
                f"{pad}p(math.maxinteger + 1 == math.mininteger)\n",
                f"{pad}p(math.mininteger - 1 == math.maxinteger)\n",
                f"{pad}p(math.maxinteger // -1)\n",
                f"{pad}p(math.mininteger // -1)\n",
                f"{pad}p(math.mininteger % -1)\n",
                f"{pad}p(math.tointeger(2.0^53)) p(math.tointeger(2.0^63) or -1)\n",
                f"{pad}p(math.type(1) == 'integer' and math.type(1.0) == 'float')\n",
                f"{pad}p(1 == 1.0) p(math.maxinteger + 0.0 == math.maxinteger)\n",
                f"{pad}p(tostring(0/0) == tostring(0/0))\n",
                f"{pad}p(3 // 0.0) p(-3 // 0.0) p(0/0 ~= 0/0)\n",
                f"{pad}p(math.fmod(-5, 3)) p(-5 % 3) p(5 % -3)\n",
                f"{pad}p((-5) // 3) p(5 // -3) p((-5.0) // 3)\n",
                f"{pad}p(string.format('%d', math.mininteger))\n",
                f"{pad}p(math.abs(math.mininteger))\n",
                f"{pad}p(2^63 == math.maxinteger + 1.0)\n",
                f"{pad}p(math.ult(-1, 0)) p(math.ult(0, -1))\n",
                f"{pad}p(({self.int_expr()}) >> 64) p(({self.int_expr()}) << 64)\n",
                f"{pad}p(0x7fffffffffffffff) p(0x8000000000000000)\n",
            ])
        if which == "gcstress":
            # allocation churn with surviving references: the collector runs
            # mid-expression, so anything it fails to trace shows up here
            return self.pick([
                f"{pad}do local keep = {{}}\n"
                f"{pad}  for q = 1, {self.rng.randint(20, 120)} do\n"
                f"{pad}    keep[#keep + 1] = {{id = q, s = 'v' .. q}}\n"
                f"{pad}    if #keep > 8 then table.remove(keep, 1) end\n"
                f"{pad}  end\n"
                f"{pad}  local s = 0 for _, v in ipairs(keep) do s = s + v.id end\n"
                f"{pad}  p(#keep) p(s)\n"
                f"{pad}end\n",
                f"{pad}do local acc = ''\n"
                f"{pad}  for q = 1, {self.rng.randint(10, 60)} do acc = acc .. tostring(q % 10) end\n"
                f"{pad}  p(#acc) p(acc:sub(1, 8))\n"
                f"{pad}end\n",
                f"{pad}do local t = {{}}\n"
                f"{pad}  for q = 1, {self.rng.randint(20, 90)} do t['k' .. q] = {{q}} end\n"
                f"{pad}  local c = 0 for _ in pairs(t) do c = c + 1 end p(c)\n"
                f"{pad}  p(t.k5 and t.k5[1] or -1)\n"
                f"{pad}end\n",
                f"{pad}do local outer = {{}}\n"
                f"{pad}  for q = 1, {self.rng.randint(5, 25)} do\n"
                f"{pad}    outer[q] = setmetatable({{v = q}}, {{__index = function() return 'd' end}})\n"
                f"{pad}  end\n"
                f"{pad}  p(#outer) p(outer[1].v) p(outer[1].missing)\n"
                f"{pad}end\n",
                f"{pad}do local fns = {{}}\n"
                f"{pad}  for q = 1, {self.rng.randint(5, 30)} do\n"
                f"{pad}    local cap = 'c' .. q\n"
                f"{pad}    fns[q] = function() return cap end\n"
                f"{pad}  end\n"
                f"{pad}  p(#fns) p(fns[1]()) p(fns[#fns]())\n"
                f"{pad}end\n",
            ])
        if which == "deep":
            return self.pick([
                f"{pad}do local function d(n) if n <= 0 then return 0 end return 1 + d(n - 1) end\n"
                f"{pad}  p(d({self.rng.randint(50, 400)}))\n"
                f"{pad}end\n",
                f"{pad}do local function build(n) if n <= 0 then return nil end return {{n, build(n - 1)}} end\n"
                f"{pad}  local t = build({self.rng.randint(10, 80)})\n"
                f"{pad}  local c = 0 while t do c = c + 1 t = t[2] end p(c)\n"
                f"{pad}end\n",
                f"{pad}do local ok = pcall(function() local function f() return 1 + f() end return f() end)\n"
                f"{pad}  p(ok)\n"
                f"{pad}end\n",
                f"{pad}do local function t(n, acc) if n <= 0 then return acc end return t(n - 1, acc + n) end\n"
                f"{pad}  p(t({self.rng.randint(100, 3000)}, 0))\n"
                f"{pad}end\n",
                f"{pad}do local co = coroutine.wrap(function()\n"
                f"{pad}    local function rec(n) if n > 0 then coroutine.yield(n) rec(n - 1) end end\n"
                f"{pad}    rec({self.rng.randint(3, 20)})\n"
                f"{pad}  end)\n"
                f"{pad}  local s = 0 for v in co do s = s + v end p(s)\n"
                f"{pad}end\n",
            ])
        if which == "patterns":
            subj = self.pick([
                "hello world", "aaa", "abcabc", "a1b2c3", "  pad  ",
                "key=value", "()", "[x]", "a.b.c", "", "AAbbCC", "n1n22n333",
            ])
            pat = self.pick([
                "%a+", "%d+", "%s", "%w+", "(%a)(%d)", "^%a", "%a$", "a.c",
                "%b()", "%f[%a]%a+", "(.)%1", "[%a%d]+", "[^%s]+", ".-",
                "()%a", "%p", "%u+", "%l+", "(%a+)=(%a+)", "%.", "a*", "a-b",
            ])
            return self.pick([
                f"{pad}p(string.match('{subj}', '{pat}') or '-')\n",
                f"{pad}p(select('#', string.find('{subj}', '{pat}')))\n",
                f"{pad}p((string.find('{subj}', '{pat}')) or -1)\n",
                f"{pad}p((string.gsub('{subj}', '{pat}', '#')))\n",
                f"{pad}p(select(2, string.gsub('{subj}', '{pat}', '#')))\n",
                f"{pad}do local n = 0 for _ in string.gmatch('{subj}', '{pat}') do n = n + 1 end p(n) end\n",
                f"{pad}do local acc = {{}}\n"
                f"{pad}  for w in string.gmatch('{subj}', '{pat}') do acc[#acc+1] = tostring(w) end\n"
                f"{pad}  p(table.concat(acc, '|'))\n"
                f"{pad}end\n",
                f"{pad}p((string.gsub('{subj}', '{pat}', function(w) return '<' .. tostring(w) .. '>' end)))\n",
                f"{pad}p((string.gsub('{subj}', '{pat}', {{}})))\n",
                f"{pad}p(pcall(string.match, '{subj}', '%'))\n",
                f"{pad}p(pcall(string.find, '{subj}', '{pat}', {self.rng.randint(-4, 6)}))\n",
            ])
        if which == "formats":
            return self.pick([
                f"{pad}p(string.format('%d', {self.int_expr()}))\n",
                f"{pad}p(string.format('%i', {self.int_expr()}))\n",
                f"{pad}p(string.format('%5d|%-5d|%05d', 42, 42, 42))\n",
                f"{pad}p(string.format('%x|%X|%o', 255, 255, 8))\n",
                f"{pad}p(string.format('%c', 65))\n",
                f"{pad}p(string.format('%e', {self.num_expr()}))\n",
                f"{pad}p(string.format('%f', 1.5))\n",
                f"{pad}p(string.format('%.3f|%.0f', 1.23456, 2.5))\n",
                f"{pad}p(string.format('%g|%G', 0.0001, 100000.0))\n",
                f"{pad}p(string.format('%s|%10s|%-10s|', 'x', 'y', 'z'))\n",
                f"{pad}p(string.format('%.2s', 'abcdef'))\n",
                f"{pad}p(string.format('%q', 'a\\nb'))\n",
                f"{pad}p(string.format('%%'))\n",
                f"{pad}p(string.format('%a', 1.0) ~= nil)\n",
                f"{pad}p(pcall(string.format, '%d', 'notanumber'))\n",
                f"{pad}p(pcall(string.format, '%d', 1.5))\n",
                f"{pad}p(string.format('%s', setmetatable({{}}, {{__tostring = function() return 'M' end}})))\n",
            ])
        if which == "func":
            n = self.pick(["fa", "fb", "fc"])
            arg = self.pick(NAMES)
            saved = list(self.scope)
            self.scope = [arg]
            body = self.block(ind + 1, 2, d + 1)
            self.scope = saved
            return (
                f"{pad}local function {n}({arg})\n{body}"
                f"{pad}  return {arg}\n{pad}end\n"
                f"{pad}p({n}({self.int_expr()}))\n"
            )
        # closures over a mutated local: boxing and upvalue identity
        return (
            f"{pad}do local n = {self.int_expr()}\n"
            f"{pad}  local function inc() n = n + 1 return n end\n"
            f"{pad}  p(inc()) p(inc()) p(n)\n"
            f"{pad}end\n"
        )

    def block(self, ind, n, d=0):
        out = ""
        for _ in range(self.rng.randint(1, n)):
            out += self.stmt(ind, d)
        if out == "":
            out = "  " * ind + "p(0)\n"
        return out


PRELUDE = """local function one(x)
  if math.type(x) == 'float' then
    if x ~= x then return 'nan' end
    if x == math.huge then return 'inf' end
    if x == -math.huge then return '-inf' end
    return string.format('%.14g', x)
  end
  return tostring(x)
end
-- prints every argument: a p() that kept only the first silently hid every
-- disagreement in a multiple-return value.
local function p(...)
  local n = select('#', ...)
  local parts = {}
  for i = 1, n do
    parts[i] = one((select(i, ...)))
  end
  print(table.concat(parts, '\\t'))
end
local function m(x)
  if math.type(x) == 'integer' then return x end
  return 0
end
"""

PRELUDE_LINES = len(PRELUDE.split("\n")) - 1


def generate(seed):
    rng = random.Random(seed)
    g = Gen(rng)
    body = ""
    for _ in range(rng.randint(3, 14)):
        body += g.stmt(0)
    return PRELUDE + body


TIMEOUT = 10


def run(binary, path):
    try:
        proc = subprocess.run([binary, path], capture_output=True, timeout=TIMEOUT)
        return (
            proc.stdout.decode(errors="replace"),
            proc.stderr.decode(errors="replace"),
            proc.returncode,
        )
    except subprocess.TimeoutExpired:
        return "", "timeout", -1


def signature(mx_err):
    for line in mx_err.strip().split("\n"):
        line = line.strip()
        if not line or line.startswith("stack traceback"):
            continue
        # strip any leading path:line: prefixes so the signature survives
        # reduction moving the failing statement around
        return re.sub(r"^.*?:\d+:\s*", "", line)
    return "no message"


def first_diff(mx_out, ref_out):
    # the first differing line pair identifies *which* disagreement this is.
    # a bare "output differs" lets reduction drift onto an unrelated one and
    # then emit a repro that no longer reproduces anything.
    a = mx_out.split("\n")
    b = ref_out.split("\n")
    for i in range(max(len(a), len(b))):
        x = a[i] if i < len(a) else "<eof>"
        y = b[i] if i < len(b) else "<eof>"
        if x != y:
            return f"got {x!r} want {y!r}"
    return "output differs"


def verdict(src, tmp, ref_bin):
    with open(tmp, "w") as f:
        f.write(src)
    mx_out, mx_err, mx_rc = run(minivm, tmp)
    ref_out, ref_err, ref_rc = run(ref_bin, tmp)
    if ref_rc != 0:
        return None
    if mx_rc != 0:
        return ("error", signature(mx_err))
    if mx_out != ref_out:
        return ("wrong", first_diff(mx_out, ref_out))
    return None


def reduce(src, tmp, ref_bin, want):
    lines = src.split("\n")
    changed = True
    while changed:
        changed = False
        i = PRELUDE_LINES
        while i < len(lines):
            if lines[i].strip() == "":
                i += 1
                continue
            trial = lines[:i] + lines[i + 1:]
            got = verdict("\n".join(trial), tmp, ref_bin)
            if got == want:
                lines = trial
                changed = True
                continue
            i += 1
    return "\n".join(lines)


def slug(sig):
    s = re.sub(r"[^a-z0-9]+", "-", sig.lower()).strip("-")
    return s[:48] or "unknown"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-n", type=int, default=1000)
    ap.add_argument("-s", type=int, default=0)
    ap.add_argument("--lua", default="lua5.5")
    ap.add_argument(
        "--timeout",
        type=float,
        default=10.0,
        help="per-run seconds; raise it for instrumented builds, where "
        "deep recursion is orders of magnitude slower and would otherwise "
        "be reported as a spurious timeout failure",
    )
    ap.add_argument(
        "--coverage",
        action="store_true",
        help="report how often each generated construct appears and exit",
    )
    args = ap.parse_args()

    global TIMEOUT
    TIMEOUT = args.timeout

    if args.coverage:
        marks = {
            "metatable": "setmetatable",
            "__close": "<close>",
            "__const": "<const>",
            "coroutine": "coroutine.",
            "goto": "goto ",
            "varargs": "select('#'",
            "pattern": "string.gsub",
            "pack": "string.pack",
            "utf8": "utf8.",
            "recursion": "rec(",
            "pcall": "pcall(",
            "sort": "table.sort",
            "closure": "local function inc",
            "numfor": "for i =",
            "load": "load(",
            "maxint": "math.maxinteger",
            "minint": "math.mininteger",
            "ipairs": "ipairs(",
            "pairs": "pairs(",
            "method": ":bump(",
            "gmatch": "gmatch(",
            "concat_chain": " .. ",
        }
        counts = {k: 0 for k in marks}
        for i in range(args.n):
            src = generate(args.s * 1000003 + i)
            for k, needle in marks.items():
                if needle in src:
                    counts[k] += 1
        for k in sorted(counts, key=lambda x: -counts[x]):
            pct = 100.0 * counts[k] / args.n
            print(f"{k:<12} {counts[k]:>6} programs  {pct:5.1f}%")
        return 0

    os.makedirs(out_dir, exist_ok=True)
    tmp = os.path.join(out_dir, "candidate.lua")
    seen = {}
    total = 0
    rejected = 0
    for i in range(args.n):
        seed = args.s * 1000003 + i
        src = generate(seed)
        with open(tmp, "w") as f:
            f.write(src)
        if run(args.lua, tmp)[2] != 0:
            # the reference itself rejects this program, so there is nothing
            # to compare. counted so a generator that mostly emits invalid
            # code cannot look like a clean run.
            rejected += 1
            continue
        got = verdict(src, tmp, args.lua)
        if got is None:
            continue
        total += 1
        if got in seen:
            seen[got] += 1
            continue
        seen[got] = 1
        small = reduce(src, tmp, args.lua, got)
        # a repro that does not reproduce is worse than none: it sends the
        # next reader chasing a program that already agrees.
        if verdict(small, tmp, args.lua) != got:
            small = src
        body = [l for l in small.split("\n")[PRELUDE_LINES:] if l.strip()]
        name = f"{got[0]}-{slug(got[1])}.lua"
        with open(os.path.join(out_dir, name), "w") as f:
            f.write(small)
        print(f"{got[0]} seed={seed} {got[1]}")
        for l in body:
            print(f"    {l}")
        print(f"    -> test/fuzz/out/{name}")
    print(
        f"cases={args.n} compared={args.n - rejected} rejected={rejected} "
        f"failures={total} distinct={len(seen)}"
    )
    for (kind, sig), count in sorted(seen.items(), key=lambda kv: -kv[1]):
        print(f"  x{count:<5} {kind}: {sig}")
    return 1 if seen else 0


if __name__ == "__main__":
    sys.exit(main())
