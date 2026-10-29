local t = setmetatable({}, {__index = function()
    return "meta"
end})
print(t.x, rawget(t, "x"))
rawset(t, "x", 1)
print(rawget(t, "x"))
print(rawequal(t, t), rawequal({}, {}))
print(rawlen({1, 2, 3}), rawlen("abcd"))
