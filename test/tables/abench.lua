
local a = {}

local x = 1
while x <= 10000000 do
    a[x] = x * 2
    x = x + 1
end


print(a[x-1] / 2)
