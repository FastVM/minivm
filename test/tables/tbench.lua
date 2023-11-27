
local t = {}

local x = 1
while x <= 10000000 do
    t[x * 2] = x
    x = x + 1
end

print(t[x * 2 - 2])
