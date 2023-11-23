
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
    local v0 = delay4(x - 1)
    local v1 = v0()
    local v2 = v1()
    local v3 = v2()
    local v4 = v3()
    x = v4
end
print(x)
