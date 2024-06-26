
local load = load or loadstring
local tostring = tostring

local t = 0
local i = 0
while i < 1000 do
    i = load("return " .. tostring(i+1))()
    t = t + i
end

print(t)

