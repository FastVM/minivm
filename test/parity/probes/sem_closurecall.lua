local function step(a, b) return a + b * 2 - 1 end
local acc = 0
for i = 1, 100 do acc = step(acc, i % 7) end
print(acc)

local y = 10
local function add_y(x) return x + y end
local sum = 0
for i = 1, 100 do sum = sum + add_y(i) end
print(sum)

local function make_adder(n)
    return function(x) return x + n end
end
local add5 = make_adder(5)
local add10 = make_adder(10)
print(add5(1), add10(1), add5(add10(1)))

local function f(x) return x + 1 end
local function g(x) return x * 2 end
local funcs = {f, g}
local r = 0
for i = 1, 100 do
    local fn = funcs[i % 2 + 1]
    r = fn(r)
end
print(r)
