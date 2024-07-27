
local t = {}

local n = tonumber(arg and arg[1]) or 1000 * 1000 * 10

for x=1, n do
    t[x * 2] = x
end

print(t[n  * 2])
