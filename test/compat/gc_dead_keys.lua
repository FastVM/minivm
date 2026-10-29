local t = {}
local collected = 0
local meta = {__gc = function() collected = collected + 1 end}
for i = 1, 8 do
    t[setmetatable({}, meta)] = i
end
t.keep = true
for k in pairs(t) do
    if k ~= "keep" then
        t[k] = nil
    end
end
collectgarbage()
collectgarbage()
print("finalized", collected)

local function drain(make)
    local d = {}
    for i = 1, 200 do
        d[make(i)] = i
    end
    local seen = 0
    for k in pairs(d) do
        d[k] = nil
        seen = seen + 1
        if seen % 7 == 0 then
            collectgarbage()
        end
    end
    return seen, next(d)
end
print("tables", drain(function() return {} end))
print("strings", drain(function(i) return "key" .. i .. string.rep("x", i % 40) end))
print("closures", drain(function() return function() end end))

local r = {}
local k = {}
r[k] = 1
r[k] = nil
collectgarbage()
r[k] = 2
print("revive", r[k])
local n = 0
for _ in pairs(r) do
    n = n + 1
end
print("revive count", n)

local weak = setmetatable({}, {__mode = "k"})
weak[false] = "f"
for _ = 1, 64 do
    weak[{}] = true
end
collectgarbage()
weak[false] = "g"
local live = 0
for _ in pairs(weak) do
    live = live + 1
end
print("weak false", weak[false], live)
