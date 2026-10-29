local n = 0
local function f(...)
    ::top::
    n = n + 1
    local t = {select('#', ...), ...}
    collectgarbage()
    if n % 3 ~= 0 then
        goto top
    end
    return table.unpack(t)
end
for _ = 1, 3 do
    print(f(1, 'a', nil, true))
end

local function g(...)
    ::top::
    local s = string.rep('x', 100) .. select('#', ...)
    collectgarbage()
    n = n + 1
    if n < 20 then
        goto top
    end
    return s, ...
end
print(g(4, 5, 6))
