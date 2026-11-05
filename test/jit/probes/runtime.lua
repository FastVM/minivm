local function show(...)
    local parts = {}
    for i = 1, select("#", ...) do
        parts[i] = tostring(select(i, ...))
    end
    print(table.concat(parts, " "))
end

local lines = {}
local function hook(ev, line)
    lines[#lines + 1] = ev .. ":" .. tostring(line)
end
local function hooked_loop(n)
    local s = 0
    for i = 1, n do
        s = s + i
        if i == 10 then
            debug.sethook(hook, "l")
        elseif i == 13 then
            debug.sethook()
        end
    end
    return s
end
for _ = 1, 3 do
    hooked_loop(20)
end
show(hooked_loop(20), #lines > 0, lines[1])
show(#lines)
lines = {}
local function callee(x) return x + 1 end
local function hooked_calls()
    local s = 0
    for i = 1, 30 do
        s = callee(s)
        if i == 15 then debug.sethook(hook, "cr") end
        if i == 20 then debug.sethook() end
    end
    return s
end
for _ = 1, 3 do hooked_calls() end
show(hooked_calls(), #lines)
lines = {}
local counts = 0
local function hooked_count()
    debug.sethook(function() counts = counts + 1 end, "", 3)
    local s = 0
    for i = 1, 100 do
        s = s + callee(i)
    end
    debug.sethook()
    return s
end
for _ = 1, 3 do hooked_count() end
show(hooked_count(), counts > 0)
show(debug.gethook())

local function locals_view()
    local a = 1
    local b = 2.5
    local c = "str"
    local d = {}
    for i = 1, 20 do
        a = a + i
        b = b * 1.5
        c = c .. "x"
        if i == 15 then
            local names = {}
            local j = 1
            while true do
                local n, v = debug.getlocal(1, j)
                if n == nil then break end
                if string.sub(n, 1, 1) ~= "(" then
                    names[#names + 1] = n .. "=" .. (type(v) == "number" and tostring(v) or type(v))
                end
                j = j + 1
            end
            show(table.concat(names, " "))
        end
    end
    return a, b, #c
end
show(locals_view())
local function other_frame_view(level)
    local n, v = debug.getlocal(level, 2)
    return n, v
end
local function viewed()
    local out
    local x = 0
    for i = 1, 20 do
        x = x + i
        local dead = x * 2
        out = {other_frame_view(2)}
    end
    return out[1], out[2], x
end
show(viewed())
local function set_local()
    local a = 1
    local s = 0
    for i = 1, 20 do
        a = a + 1
        if i == 12 then
            debug.setlocal(1, 1, 1000)
        end
        if i == 14 then
            debug.setlocal(1, 1, "str")
            a = 0
        end
        s = s + a
    end
    return a, s
end
show(set_local())
local function set_in_caller()
    debug.setlocal(2, 1, 99.5)
end
local function kind_change()
    local v = 1
    local s = 0
    for i = 1, 12 do
        s = s + v
        if i == 6 then
            set_in_caller()
        end
    end
    return v, s
end
show(kind_change())
local function info()
    local out = {}
    for i = 1, 12 do
        local d = debug.getinfo(1, "lS")
        out[#out + 1] = d.currentline
    end
    return out[1], out[12]
end
show(info())
show(string.gsub(debug.traceback("tb", 1), "\n.*", ""))
local function tb_inner()
    return debug.traceback("inner")
end
local function tb_outer()
    local r
    for _ = 1, 12 do
        r = tb_inner()
    end
    return r
end
show(tb_outer())

local co = coroutine.wrap(function(a)
    local s = a
    for i = 1, 30 do
        s = s + coroutine.yield(s, i)
    end
    return "done", s
end)
local v1, v2 = co(1)
for i = 1, 30 do
    v1, v2 = co(i)
end
show(v1, v2)
local function gen(n)
    return coroutine.wrap(function()
        for i = 1, n do
            coroutine.yield(i, i * 0.5, "s" .. i)
        end
    end)
end
local total = 0
for a, b, c in gen(50) do
    total = total + a + b + #c
end
show(total)
local function pc_yield()
    local co2 = coroutine.create(function()
        for i = 1, 20 do
            local ok, v = pcall(function()
                local x = coroutine.yield(i)
                if x == 5 then error("five") end
                return x
            end)
            coroutine.yield(ok, v)
        end
        return "end"
    end)
    local out = {}
    local _, v = coroutine.resume(co2)
    for i = 1, 20 do
        local _, ok, val = coroutine.resume(co2, i)
        out[#out + 1] = tostring(ok) .. ":" .. tostring(val)
        _, v = coroutine.resume(co2)
    end
    return table.concat(out, ","), v
end
show(pc_yield())
local mco = coroutine.wrap(function()
    local t = setmetatable({}, {
        __index = function(_, k) return coroutine.yield(k) end,
        __add = function(a, b) return coroutine.yield("add") end,
    })
    local s = 0
    for i = 1, 12 do
        s = s + t[i]
        s = s + (t + 1)
    end
    return "total", s
end)
local a, b = mco()
for i = 1, 24 do
    a, b = mco(i)
end
show(a, b)
local function growstack(depth)
    if depth == 0 then
        return coroutine.yield("deep")
    end
    return 1 + growstack(depth - 1)
end
local gco = coroutine.wrap(function()
    local s = 0
    for _ = 1, 3 do
        s = s + growstack(5000)
    end
    return s
end)
show(gco())
show(gco(1))
show(gco(1))
show(gco(1))
local dead = coroutine.create(function() error("in co") end)
show(coroutine.resume(dead))
show(coroutine.resume(dead))
local closeco = coroutine.create(function()
    local v <close> = setmetatable({}, {__close = function() print("closed") end})
    coroutine.yield(1)
end)
coroutine.resume(closeco)
show(coroutine.close(closeco))

local function churn()
    local keep = {}
    for i = 1, 3000 do
        local t = {i, i + 1, name = "n" .. i}
        local s = t.name .. "x"
        local f = function() return i end
        if i % 100 == 0 then
            keep[#keep + 1] = {t, s, f}
        end
    end
    local sum = 0
    for _, e in ipairs(keep) do
        sum = sum + e[1][1] + #e[2] + e[3]()
    end
    return sum
end
for _ = 1, 5 do
    show(churn())
end
local function live_across_gc()
    local a = {1}
    local b = "s" .. a[1]
    local c = 2.5
    local d = {x = a}
    local e = function() return a end
    for i = 1, 200 do
        local junk = {}
        for j = 1, 50 do junk[j] = {j} end
        if i % 40 == 0 then collectgarbage() end
        a[1] = a[1] + 1
        c = c + 0.5
        b = b .. "y"
        d.x = e()
    end
    return a[1], #b, c, d.x == a
end
show(live_across_gc())
local weak = setmetatable({}, {__mode = "v"})
local function weak_test()
    for i = 1, 200 do
        weak[i] = {i}
    end
    collectgarbage()
    collectgarbage()
    local n = 0
    for _ in pairs(weak) do n = n + 1 end
    return n
end
show(weak_test())
local finalized = 0
local function final_test()
    for i = 1, 200 do
        setmetatable({}, {__gc = function() finalized = finalized + 1 end})
    end
    collectgarbage()
    collectgarbage()
    return finalized
end
show(final_test())
local function gc_in_finalizer_loop()
    local s = 0
    for i = 1, 2000 do
        setmetatable({}, {__gc = function() s = s + 1 end})
        s = s + 0
    end
    collectgarbage()
    return s
end
show(gc_in_finalizer_loop())
local function load_code()
    local s = 0
    for i = 1, 40 do
        local f = load("local a, b = ... ; return a + b * " .. i)
        for _ = 1, 8 do
            s = s + f(i, 2)
        end
    end
    return s
end
show(load_code())
local dumped = string.dump(function(x) local s = 0; for i = 1, x do s = s + i end return s end)
local df = load(dumped, "d", "b")
for _ = 1, 12 do df(10) end
show(df(100))
local str_num = 0
for i = 1, 30 do
    str_num = str_num + ("10" + i) + ((i .. "") + 1)
end
show(str_num)
local cases = {}
for i = 1, 30 do
    if i % 3 == 0 then
        goto skip
    end
    if i > 25 then
        break
    end
    cases[#cases + 1] = i
    ::skip::
end
show(table.concat(cases, ","))
local nested = 0
for i = 1, 30 do
    for j = i, 1, -1 do
        for k = 1.0, 2.0, 0.5 do
            nested = nested + i * j + k
        end
    end
end
show(nested)
local rept = 0
repeat
    local u = rept + 1
    rept = u
until u >= 20
show(rept)
for i = math.maxinteger - 2, math.maxinteger do show(i) end
for i = math.mininteger + 2, math.mininteger, -1 do show(i) end
for i = 1, 0 do show("never") end
show(pcall(function() for i = 1, 10, 0 do end end))
show(pcall(function() for i = "a", 10 do end end))
local big_frame = load((function()
    local src = {}
    for i = 1, 150 do src[#src + 1] = "local v" .. i .. " = " .. i .. " + (... or 0)" end
    src[#src + 1] = "local s = 0"
    for i = 1, 150 do src[#src + 1] = "s = s + v" .. i end
    src[#src + 1] = "return s"
    return table.concat(src, "\n")
end)())
for _ = 1, 12 do big_frame(1) end
show(big_frame(1))
local many_args = load((function()
    local names, vals = {}, {}
    for i = 1, 100 do names[i] = "a" .. i; vals[i] = tostring(i) end
    return "local function f(" .. table.concat(names, ",") .. ") return a1 + a100 end\n"
        .. "local s = 0\nfor i = 1, 12 do s = s + f(" .. table.concat(vals, ",") .. ") end\nreturn s"
end)())
show(many_args())
