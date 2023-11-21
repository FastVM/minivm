
local build_closure = vm.closure
local x = "unused"

local i = 10000000
while i > 0 do
    x = build_closure(function(c, i)
        return i - 1
    end)
    i = x(i)
end

print(i)
