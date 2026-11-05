local function show(...)
    local parts = {}
    for i = 1, select("#", ...) do
        parts[i] = tostring(select(i, ...))
    end
    print(table.concat(parts, " "))
end

local function add(a, b) return a + b end
local function loop_add(n)
    local s = 0
    for i = 1, n do
        s = add(s, i)
    end
    return s
end
show(loop_add(1000))
show(loop_add(1000.0 // 1 | 0))

local function fadd(a, b) return a + b end
local function loop_kinds(n)
    local s = 0
    for i = 1, n do
        if i == n // 2 then s = s + 0.5 end
        s = fadd(s, i)
    end
    return s
end
show(loop_kinds(1000))

local function two(a) return a, a + 1 end
local function none() end
local function drop(a) return a end
local function shapes(n)
    local s = 0
    for i = 1, n do
        local a = two(i)
        local b, c = two(i)
        local d, e, f = two(i)
        none()
        local g = none()
        local h, j = drop(i)
        drop(i)
        assert(e == i + 1 and f == nil and g == nil and j == nil)
        s = s + a + b + c + d + h
    end
    return s
end
show(shapes(100))

local function thrower(i)
    if i == 80 then
        error("thrown " .. i)
    end
    return i
end
local function loop_throw()
    local s = 0
    for i = 1, 100 do
        s = s + thrower(i)
    end
    return s
end
show(pcall(loop_throw))

local function arith_err(x, i)
    if i == 80 then
        return x + {}
    end
    return x + i
end
local function loop_arith_err()
    local s = 0
    for i = 1, 100 do
        s = arith_err(s, i)
    end
    return s
end
show(pcall(loop_arith_err))

local function tb(i)
    if i == 80 then
        return debug.traceback("here")
    end
    return nil
end
local function loop_tb()
    local r
    for i = 1, 100 do
        r = tb(i) or r
    end
    return r
end
show(loop_tb())

local function peek(i)
    local local_in_callee = i * 2
    if i == 80 then
        local names = {}
        for level = 1, 2 do
            local j = 1
            while true do
                local n, v = debug.getlocal(level, j)
                if n == nil then break end
                if string.sub(n, 1, 1) ~= "(" and n ~= "r" then
                    names[#names + 1] = level .. ":" .. n .. "=" .. (type(v) == "number" and tostring(v) or type(v))
                end
                j = j + 1
            end
        end
        return table.concat(names, " ")
    end
    return local_in_callee
end
local function loop_peek()
    local caller_local = 7
    local r
    for i = 1, 100 do
        caller_local = caller_local + 1
        r = peek(i)
        if type(r) == "string" then
            return r, caller_local
        end
    end
end
show(loop_peek())

local function set_caller(i)
    if i == 80 then
        debug.setlocal(2, 1, 1000)
    end
    return i
end
local function loop_set_caller()
    local v = 0
    for i = 1, 100 do
        v = v + set_caller(i)
    end
    return v
end
show(loop_set_caller())

local function info(i)
    if i == 80 then
        local d = debug.getinfo(1, "lSn")
        local c = debug.getinfo(2, "l")
        return d.currentline, d.name, c.currentline
    end
    return 0, 0, 0
end
local function loop_info()
    local a, b, c
    for i = 1, 100 do
        local x, y, z = info(i)
        if x ~= 0 then a, b, c = x, y, z end
    end
    return a, b, c
end
show(loop_info())

local function yielder(i)
    if i % 25 == 0 then
        return coroutine.yield(i)
    end
    return i
end
local co = coroutine.wrap(function()
    local s = 0
    for i = 1, 100 do
        s = s + yielder(i)
    end
    return "done", s
end)
local got = {co()}
local round = 0
while got[1] ~= "done" do
    round = round + 1
    got = {co(round)}
end
show(got[1], got[2], round)

local function deep_callee(x)
    if x == 0 then
        return 0
    end
    return x
end
local function deep(n)
    local a = deep_callee(n)
    if n == 0 then return a end
    return a + deep(n - 1)
end
show(deep(20000))
local function overflower(n)
    local a = deep_callee(n)
    return a + overflower(n + 1)
end
local ok, msg = pcall(overflower, 1)
show(ok, string.find(tostring(msg), "stack overflow") ~= nil)
for _ = 1, 3 do
    ok, msg = pcall(overflower, 1)
    show(ok, string.find(tostring(msg), "stack overflow") ~= nil)
end

local impl = function(x) return x + 1 end
local function call_upval(x)
    return impl(x)
end
local function loop_upval(n)
    local s = 0
    for i = 1, n do
        s = call_upval(s)
    end
    return s
end
show(loop_upval(200))
impl = function(x) return x + 2 end
show(loop_upval(200))
impl = function(x, y) return x + (y or 3) end
show(loop_upval(200))
impl = function(...) return (...) + select("#", ...) end
show(loop_upval(200))
impl = setmetatable({}, {__call = function(self, x) return x + 5 end})
show(pcall(loop_upval, 200))
impl = print
show(pcall(function() local r = impl("printed"); return r end))
impl = nil
show(pcall(loop_upval, 2))

local M = {}
function M.get(self) return self.value end
function M.set(self, v) self.value = v; return self end
local obj = {value = 1}
local function loop_method(n)
    for i = 1, n do
        M.set(obj, M.get(obj) + i)
    end
    return obj.value
end
show(loop_method(500))
local gm = {}
gm.__index = gm
function gm:inc(n) self.x = self.x + n; return self end
function gm:val() return self.x end
local inst = setmetatable({x = 0}, gm)
local function loop_oop(n)
    for i = 1, n do
        inst:inc(i)
    end
    return inst:val()
end
show(loop_oop(500))
inst = setmetatable({x = 0.5}, gm)
show(loop_oop(500))
function gm:inc(n) self.x = self.x - n; return self end
show(loop_oop(500))
inst = {x = 1, inc = function(self, n) self.x = self.x * 2 % 1000; return self end, val = function(self) return "plain", self.x end}
show(loop_oop(500))

local function sum_array(t)
    local s = 0
    for i = 1, #t do
        s = s + t[i]
    end
    return s
end
local arr = {}
for i = 1, 1000 do arr[i] = i end
for _ = 1, 20 do sum_array(arr) end
show(sum_array(arr))
for i = 1, 1000 do arr[i] = i + 0.5 end
for _ = 1, 20 do sum_array(arr) end
show(sum_array(arr))
for i = 1, 1000 do
    if i % 2 == 0 then
        arr[i] = i
    else
        arr[i] = i + 0.25
    end
end
for _ = 1, 20 do sum_array(arr) end
show(sum_array(arr))
arr[500] = "12"
show(sum_array(arr))
arr[501] = "oops"
show((pcall(sum_array, arr)))

local function field_kinds(t, n)
    local s = 0
    for i = 1, n do
        local v = t.f
        if type(v) == "number" then
            s = s + v
        elseif type(v) == "string" then
            s = s + #v
        elseif v == nil then
            s = s + 1
        else
            s = s + 2
        end
        if i == n // 4 then t.f = 1.5 end
        if i == n // 2 then t.f = "str" end
        if i == 3 * n // 4 then t.f = nil end
        if i == 7 * n // 8 then t.f = {} end
    end
    return s
end
show(field_kinds({f = 1}, 8000))
show(field_kinds(setmetatable({}, {__index = {f = 2}}), 8000))

local function branch_phase(n)
    local a, b = 0, 0
    local x = 0
    for i = 1, n do
        x = x + 1
        if x < n // 2 then
            a = a + i
        else
            b = b + i * 2
        end
        if x % 1000 == 0 then
            a, b = b, a
        end
    end
    return a, b
end
show(branch_phase(100000))

local function gc_in_callee(i)
    local t = {i, i + 1, i + 2}
    local s = "s" .. i
    return #t + #s
end
local function loop_gc(n)
    local s = 0
    local keep = {}
    for i = 1, n do
        s = s + gc_in_callee(i)
        if i % 500 == 0 then
            keep[#keep + 1] = {i}
            collectgarbage("step")
        end
    end
    local k = 0
    for _, e in ipairs(keep) do k = k + e[1] end
    return s, k
end
show(loop_gc(5000))

local function ret_cell()
    local c = 0
    local function bump() c = c + 1; return c end
    local s = 0
    for i = 1, 100 do
        local v = bump()
        s = s + v + c
    end
    return s, c
end
show(ret_cell())
local function inner_cell(i)
    local v = i
    local f = function() v = v + 1; return v end
    f()
    return v + f()
end
local function loop_inner_cell()
    local s = 0
    for i = 1, 100 do
        s = s + inner_cell(i)
    end
    return s
end
show(loop_inner_cell())

local function hook_callee(x) return x + 1 end
local function loop_hook()
    local count = 0
    local s = 0
    for i = 1, 200 do
        s = hook_callee(s)
        if i == 100 then
            debug.sethook(function() count = count + 1 end, "cr")
        elseif i == 120 then
            debug.sethook()
        end
    end
    return s, count
end
show(loop_hook())

local function a3(x) return x + 1 end
local function a2(x) return a3(x) * 2 end
local function a1(x) return a2(x) + 1 end
local function a0(x) return a1(x) - 1 end
local function loop_nest(n)
    local s = 0
    for i = 1, n do
        s = (s + a0(i)) % 1000003
    end
    return s
end
show(loop_nest(1000))

local function rec_a(n)
    if n <= 0 then return 0 end
    return n + rec_a(n - 1)
end
local function loop_rec()
    local s = 0
    for i = 1, 50 do s = s + rec_a(i) end
    return s
end
show(loop_rec())

local function many(a, b, c, d, e, f, g, h, i, j, k, l)
    return a + b + c + d + e + f + g + h + i + j + k + l
end
local function loop_many(n)
    local s = 0
    for i = 1, n do
        s = many(i, i + 1, i + 2, i + 3, i + 4, i + 5, i + 6, i + 7, i + 8, i + 9, i + 10, s % 7)
    end
    return s
end
show(loop_many(300))
local function fewer(a, b, c, d)
    return a, b, c, d
end
local function loop_fewer()
    local r1, r2, r3, r4
    for i = 1, 100 do
        r1, r2, r3, r4 = fewer(i)
    end
    return r1, r2, r3, r4
end
show(loop_fewer())
local function extra(a) return a end
local function loop_extra()
    local r
    for i = 1, 100 do
        r = extra(i, i + 1, i + 2)
    end
    return r
end
show(loop_extra())
local function ret_mixed(i)
    if i % 3 == 0 then return i end
    if i % 3 == 1 then return i + 0.5 end
    return "s"
end
local function loop_mixed()
    local a, b, c = 0, 0.0, 0
    for i = 1, 300 do
        local v = ret_mixed(i)
        if math.type(v) == "integer" then a = a + v
        elseif math.type(v) == "float" then b = b + v
        else c = c + #v end
    end
    return a, b, c
end
show(loop_mixed())
