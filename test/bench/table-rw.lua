local n = 100000
local t = {}

for i = 1, n do
    t[i] = i
end

local checksum = 0
for pass = 1, 100 do
    local i = 1
    while i <= n do
        t[i] = t[i] + (i % 3)
        i = i + 1
    end
    checksum = checksum + t[pass * 997 % n + 1]
end

local total = 0
for i = 1, n do
    total = total + t[i]
end

print(total)
print(checksum)
