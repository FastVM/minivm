
local function field(t)
    local r
    for i = 1, 40 do
        r = t.x
    end
    return r
end
print(field({ x = 1 }), field({ x = 1.5 }), field({ x = "s" }), field({ x = false }), field({}), field({ y = 1 }))
print(field({ a = 1, b = 2, c = 3, d = 4, e = 5, f = 6, x = 7 }))
print(field(setmetatable({}, { __index = { x = "inherited" } })))
print(field(setmetatable({}, { __index = setmetatable({}, { __index = { x = "deep" } }) })))
print(field(setmetatable({}, { __index = function(t, k) return k .. "!" end })))
print(field(setmetatable({}, { __index = {} })))
print(field(setmetatable({}, {})))
print(field(setmetatable({ x = "own" }, { __index = { x = "inherited" } })))
print(field(setmetatable({ x = nil }, { __index = { x = "fallback" } })))
print((pcall(field, setmetatable({}, { __index = 5 }))))
print(field(setmetatable({}, { __index = "abc" })))

local function shapes()
    local out = 0
    local a = { x = 1 }
    local b = { y = 2, x = 2 }
    local c = { q = 1, w = 2, e = 3, r = 4, x = 3 }
    local d = setmetatable({}, { __index = { x = 4 } })
    local e = {}
    local list = { a, b, c, d, e }
    for round = 1, 40 do
        for i = 1, #list do
            local v = list[i].x
            if v then
                out = out + v
            else
                out = out + 100
            end
        end
    end
    return out
end
print(shapes())

local function optional(t)
    local n = 0
    for i = 1, 40 do
        if t.flag then
            n = n + 1
        else
            n = n + 2
        end
    end
    return n
end
print(optional({}), optional({ flag = true }), optional({ flag = false }), optional({ other = 1 }), optional({ flag = 0 }))

local Base = {}
Base.__index = Base
function Base.new(v)
    return setmetatable({ v = v }, Base)
end
function Base:get()
    return self.v
end
function Base:inc()
    self.v = self.v + 1
    return self
end
local Derived = setmetatable({}, { __index = Base })
Derived.__index = Derived
function Derived.new(v)
    return setmetatable({ v = v }, Derived)
end
function Derived:twice()
    return self:get() * 2
end
local Leaf = setmetatable({}, { __index = Derived })
Leaf.__index = Leaf
function Leaf.new(v)
    return setmetatable({ v = v, extra = true }, Leaf)
end

local function use(o)
    local s = 0
    for i = 1, 60 do
        o:inc()
        s = s + o:get()
    end
    return s
end
print(use(Base.new(1)), use(Derived.new(2)), use(Leaf.new(3)))
local function use_twice(o)
    local s = 0
    for i = 1, 60 do
        s = s + o:twice()
    end
    return s
end
print(use_twice(Derived.new(5)), use_twice(Leaf.new(6)))

local function override()
    local o = Leaf.new(1)
    local s = 0
    for i = 1, 80 do
        if i == 40 then
            function Leaf:get()
                return -1
            end
        end
        s = s + o:get()
    end
    Leaf.get = nil
    return s
end
print(override())

local function delete_key()
    local t = { a = 1, x = 2 }
    local s = 0
    for i = 1, 80 do
        if i == 40 then
            t.x = nil
        end
        local v = t.x
        if v then
            s = s + v
        else
            s = s + 1000
        end
    end
    return s
end
print(delete_key())

local function grow_table()
    local t = { x = 1 }
    local s = 0
    for i = 1, 200 do
        t["k" .. i] = i
        s = s + t.x
    end
    return s
end
print(grow_table())

local function swap_meta()
    local a = { x = "a" }
    local b = { x = "b" }
    local o = setmetatable({}, { __index = a })
    local r = {}
    for i = 1, 80 do
        if i == 40 then
            setmetatable(o, { __index = b })
        end
        r[1] = o.x
        if i == 39 or i == 80 then
            print(r[1])
        end
    end
end
swap_meta()

local function index_nil_value()
    local mt = { __index = nil }
    local o = setmetatable({}, mt)
    local r = {}
    for i = 1, 60 do
        if i == 40 then
            mt.__index = { x = "late" }
        end
        r[1] = o.x
    end
    return r[1]
end
print(index_nil_value())

local function index_changes_kind()
    local mt = { __index = {} }
    local o = setmetatable({}, mt)
    local r
    for i = 1, 60 do
        if i == 40 then
            mt.__index = function() return "fn" end
        end
        r = o.x
    end
    return r
end
print(index_changes_kind())

local function cycle()
    local a = {}
    local b = setmetatable({}, { __index = a })
    setmetatable(a, { __index = b })
    local r
    for i = 1, 30 do
        r = rawget(a, "x")
    end
    return pcall(function()
        return a.x
    end)
end
print((cycle()))

local function var_key(t, k)
    local r
    for i = 1, 40 do
        r = t[k]
    end
    return r
end
print(var_key({ x = 1 }, "x"), var_key({ x = 1 }, "y"), var_key({}, "y"))
print(var_key(setmetatable({}, { __index = { z = "up" } }), "z"), var_key(setmetatable({}, { __index = {} }), "z"))
print(var_key({ a = 1, b = 2, c = 3, d = 4, e = 5, f = 6, g = 7, h = 8 }, "h"))
print(var_key(setmetatable({}, { __index = function(_, k) return k end }), "fn"))

