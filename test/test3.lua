
local add = function(x)
    return function(y)
        return x + y
    end
end

print(add(1)(2))
