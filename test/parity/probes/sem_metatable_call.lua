local t = setmetatable({}, {__call = function(self, x)
    return x * 2
end})
print(t(21))
