
local function sum_range(a, b, s)
    local total = 0
    for i = a, b, s do
        total = total + i
    end
    return total
end
for round = 1, 40 do
    sum_range(1, 10, 1)
end
print(sum_range(1, 10, 1), sum_range(10, 1, -1), sum_range(1, 10, 3), sum_range(10, 1, -3), sum_range(5, 1, 1), sum_range(1, 5, -1))
print(sum_range(1.0, 3, 1), sum_range(1, 3.5, 1), sum_range(1, 3, 0.5), sum_range(1.5, 3.5, 1.0))
print(pcall(sum_range, 1, 3, 0))
print(pcall(sum_range, "x", 3, 1))
print(pcall(sum_range, 1, {}, 1))

local function up(n)
    local total = 0
    for i = 1, n do
        total = total + i
    end
    return total
end
local function down(n)
    local total = 0
    for i = n, 1, -1 do
        total = total + i
    end
    return total
end
local function by_two(n)
    local total = 0
    for i = 1, n, 2 do
        total = total + i
    end
    return total
end
local function float_up(n)
    local total = 0.0
    for x = 0.5, n do
        total = total + x
    end
    return total
end
for round = 1, 40 do
    up(5)
    down(5)
    by_two(5)
    float_up(5)
end
print(up(0), up(-3), up(5), down(0), down(-3), down(5), by_two(0), by_two(6))
print(up(2.5), down(2.5), by_two(4.5), float_up(2), float_up(2.5))
print(up(-1e300), down(-1e300) == nil)
print(pcall(up, "x"))
print(pcall(down, {}))

local function edge(lo, hi)
    local n = 0
    local last = nil
    for i = lo, hi do
        n = n + 1
        last = i
        if n > 3 then
            break
        end
    end
    return n, last
end
for round = 1, 40 do
    edge(1, 2)
end
print(edge(math.maxinteger - 1, math.maxinteger))
print(edge(math.mininteger, math.mininteger + 1))
print(edge(1, math.mininteger))

local function nested(n)
    local count = 0
    for i = 1, n do
        for j = 1, i do
            count = count + j
        end
    end
    return count
end
for round = 1, 20 do
    nested(5)
end
print(nested(10))

local sub = string.sub
local text = "hello\0world\255"
local indices = { 1, 2, 5, 6, 12, 13, 14, 0, -1, -12, -13, -100, math.maxinteger, math.mininteger }
for i = 1, #indices do
    local at = indices[i]
    local r = "unset"
    for j = 1, 30 do
        r = sub(text, at, at)
    end
    print(at, #r, r:byte())
end

local function chars(s)
    local out = {}
    for i = 1, #s do
        local c = s:sub(i, i)
        out[#out + 1] = c
    end
    return table.concat(out, ",")
end
for round = 1, 20 do
    chars("abc")
end
print(chars("hello"), chars(""), chars("\0\1\255"))

local function first_last(s)
    local a, b = "", ""
    for i = 1, 30 do
        a = s:sub(1, 1)
        b = s:sub(-1, -1)
    end
    return a, b
end
print(first_last("xyz"))
print(first_last(""))

local function not_same(s, i, j)
    local r = ""
    for k = 1, 30 do
        r = s:sub(i, j)
    end
    return r
end
print(not_same("abcdef", 2, 4), not_same("abcdef", 4, 2), not_same("abcdef", 3), not_same("abcdef", 3, 3))

local long = string.rep("ab", 200)
for i = 1, 30 do
    local rope = long .. string.rep("cd", 200) .. tostring(i)
    local c = rope:sub(401, 401)
    if i == 30 then
        print(c, rope:sub(800, 800), rope:sub(#rope, #rope))
    end
end

for i = 1, 30 do
    local s = "s" .. i
    local same = tostring(s)
    local n = tostring(i)
    local f = tostring(i + 0.5)
    local b = tostring(i % 2 == 0)
    local nn = tostring(nil)
    if i == 30 then
        print(same, s == same, n, f, b, nn, math.type(i))
    end
end
print(pcall(tostring))
local with_meta = setmetatable({}, { __tostring = function() return "custom" end, __name = "T" })
for i = 1, 30 do
    tostring(with_meta)
end
print(tostring(with_meta))
print(pcall(tostring, setmetatable({}, { __tostring = function() return 1 end })))

local function walk(t)
    local sum = 0
    local n = 0
    for i, v in ipairs(t) do
        sum = sum + v
        n = i
    end
    return sum, n
end
for round = 1, 40 do
    walk({ 1, 2, 3 })
end
print(walk({ 1, 2, 3 }), walk({}), walk({ 1, 2, nil, 4 }), walk({ 1.5, 2.5 }), walk({ 1, 2.5, 3 }))

local function walk_any(t)
    local parts = {}
    for i, v in ipairs(t) do
        parts[i] = type(v)
    end
    return table.concat(parts, " ")
end
for round = 1, 40 do
    walk_any({ "a", "b" })
end
print(walk_any({ "a", 1, 1.5, true, {}, print }))

local proxy = setmetatable({}, { __index = function(_, i)
    if i <= 3 then
        return i * 10
    end
end })
print(walk(proxy))
local base = setmetatable({}, { __index = { 7, 8, 9 } })
print(walk(base))
local hashed = {}
hashed[3] = 30
hashed[2] = 20
hashed[1] = 10
print(walk(hashed))

local function mutate()
    local t = { 1, 2, 3, 4, 5 }
    local seen = 0
    for i, v in ipairs(t) do
        seen = seen + v
        if i == 2 then
            t[3] = nil
        end
    end
    return seen
end
for round = 1, 40 do
    mutate()
end
print(mutate())

local function grow()
    local t = { 1 }
    local n = 0
    for i, v in ipairs(t) do
        n = n + 1
        if i < 200 then
            t[i + 1] = i
        end
    end
    return n
end
for round = 1, 5 do
    grow()
end
print(grow())

local function three()
    local t = { 1, 2, 3 }
    local a = 0
    local f, s, c = ipairs(t)
    while true do
        local i, v, extra = f(s, c)
        if i == nil then
            break
        end
        c = i
        a = a + v
        assert(extra == nil)
    end
    return a
end
for round = 1, 40 do
    three()
end
print(three())
