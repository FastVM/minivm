local t = setmetatable({}, {__tostring = function()
    return "custom"
end})
print(tostring(t))
print(t)
