local function show(...)
    local parts = {}
    for i = 1, select("#", ...) do
        parts[i] = tostring(select(i, ...))
    end
    print(table.concat(parts, " "))
end

local function fill(n)
    local t = {}
    for i = 1, n do
        t[i] = i * 2
    end
    return t
end
for _, n in ipairs({0, 1, 2, 3, 4, 5, 16, 17, 100}) do
    local t = fill(n)
    show(#t, t[1], t[n], t[n + 1])
end

local function append(n)
    local t = {}
    for i = 1, n do
        t[#t + 1] = "v" .. i
    end
    return t
end
local t = append(50)
show(#t, t[1], t[50], t[51])

local function holes()
    local t = {1, 2, 3, nil, 5}
    for _ = 1, 12 do
        t[4] = nil
        t[6] = 6
        t[5] = nil
        t[5] = 5
    end
    return #t, t[4], t[5], t[6]
end
show(holes())

local function intkeys()
    local t = {}
    for i = 10, 1, -1 do
        t[i] = i
    end
    t[0] = "zero"
    t[-1] = "neg"
    t[2^53] = "big"
    t[1.5] = "half"
    t[2.0] = "two"
    local s = 0
    for i = 1, 10 do
        s = s + (tonumber(t[i]) or 0)
    end
    return s, t[0], t[-1], t[2^53], t[1.5], t[2], #t
end
for _ = 1, 12 do
    intkeys()
end
show(intkeys())

local function fields()
    local o = {a = 1, b = 2.5, c = "s", d = true, e = {}}
    local s = 0
    for _ = 1, 20 do
        o.a = o.a + 1
        o.b = o.b + 0.5
        o.c = o.c .. "x"
        o.d = not o.d
        s = s + o.a + o.b
    end
    return s, o.a, o.b, #o.c, o.d, type(o.e), o.missing
end
show(fields())

local function delete_and_readd()
    local o = {x = 1, y = 2, z = 3}
    for i = 1, 20 do
        o.x = nil
        o.w = i
        o.x = i
        o.w = nil
    end
    return o.x, o.y, o.z, o.w
end
show(delete_and_readd())

local function shapes(o)
    return o.a, o.b
end
local objs = {
    {a = 1, b = 2},
    {b = 3, a = 4},
    {x = 1, y = 2, z = 3, a = 5, b = 6},
    {a = "s"},
    {b = 1.5},
    {},
}
for round = 1, 3 do
    for _, o in ipairs(objs) do
        for _ = 1, 8 do
            shapes(o)
        end
        show(round, shapes(o))
    end
end

local function growing()
    local o = {}
    local keys = {"alpha", "beta", "gamma", "delta", "eps", "zeta", "eta", "theta", "iota", "kappa"}
    local s = 0
    for _, k in ipairs(keys) do
        o[k] = #k
        for _, q in ipairs(keys) do
            s = s + (o[q] or 0)
        end
    end
    return s, o.alpha, o.kappa, o.nope
end
for _ = 1, 12 do
    growing()
end
show(growing())

local function varkeys(t, k)
    return t[k]
end
local kt = {[1] = "one", x = "ex", [2.5] = "float", [true] = "bool"}
local fnkey = print
kt[fnkey] = "fn"
for _ = 1, 3 do
    for _, k in ipairs({1, "x", 2.5, true, false, fnkey, 99, "y", 1.0}) do
        for _ = 1, 8 do
            varkeys(kt, k)
        end
        show(varkeys(kt, k))
    end
end
show(pcall(varkeys, 5, 1))
show(pcall(varkeys, nil, "x"))
show(pcall(function() local t = {}; t[nil] = 1 end))
show(pcall(function() local t = {}; t[0/0] = 1 end))

local function setvar(t, k, v)
    t[k] = v
end
local st = {}
for _ = 1, 12 do
    setvar(st, "k", 1)
    setvar(st, 1, 2)
    setvar(st, 2.5, 3)
    setvar(st, true, 4)
end
show(st.k, st[1], st[2.5], st[true])
for _ = 1, 12 do
    show(pcall(setvar, st, nil, 1))
end
show(pcall(setvar, 5, 1, 1))

local base = {inherited = "yes", n = 7}
local meta = {__index = base}
local function index_chain(o)
    return o.inherited, o.n, o.own, o.none
end
local chained = setmetatable({own = "mine"}, meta)
for _ = 1, 12 do
    index_chain(chained)
end
show(index_chain(chained))
base.inherited = "changed"
show(index_chain(chained))
base.inherited = nil
show(index_chain(chained))

local fmeta = setmetatable({}, {__index = function(_, k) return "dyn:" .. tostring(k) end})
local function idxfn(o)
    return o.x, o[1], o.y
end
for _ = 1, 12 do
    idxfn(fmeta)
end
show(idxfn(fmeta))

local log = {}
local proxy = setmetatable({}, {
    __newindex = function(t, k, v)
        log[#log + 1] = tostring(k) .. "=" .. tostring(v)
        rawset(t, k, v)
    end,
})
local function newidx(p, i)
    p.a = i
    p[i] = i
    p.a = nil
end
for i = 1, 12 do
    newidx(proxy, i)
end
show(#log, log[1], log[#log], proxy.a, proxy[3])

local function set_after_meta()
    local a = {1, 2, 3}
    local seen = 0
    for i = 1, 12 do
        a[i] = i
        if i == 6 then
            setmetatable(a, {__newindex = function(t, k, v) seen = seen + 1; rawset(t, k, v) end})
        end
    end
    return seen, #a
end
show(set_after_meta())

local function get_after_meta()
    local a = {x = 1}
    local out = {}
    for i = 1, 12 do
        if i == 6 then
            a.x = nil
            setmetatable(a, {__index = function() return "m" end})
        end
        out[i] = a.x
    end
    return out[1], out[5], out[6], out[12]
end
show(get_after_meta())

local arith_meta = {
    __add = function(a, b) return "add" end,
    __sub = function(a, b) return "sub" end,
    __mul = function(a, b) return "mul" end,
    __div = function(a, b) return "div" end,
    __mod = function(a, b) return "mod" end,
    __pow = function(a, b) return "pow" end,
    __unm = function(a) return "unm" end,
    __idiv = function(a, b) return "idiv" end,
    __band = function(a, b) return "band" end,
    __bor = function(a, b) return "bor" end,
    __bxor = function(a, b) return "bxor" end,
    __shl = function(a, b) return "shl" end,
    __shr = function(a, b) return "shr" end,
    __bnot = function(a) return "bnot" end,
    __concat = function(a, b) return "concat" end,
    __len = function(a) return "len" end,
    __eq = function(a, b) return true end,
    __lt = function(a, b) return true end,
    __le = function(a, b) return false end,
    __call = function(self, x) return "called", x end,
    __tostring = function() return "obj" end,
}
local function metaops(a, b)
    return a + 1, 1 - a, a * b, a / 2, a % 2, a ^ 2, -a, a // 2, a & 1, a | 1, a ~ 1, a << 1, a >> 1, ~a, a .. "x", #a, a == b, a < b, a <= b, a(5)
end
local mobj = setmetatable({}, arith_meta)
local mobj2 = setmetatable({}, arith_meta)
for _ = 1, 12 do
    metaops(mobj, mobj2)
end
show(metaops(mobj, mobj2))
show(tostring(mobj))

local function barrier_stress()
    local old = {}
    for i = 1, 200 do
        old[i] = {}
    end
    collectgarbage()
    collectgarbage()
    for round = 1, 20 do
        for i = 1, 200 do
            old[i] = {v = i + round}
        end
        old.field = {round}
        collectgarbage("step")
    end
    local s = 0
    for i = 1, 200 do
        s = s + old[i].v
    end
    return s, old.field[1]
end
show(barrier_stress())

local function constructors()
    local out = 0
    for i = 1, 30 do
        local a = {i, i + 1, i + 2}
        local b = {x = i, y = i + 1}
        local c = {i, x = i, [i + 10] = i, "s", nil, 3.5}
        local d = {f(i)}
        out = out + a[3] + b.y + c[1] + c[i + 10] + #c + (d[1] or 0)
    end
    return out
end
function f(x) return x, x end
show(constructors())

local function strmethods(s)
    return s:upper(), s:len(), s:sub(2, 3), s:byte(1), s:rep(2), s:find("l"), #s
end
for _ = 1, 12 do
    strmethods("hello")
end
show(strmethods("hello"))
show(pcall(function() local v = 5; for _ = 1, 12 do v = v:foo() end end))

local function concats()
    local s = ""
    for i = 1, 30 do
        s = s .. i .. 1.5 .. "x"
    end
    local n = 5
    return #s, s:sub(1, 12), n .. "", "" .. n, 1 .. 2, 1.5 .. 2.5, n .. n
end
show(concats())

for k, v in pairs({10, 20, 30, a = 1}) do
    show(k, v)
end
local function iter()
    local s = 0
    local tt = {}
    for i = 1, 50 do tt[i] = i end
    for _ = 1, 12 do
        for i, v in ipairs(tt) do s = s + i * v end
        for k, v in pairs(tt) do s = s + k - v end
        for i = #tt, 1, -1 do s = s + tt[i] end
    end
    return s
end
show(iter())
