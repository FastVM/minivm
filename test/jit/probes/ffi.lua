
local function want0()
    local n = 0
    for i = 1, 40 do
        string.rep("x", 2)
        n = n + 1
    end
    return n
end
print(want0())

local function want1()
    local total = 0
    for i = 1, 40 do
        total = total + #string.format("%d", i)
    end
    return total
end
print(want1())

local function want2()
    local x, y = 0, 0
    for i = 1, 40 do
        local a, b = string.find("hello world", "o w")
        x = x + a
        y = y + b
    end
    return x, y
end
print(want2())

local function want3()
    local x, y, z = 0, 0, ""
    for i = 1, 40 do
        local a, b, c = string.find("hello", "(l+)")
        x = x + a
        y = y + b
        z = c
    end
    return x, y, z
end
print(want3())

local function want4()
    local s = 0
    for i = 1, 40 do
        local a, b, c, d = string.find("hello", "(l)(l)")
        s = s + a + b + #c + #d
    end
    return s
end
print(want4())

local function multi()
    local n = 0
    for i = 1, 40 do
        local t = { string.find("hello", "(l)(l)") }
        n = n + #t
    end
    return n
end
print(multi())

local function tail(s)
    return string.upper(s)
end
for i = 1, 40 do
    tail("x")
end
print(tail("abc"))

local function fewer()
    local a, b, c
    for i = 1, 40 do
        a, b, c = string.upper("x")
    end
    return a, b, c
end
print(fewer())

local function poly_result(s)
    local r
    for i = 1, 40 do
        r = string.match(s, "%d+")
    end
    return r
end
for i = 1, 20 do
    poly_result("a1")
    poly_result("none")
end
print(poly_result("a12"), poly_result("none"))

local function poly_kinds(v)
    local r
    for i = 1, 40 do
        r = rawlen(v)
    end
    return r
end
print(poly_kinds("abc"), poly_kinds({ 1, 2 }))

local function nothing()
    local r = "unset"
    for i = 1, 40 do
        r = print
        r = io.write()
    end
    return r == io.stdout
end
print(nothing())

local function errors()
    local ok, bad = 0, 0
    for i = 1, 60 do
        local good, msg = pcall(string.rep, i % 3 == 0 and {} or "x", 2)
        if good then
            ok = ok + 1
        else
            bad = bad + 1
        end
    end
    return ok, bad
end
print(errors())

local function direct_error(v)
    local n = 0
    for i = 1, 60 do
        n = n + #string.upper(i == 50 and v or "x")
    end
    return n
end
print(pcall(direct_error, {}))
print(pcall(direct_error, "ok"))

local function raises(i)
    if i % 2 == 0 then
        error("even " .. i)
    end
    return i
end
local function catch()
    local total = 0
    local last
    for i = 1, 40 do
        local ok, v = pcall(raises, i)
        if ok then
            total = total + v
        else
            last = v
        end
    end
    return total, last
end
print(catch())

local function selects(...)
    local n = 0
    for i = 1, 40 do
        n = n + select("#", ...)
    end
    return n
end
print(selects(), selects(1), selects(1, nil, 3))

local co = coroutine.wrap(function()
    local total = 0
    for i = 1, 60 do
        total = total + coroutine.yield(i)
    end
    return total
end)
local sum = co()
for i = 1, 60 do
    sum = sum + co(i)
end
print(sum)

local function gen(n)
    return coroutine.wrap(function()
        for i = 1, n do
            coroutine.yield(i, i * 2)
        end
    end)
end
local acc = 0
for round = 1, 10 do
    for a, b in gen(10) do
        acc = acc + a + b
    end
end
print(acc)

local function pc_yield()
    local co2 = coroutine.create(function()
        local t = 0
        for i = 1, 40 do
            local ok, v = pcall(coroutine.yield, i)
            t = t + v
        end
        return t
    end)
    local last
    local ok, v = coroutine.resume(co2)
    for i = 1, 40 do
        ok, v = coroutine.resume(co2, i)
    end
    return ok, v
end
print(pc_yield())

local function deep(n)
    local s = string.format("%d", n)
    if n == 0 then
        return #s
    end
    return #s + deep(n - 1)
end
print(deep(10000))
print(deep(100))

local function grow_args(...)
    return select("#", ...)
end
local function many_args()
    local r = 0
    for i = 1, 40 do
        r = r + math.max(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20)
    end
    return r
end
print(many_args())

local function sorted()
    local out = 0
    for i = 1, 30 do
        local t = { 5, 3, 1, 4, 2 }
        table.sort(t, function(a, b) return a > b end)
        out = out + t[1]
    end
    return out
end
print(sorted())

local function replacer()
    local n = 0
    for i = 1, 30 do
        local s = string.gsub("a b c", "%a", function(c)
            n = n + 1
            return c:upper()
        end)
        n = n + #s
    end
    return n
end
print(replacer())

local function replacer_err()
    return pcall(string.gsub, "a", "a", function() error("boom", 0) end)
end
for i = 1, 30 do
    replacer_err()
end
print(replacer_err())

local calls = 0
local function hooked()
    local n = 0
    for i = 1, 200 do
        n = n + #string.format("%d", i)
        if i == 100 then
            debug.sethook(function() calls = calls + 1 end, "c")
        end
    end
    debug.sethook()
    return n
end
print(hooked(), calls > 0)

local lines = 0
local function line_hooked()
    local n = 0
    for i = 1, 100 do
        n = n + #string.upper("x")
        if i == 50 then
            debug.sethook(function() lines = lines + 1 end, "l")
        end
    end
    debug.sethook()
    return n
end
print(line_hooked(), lines > 0)

local function swapped()
    local f = string.upper
    local r
    for i = 1, 60 do
        if i == 40 then
            f = string.lower
        end
        r = f("aB")
    end
    return r
end
print(swapped())

local function swapped_kind()
    local f = string.upper
    local r
    for i = 1, 60 do
        if i == 40 then
            f = function(s) return s .. "!" end
        end
        r = f("aB")
    end
    return r
end
print(swapped_kind())

local function callable()
    local t = setmetatable({}, { __call = function(_, x) return x + 1 end })
    local f = string.len
    local r = 0
    for i = 1, 60 do
        if i > 40 then
            r = r + t(1)
        else
            r = r + f("ab")
        end
    end
    return r
end
print(callable())

local function with_close()
    local n = 0
    local mt = { __close = function() n = n + 1 end }
    for i = 1, 40 do
        local c <close> = setmetatable({}, mt)
        n = n + #string.upper("ab")
    end
    return n
end
print(with_close())

local function in_inline(x)
    return #string.format("%d", x)
end
local function outer_inline()
    local n = 0
    for i = 1, 40 do
        n = n + in_inline(i)
    end
    return n
end
print(outer_inline())

local function reads_after(s)
    local a = 1
    local b = 2.5
    local c = "z"
    for i = 1, 40 do
        local u = string.upper(s)
        a = a + #u
        b = b + #u
        c = u
    end
    return a, b, c
end
print(reads_after("abc"))

local function collect_during()
    local n = 0
    for i = 1, 200 do
        local t = {}
        for j = 1, 5 do
            t[j] = string.rep("k", j + 40)
        end
        n = n + #table.concat(t)
        if i % 50 == 0 then
            collectgarbage()
        end
    end
    return n
end
print(collect_during())
