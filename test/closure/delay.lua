
local delay4 = function(x)
    return function()
        return function()
            return function()
                return function()
                    return x
                end
            end
        end
    end
end

local x = 1000 * 1000
while x ~= 0 do
    x = delay4(x - 1)()()()()
end
print(x)
