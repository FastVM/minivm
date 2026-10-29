local function counter()
    local n = 0
    return function()
        n = n + 1
        return n
    end
end
local c1 = counter()
local c2 = counter()
c1()
print(c1(), c2())
