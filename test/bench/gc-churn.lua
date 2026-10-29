local sum = 0
local i = 0
while i < 5000000 do
    local p = {i % 100, i % 7}
    sum = sum + p[1] + p[2]
    i = i + 1
end

print(sum)
