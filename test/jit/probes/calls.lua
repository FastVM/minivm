local function show(...)
    local parts = {}
    for i = 1, select("#", ...) do
        parts[i] = tostring(select(i, ...))
    end
    print(table.concat(parts, " "))
end

local function a1(x) return "a1", x end
local function a2(x) return "a2", x end
local function a3(x, y) return "a3", x, y end
local function a4(...) return "a4", select("#", ...), ... end
local function a5() return "a5" end
local function call_it(f, ...)
    return f(1, 2)
end
local function site(f)
    return f(1, 2)
end
for round = 1, 4 do
    for _, f in ipairs({a1, a2, a3, a4, a5}) do
        for _ = 1, 8 do
            site(f)
        end
        show(round, site(f))
    end
end
for _, f in ipairs({a1, a2, a3, a4, a5, a1, a2}) do
    show(site(f))
end

local function site3(f, n)
    local s
    for i = 1, n do
        s = f(i)
    end
    return s
end
local function ri(x) return x end
local function rf(x) return x + 0.5 end
local function rs(x) return "s" .. x end
local function rn(x) return nil end
local function rt(x) return {x} end
local function rv(x) end
for round = 1, 3 do
    for _, f in ipairs({ri, rf, rs, rn, rt, rv}) do
        show(type(site3(f, 12)))
    end
end

local function ret_kinds(i)
    if i % 4 == 0 then return i
    elseif i % 4 == 1 then return i + 0.5
    elseif i % 4 == 2 then return "s"
    else return nil end
