local total = 0
local count = 0
local i = 0
while i < 1500000 do
    local s = "k" .. tostring(i % 1000) .. ":" .. tostring(i % 13)
    total = total + string.len(s)
    count = count + 1
    i = i + 1
end

print(count)
print(total)