local set = { a = true, b = true, c = true, d = true, e = true, f = true, g = true, h = true, i = true }
local function members(s)
    local n = 0
    for i = 1, #s do
        if set[s:sub(i, i)] then
            n = n + 1
        end
    end
    return n
end
for i = 1, 40 do
    members("abcxyz")
end
print(members("abcxyz"), members("hij"), members("qrs"), members(""))

local function var_mixed()
    local t = { one = 1, two = "2", three = 3.5, four = true, five = { 5 } }
    local names = { "one", "two", "three", "four", "five", "six" }
    local parts = {}
    for round = 1, 40 do
        for i = 1, #names do
            local v = t[names[i]]
            parts[i] = type(v)
        end
    end
    return table.concat(parts, " ")
end
print(var_mixed())

local long1 = string.rep("k", 40)
local long2 = string.rep("k", 39) .. "k"
local long_t = { [long1] = "long" }
local function long_key(k)
    local r
    for i = 1, 40 do
        r = long_t[k]
    end
    return r
end
print(long_key(long1), long_key(long2), long_key(string.rep("j", 40)))

local function rope_key()
    local base = string.rep("ab", 200)
    local t = { [base .. base] = "rope" }
    local r
    for i = 1, 40 do
        local k = base .. base
        r = t[k]
    end
    return r
end
print(rope_key())

local function ref_keys()
    local k1, k2, k3 = {}, {}, {}
    local f1 = function() end
    local co = coroutine.create(f1)
    local t = { [k1] = "t1", [k2] = 2, [f1] = 2.5, [co] = true, [print] = "ffi" }
    local parts = {}
    local keys = { k1, k2, k3, f1, co, print }
    for round = 1, 40 do
        for i = 1, #keys do
            parts[i] = tostring(t[keys[i]])
        end
    end
    return table.concat(parts, " ")
end
print(ref_keys())

local function ref_seen(list)
    local seen = {}
    local n = 0
    for round = 1, 3 do
        for i = 1, #list do
            local item = list[i]
            if seen[item] then
                n = n + 1
            else
                seen[item] = true
            end
        end
    end
    return n
end
local objs = {}
for i = 1, 200 do
    objs[i] = {}
end
for round = 1, 5 do
    ref_seen(objs)
end
print(ref_seen(objs))

local function ref_meta(key)
    local plain = setmetatable({}, { __index = function(_, k) return type(k) end })
    local chain = setmetatable({}, { __index = { [key] = "up" } })
    local r1, r2
    for i = 1, 40 do
        r1 = plain[key]
        r2 = chain[key]
    end
    return r1, r2
end
print(ref_meta({}), ref_meta(print))

local function ref_int_clash()
    local o = {}
    local t = { [o] = "obj" }
    local r
    for i = 1, 40 do
        r = t[o]
    end
    t[o] = nil
    local a
    for i = 1, 40 do
        a = t[o]
    end
    t[o] = "back"
    return r, a, t[o]
end
print(ref_int_clash())

local function ref_gc()
    local t = {}
    local keep = {}
    for i = 1, 100 do
        local k = {}
        keep[i] = k
        t[k] = i
    end
    local s = 0
    for round = 1, 5 do
        collectgarbage()
        for i = 1, 100 do
            s = s + t[keep[i]]
        end
    end
    return s
end
print(ref_gc())

local function ref_weak()
    local t = setmetatable({}, { __mode = "k" })
    local k = {}
    t[k] = 1
    local s = 0
    for i = 1, 40 do
        s = s + t[k]
        collectgarbage("step")
    end
    return s
end
print(ref_weak())

local function full_small()
    local t = { p = 1, q = 2 }
    local r
    for i = 1, 40 do
        r = t.absent
    end
    return r, t.p, t.q
end
print(full_small())

local function int_float_keys()
    local t = { [1] = "i", [1.5] = "f", x = "s", [true] = "b" }
    local r = {}
    for i = 1, 40 do
        r[1] = t[1]
        r[2] = t[1.5]
        r[3] = t.x
        r[4] = t[true]
        r[5] = t[2]
    end
    return r[1], r[2], r[3], r[4], r[5]
end
print(int_float_keys())

local function dead_keys()
    local t = {}
    for i = 1, 20 do
        t["k" .. i] = i
    end
    for i = 1, 20 do
        t["k" .. i] = nil
    end
    collectgarbage()
    t.x = "alive"
    local r
    for i = 1, 40 do
        r = t.x
    end
    local a
    for i = 1, 40 do
        a = t.k5
    end
    return r, a
end
print(dead_keys())

local function setop_change()
    local o = setmetatable({}, { __index = { x = "chain" } })
    local r
    for i = 1, 40 do
        r = o.x
    end
    return r
end
print(setop_change())

local function with_cells()
    local t = { x = 1 }
    local captured = 0
    local f = function() captured = captured + 1 end
    for i = 1, 40 do
        captured = t.x
        local absent = t.y
        if absent then
            captured = 0
        end
        f()
    end
    return captured
end
print(with_cells())

local function collecting()
    local proto = { value = 7 }
    proto.__index = proto
    local s = 0
    for i = 1, 300 do
        local o = setmetatable({}, proto)
        s = s + o.value
        if i % 100 == 0 then
            collectgarbage()
        end
    end
    return s
end
print(collecting())
