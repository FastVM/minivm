-- pairs over a large string-keyed table, many passes
local t = {}
for i = 1, 200000 do
    t["key" .. i] = i
end
local s = 0
for j = 1, 30 do
    for k, v in pairs(t) do
        s = s + v
    end
end
print(s)
