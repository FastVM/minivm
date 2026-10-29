local function pair()
    local shared = 10
    local function get() return shared end
    local function twice() return shared * 2 end
    return get, twice
end
local get, twice = pair()
local get2, twice2 = pair()
print(debug.upvalueid(get, 1) == debug.upvalueid(twice, 1))
print(debug.upvalueid(get, 1) == debug.upvalueid(get2, 1))
print(debug.setupvalue(get, 1, 50))
print(get(), twice(), get2(), twice2())
local function other() return 7 end
local seven = 7
local function useseven() return seven end
debug.upvaluejoin(get2, 1, useseven, 1)
print(get2(), twice2())
local function locals(a, b)
    local c = a + b
    local names = {}
    local n = 1
    while true do
        local name, value = debug.getlocal(1, n)
        if name == nil then
            break
        end
        names[#names + 1] = name .. "=" .. tostring(value)
        n = n + 1
    end
    return table.concat(names, " ", 1, 3)
end
print(locals(3, 4))
