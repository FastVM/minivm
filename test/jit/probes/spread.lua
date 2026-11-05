
local function id(a)
    return a
end
local function two(a, b)
    return a, b
end
local function none() end
local function pair(x)
    return x, x + 1
end
local function add(a, b)
    if b == nil then
        return a
    end
    return a + b
end
local function count(...)
    return select("#", ...)
end

local function lua_of_fast(v)
    local r
    for i = 1, 40 do
        r = id(tostring(v))
    end
    return r
end
print(lua_of_fast("s"), lua_of_fast(1), lua_of_fast(1.5), lua_of_fast(nil))

local function lua_of_type(v)
    local r
    for i = 1, 40 do
        r = id(type(v))
    end
    return r
end
print(lua_of_type("s"), lua_of_type(1), lua_of_type({}), lua_of_type(nil))

local function lua_of_math(v)
    local r
    for i = 1, 40 do
        r = id(math.floor(v))
    end
    return r
end
print(lua_of_math(1.5), lua_of_math(3), lua_of_math(1e300), lua_of_math(2^53))

local function float_spread(v)
    local r = 0.0
    for i = 1, 40 do
        r = r + id(math.sqrt(v))
    end
    return r
end
print(float_spread(4.0), float_spread(9))

local function lua_of_lua(n)
    local r = 0
    for i = 1, 40 do
        r = r + id(id(n))
    end
    return r
end
print(lua_of_lua(2), lua_of_lua(2.5))

local function lua_of_many()
    local a, b, c = 0, 0, 0
    for i = 1, 40 do
        a = a + add(1, id(i))
        b = b + add(pair(i))
        c = c + count(pair(i))
    end
    return a, b, c
end
print(lua_of_many())

local function lua_of_none()
    local r = 0
    for i = 1, 40 do
        r = r + count(none())
        r = r + add(i, none())
    end
    return r
end
print(lua_of_none())

local function changing_count()
    local r = 0
    for i = 1, 80 do
        if i <= 40 then
            r = r + add(1, id(i))
        else
            r = r + add(1, none())
        end
    end
    return r
end
print(changing_count())

local function changing_kind()
    local r = 0
    for i = 1, 80 do
        local v
        if i <= 40 then
            v = i
        else
            v = i + 0.5
        end
        r = r + add(1, id(v))
    end
    return r
end
print(changing_kind())

local function changing_count_loop(f)
    local r = 0
    for i = 1, 40 do
        r = r + add(1, f(i))
    end
    return r
end
print(changing_count_loop(id), changing_count_loop(function(x) return x, x end), changing_count_loop(none))

local function ffi_of_ffi(v)
    local r
    for i = 1, 40 do
        r = string.upper(tostring(v))
    end
    return r
end
print(ffi_of_ffi("abc"), ffi_of_ffi(10), ffi_of_ffi(nil))

local function ffi_of_lua(n)
    local r
    for i = 1, 40 do
        r = string.format("%d %d", pair(n))
    end
    return r
end
print(ffi_of_lua(1), ffi_of_lua(2))

local function ffi_one(n)
    local r
    for i = 1, 40 do
        r = string.len(id(n))
    end
    return r
end
print(ffi_one("abc"), ffi_one(12345))
print(pcall(ffi_one, {}))
print(pcall(string.len, none()))

local function ffi_changing(f)
    local r
    for i = 1, 40 do
        r = string.rep("ab", f(i))
    end
    return r
end
print(ffi_changing(function() return 2 end))
print(ffi_changing(function() return 2, "-" end))
print(pcall(ffi_changing, none))

local function concat_results()
    local n = 0
    for i = 1, 40 do
        n = n + #table.concat({ pair(i) }, ",")
    end
    return n
end
print(concat_results())

local function nested_multi()
    local n = 0
    for i = 1, 40 do
        n = n + count(id(pair(i)))
        n = n + count(pair(id(i)))
        n = n + add(id(i), id(i))
    end
    return n
end
print(nested_multi())

local function return_multi(i)
    return pair(id(i))
end
local function use_return_multi()
    local a, b = 0, 0
    for i = 1, 40 do
        local x, y = return_multi(i)
        a = a + x
        b = b + y
    end
    return a, b
end
print(use_return_multi())

local function tail_multi(v)
    return id(tostring(v))
end
for i = 1, 40 do
    tail_multi(i)
end
print(tail_multi("t"), tail_multi(1))

local function vararg_callee(...)
    local a, b = ...
    return a, b, select("#", ...)
end
local function call_vararg()
    local x, y, z
    for i = 1, 40 do
        x, y, z = vararg_callee(id(i))
    end
    return x, y, z
end
print(call_vararg())

local closure_upval = 0
local function with_upval(a)
    closure_upval = closure_upval + a
    return closure_upval
end
local function call_with_upval()
    local r = 0
    for i = 1, 40 do
        r = with_upval(id(i))
    end
    return r
end
print(call_with_upval())

local function swapped_callee()
    local f = id
    local r
    for i = 1, 60 do
        if i == 40 then
            f = function(x) return x * 2 end
        end
        r = f(id(i))
    end
    return r
end
print(swapped_callee())

local obj = { v = 1 }
function obj:get(n)
    return self.v + n
end
local function method_spread()
    local r = 0
    for i = 1, 40 do
        r = r + obj:get(id(i))
    end
    return r
end
print(method_spread())

local co = coroutine.wrap(function()
    local r = 0
    for i = 1, 40 do
        r = r + id(coroutine.yield(i))
    end
    return r
end)
local v = co()
for i = 1, 40 do
    v = co(i)
end
print(v)

print(pcall(function()
    for i = 1, 40 do
        id(error("boom " .. i, 0))
    end
end))
