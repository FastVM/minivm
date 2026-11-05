
local function a(x) return x + 1 end
local function b(x) return x * 2 end
local function c(x, y) return x - (y or 0) end
local function d(x) return tostring(x) end
local function e(...) return select("#", ...) end
local function f() end

local fns = { a, b, c, d, e, f }

local function call_each(list, n)
    local out = {}
    for round = 1, 40 do
        for i = 1, n do
            out[i] = list[i](round)
        end
    end
    return out
end
local r = call_each(fns, 2)
print(r[1], r[2])
r = call_each(fns, 3)
print(r[1], r[2], r[3])
r = call_each(fns, 4)
print(r[1], r[2], r[3], r[4])
r = call_each(fns, 5)
print(r[1], r[2], r[3], r[4], r[5])
r = call_each(fns, 6)
print(r[1], r[2], r[3], r[4], r[5], r[6])

local function tail_call(g, x)
    return g(x)
end
local function tail_each(list, n)
    local out = {}
    for round = 1, 40 do
        for i = 1, n do
            out[i] = tail_call(list[i], round)
        end
    end
    return out
end
r = tail_each(fns, 2)
print(r[1], r[2])
r = tail_each(fns, 4)
print(r[1], r[2], r[3], r[4])
r = tail_each(fns, 6)
print(r[1], r[2], r[3], r[4], r[5], r[6])

local function multi_arg(g, x, y, z)
    return g(y, z, x)
end
local function p3(x, y, z) return z, y, x end
local function p1(x) return x end
local function p0() return "none" end
local function pv(...) return select("#", ...), ... end
for i = 1, 40 do
    multi_arg(p3, 1, 2, 3)
    multi_arg(p1, 1, 2, 3)
    multi_arg(p0, 1, 2, 3)
    multi_arg(pv, 1, 2, 3)
end
print(multi_arg(p3, 1, 2, 3))
print(multi_arg(p1, 1, 2, 3))
print(multi_arg(p0, 1, 2, 3))
print(multi_arg(pv, 1, 2, 3))

local function overlap(g, x, y)
    return g(y, x)
end
for i = 1, 40 do
    overlap(function(p, q) return p - q end, i, 1)
    overlap(function(p, q) return q - p end, i, 1)
end
print(overlap(function(p, q) return p - q end, 10, 1), overlap(function(p, q) return q - p end, 10, 1))

local function spread_poly(g, h, x)
    return g(h(x))
end
local function inc(x) return x + 1 end
local function dbl(x) return x * 2 end
local function fl(x) return x + 0.5 end
for i = 1, 40 do
    spread_poly(inc, inc, i)
    spread_poly(dbl, inc, i)
    spread_poly(fl, fl, i)
    spread_poly(inc, fl, i)
end
print(spread_poly(inc, inc, 1), spread_poly(dbl, inc, 1), spread_poly(fl, fl, 1), spread_poly(inc, fl, 1))
print(pcall(spread_poly, tostring, f, 1))
print(pcall(spread_poly, inc, f, 1))

local upv = 1
local function read_up()
    return upv
end
local function set_up(v)
    upv = v
end
local function up_kinds()
    local parts = {}
    local vals = { 1, 1.5, "s", true, false, {}, print, nil }
    for round = 1, 30 do
        for i = 1, 8 do
            set_up(vals[i])
            parts[i] = type(read_up())
        end
    end
    return table.concat(parts, " ")
end
print(up_kinds())

local nil_up
local function read_nil()
    return nil_up
end
for i = 1, 30 do
    read_nil()
end
print(read_nil())
nil_up = 5
print(read_nil())
for i = 1, 30 do
    read_nil()
end
nil_up = "s"
print(read_nil())
nil_up = nil
print(read_nil())

local function open_up()
    local v = 1
    local function r()
        return v
    end
    local out = {}
    for i = 1, 30 do
        v = i
        out[1] = r()
        v = i + 0.5
        out[2] = r()
        v = "x" .. i
        out[3] = r()
    end
    return out[1], out[2], out[3]
end
print(open_up())

local function mk_closed(v)
    return function() return v end, function(n) v = n end
end
local get1, put1 = mk_closed(1)
local total = 0
for i = 1, 60 do
    total = total + get1()
    if i == 30 then
        put1(2.5)
    end
end
print(total)
put1("str")
print(get1())

local function var_set()
    local t = { a = 1, b = 2, c = 3 }
    local keys = { "a", "b", "c", "d", "e" }
    for round = 1, 40 do
        for i = 1, #keys do
            local k = keys[i]
            t[k] = (t[k] or 0) + 1
        end
    end
    return t.a, t.b, t.c, t.d, t.e
