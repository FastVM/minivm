local function sum_down(n)
    if n == 0 then
        return 0
    end
    return n + sum_down(n - 1)
end

local total = 0
local i = 0
while i < 300 do
    total = total + sum_down(100000)
    i = i + 1
end

print(total)
