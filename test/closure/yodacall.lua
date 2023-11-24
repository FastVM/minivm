
local add = function(x)
    return function(y)
        return x + y
    end
end

local i = 0
while i < 1000 * 1000 * 10 do
    i = add(1)(i)
end

print(i)
