

local delay = function(k, x)
    local ret = x
    for i=1, k do
        local last = ret
        ret = function() return last end 
    end
    return ret
end

local x = tonumber(arg and arg[1]) or 1000000
local k = tonumber(arg and arg[2]) or 10
while x ~= 0 do
    x = delay(k, x - 1)
    for i=1, k do
        x = x()
    end
end
print(x)
