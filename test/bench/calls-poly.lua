local function add3(x)
    return x + 3
end

local function double(x)
    return x * 2
end

local function halve(x)
    return x / 2
end

local function dec(x)
    return x - 1
end

local funcs = {add3, double, halve, dec}

local acc = 1
local i = 0
while i < 10000000 do
    local f = funcs[i % 4 + 1]
    acc = f(acc)
    if acc > 1000000 then
        acc = acc - 1000000
    end
    i = i + 1
end

print(acc)
