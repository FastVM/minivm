local x = 1.5
local sum = 0.0
local i = 0
while i < 60000000 do
    x = x * 1.0000001 + 0.5
    if x > 100.0 then
        x = x / 3.0 - 1.0
    end
    if x < 10.0 then
        sum = sum + x
    else
        sum = sum - x * 0.25
    end
    i = i + 1
end

print(sum)
