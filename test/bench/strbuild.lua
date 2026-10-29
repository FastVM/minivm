local parts = {}
for i = 1, 200000 do
    parts[#parts + 1] = "item" .. i
end
local s = table.concat(parts, ",")
print(#s)
local count = 0
for word in s:gmatch("[^,]+") do
    count = count + 1
end
print(count)
