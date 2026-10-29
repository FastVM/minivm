local function make_adder(k)
    return function(x)
        return x + k
    end
end

local acc = 0
local i = 0
while i < 5000000 do
    local f = make_adder(i % 10)
    acc = f(acc)
    if acc > 1000000 then
        acc = acc - 1000000
    end
    i = i + 1
end

print(acc)
