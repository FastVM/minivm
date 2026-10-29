local t = {10, 20, 30}
local sum = 0
for k, v in pairs(t) do
    sum = sum + k * v
end
print(sum)
local acc = {}
for i, v in ipairs(t) do
    acc[#acc + 1] = i .. ":" .. v
end
print(table.concat(acc, ","))
print(next({}), (next({a = 1})))
