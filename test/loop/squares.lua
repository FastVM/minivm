
local function sqrt(n)
    local f = 1
    local i = 3
    while true do
        f = f + i
        if f > n then
            return (i - 1) / 2
        end
        i = i + 2
    end
end

local function check(i)
    print('sqrt(' .. tostring(i) .. ') = ' .. tostring(sqrt(i)))
end

for i=2, 10 do
    assert(sqrt(i*i-1) == i-1)
    assert(sqrt(i*i) == i)
    assert(sqrt(i*i+1) == i)
end