end
local function poly_ret()
    local out = {}
    for i = 1, 40 do
        local v = ret_kinds(i)
        out[#out + 1] = tostring(v)
    end
    return table.concat(out, ",")
end
show(poly_ret())

local function zero() end
local function one() return 1 end
local function two() return 1, 2 end
local function three() return 1, 2.5, "x" end
local function four() return 1, 2, 3, 4 end
local function mres()
    local s = 0
    for _ = 1, 30 do
        zero()
        one()
        two()
        local a = zero()
        local b = one()
        local c = two()
        local d, e = zero()
        local f, g = one()
        local h, i = two()
        local j, k = three()
        local l, m, n = three()
        local o, p, q, r = three()
        local t, u = four()
        s = s + (b + c + f + h + i + j + l + t + u)
        assert(a == nil and d == nil and e == nil and g == nil and o == 1 and r == nil)
        assert(k == 2.5 and m == 2.5 and n == "x" and p == 2.5 and q == "x")
    end
    return s
end
show(mres())
local function spread_ret()
    return (two()), two()
end
local function spread_args(...)
    return select("#", ...), ...
end
for _ = 1, 12 do
    spread_args(two())
    spread_args(1, two())
    spread_args(two(), 1)
    spread_args()
end
show(spread_ret())
show(spread_args(two()))
show(spread_args(1, four()))

local function missing(a, b, c, d)
    return a, b, c, d
end
local function extra(a)
    return a
end
for _ = 1, 12 do
    missing()
    missing(1)
    missing(1, 2, 3, 4)
    extra(1, 2, 3, 4, 5)
    extra()
end
show(missing())
show(missing(1))
show(missing(1, "x", 2.5, true))
show(extra(1, 2, 3))
show(extra())

local function vararg(...)
    local a, b = ...
    local t = {...}
    return select("#", ...), a, b, #t
end
for _ = 1, 12 do
    vararg()
    vararg(1)
    vararg(1, 2, 3)
end
show(vararg())
show(vararg(nil))
show(vararg(1, nil, 3))

local function swap_tail(a, b, n)
    if n == 0 then return a, b end
    return swap_tail(b, a, n - 1)
end
show(swap_tail(1, 2, 21))
show(swap_tail(1, 2, 20))
show(swap_tail("x", 1.5, 21))
local function rotate_tail(a, b, c, d, n)
    if n == 0 then return a, b, c, d end
    return rotate_tail(d, a, b, c, n - 1)
end
show(rotate_tail(1, 2, 3, 4, 41))
show(rotate_tail(1, "b", 3.5, true, 42))
local function dup_tail(a, b, c, n)
    if n == 0 then return a, b, c end
    return dup_tail(c, c, a, n - 1)
end
show(dup_tail(1, 2, 3, 31))
local function shift_tail(a, b, c, n)
    if n == 0 then return a, b, c end
    return shift_tail(b, c, n, n - 1)
end
show(shift_tail(1, 2, 3, 31))
local function lit_tail(a, b, n)
    if n == 0 then return a, b end
    return lit_tail(n, 7, n - 1)
end
show(lit_tail(1, 2, 31))
local function drop_tail(n, x)
    if n == 0 then return x end
    return drop_tail(n - 1)
end
show(drop_tail(31, 5))
show(drop_tail(0, 5))
local ping, pong
function ping(n, acc)
    if n == 0 then return "ping", acc end
    return pong(n - 1, acc + 1)
end
function pong(n, acc)
    if n == 0 then return "pong", acc end
    return ping(n - 1, acc + 2)
end
show(ping(100000, 0))
show(ping(100001, 0))
local function tail_var(f, n, ...)
    if n == 0 then return select("#", ...) end
    return tail_var(f, n - 1, f(...))
end
show(tail_var(function(...) return ..., 1 end, 20, 1))
local function tail_cfn(s)
    return tostring(s)
end
for _ = 1, 12 do tail_cfn(1) end
show(tail_cfn(1))
local function tail_poly(f, x)
    return f(x)
end
for _ = 1, 3 do
    for _, f in ipairs({ri, rf, rs, tostring, tail_cfn}) do
        for _ = 1, 8 do tail_poly(f, 1) end
        show(tail_poly(f, 1))
    end
end

local function recurse(n)
    if n == 0 then return 0 end
    return 1 + recurse(n - 1)
end
show(recurse(50000))
local function overflow(n)
    return 1 + overflow(n + 1)
end
local ok, msg = pcall(overflow, 1)
show(ok, string.find(tostring(msg), "stack overflow") ~= nil)
ok, msg = pcall(overflow, 1)
show(ok, string.find(tostring(msg), "stack overflow") ~= nil)
local co = coroutine.wrap(function()
    local ok2, m2 = pcall(overflow, 1)
    coroutine.yield(ok2, string.find(tostring(m2), "stack overflow") ~= nil)
    coroutine.yield(recurse(50000))
end)
show(co())
show(co())

local function errline(n)
    local x = n
    x = x + 1
    if n == 20 then
        error("boom " .. x)
    end
    return x
end
for i = 15, 25 do
    show(pcall(errline, i))
end
local function errarith(t)
    local s = 0
    for i = 1, 20 do
        s = s + t[i]
    end
    return s
end
local et = {}
for i = 1, 20 do et[i] = i end
show(errarith(et))
et[15] = "not a number"
show(pcall(errarith, et))
et[15] = nil
show((pcall(errarith, et)))
show(pcall(function() local n; for _ = 1, 12 do n() end end))
show(pcall(function() local n = {}; for _ = 1, 12 do n.x.y = 1 end end))
show((xpcall(errarith, function(m) return "handled: " .. tostring(m) end, et)))
show(type(select(2, xpcall(errarith, debug.traceback, et))))

local function fastcalls(x)
    return math.floor(x), math.abs(x), math.max(x, 1), type(x), tostring(x), select("#", x), rawlen("abc"), #tostring(x), math.sqrt(x * x)
end
for _ = 1, 12 do
    fastcalls(2.5)
    fastcalls(3)
end
show(fastcalls(2.5))
show(fastcalls(3))
local function floorpoly(v)
    return math.floor(v)
end
for _ = 1, 3 do
    for _, v in ipairs({1, 1.5, -1.5, "7", 2^40}) do
        for _ = 1, 8 do floorpoly(v) end
        show(floorpoly(v))
    end
end
show(pcall(floorpoly, "x"))
show(pcall(floorpoly, {}))
local function replaced(x)
    return math.floor(x)
end
for _ = 1, 12 do replaced(1.5) end
show(replaced(1.5))
local saved = math.floor
math.floor = function(x) return "patched" end
show(replaced(1.5))
math.floor = saved
show(replaced(1.5))
local function fn_arg(f, x)
    return f(x)
end
for _ = 1, 3 do
    for _, f in ipairs({math.floor, math.abs, tostring, type, ri}) do
        for _ = 1, 8 do fn_arg(f, -1.5) end
        show(fn_arg(f, -1.5))
    end
end
local function want0(x)
    tostring(x)
    math.floor(x)
    return x
end
for _ = 1, 12 do want0(1.5) end
show(want0(1.5))
local function want2(x)
    local a, b = math.floor(x)
    local c, d = string.find("hello", "l")
    local e, f = math.modf(x)
    return a, b, c, d, e, f
end
for _ = 1, 12 do want2(1.5) end
show(want2(1.5))
local function nores()
    local a = print
    local b = select(2, "a")
    local c = select(5, "a")
    return b, c, a ~= nil
end
for _ = 1, 12 do nores() end
show(nores())

local callable = setmetatable({}, {__call = function(self, a, b) return "callable", a, b end})
local function callit(f)
    return f(1, 2)
end
for _ = 1, 12 do callit(callable) end
show(callit(callable))
show(pcall(callit, {}))
show(pcall(callit, 5))
show(callit(callable))

local function closures()
    local fs = {}
    for i = 1, 20 do
        local j = i
        fs[i] = function()
            j = j + 1
            return j
        end
    end
    local s = 0
    for round = 1, 3 do
        for i = 1, 20 do
            s = s + fs[i]()
        end
    end
    return s
end
show(closures())
local function counter()
    local n = 0
    local k = 0.5
    return function()
        n = n + 1
        k = k * 2
        return n, k
    end
end
local c1, c2 = counter(), counter()
for _ = 1, 12 do c1() end
show(c1())
show(c2())
local function retype_up()
    local v = 1
    local function get() return v end
    local function set(x) v = x end
    local out = {}
    for i = 1, 30 do
        if i == 10 then set(1.5) end
        if i == 20 then set("s") end
        if i == 25 then set(nil) end
        out[#out + 1] = tostring(get())
    end
    return table.concat(out, ",")
end
show(retype_up())
local function shared_cell()
    local x = 1
    local inc = function() x = x + 1 end
    for i = 1, 20 do
        x = x + 1
        inc()
        x = x * 1
    end
    return x
end
show(shared_cell())
local function shared_float()
    local x = 1
    local get = function() return x end
    local s = 0
    for i = 1, 20 do
        x = i
        s = s + get()
        x = i + 0.5
        s = s + get()
    end
    return s
end
show(shared_float())
local function loop_capture()
    local fs = {}
    local i = 1
    while i <= 15 do
        local v = i * 2
        fs[i] = function() v = v + 1; return v end
        i = i + 1
    end
    local s = 0
    for k = 1, 15 do s = s + fs[k]() + fs[k]() end
    return s
end
show(loop_capture())
local function upjoin()
    local a = 1
    local b = 2
    local function fa() return a end
    local function fb() return b end
    local s = 0
    for i = 1, 12 do s = s + fa() end
    debug.upvaluejoin(fa, 1, fb, 1)
    for i = 1, 12 do s = s + fa() end
    return s, fa(), fb()
end
show(upjoin())
local function self_alias()
    local x = 1
    local function peek() return x end
    local function body()
        local y = 10
        local function gy() return y end
        debug.upvaluejoin(peek, 1, gy, 1)
        local s = 0
        for i = 1, 12 do
            y = i
            s = s + peek()
        end
        return s
    end
    return body()
end
show(self_alias())
local function tbc()
    local log = {}
    for i = 1, 12 do
        local v <close> = setmetatable({}, {__close = function() log[#log + 1] = i end})
        if i % 2 == 0 then
            goto cont
        end
        log[#log + 1] = -i
        ::cont::
    end
    return table.concat(log, ",")
end
show(tbc())
local function tbc_err()
    local ok3, e = pcall(function()
        local v <close> = setmetatable({}, {__close = function(_, m) error("close:" .. tostring(m)) end})
        error("first")
    end)
    return ok3, e
end
for _ = 1, 12 do tbc_err() end
show(tbc_err())
local g1 = 1
function glob_inc() g1 = g1 + 1 end
glob = 0
for i = 1, 30 do
    glob = glob + i
    glob_inc()
end
show(glob, g1)
_ENV = setmetatable({}, {__index = _ENV, __newindex = function(t, k, v) rawset(t, k, v * 2) end})
gg = 5
show(gg)
