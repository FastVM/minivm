
local add = function(x)
    return function(y)
        return x + y
    end
end

local max = tonumber(arg and arg[1]) or 1000000

local i = 0
while i < max do
    i = add(i)(1)
end

print(i)
