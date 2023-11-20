
local x = "unused"

local i = 10000000
while i > 0 do
    x = function(c, i)
        return i - 1
    end
    i = x(i)
end

print(i)
