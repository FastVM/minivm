
local x = function()
    return 0
end
local i = 1000000
while i > 0 do
    x = function(i)
        return i - 1
    end
    i = x(i)
end

print(i)
