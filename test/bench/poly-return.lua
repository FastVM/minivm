local function poly(i)
    local m = i % 3
    if m == 0 then
        return i % 97
    end
    if m == 1 then
        return {i % 53}
    end
    return tostring(i % 10)
end

local total = 0
local i = 0
while i < 3000000 do
    local v = poly(i)
    local t = type(v)
    if t == "number" then
        total = total + v
    elseif t == "table" then
        total = total + v[1] * 2
    else
        total = total + string.len(v)
    end
    i = i + 1
end

print(total)
