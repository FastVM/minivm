
local function classify(x)
    local t = type(x)
    if t == "number" then
        return 1
    elseif t == "string" then
        return 2
    elseif t == "table" then
        return 3
    elseif t == "nil" then
        return 4
    elseif t == "boolean" then
        return 5
    elseif t == "function" then
        return 6
    end
    return 7
end

local function direct(x)
    if type(x) == "table" then
        return "t"
    elseif type(x) ~= "number" then
        return "other"
    end
    return "n"
end

local vals = { 1, 2.5, "s", {}, false, print, classify, coroutine.create(print), io.stdout }
vals[#vals + 1] = 0
for round = 1, 40 do
    local s = 0
    local d = ""
    for i = 1, #vals do
        s = s * 8 + classify(vals[i])
        d = d .. direct(vals[i])
    end
    s = s + classify(nil)
    if round == 40 then
        print(s, d)
    end
end

local names = {}
for i = 1, 40 do
    local a = type(i)
    local b = type(i + 0.5)
    local c = type(tostring(i))
    names[1] = a .. b .. c
end
print(names[1])

local function replaced()
    local out = {}
    local f = type
    for i = 1, 60 do
        if i == 40 then
            f = function() return "number!" end
        end
        out[1] = f(i)
    end
    return out[1]
end
print(replaced())

local real_type = type
for i = 1, 60 do
    if i == 40 then
        type = function() return "global" end
    end
    if i == 39 or i == 40 then
        print(type(1))
    end
end
type = real_type

local floats = { 0.0, -0.0, 0.5, -0.5, 1.5, -1.5, 2.0, -2.0, 1e15, -1e15, 2^53, -(2^53), 2^62, -(2^62), 2^63, -(2^63), 1e300, -1e300, 1/0, -1/0, 0/0, 9007199254740993.0, 0.1, -0.1, 0.9999999999999999, -0.9999999999999999 }
local function show(v)
    if v ~= v then
        return "nan"
    elseif v == 1/0 then
        return "inf"
    elseif v == -1/0 then
        return "-inf"
    end
    return math.type(v) .. ":" .. tostring(v)
end

local floor, ceil, sqrt, abs, max, min = math.floor, math.ceil, math.sqrt, math.abs, math.max, math.min
for round = 1, 30 do
    local line = {}
    for i = 1, #floats do
        local v = floats[i]
        line[#line + 1] = show(floor(v))
        line[#line + 1] = show(ceil(v))
        line[#line + 1] = show(abs(v))
    end
    if round == 30 then
        print(table.concat(line, " "))
    end
end

local function hot_floor(v)
    local r = 0
    for i = 1, 40 do
        r = floor(v)
    end
    return show(r)
end
local function hot_ceil(v)
    local r = 0
    for i = 1, 40 do
        r = ceil(v)
    end
    return show(r)
end
for i = 1, #floats do
    print(hot_floor(floats[i]), hot_ceil(floats[i]))
end

local ints = { 0, 1, -1, 7, -7, math.maxinteger, math.mininteger, 1 << 53 }
for i = 1, #ints do
    local v = ints[i]
    local a, b, c, d = 0, 0, 0, 0
    for j = 1, 30 do
        a = floor(v)
        b = ceil(v)
        c = abs(v)
        d = sqrt(v * v % 1000)
    end
    print(show(a), show(b), show(c), show(d))
end

for i = 1, 30 do
    local x = sqrt(i * 1.0)
    local y = sqrt(i)
    if i == 30 then
        print(x, y, x == y)
    end
end
local neg = 0.0
for i = 1, 30 do
    neg = sqrt(-1.0 * i)
end
print(neg ~= neg)

local pairs_i = { { 1, 2 }, { 2, 1 }, { 3, 3 }, { -5, 5 }, { math.mininteger, math.maxinteger }, { math.maxinteger, math.mininteger } }
for i = 1, #pairs_i do
    local a, b = pairs_i[i][1], pairs_i[i][2]
    local mx, mn = 0, 0
    for j = 1, 30 do
        mx = max(a, b)
        mn = min(a, b)
    end
    print(mx, mn)
end

local nan = 0/0
local pairs_f = { { 1.5, 2.5 }, { 2.5, 1.5 }, { 0.0, -0.0 }, { -0.0, 0.0 }, { nan, 1.0 }, { 1.0, nan }, { 1/0, -1/0 } }
for i = 1, #pairs_f do
    local a, b = pairs_f[i][1], pairs_f[i][2]
    local mx, mn = 0.0, 0.0
    for j = 1, 30 do
        mx = max(a, b)
        mn = min(a, b)
    end
    print(show(mx), show(mn), tostring(mx) == tostring(a) or tostring(mx) == tostring(b))
end

local mixed = { { 1, 2.5 }, { 2.5, 1 }, { 2, 2.0 }, { 2.0, 2 } }
for i = 1, #mixed do
    local a, b = mixed[i][1], mixed[i][2]
    local mx, mn = 0, 0
    for j = 1, 30 do
        mx = max(a, b)
        mn = min(a, b)
    end
    print(show(mx), show(mn))
end

for i = 1, 30 do
    local x = max(i, 10, 20)
    local y = min(i, 10, -20)
    local z = max(i)
    if i == 30 then
        print(x, y, z)
    end
end

local sbyte, slen = string.byte, string.len
local text = "hello\0world\255"
local indices = { 1, 2, 5, 6, 12, 13, 14, 0, -1, -12, -13, -100, math.maxinteger, math.mininteger }
for i = 1, #indices do
    local at = indices[i]
    local r = "unset"
    for j = 1, 30 do
        r = sbyte(text, at)
    end
    print(at, r)
end

local function byte_loop(s)
    local sum = 0
    for i = 1, #s do
        sum = sum * 31 + sbyte(s, i)
    end
    return sum
end
print(byte_loop(text), byte_loop(""), byte_loop(string.rep("abc", 100)))

local function byte_first(s)
    local r = nil
    for i = 1, 30 do
        r = sbyte(s)
    end
    return r
end
print(byte_first("Z"), byte_first(""), byte_first("\0"))

local function byte_lit(s)
    local a, b = nil, nil
    for i = 1, 30 do
        a = sbyte(s, 2)
        b = sbyte(s, -1)
    end
    return a, b
end
print(byte_lit("xy"), byte_lit("x"), byte_lit(""))

local long = string.rep("ab", 200)
local function make_rope(i)
    return long .. string.rep("cd", 200) .. tostring(i)
end
for i = 1, 30 do
    local rope = make_rope(i)
    local n = slen(rope)
    local b = sbyte(rope, 401)
    local c = sbyte(rope, n)
    if i == 30 then
        print(n, b, c)
    end
end
for i = 1, 30 do
    local rope = make_rope(i)
    local n = slen(rope)
    local m = #rope
    if i == 30 then
        print(n, m, sbyte(rope))
    end
end

local function len_loop(s)
    local n = 0
    for i = 1, 30 do
        n = n + slen(s)
    end
    return n
end
print(len_loop(""), len_loop("abc"), len_loop(text))

print(pcall(function()
    local r = 0
    for i = 1, 30 do
        r = floor(i < 25 and 1.5 or "x")
    end
    return r
end))
print(pcall(function()
    local r = 0
    for i = 1, 30 do
        r = sqrt(i < 25 and 2.0 or {})
    end
    return r
end))
print(pcall(function()
    local r = 0
    for i = 1, 30 do
        r = sbyte(i < 25 and "abc" or {}, 1)
    end
    return r
end))
print(select(2, pcall(function()
    local r = 0
    for i = 1, 30 do
        r = sbyte("abc", i < 25 and 1 or {})
    end
    return r
end)))

local swap = math.floor
local function call_swap(v)
    return swap(v)
end
for i = 1, 40 do
    if i == 30 then
        swap = math.ceil
    end
    if i == 29 or i == 30 then
        print(call_swap(1.5))
    end
    call_swap(1.5)
end

local function const_same(a)
    local t = type(a)
    local u = t
    if u == "string" then
        return 1
    end
    return 2
end
for i = 1, 30 do
    const_same("x")
    const_same(i)
end
print(const_same("x"), const_same(1), const_same(nil))

local function str_eq(a, b)
    if a == b then
        return true
    end
    return false
end
local big = string.rep("q", 64)
for i = 1, 30 do
    str_eq("number", type(1))
    str_eq(big, string.rep("q", 64))
end
print(str_eq("number", type(1)), str_eq("string", type(1)), str_eq(big, string.rep("q", 64)), str_eq(big, string.rep("q", 65)))

local function two_paths(x, flag)
    local t
    if flag then
        t = type(x)
    else
        t = "table"
    end
    if t == "table" then
        return "T"
    end
    return "F"
end
for i = 1, 40 do
    two_paths(1, i % 2 == 0)
end
print(two_paths(1, true), two_paths(1, false), two_paths({}, true), two_paths({}, false))
