
local x = 1000 * 1000 * 10

local deep = function()
    return "deep closure"
end

local i = 0
while i < x do
    local tmp = deep
    deep = function()
        return tmp
    end
    i = i + 1
end

local shallow = deep()

local j = 1
while j < x do
    shallow = shallow()
    j = j + 1
end

print(shallow())
