
local load = load or loadstring
local tostring = tostring

local n = tonumber(arg and arg[1]) or 1000

local t = 0
local i = 0
while i < n do
    i = load("return " .. tostring(i+1))()
    t = t + i
end

print(t)

