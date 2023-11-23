
local add = function(x)
    return function(y)
        return x + y
    end
end

local inc = add(1)

local i = 0
while i < 1000 * 1000 * 100 do
    i = inc(i)
end

print(i)
