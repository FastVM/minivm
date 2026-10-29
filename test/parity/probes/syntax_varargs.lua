local function f(...)
    local a, b = ...
    return select("#", ...), a + b
end
local n, s = f(1, 2, 3)
print(n, s)