end
print(var_set())

local function var_set_kinds()
    local t = { x = 1 }
    local k = "x"
    local parts = {}
    for i = 1, 60 do
        if i % 3 == 0 then
            t[k] = i
        elseif i % 3 == 1 then
            t[k] = "s" .. i
        else
            t[k] = { i }
        end
        parts[1] = type(t[k])
    end
    return parts[1]
end
print(var_set_kinds())

local function var_set_nil()
    local t = { x = 1, y = 2 }
    local k = "x"
    local n = 0
    for i = 1, 60 do
        t[k] = nil
        n = n + (t[k] == nil and 1 or 0)
        t[k] = i
    end
    return n, t.x
end
print(var_set_nil())

local function var_set_meta()
    local log = {}
    local t = setmetatable({ present = 1 }, { __newindex = function(self, k, v)
        log[#log + 1] = k
        rawset(self, k, v)
    end })
    local names = { "present", "absent" }
    for i = 1, 40 do
        t[names[1]] = i
    end
    t[names[2]] = 1
    t[names[2]] = 2
    return #log, t.present, t.absent
end
print(var_set_meta())

local function var_set_old()
    local t = { x = false }
    collectgarbage()
    collectgarbage()
    local k = "x"
    for i = 1, 200 do
        t[k] = { i }
        if i % 50 == 0 then
            collectgarbage("step")
        end
    end
    collectgarbage()
    return t.x[1]
end
print(var_set_old())

local function var_set_other_key()
    local t = {}
    local out = 0
    for i = 1, 40 do
        t[i] = i
        t[i + 0.5] = i
        t[true] = i
        out = out + t[i] + t[i + 0.5] + t[true]
    end
    return out
end
print(var_set_other_key())

local function var_set_long()
    local k1 = string.rep("z", 50)
    local t = { [k1] = 0 }
    for i = 1, 40 do
        local k2 = string.rep("z", 50)
        t[k2] = t[k2] + 1
    end
    return t[k1]
end
print(var_set_long())

local function fast_spread(x)
    local a1, a2, a3
    for i = 1, 40 do
        a1 = tonumber(tostring(x))
        a2 = math.floor(tonumber(x) or 0)
        a3 = select("#", tostring(x))
    end
    return a1, a2, a3
end
print(fast_spread(5), fast_spread("7"), fast_spread("nope"))

local function fast_fails(x)
    local ok, msg
    for i = 1, 40 do
        ok, msg = pcall(function()
            return math.floor(tostring(x))
        end)
    end
    return ok, msg
end
print(fast_fails(1.5), fast_fails("abc"))

local function fast_with_cell()
    local v = 0
    local get = function() return v end
    for i = 1, 40 do
        v = math.floor(i / 2)
        v = string.upper("x") .. get()
        v = i
    end
    return v
end
print(fast_with_cell())

local function pairs_many()
    local t = { 1, 2, 3, x = 1 }
    local n = 0
    for i = 1, 30 do
        for k, v in pairs(t) do
            n = n + v
        end
        for k, v in ipairs(t) do
            n = n + v
        end
        for k, v in next, t do
            n = n + v
        end
    end
    return n
end
print(pairs_many())

local function six()
    return 1, 2, 3, 4, 5, 6
end
local function want_many()
    local s = 0
    for i = 1, 40 do
        local a1, a2, a3, a4, a5, a6, a7 = six()
        s = s + a1 + a2 + a3 + a4 + a5 + a6 + (a7 or 0)
    end
    return s
end
print(want_many())

local function many_cells()
    local a1, a2, a3, a4 = 0, 0, 0, 0
    local peek = function() return a1 + a2 + a3 + a4 end
    local s = 0
    for i = 1, 40 do
        a1, a2, a3, a4 = (function() return i, 1, 2, 3 end)()
        s = s + peek()
        a1, a2 = string.find("abc", "b")
        s = s + peek()
        a1 = rawlen("xx")
        s = s + peek()
    end
    return s
end
print(many_cells())

local function tail_ffi(s)
    return string.upper(s)
end
local function tail_ffi_multi(s)
    return string.find(s, "b")
end
local function use_tail_ffi()
    local a1, a2, a3
    for i = 1, 40 do
        a1 = tail_ffi("x")
        a2, a3 = tail_ffi_multi("abc")
    end
    return a1, a2, a3
end
print(use_tail_ffi())
print((function(...) return select("#", ...) end)(tail_ffi_multi("abc")))
print(pcall(tail_ffi, {}))
