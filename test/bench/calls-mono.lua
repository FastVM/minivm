local function step(a, b)
    return a + b * 2 - 1
end

local acc = 0
local i = 0
while i < 30000000 do
    acc = step(acc, i % 7)
    i = i + 1
end

print(acc)
