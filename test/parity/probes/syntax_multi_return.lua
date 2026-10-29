local function f()
    return 1, 2, 3
end
local x, y, z = f()
print(x, y, z)
print((f()))
