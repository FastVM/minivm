local mt = {__add = function(a, b)
    return a.v + b.v
end}
local a = setmetatable({v = 1}, mt)
local b = setmetatable({v = 2}, mt)
print(a + b)
