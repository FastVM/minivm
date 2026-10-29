local t = {}
local keys = {}
for i = 1, 2000 do
    local k = coroutine.create(function() end)
    keys[i] = k
    t[k] = i
end
for i = 1, 2000 do
    t[string.format] = i
end
local sum = 0
for i = 1, 2000 do
    sum = sum + t[keys[i]]
end
print("threads", sum, t[string.format])
local n = 0
for _ in pairs(t) do
    n = n + 1
end
print("count", n)
