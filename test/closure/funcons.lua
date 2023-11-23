
local x = "unused"

local i = 10000000
while i > 0 do
    x = function(i)
        return i - 1
    end
    i = x(i)
end

print(i)
