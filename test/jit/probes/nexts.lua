
local function count(t)
    local n, s = 0, 0
    for k, v in pairs(t) do
        n = n + 1
        if math.type(v) then
            s = s + v
        end
    end
    return n, s
end
for round = 1, 40 do
    count({ 1, 2, 3 })
    count({ a = 1, b = 2 })
    count({ 1, 2, a = 3 })
    count({})
end
print(count({ 1, 2, 3 }))
print(count({ a = 1, b = 2, c = 3 }))
print(count({ 1, 2, nil, 4, x = 5, y = 6 }))
print(count({ [1.5] = 1, [2.5] = 2, [true] = 3, [false] = 4 }))
print(count({ nil, nil, 3 }))
print(count({}))

local function keys_sorted(t)
    local out = {}
    for k in pairs(t) do
        out[#out + 1] = tostring(k)
    end
    table.sort(out)
    return table.concat(out, ",")
end
for round = 1, 40 do
    keys_sorted({ a = 1, b = 2 })
end
print(keys_sorted({ a = 1, b = 2, c = 3, [1] = 1, [2] = 2, [10] = 3, [2.5] = 4 }))

local function clearing()
    local t = {}
    for i = 1, 50 do
        t["k" .. i] = i
        t[i] = i
    end
    local seen = 0
    for k, v in pairs(t) do
        t[k] = nil
        seen = seen + 1
    end
    return seen, next(t)
end
for round = 1, 10 do
    clearing()
end
print(clearing())

local function updating()
    local t = { a = 1, b = 2, c = 3, 10, 20 }
    local s = 0
    for round = 1, 30 do
        for k, v in pairs(t) do
            t[k] = v + 1
            s = s + 1
        end
    end
    return s, t.a, t.b, t.c, t[1], t[2]
end
print(updating())

local function collect_during()
    local t = {}
    for i = 1, 40 do
        t["key" .. i] = { i }
    end
    local n = 0
    for round = 1, 10 do
        for k, v in pairs(t) do
            n = n + v[1]
            if v[1] == 20 then
                collectgarbage()
            end
        end
    end
    return n
end
print(collect_during())

local function direct(t, k)
    local a, b
    for i = 1, 40 do
        a, b = next(t, k)
    end
    return a, b
end
print(direct({ 5 }, nil))
print(direct({ 5 }, 1))
print(direct({ 5, 6 }, 1))
print(direct({ x = 1 }, "x"))
print(direct({ x = 1 }, nil))
print(direct({}, nil))
print((pcall(direct, {}, "missing")))
print((pcall(direct, { 1 }, 5)))
print((pcall(direct, { 1 }, 1.5)))
print((pcall(next, nil)))
print((pcall(next, 5)))

local function hopping()
    local t = { a = 1, b = 2, c = 3, d = 4 }
    local n = 0
    for round = 1, 30 do
        local k = next(t)
        local k2 = next(t, k)
        local k3 = next(t, k)
        if k2 == k3 then
            n = n + 1
        end
    end
    return n
end
print(hopping())

local function two_walks()
    local t = { a = 1, b = 2, c = 3 }
    local n = 0
    for round = 1, 20 do
        for k1 in pairs(t) do
            for k2 in pairs(t) do
                n = n + 1
            end
        end
    end
    return n
end
print(two_walks())

local function meta_pairs()
    local t = setmetatable({}, { __pairs = function(self)
        return function(_, k)
            if not k then
                return 1, "one"
            end
        end, self, nil
    end })
    local out
    for round = 1, 30 do
        for k, v in pairs(t) do
            out = v
        end
    end
    return out
end
print(meta_pairs())

local function with_meta()
    local t = setmetatable({ a = 1 }, { __index = { b = 2 } })
    local n = 0
    for round = 1, 30 do
        for k, v in pairs(t) do
            n = n + v
        end
    end
    return n
end
print(with_meta())

local function kinds_mix()
    local t = { 1, "two", 3.5, true, {}, print, x = 1, y = "s", z = 2.5 }
    local out = {}
    for round = 1, 30 do
        local i = 0
        for k, v in pairs(t) do
            i = i + 1
            out[i] = type(k) .. ":" .. type(v)
        end
    end
    table.sort(out)
    return table.concat(out, " ")
end
print(kinds_mix())

local function stateful_next()
    local n = 0
    local nx = next
    local t = { 1, 2, 3 }
    for round = 1, 60 do
        if round == 40 then
            nx = function(s, k)
                if not k then
                    return 1, 100
                end
            end
        end
        for k, v in nx, t do
            n = n + v
        end
    end
    return n
end
print(stateful_next())

local function three_vals()
    local n = 0
    for round = 1, 40 do
        local a, b, c = next({ 7 })
        n = n + a + b + (c or 0)
        local d = next({ 7 })
        n = n + d
    end
    return n
end
print(three_vals())

local function grow_array()
    local t = { 1, 2, 3 }
    local n = 0
    for round = 1, 30 do
        for i, v in pairs(t) do
            n = n + v
        end
        t[#t + 1] = round
    end
    return n
end
print(grow_array())

local function shrink_hash()
    local t = {}
    for i = 1, 20 do
        t["k" .. i] = i
    end
    local n = 0
    for i = 1, 20 do
        t["k" .. i] = nil
        for k, v in pairs(t) do
            n = n + v
        end
    end
    return n
end
print(shrink_hash())
