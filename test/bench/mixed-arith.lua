local count = 0
local x = 0
local i = 0
while i < 4000000 do
    if i % 3 == 0 then
        x = i % 1000 + 1
    else
        x = "s" .. tostring(i % 10)
    end
    if type(x) == "number" then
        count = count + x
    else
        count = count + string.len(x)
    end
    i = i + 1
end

print(count)
