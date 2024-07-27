
local function sqrt(n)
    local l = 1
    local i = 1
    while i <= n do
        l = i / l + l / i
        i = i + 0.1
    end
    return l - 0.75 / l
end

local diff = 0
local err = 0

for i=100, 200 do
    local j = sqrt(i*i)
    diff = diff + i - j
    if i < j then
        err = err + j - i
    else
        err = err + i - j
    end
end

print(diff, err)
