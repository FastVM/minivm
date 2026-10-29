local function f(a, ...)
    local i = debug.getinfo(1, "Sl")
    print(i.what, i.currentline, i.linedefined)
    print(debug.getlocal(1, 1))
    print(debug.getlocal(1, 2))
    print(debug.getlocal(1, -1))
    print(debug.getlocal(1, -2))
    print(debug.getlocal(1, -3))
    print(debug.setlocal(1, -1, "zz"))
    print(debug.setlocal(1, -9, "zz"))
    print(...)
    return a
end
print(f(7, "va", "vb"))
local function g(...)
    print(debug.getlocal(1, -1))
    return select("#", ...)
end
print(g())
print(g(nil, nil))
local r = debug.getinfo(f, "r")
print(r.ftransfer, r.ntransfer)
local lines = debug.getinfo(g, "L").activelines
print(lines[16], lines[17], lines[15], lines[99])
debug.setmetatable(1, {__index = function(n, k) return k .. n end})
print((5).abc)
debug.setmetatable(1, nil)
print(pcall(function() return (5).abc end) == false)
debug.setmetatable(nil, {__index = function() return "nilidx" end})
print((nil).anything)
debug.setmetatable(nil, nil)
local function counter()
    local c = 0
    return function() c = c + 1 return c end, function() return c end
end
local inc, get = counter()
print(debug.upvalueid(inc, 1) == debug.upvalueid(get, 1))
print(debug.upvalueid(inc, 1) ~= nil, debug.upvalueid(inc, 2))
